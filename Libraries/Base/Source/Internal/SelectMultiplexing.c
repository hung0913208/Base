#include <Macro.h>
#include <Monitor.h>
#include <Logcat.h>
#include <Type.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

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

typedef fd_set FdSet;

typedef struct Pool {
  Void* Pool;
  Int Status, *Referral;

  struct {
    FdSet* Poll;

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
  FdSet* (*Build)(struct Pool* pool);
} Pool;

typedef Int (*Handler)(Pool*, Int, Int);

FdSet* SelectBuild(Pool* pool);

Int SelectAppend(Void* ptr, Int socket, Int mode) {
  FdSet* fds = (FdSet*)(ptr);

  if (mode == EWaiting) {
    FD_SET(socket, fds);
  } else {
    return Error(ENoSupport, "mode isn\'t supported");
  }

  return ENoError;
}

Int SelectModify(Void* UNUSED(ptr), Int UNUSED(socket),
                            Int UNUSED(mode)) {
  return Error(ENoSupport, "mode isn\'t supported");
}

Int SelectRelease(void* ptr, Int socket){
  Pool* pool = (struct Pool*)(ptr);

  if (socket < 0) {
    Int fd = 0, error = 0;

    for (fd = 0; fd < FD_SETSIZE; ++fd) {
      if (!FD_ISSET (fd, pool->ll.Poll)) {
        continue;
      } else if ((error = SelectRelease(ptr, fd))) {
        return error;
      }
    }

    free(pool->ll.Poll);
  } else {
    Int error;

    if (!(error = pool->Remove(pool, socket))) {
      if ((error = SelectModify(pool->ll.Poll, socket, EReleasing))) {
        return error;
      } else {
        close(socket);
      }
    } else {
      return error;
    }
  }

  return ENoError;
}

Int SelectHandler(Pool* pool, Int timeout, Int UNUSED(backlog)) {
  FdSet* context = None;
  Int error = ENoError;

  if (!pool->ll.Poll) {
    context = SelectBuild(pool);
    pool->ll.Poll = (void*)context;
  } else {
    context = (FdSet*)pool->ll.Poll;
  }

  if (pool->Status == IDLE) {
    return EDoNothing;
  }

  if (INIT < pool->Status && pool->Status < RELEASING) {
    Bool is_interrupted = pool->Status == INTERRUPTED;
    struct timeval tw;

    /* @NOTE: Initialize the timeout data structure. */
    memset(&tw, 0, sizeof(tw));
    tw.tv_usec = timeout;

    do {
      Int fd = 0;

      if (select(FD_SETSIZE, context, None, None, &tw) <= 0) {
        if (is_interrupted) {
          break;
        } else {
          continue;
        }
      }

      for (fd = 0; fd < FD_SETSIZE; ++fd) {
        if (!FD_ISSET (fd, context)) {
          continue;
        }

        switch((error = pool->Trigger(pool, True, fd))) {
        default:
          if ((error = pool->ll.Release(pool, fd))) {
            pool->Status = PANICING;
          }

        case ENoError:
        case EBadAccess:
        case EKeepContinue:
          break;
        }

        /* @NOTE: check heartbeat again to keep in track that the socket has been
         * closed or not */
        if (pool->Heartbeat && (error = pool->Heartbeat(pool, &fd))) {
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

checking:
        if (is_interrupted) {
          break;
        }
      }
    } while (pool->Status < RELEASING && pool->Status != INTERRUPTED);
  }

  /* @NOTE: release the context's fd */
  if (pool->Status != INTERRUPTED && pool->Status != INIT) {
    Int fd = 0;

    for (; fd < FD_SETSIZE; ++fd) {
      if (FD_ISSET (fd, context)) {
        pool->ll.Release(pool, fd);
      }
    }
  }

  return pool->Status != INTERRUPTED? ENoError: error;
}

FdSet* SelectBuild(Pool* pool) {
  FdSet* result;
 
  if (pool->Status == INIT) {
    result = (FdSet*)malloc(sizeof(FdSet));
  } else if (pool->Status == PANICING) {
    result = pool->ll.Poll;
  } else {
    return pool->ll.Poll;
  }

  FD_ZERO(result);

  pool->ll.Release = SelectRelease;
  pool->ll.Modify = SelectModify;
  pool->ll.Append = SelectAppend;

  pool->Build = SelectBuild;
  pool->Run = SelectHandler;
  return result;
}

Handler Select(Pool* pool){
  if (pool) {
    if (!(pool->ll.Poll =  SelectBuild(pool))) {
      return None;
    }
  }

  return (Handler)SelectHandler; }
