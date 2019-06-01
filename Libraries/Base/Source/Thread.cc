#include <Exception.h>
#include <Thread.h>
#include <Vertex.h>
#include <Utils.h>

namespace Base {
namespace Internal {
Function<void(Bool)> WatchStopper(Thread& thread);
Bool UnwatchStopper(Thread& thread);
Bool RunAsDaemon(Thread& thread);
}  // namespace Internal

Thread::Thread(Bool daemon) : _ThreadId{0}, _Status{Unknown} {
  _Count = new Int{0};

  if (daemon && !Internal::RunAsDaemon(*this)) {
    throw Except(EBadAccess, "Can\'t implement a daemon");
  }
}

Thread::Thread(const Thread& src){
  _Watcher = src._Watcher;
  _Delegate = src._Delegate;
  _ThreadId = src._ThreadId;
  _Status = src._Status;
  _Count = src._Count;

  (*_Count)++;
}

Thread::Thread(Thread& src){
  _Watcher = src._Watcher;
  _Delegate = src._Delegate;
  _ThreadId = src._ThreadId;
  _Status = src._Status;
  _Count = src._Count;

  (*_Count)++;
}

Thread::~Thread() {
  if (*_Count == 0) {
    if (_Delegate != None && _Status == Unknown) {
      Start();
    }

    if (_Status != Expiring && _Status != Unknown) {
      Base::Vertex<void> escaping{[&]() {
#if USE_TID
        _Watcher = Internal::WatchStopper(*this);
#endif
        _Watcher(True);
        pthread_join(_ThreadId, None);
      },
      [&]() {
        _Watcher(False);

        while (!Internal::UnwatchStopper(*this)) {
          usleep(1);
        }
      }};
    } else {
      /* @NOTE: it seems except the first one won't run this, the other should
       * call this in order to fix the memory leak of pthread_create */

      pthread_join(_ThreadId, None);
    }

    delete _Count;
  } else {
    (*_Count)--;
  }
}

void* Booting(void* thiz);

Bool Thread::Start(Function<void()> delegate) {
  if (pthread_self() == _ThreadId) {
    return False;
  }

  if (_Status == Unknown) {
    if (delegate != None) _Delegate = delegate;

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
#if !USE_TID
      _Watcher = Internal::WatchStopper(*this);
#endif
      _Status = Initing;
    }

    return True;
  } else {
    return False;
  }
}

ULong Thread::Identity() { return (ULong)_ThreadId; }
}  // namespace Base
