#include <Atomic.h>
#include <Macro.h>
#include <Logcat.h>

#if LINUX
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/epoll.h>

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

typedef struct epoll_event Event;

typedef struct Context {
  Event* events;
  Int fd, nevent, backlog;
} Context;

typedef struct Pool {
  Void* Pool;
  Int Status, *Referral;

  struct {
    Context* Poll;

    Int(*Append)(Void* ptr, Int socket, Int mode);
    Int(*Modify)(Void* ptr, Int socket, Int mode);
    Int(*Probe)(Void* ptr, Int socket, Int mode);
    Int(*Release)(Void* ptr, Int socket);
  } ll;

  Int (*Trigger)(Void* ptr, Int socket, Bool waiting);
  Int (*Heartbeat)(Void* ptr, Int* socket);
  Int (*Remove)(Void* ptr, Int socket);
  Int (*Flush)(Void* ptr, Int socket);
  Int (*Run)(struct Pool*, Int, Int);
  Context* (*Build)(struct Pool* pool);
} Pool;

typedef Int (*Handler)(Pool*, Int);

Context* EpollBuildW(Pool* pool, Int backlog);

Int EpollAppend(void* ptr, Int socket, Int mode){
  Context* poll = (struct Context*)(ptr);
  Event event;

  memset(&event, 0, sizeof(struct epoll_event));
  event.data.fd = socket;

  if (mode == EWaiting) {
    event.events = EPOLLIN | EPOLLET;
  } else if (mode == ELooping) {
    event.events = EPOLLOUT | EPOLLET;
  } else {
    return Error(ENoSupport, "socket type only adapts value 0 or 1");
  }

  if (!poll) {
    return Error(EBadLogic, "context is not null");
  } else if (epoll_ctl(poll->fd, EPOLL_CTL_ADD, socket, &event) == -1) {
    return Error(EBadAccess, "epoll_ctl got an error");
  } else {
    INC(&poll->nevent);
    return 0;
  }
}

Int EpollModify(void* ptr, Int socket, Int mode){
  Context* poll = (Context*)(ptr);
  Event event;

  memset(&event, 0, sizeof(struct epoll_event));
  event.data.fd = socket;

  if (mode == EWaiting) {
    event.events = EPOLLIN | EPOLLET;
  } else if (mode == ELooping) {
    event.events = EPOLLOUT | EPOLLET;
  } else if (mode == EReleasing) {
    if (epoll_ctl(poll->fd, EPOLL_CTL_DEL, socket, NULL) == -1) {
      return Error(EBadLogic, "epoll_ctl got an error when it tries to delete a socket");
    } else {
      return 0;
    }
  } else {
    return Error(ENoSupport, "mode isn\'t supported");
  }

  if (epoll_ctl(poll->fd, EPOLL_CTL_MOD, socket, &event) == -1) {
    return Error(EBadAccess, "epoll_ctl got an error");
  } else {
    return 0;
  }
}

Int EpollRelease(void* ptr, Int socket) {
  Int error;
  Pool* pool = (Pool*)ptr;
  Context* context = (Context*)pool->ll.Poll;

  if (socket < 0) {
    if (context->nevent > 0) {
      return Error(EBadLogic, "detach epoll before closing");
    } else if (context->fd < 0) {
      return Error(EDoNothing, "it seem we can't create epoll fd as expected");
    }

    close(context->fd);
    free(context->events);
  } else if (!(error = pool->Remove(pool, socket))) {
    if (!(error = pool->ll.Modify(context, socket, EReleasing))) {
      DEC(&context->nevent);
      close(socket);
    } else {
      return error;
    }
  } else {
    return error;
  }

  return 0;
}

