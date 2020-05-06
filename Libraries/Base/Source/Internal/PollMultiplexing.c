#define USE_MUTEX_DEBUG 1
#include <Atomic.h>
#include <Macro.h>
#include <Logcat.h>
#include <Type.h>
#include <Utils.h>

#if LINUX
#include <stdio.h>
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
  Mutex* mutex;
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
  Int (*Heartbeat)(Void* ptr, Int* socket);
  Int (*Remove)(Void* ptr, Int socket);
  Int (*Flush)(Void* ptr, Int socket);
  Int (*Run)(struct Pool*, Int, Int);
  Context* (*Build)(struct Pool* pool);
} Pool;

typedef Int (*Handler)(Pool*, Int, Int);

Context* PollBuild(Pool* pool);

Int PollAppend(void* ptr, Int socket, Int mode){
  Context* poll = (struct Context*)(ptr);
  Int index = 0, result = 0;

  if (mode != EWaiting && mode != ELooping) {
    return Error(ENoSupport, "append only support EWaiting and ELooping");
  }

  BSLockMutex(poll->mutex);

  index = poll->nevents;

  if (!poll->events) {
    poll->events = (Event*)malloc(sizeof(Event));

    if (!poll->events) {
      result = Error(EDrainMem, "when use malloc to append socket");
    }

    poll->size = 1;
    poll->nevents = 1;
  } else if (poll->nevents == poll->size) {
    Event* tmp = (Event*)malloc(sizeof(Event)*(poll->nevents + 1));

    if (!tmp) {
      result = Error(EDrainMem, "when use realloc to append socket");
    }

    /* @NOTE: it seems realloc can't duplicate memory to tmp as i expected */
    memcpy(tmp, poll->events, sizeof(Event)*poll->nevents);
    free(poll->events);

    poll->events = tmp;
    INC(&poll->size);
    INC(&poll->nevents);
  } else {
    INC(&poll->nevents);
  }

  BSUnlockMutex(poll->mutex);

  if (result) {
    return result;
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

    BSLockMutex(poll->mutex);

    if (mode == EWaiting) {
      poll->events[index].events = POLLIN | POLLPRI | POLLHUP;
    } else if (mode == ELooping) {
      poll->events[index].events = POLLOUT | POLLHUP;
    } else if (mode == EReleasing) {
      Int next = index + 1;

      for (; next < poll->nevents; ++next) {
        poll->events[next - 1] = poll->events[next];
      }

      DEC(&poll->nevents);
    }

    BSUnlockMutex(poll->mutex);
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
  Int error, fidx, result;
  Pool* pool = (Pool*)ptr;
  Context* context = (Context*)pool->ll.Poll;

  result = 0;

  if (socket < 0) { 
    if (pool->Status != RELEASING) { 
      pool->Status = RELEASING;
    } else {
      goto finish;
    }

    BSLockMutex(context->mutex);

    for (fidx = 0; fidx < context->nevents; ++fidx) {
      BSUnlockMutex(context->mutex);

      if ((error = PollRelease(ptr, context->events[fidx].fd))) {
        if (!result) {
          result = error;
        }
      }

      BSLockMutex(context->mutex);
    }

    /* @NOTE: it's safe to remove this mutex here since we have remove everything
     * completedly */
    
    BSUnlockMutex(context->mutex);
    BSDestroyMutex(context->mutex);

    /* @NOTE: we can remove the poll's objects here since every fd is closed and
     * the mutex is destroy */

    free(pool->ll.Poll->events);

  } else if (!(error = pool->Remove(pool, socket))) {
    if (!(error = pool->ll.Modify(context, socket, EReleasing))) {
      close(socket);
    } else {
      return error;
    }
  } else {
    return error;
  }

finish:
  return result;
}

Int PollHandler(Pool* pool, Int timeout, Int UNUSED(backlog)) {
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
    BSLockMutex(context->mutex);

    for (fidx = 0; fidx < context->nevents; ++fidx) {
      Int fd = context->events[fidx].fd;
      Int ev = context->events[fidx].revents;

      BSUnlockMutex(context->mutex);

      if (!ev) goto finish1;
      if (!(ev & (POLLOUT | POLLIN))) {
        if (pool->Flush) {
          if (pool->Flush(pool, fd)) {
            goto finish1;
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

      if ((error = pool->Heartbeat(pool, &fd))) {
        if (pool->Flush) {
          if (pool->Flush(pool, fd)) {
            goto finish1;
          }
        }

        if ((error = pool->ll.Release(pool, fd))) {
          /* @TODO: well it seem we release a closed fd so we might face this
           * issue recently so we don't need to care about it too much but we
           * should provide something to handle this case */
        }
      }

      continue;
finish1:
      BSLockMutex(context->mutex);
    }

    BSUnlockMutex(context->mutex);

    if (nevent == 0) {
      Int cidx = 0;

      BSLockMutex(context->mutex);

      for(; cidx < context->nevents; ++cidx) {
        Int fd = context->events[cidx].fd;

        BSUnlockMutex(context->mutex);
        if ((error = pool->Heartbeat(pool, &fd))) {
          if (pool->Flush) {
            if (pool->Flush(pool, fd)) {
              goto finish2;
            }
          }

          if ((error = pool->ll.Release(pool, fd))) {
            pool->Status = PANICING;
            break;
          }
        }
        
finish2:
        BSUnlockMutex(context->mutex);
      }

      BSUnlockMutex(context->mutex);
    }
  } while (pool->Status < RELEASING && pool->Status != INTERRUPTED);

  if (pool->Status != INTERRUPTED && pool->Status != INIT) {
    for (UInt i = 0; i < context->nevents; ++i) {
      close(context->events[i].fd);
    }
  }

  return error;
}

Context* PollBuild(Pool* pool) {
  Context* result;

  if (pool->Status == INIT) {
    result = (Context*) malloc(sizeof(Context));
  
    if (!result) {
      return None;
    } else {
      memset(result, 0, sizeof(Context));

      if (!(result->mutex = BSCreateMutex())) {
        goto fail;
      }
    }
  } else if (pool->Status == PANICING) {
    result = pool->ll.Poll;
  }

  pool->ll.Release = PollRelease;
  pool->ll.Modify = PollModify;
  pool->ll.Probe = PollProbe;
  pool->ll.Append = PollAppend;

  pool->Build = PollBuild;
  pool->Run = PollHandler;

  return result;

fail:  
  free(result);
  return None;
}

Handler Poll(Pool* pool) {
  if (pool) {
    if (!(pool->ll.Poll =  PollBuild(pool))) {
      return None;
    }
  }

  return (Handler)PollHandler;
}
#endif
