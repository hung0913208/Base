#include <Macro.h>
#include <Monitor.h>
#include <Logcat.h>
#include <Type.h>

#if MACOS || BSD
#include <sys/event.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INIT 0
#define IDLE 1
#define RUNNING 2
#define INTERRUPTED 3
#define RELEASING 4
#define PANICING 5

enum Mode {
  EWaiting = 0,
  ELooping = 1,
  EReleasing = 2
};

typedef struct kevent Kevent;

typedef Int (*Run)(Pool*, Int, Int);

typedef struct Context {
  Kevent* events;
  Int fd, nevent;
} Context;

typedef struct Pool {
  Void* Pool;
  Int Status;

  struct {
    Context* Poll;

    Int(*Append)(Void* ptr, Int socket, Int mode);
    Int(*Modify)(Void* ptr, Int socket, Int mode);
    Int(*Probe)(Void* ptr, Int socket, Int mode);
    Int(*Release)(Void* ptr, Int socket);
  } ll;

  Int (*Trigger)(Void* ptr, Int socket, Bool waiting);
  Int (*Heartbeat)(Void* ptr, Int socket);
  Int (*Remove)(Void* ptr, Int socket);
  Int (*Flush)(Void* ptr, Int socket);
} Pool;

enum ErrorCodeE KqueueAppend(Void* ptr, Int socket, Int mode) {
  Context* poll = (struct Context*)(ptr);
  Kevent event;

  if (mode == EWaiting) {
    EV_SET(&eventt, localFd, EVFILT_READ, EV_ADD, 0, 0, NULL);
  } else if (mode == ERepeating) {
    EV_SET(&eventt, localFd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
  } else {
    return Error(ENoSupport, "mode isn\'t supported");
  }

  if (!poll) {
    return Error(EBadLogic, "pool is not null");
  } else if (kevent(poll->fd, &event, 1, NULL, 0, NULL) == -1) {
    return Error(EBadAccess, "epoll_ctl got an error");
  } else {
    INC(&poll->nevent);
  }

  return ENoError;
}

enum ErrorCodeE KqueueModify(Void* ptr, Int socket, Int mode) {
  Context* poll = (struct Context*)(ptr);
  epoll_event event;

  memset(&event, 0, sizeof(struct epoll_event));
  event.data.fd = socket;

