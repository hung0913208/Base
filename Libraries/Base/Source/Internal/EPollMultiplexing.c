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
  Int fd, nevent;
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
  Int (*Heartbeat)(Void* ptr, Int socket);
  Int (*Remove)(Void* ptr, Int socket);
  Int (*Flush)(Void* ptr, Int socket);
} Pool;

typedef Int (*Run)(Pool*, Int);

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

Context* EpollBuild(Pool* pool, Int backlog) {
  Context* result = (Context*) malloc(sizeof(Context));

  if (!result) {
    return None;
  } else {
    memset(result, 0, sizeof(Context));
  }

  pool->ll.Release = EpollRelease;
  pool->ll.Modify = EpollModify;
  pool->ll.Append = EpollAppend;
  result->fd = -1;

  if (!(result->events = (Event*) malloc(sizeof(Event)*backlog))) {
    return result;
  }

  if ((result->fd = epoll_create1(0)) == -1) {
    return result;
  }

  return result;
}

Int EpollRun(Pool* pool, Int timeout, Int backlog) {
  Context* poll;

  if (!pool->ll.Poll) {
    poll = EpollBuild(pool, backlog);

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
    Int error = 0, idx = 0, nevent = 0;

    nevent = epoll_wait(poll->fd, poll->events, backlog, timeout);
    for (; idx < nevent; ++idx) {
      Int fd = poll->events[idx].data.fd;
      Int ev = poll->events[idx].events;

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
    }

    if (nevent == 0) {
    }
  } while (pool->Status < RELEASING && pool->Status != INTERRUPTED);

  if (pool->Status != INTERRUPTED && pool->Status != INIT) {
    pool->Status = RELEASING;

    memset(&pool->ll, 0, sizeof(pool->ll));
    close(poll->fd);
    free(poll->events);
    free(poll);
  }
  return 0;
}

Run EPoll(Pool* pool, Int backlog){
  if (pool) {
    if (!(pool->ll.Poll =  EpollBuild(pool, backlog))) {
      return None;
    }
  }

  return (Run)EpollRun;
}
#endif
