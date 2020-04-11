#include <Macro.h>
#include <Logcat.h>
#include <Type.h>

#if LINUX
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/poll.h>

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

typedef struct pollfd Event;

typedef struct Context {
  Event* events;
  Int nevents, size;
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

typedef Int (*Run)(Pool*, Int, Int);

Int PollAppend(void* ptr, Int socket, Int mode){
  Context* poll = (struct Context*)(ptr);
  Int index = 0;

  if (mode != EWaiting && mode != ELooping) {
    return Error(ENoSupport, "append only support EWaiting and ELooping");
  }

  index = poll->nevents;

  if (!poll->events) {
    poll->events = (Event*)malloc(poll->nevents*sizeof(Event));

    if (!poll->events) {
      return Error(EDrainMem, "when use malloc to append socket");
    }

    poll->size = 1;
    poll->nevents = 1;
  } else if (poll->nevents == poll->size) {
    Event* tmp = (Event*)malloc(sizeof(Event)*(poll->nevents + 1));

    if (!tmp) {
      return Error(EDrainMem, "when use realloc to append socket");
    }

    /* @NOTE: it seems realloc can't duplicate memory to tmp as i expected */
    memcpy(tmp, poll->events, sizeof(Event)*poll->nevents);
    free(poll->events);

    poll->events = tmp;
    poll->size++;
    poll->nevents++;
  } else {
    poll->nevents++;
  }

  poll->events[index].fd = socket;

  if (mode == EWaiting) {
    poll->events[index].events = POLLIN | POLLPRI | POLLHUP;
    poll->events[index].revents = 0;
  } else if (mode == ELooping) {
    poll->events[index].events = POLLOUT | POLLHUP;
    poll->events[index].revents = 0;
  }

  return 0;
}

Int PollModify(void* ptr, Int socket, Int mode){
  Context* poll = (Context*)(ptr);
  Int index = 0;

  for (; index < poll->nevents; ++index) {
    if (poll->events[index].fd != socket) {
      continue;
    }

    if (mode == EWaiting) {
      poll->events[index].events = POLLIN | POLLPRI | POLLHUP;
    } else if (mode == ELooping) {
      poll->events[index].events = POLLOUT | POLLHUP;
    } else if (mode == EReleasing) {
      Int next = index + 1;

      for (; next < poll->nevents; ++next) {
        poll->events[next - 1] = poll->events[next];
      }

      poll->nevents--;
    }

    return 0;
  }

  return Error(ENotFound, "you\'re modify a non-existing socket");
}

Int PollProbe(Void* ptr, Int socket, Int mode) {
  Context* poll = (Context*)(ptr);
  Int index = 0;

  for (; index < poll->nevents; ++index) {
    if (poll->events[index].fd != socket) {
      continue;
    }

    if (mode == EWaiting) {
      return poll->events[index].events & POLLIN;
    } else if (mode == ELooping) {
      return poll->events[index].events & POLLOUT;
    } else {
      return -Error(ENoSupport, "only support EWaiting and ELooping");
    }
  }

  return -Error(ENotFound, "you\'re probing a non-existing socket");
}

Int PollRelease(void* ptr, Int socket) {
  Int error, fidx;
  Pool* pool = (Pool*)ptr;
  Context* context = (Context*)pool->ll.Poll;

  if (socket < 0) {
    for (fidx = 0; fidx < context->nevents; ++fidx) {
      if ((error = PollRelease(ptr, context->events[fidx].fd))) {
        return error;
      }
    }

    free(pool->ll.Poll->events);
    free(pool->ll.Poll);
  } else if (!(error = pool->Remove(pool, socket))) {
    if (!(error = pool->ll.Modify(context, socket, EReleasing))) {
      close(socket);
    } else {
      return error;
    }
  } else {
    return error;
  }

  return 0;
}

Context* PollBuild(Pool* pool) {
  Context* result = (Context*) malloc(sizeof(Context));

  if (!result) {
    return None;
  } else {
    memset(result, 0, sizeof(Context));
  }

  pool->ll.Release = PollRelease;
  pool->ll.Modify = PollModify;
  pool->ll.Probe = PollProbe;
  pool->ll.Append = PollAppend;

  return result;
}

Int PollRun(Pool* pool, Int timeout, Int UNUSED(backlog)) {
  Context* context;
  Int error = 0;

  if (!pool->ll.Poll) {
    context = PollBuild(pool);

    if (!context) {
      return Error(EBadAccess, "");
    }
  } else {
    context = (Context*)pool->ll.Poll;
  }

  if (pool->Status == IDLE && context->nevents == 0) {
    return EDoNothing;
  }

  do {
    Int fidx = 0, nevent = 0;

    nevent = poll(context->events, context->nevents, timeout);
    for (fidx = 0; fidx < context->nevents; ++fidx) {
      Int fd = context->events[fidx].fd;
      Int ev = context->events[fidx].revents;

      if (!ev) continue;
      if (!(ev & (POLLOUT | POLLIN))) {
        if (pool->Flush) {
          if (pool->Flush(pool, fd)) {
            continue;
          }
        }

        if ((error = pool->ll.Release(pool, fd))) {
          pool->Status = PANICING;
          break;
        } else {
          fidx--;
        }
      }

      do {
        if (ev & POLLIN) {
          switch (pool->Trigger(pool, fd, True)) {
          default:
            if ((error = pool->ll.Release(pool, fd))) {
              pool->Status = PANICING;
            }

          case ENoError:
          case EBadAccess:
          case EKeepContinue:
            ev &= ~POLLIN;
            break;

          case EDoAgain:
            break;
          }
        }

        if (ev & POLLOUT) {
          switch (pool->Trigger(pool, fd, False)) {
          default:
            if ((error = pool->ll.Release(pool, fd))) {
              pool->Status = PANICING;
            }

          case ENoError:
            pool->ll.Modify(pool, fd, EWaiting);

          case EBadAccess:
          case EKeepContinue:
            ev &= ~POLLOUT;
            break;

          case EDoAgain:
            break;
          }
        }
      } while (ev & (POLLIN | POLLOUT));

      if ((error = pool->Heartbeat(pool, fd))) {
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
      Int cidx = 0;

      for(; cidx < context->nevents; ++cidx) {
        Int fd = context->events[cidx].fd;

        if ((error = pool->Heartbeat(pool, fd))) {
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
    }
  } while (pool->Status < RELEASING && pool->Status != INTERRUPTED);

  if (pool->Status != INTERRUPTED && pool->Status != INIT) {
    pool->Status = RELEASING;

    for (UInt i = 0; i < context->nevents; ++i) {
      close(context->events[i].fd);
    }

    memset(&pool->ll, 0, sizeof(pool->ll));
    free(context->events);
    free(context);
  }

  return error;
}

Run Poll(Pool* pool) {
  if (pool) {
    if (!(pool->ll.Poll =  PollBuild(pool))) {
      return None;
    }
  }

  return (Run)PollRun;
}
#endif