Int EpollHandler(Pool* pool, Int timeout, Int backlog) {
  Context* poll;

  if (!pool->ll.Poll) {
    poll = EpollBuildW(pool, backlog);

    if (!poll) {
      return Error(EBadAccess, "");
    } else if (poll->fd < 0) {
      return Error(EBadAccess, "");
    }
  } else {
    poll = (Context*)pool->ll.Poll;
  }

  if (pool->Status == IDLE && poll->nevent == 0) {
    return EDoNothing;
  }

  do {
    Int error = 0, idx = 0, nevent = 0, ev = -1, fd = -1;

    nevent = epoll_wait(poll->fd, poll->events, backlog, timeout);

    for (; idx < nevent; ++idx) {
      fd = poll->events[idx].data.fd;
      ev = poll->events[idx].events;

      if (ev & (EPOLLERR | EPOLLHUP | ~(EPOLLOUT | EPOLLIN))) {
        if (pool->Flush) {
          if (pool->Flush(pool, fd)) {
            continue;
          }
        }

        if ((error = pool->ll.Release(pool, fd))) {
          pool->Status = PANICING;
          break;
        } else {
          continue;
        }
      }

      do {
        if (ev & EPOLLIN) {
          switch (pool->Trigger(pool, fd, True)) {
          default:
            if ((error = pool->ll.Release(pool, fd))) {
              pool->Status = PANICING;
            }

          case ENoError:
          case EBadAccess:
          case EKeepContinue:
            ev &= ~EPOLLIN;
            break;

          case EDoAgain:
            break;
          }
        }

        if (ev & EPOLLOUT) {
          switch (pool->Trigger(pool, fd, False)) {
          default:
            if ((error = pool->ll.Release(pool, fd))) {
              pool->Status = PANICING;
            }

          case ENoError:
            pool->ll.Modify(pool, fd, EWaiting);

          case EBadAccess:
          case EKeepContinue:
            ev &= ~EPOLLOUT;
            break;

          case EDoAgain:
            break;
          }
        }
      } while (ev & (EPOLLIN | EPOLLOUT));

      if ((error = pool->Heartbeat(pool, &fd))) {
        if (pool->Flush) {
          if (pool->Flush(pool, fd)) {
            continue;
          }
        }

        if ((error = pool->ll.Release(pool, fd))) {
          pool->Status = PANICING;
          break;
        }
      }
    }

    if (nevent == 0) {
      do {
        if ((error = pool->Heartbeat(pool, &fd))) {
          if (fd >= 0 && pool->Flush) {
            if (pool->Flush(pool, fd)) {
              continue;
            }
          }
        }
      } while (False);
    }
  } while (pool->Status < RELEASING && pool->Status != INTERRUPTED);

  return 0;
}

Context* EpollBuild(Pool* pool) {
  if (pool->Status != INIT) {
    return EpollBuildW(pool, pool->ll.Poll->backlog);
  } else {
    return None;
  }
}

Context* EpollBuildW(Pool* pool, Int backlog) {
  Context* result;

  if (pool->Status == INIT) {
    result = (Context*) malloc(sizeof(Context));

    if (!result) {
      return None;
    } else {
      memset(result, 0, sizeof(Context));
    }
  } else if (pool->Status == PANICING) {
    result = pool->ll.Poll;
  } else {
    return pool->ll.Poll;
  }

  pool->ll.Release = EpollRelease;
  pool->ll.Modify = EpollModify;
  pool->ll.Append = EpollAppend;

  pool->Build = EpollBuild;
  pool->Run = EpollHandler;

  if (pool->Status != INIT) {
    close(result->fd);
  }

  result->fd = -1;

  if (pool->Status == INIT) {
    if (!(result->events = (Event*) malloc(sizeof(Event)*backlog))) {
      return result;
    }
  }

  if ((result->fd = epoll_create1(0)) == -1) {
    return result;
  } else {
    result->backlog = backlog;
  }

  return result;
}

Handler EPoll(Pool* pool, Int backlog){
  if (pool) {
    if (!(pool->ll.Poll =  EpollBuildW(pool, backlog))) {
      return None;
    }
  }

  return (Handler)EpollHandler;
}
#endif
