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

enum Mode {
  EWaiting = 0,
  ELooping = 1,
  EReleasing = 2
};

typedef fd_set FdSet;

typedef struct Pool {
  Void* Pool;
  Int Status;

  struct {
    FdSet* Poll;

    Int(*Append)(Void* ptr, Int socket, Int mode);
    Int(*Modify)(Void* ptr, Int socket, Int mode);
    Int(*Release)(Void* ptr, Int socket);
  } ll;

  Int (*Trigger)(Void* ptr, Int socket, Bool waiting);
  Int (*Heartbeat)(Void* ptr, Int socket);
  Int (*Remove)(Void* ptr, Int socket);
} Pool;

typedef Int (*Run)(Pool*, Int, Int);

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

  if (socket >= 0) {
    if (pool->Remove(pool, socket)) {
      enum ErrorCodeE error;

      if ((error = SelectModify(pool->ll.Poll, socket, EReleasing))) {
        return error;
      } else {
        close(socket);
      }
    }
  }

  return ENoError;
}

FdSet* SelectBuild(Pool* pool) {
  FdSet* result = (FdSet*)malloc(sizeof(FdSet));

  FD_ZERO(result);
  pool->ll.Release = SelectRelease;
  pool->ll.Modify = SelectModify;
  pool->ll.Append = SelectAppend;

  return result;
}

Int SelectRun(Pool* pool, Int timeout, Int UNUSED(backlog)) {
  FdSet* context = None;
  Int error = ENoError;

  if (!pool->ll.Poll) {
    context = SelectBuild(pool);
    pool->ll.Poll = (void*)context;
  } else {
    context = (FdSet*)pool->ll.Poll;
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
          error = pool->ll.Release(pool, fd);

        case ENoError:
        case EBadAccess:
        case EKeepContinue:
          break;
        }

        /* @NOTE: check heartbeat again to keep in track that the socket has been
         * closed or not */
        if (pool->Heartbeat && (error = pool->Heartbeat(pool, fd))) {
          error = pool->ll.Release(pool, fd);
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

    memset(&pool->ll, 0, sizeof(pool->ll));
    free(context);
  }

  return pool->Status != INTERRUPTED? ENoError: error;
}

Run Select(Pool* pool){
  if (pool) {
    if (!(pool->ll.Poll =  SelectBuild(pool))) {
      return None;
    }
  }

  return (Run)SelectRun; }