  if (mode == EWaiting) {
    EV_SET(&event, socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
  } else if (mode == ERepeating) {
    EV_SET(&event, socket, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
  } else if (mode == EReleasing) {
    /* @NOTE: socket should be remove automatically at the time
     * when you close connection  */

    return EDoNothing;
  } else {
    return Error(ENoSupport, "mode isn\'t supported");
  }

  if (kevent(poll->fd, &event, 1, NULL, 0, NULL) == -1) {
    return Error(EBadAccess, "epoll_ctl got an error");
  } else {
    return ENoError;
  }
}

enum ErrorCodeE KqueueRelease(void* ptr, Int socket, ErrorCodeE reason){
  Pool* pool = (struct Pool*)(ptr);

  if (socket >= 0) {
    if (pool->Remove(pool, socket)) {
      Kqueue_Modify(pool->ll.Poll, socket, EReleasing);
      DEC(&pool->ll.Poll->nevent);
      close(socket);
    } else {
      return EDoAgain;
    }
  }
  return reason;
}

Context* KqueueBuild(Pool* pool) {
  Context* result = (Context*)malloc(sizeof(Context));

  memset(result, 0, sizeof(Context));
  pool->ll.Release = KqueueRelease;
  pool->ll.Modify = KqueueModify;
  pool->ll.Append = KqueueAppend;

  result->fd = -1;
  result->events = (Kevent*)malloc(sizeof(Kevent)*pool->MaxEvents);

  if (!result->events) {
    return result;
  } else if ((result->fd = kqueue()) == -1) {
    return result;
  } else {
    pool->ll.Poll = result;
  }
  return result;
}

Int KqueueRun(Pool* pool, Int timeout, Int backlog) {
  Context* poll;

  if (!pool->ll.Poll) {
    poll = Kqueue_Build(pool);

    if (!poll->events) {
      return Error(EDrainMem, "");
    } else if (poll->fd < 0) {
      return Error(EBadAccess, "");
    } else {
      pool->ll.Poll = (void*)poll;
    }
  } else {
    poll = (Context*)pool->ll.Poll;
  }

  if (pool->Status == IDLE) {
    return EDoNothing;
  }

  if (INIT < pool->Status && pool->Status < RELEASE) {
    Bool is_interrupted = pool->Status == INTERRUPT;

    do {
      Int error = 0, idevent = 0, nevent = 0;

      if (pool->Timeout > 0) {
        struct timespec timeout;

        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += pool->Timeout;

        nevent = kevent(poll->fd, None, 0
                        poll->events, pool->MaxEvents,
                        &timeout);
      } else {
        nevent = kevent(poll->fd, None, 0
                        poll->events, pool->MaxEvents,
                        None);
      }

      for (; idevent < nevent; ++idevent) {
        Int flagss = poll->events[idevent].flags;
        Int events = poll->events[idevent].filter;
        Int fd = poll->events[idevent].ident;

        /* @NOTE: if we found out that this fd don't have any I/O event, we
         * should close it now */
        if (flags & EV_EOF) {
          if ((error = pool->ll.Release(pool, fd))) {
            pool->Status = PANICING;
          }

          goto checking;
        } else if (pool->Heartbeat && pool->Heartbeat(pool, fd)) {
          if (pool->Flush) {
            if (pool->Flush(pool, fd)) {
              continue;
            }
          }

          if ((error = pool->ll.Release(pool, fd))) {
            pool->Status = PANICING;
            break;
          }

          goto checking;
        }

        do {
          if (events & EVFILT_READ) {
            /* @NOTE: trigger event with flag READ is ON */

            switch((error = pool->Trigger(pool, True, fd))) {
            default:
              if ((error = pool->ll.Release(pool, fd))) {
                pool->Status = PANICING;
              }

            case EBadAccess:
            case ENoError:
            case EKeepContinue:
              events &= ~EVFILT_READ;

            case EDoAgain:
              break;
            }
          }

          if (events & EVFILT_WRITE) {
            /* @NOTE: trigger event with flag READ is OFF */

            switch((error = pool->Trigger(pool, False, fd))) {
            default:
              if ((error = pool->ll.Release(pool, fd))) {
                pool->Status = PANICING;
              }

            case EBadAccess:
            case ENoError:
            case EKeepContinue:
              events &= ~EVFILT_WRITE;

            case EDoAgain:
              break;
            }
          }

          if (status && !status(pool)) { /* @NOTE: check status of this pool */
            break;
          }
        } while (events & (EVFILT_READ | EVFILT_WRITE));

        /* @NOTE: check heartbeat again to keep in track that the socket has been
         * closed or not */
        if (pool->Heartbeat && pool->Heartbeat(pool, fd)) {
          if ((error = pool->ll.Release(pool, fd))) {
            pool->Status = PANICING;
            break;
          }

          continue;
        }

checking:
        if (is_interrupted) {
          break;
        }
      }
    } while (pool->Status < RELEASING && pool->Status != INTERRUPTED);
  }

  /* @NOTE: release the epoll's fd */
  if (pool->Status != INTERRUPTED && pool->Status != INIT) {
    pool->Status = RELEASING;

    memset(&pool->ll, 0, sizeof(pool->ll));
    close(poll->fd);
    free(poll->events);
  }

  return ENoError;
}

Run KQueue(Pool* pool){
  if (pool) {
    if (!(pool->ll.Poll =  KqueueBuild(pool))) {
      return None;
    }
  }
  return (Run)KqueueRun; }
#endif
