#include <Atomic.h>
#include <Exception.h>
#include <Logcat.h>
#include <Thread.h>
#include <Vertex.h>
#include <Utils.h>

namespace Base {
using TimeSpec = struct timespec;

namespace Internal {
void WatchStopper(Thread& thread);
void Idle(TimeSpec* spec);
}  // namespace Internal

Thread::Thread(Bool daemon) : _ThreadId{0}, _Status{Unknown} {
  _Watcher = None;
  _Count = new Int{0};
  _UUID = 0;
  _Registering = False;
  _Context = None;
  _Daemon = daemon;
}

Thread::Thread(const Thread& src) {
  _Watcher = src._Watcher;
  _Delegate = src._Delegate;
  _ThreadId = src._ThreadId;
  _Status = src._Status;
  _Count = src._Count;
  _Registering = src._Registering;
  _UUID = src._UUID;
  _Context = src._Context;

  (*_Count)++;
}

Thread::Thread(Thread& src) {
  _Watcher = src._Watcher;
  _Delegate = src._Delegate;
  _ThreadId = src._ThreadId;
  _Status = src._Status;
  _Count = src._Count;
  _Registering = src._Registering;
  _UUID = src._UUID;
  _Context = src._Context;

  (*_Count)++;
}

Thread::~Thread() {
  UInt timeout{1}, count{0}, level{2};

  if (_Daemon) {
    pthread_join(_ThreadId, None);
  } else if (*_Count == 0) {
    if (_ThreadId == 0) {
      /* @NOTE: auto start the thread it's not */

      if (!Start()) {
        goto finish;
      }
    }

    while (!CMP(&_Status, Expiring)) {
      TimeSpec spec{.tv_sec=0, .tv_nsec=0};

      while (CMP(&_Status, Unknown) || CMP(&_Status, Initing) ||
             CMP(&_Registering, True)) {
        timeout = (timeout * level) % ULong(1e8);
        count += 1;

        /* @NOTE: we should wait untill the thread is registered completely to
         * avoid unexpected behavious */

        if (CMP(&_Status, Expiring)) {
          pthread_join(_ThreadId, None);
          goto finish;
        }

        /* @NOTE: timeout so we should retry later to prevent stress up so much
         * to our system */

        if (count >= 10) {
          break;
        }

        spec.tv_nsec = timeout;

        Internal::Idle(&spec);
        RELAX();
      }

      if (count < 10) {
        if (CMPXCHG(&_Registering, False, True)) {
          /* @NOTE: change the value of _Registering to switch to locking state */
          break;
        }
      } else {
        level *= 2;
        count = 0;
      }

      RELAX();
    }

    if (_Status != Unknown) {
      Base::Vertex<void> escaping{[&]() {
        if (_Watcher) {
          _Watcher(_Context, True);
        }
      },
      [&]() {
        if (_Watcher) {
          _Watcher(_Context, False);
        }
      }};

      pthread_join(_ThreadId, None);
    }

finish:
    delete _Count;
  } else {
    (*_Count)--;
  }
}

void* Booting(void* thiz);

Bool Thread::Start(Function<void()> delegate) {
  if (_Status == Unknown && LIKELY(_Registering, False)) {
    if (ACK(&_Registering, True)) {
      return False;
    }

    if (delegate != None) _Delegate = delegate;

    Internal::WatchStopper(*this);
    switch (pthread_create(&_ThreadId, NULL, Booting, this)) {
      case EAGAIN:
        throw Except(EDoNothing, "Insufficient resources or limited");

      case EINVAL:
        throw Except(EBadLogic, "Invalid settings in attr");

      case EPERM:
        throw Except(EBadAccess, "No permission to set parameters attr");

      default:
        throw Except(EWatchErrno, "an unknown error");

      case 0:
        break;
    }
    
    return _Daemon || _Registering ||  _Status != Unknown;
  } else {
    return False;
  }
}

Bool Thread::Daemonize(Bool value) {
  Bool result = False;

  _Daemon = value && (result = _Status == Unknown && !_Registering);
  return result;
}

ULong Thread::Identity(Bool uuid) {
  if (uuid) {
    return (ULong)_UUID;
  } else {
    return (ULong)_ThreadId;
  }
}

Bool Thread::Daemon() { return _Daemon; }

Thread::StatusE Thread::Status() {
  if (LIKELY(_Registering, True) && LIKELY(_Status, Unknown) &&
      LIKELY(_UUID, 0) && UNLIKELY(_ThreadId, 0)) {
    return Initing;
  }

  return _Status;
}
}  // namespace Base
