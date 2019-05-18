#include <Exception.h>
#include <Macro.h>
#include <Monitor.h>
#include <Logcat.h>
#include <Vertex.h>

extern "C" {
typedef struct Pool {
  Void* Pool;
  Int Status;

  struct {
    Void* Poll;

    Int(*Append)(Void* ptr, Int socket, Int mode);
    Int(*Modify)(Void* ptr, Int socket, Int mode);
    Int(*Release)(Void* ptr, Int socket);
  } ll;

  Int (*Trigger)(Void* ptr, Int socket, Bool waiting);
  Int (*Heartbeat)(Void* ptr, Int socket);
  Int (*Remove)(Void* ptr, Int socket);
} Pool;

typedef Int (*Run)(Pool*, Int, Int);

enum Mode {
  EWaiting = 0,
  ELooping = 1,
  ERelease = 2
};

#if LINUX
Run EPoll(Pool* pool, Int backlog);
Run Poll(Pool* pool);
Run Select(Pool* pool);
#elif MACOS || BSD
Run KQueue(Pool* pool);
Run Poll(Pool* pool);
Run Select(Pool* pool);
#elif WINDOW
Run IOStat(Pool* pool);
Run Select(Pool* pool);
#endif
}

namespace Base {
namespace Internal {
ULong GetUniqueId();

namespace Fildes {
Int Trigger(Void* ptr, Int fd, Bool reading);
Int Heartbeat(Void* ptr, Int fd);
Int Remove(Void* ptr, Int fd);
} // namespace Fildes
} // namespace Internal

class Fildes: public Monitor {
 public:
#if LINUX
  explicit Fildes(String name, Monitor::TypeE type, Int system, Int backlog = 100) :
#else
  explicit Fildes(String name, Monitor::TypeE type, Int system) :
#endif
      Monitor(name, type) {
    _Pool.Heartbeat = Base::Internal::Fildes::Heartbeat;
    _Pool.Trigger = Base::Internal::Fildes::Trigger;
    _Pool.Remove = Base::Internal::Fildes::Remove;

    if (Monitor::Head(type) == dynamic_cast<Monitor*>(this)) {
      switch (system) {
      case 0:
#if LINUX
        _Run = EPoll(&_Pool, backlog);
#elif MACOS || BSD
        _Run = KQueue(&_Pool);
#elif WINDOW
        _Run = IOStat(&_Pool);
#else
        throw Except(ENoSupport, "");
#endif
        break;

      case 1:
#if UNIX
        _Run = Poll(&_Pool);
#elif WINDOW
        _Run = Select(&_Pool);
#else
        throw Except(ENoSupport, "");
#endif
        break;

      case 2:
#if UNIX
        _Run = Select(&_Pool);
#else
        throw Except(ENoSupport, "");
#endif
        break;
      }

      if (!_Run) {
        throw Except(EDrainMem, "can\'t allocate memory to polling system");
      }
    } else {
      _Pool.ll = dynamic_cast<Fildes*>(Monitor::Head(type))->_Pool.ll;
      _Run = dynamic_cast<Fildes*>(Monitor::Head(type))->_Run;
    }
  }

 protected:
  friend Int Internal::Fildes::Trigger(Void* ptr, Int fd, Bool reading);
  friend Int Internal::Fildes::Heartbeat(Void* ptr, Int fd);
  friend Int Internal::Fildes::Remove(Void* ptr, Int fd);

  /* @NOTE: this function is used to register a triggering base on the event */
  ErrorCodeE _Trigger(Auto UNUSED(event), Monitor::Perform UNUSED(perform)) {
    return ENoSupport;
  }

  /* @NOTE: this function is used to append a new fd to polling system */
  ErrorCodeE _Append(Auto fd, Int mode) {
    try {
      return (ErrorCodeE) _Pool.ll.Append( _Pool.ll.Poll, fd.Get<Int>(), mode);
    } catch(Base::Exception& except) {
      return except.code();
    }
  }

  /* @NOTE: this function is used to change mode of fd by using polling
   * system callbacks */
  ErrorCodeE _Modify(Auto fd, Int mode) final {
    try {
      if (mode != ERelease) {
        return (ErrorCodeE) _Pool.ll.Modify(_Pool.ll.Poll, fd.Get<Int>(), mode);
      } else {
        return ENoSupport;
      }
    } catch(Base::Exception& except) {
      return except.code();
    }
  }


  /* @NOTE: this function is used to find the fd inside Fildes */
  ErrorCodeE _Find(Auto fd) final {
    try {
      if (_Entries.find(fd.Get<Int>()) == _Entries.end()) {
        return ENotFound;
      }

      return ENoError;
    } catch(Base::Exception& except) {
      return except.code();
    }
  }

  /* @NOTE: this function is used to remove fd out of polling system */
  ErrorCodeE _Remove(Auto fd) final {
    try {
      _Entries.erase(fd.Get<Int>());
      return ENoError;
    } catch(Base::Exception& except) {
      return except.code();
    }
  }

  /* @NOTE: this method is used to check status of system or a specific
   * fd whihch is installed into the monitor */
  ErrorCodeE _Status(String name, Auto fd) final {
    if (fd.Type() == typeid(Int)) {
      if (name == "online") {
        /* @NOTE: state online only happens when fd is set and i will check
         * if the fd is existed inside the monitor or not */

        if (_Entries.find(fd.Get<Int>()) != _Entries.end()) {
          return ENoError;
        }
      } else if (fd.Get<Int>() >= 0) {
        return EInterrupted;
      }
    }

    if (name == "interrupted") {
      /* @NOTE: state interrupted would mean you don't want polling system
       * to wait continuously events from fds. Instead we will react wait
       * in a short pan and catch an event to process we found it. Using
       * it with timeout will help you to avoid hanging issues. */

      return _Tid >= 0? EInterrupted: ENoError;
    } else if (name == "running") {
      /* @NOTE: state running would mean that we must has run on different
       * thread and we don't request this status inside triggered callbacks.
       * The reason for that is very simple, when the polling system start
       * it must stuck to wait events from another fd thue, if you're able
       * to call this method, it would mean HEAD are running recently */

      if ((Long)Internal::GetUniqueId() !=_Tid) {
        return _Tid >= 0? ENoError: EInterrupted;
      } else {
        /* @NOTE: there are 2 way to run to this situation:
         * - You're running on Interrupted mode
         * - You're checking status inside triggering callbacks, maybe you
         * don't rent any threads to perform this callback */

        return EInterrupted;
      }
    } else if (name == "idle") {
      /* @NOTE: state idle would mean we don't have any fd waiting or working
       * from another side */

      if (_Entries.size() > 0) {
        return EInterrupted;
      } else {
        /* @NOTE: check from children, we wouldn't know it without checking
         * them because some polling system don't support checking how many
         * fd has waiting */

        for (auto& child: _Children) {
          if (child->Status(name)) {
            return EInterrupted;
          }
        }

        return ENoError;
      }
    }

    return EInterrupted;
  }

  /* @NOTE: access context of each fd */
  Auto& _Access(Auto fd) {
    if (_Find(fd)) {
      throw Except(ENotFound, ToString(fd.Get<Int>()));
    }

    return _Entries[fd.Get<Int>()];
  }

  /* @NOTE: this method is used to interact with lowlevel */
  ErrorCodeE _Interact(Monitor* child, Int timeout, Int backlog = 100) final {
    if (_Tid < 0) {
      Vertex<void> escaping{[&](){ _Tid = Internal::GetUniqueId(); },
                            [&](){ _Tid = -1; }};
      Fildes* fildes = dynamic_cast<Fildes*>(child);

      if (fildes) {
        return (ErrorCodeE) _Run(&fildes->_Pool, timeout, backlog);
      } else {
        return BadLogic("child should be Fildes").code();
      }
    } else {
      return EBadAccess;
    }
  }

  /* @NOTE: this method is used to collect jobs appear at mode waiting */
  ErrorCodeE OnWaiting(Int socket) {
    Vector<Monitor::Perform*> jobs{};
    Bool passed{False};
    Auto fd{Auto::As<Int>(socket)};

    /* @NOTE: collect jobs from HEAD */
    if (!_Find(fd)) {
      ErrorCodeE error;

      if ((error = Scan(fd, EWaiting, jobs)) &&
           error != ENotFound) {
        return error;
      }

      for (auto& job: jobs) {
        if ((error = Reroute(this, fd, *job))) {
          return error;
        }
      }

      return ENoError;
    }

    /* @NOTE: collect jobs from CHILD */
    for (auto& child : _Children) {
      ErrorCodeE error;

      if (child->Find(fd)) {
        continue;
      } else if ((error = child->Scan(fd, EWaiting, jobs)) &&
                  error != ENotFound) {
        return error;
      } else {
        passed = True;
      }

      for (auto& job: jobs) {
        if ((error = Reroute(child, fd, *job))) {
          return error;
        }
      }
    }

    return passed? ENoError: ENotFound;
  }

  ErrorCodeE OnLooping(Int socket) {
    Vector<Monitor::Perform*> jobs{};
    Bool passed{False};
    Auto fd{Auto::As<Int>(socket)};

    /* @NOTE: collect jobs from HEAD and route them to consumers */
    if (!_Find(fd)) {
      ErrorCodeE error;

      if ((error = Scan(fd, ELooping, jobs)) && error != ENotFound) {
        return error;
      } else {
        passed = True;
      }

      for (auto& job: jobs) {
        if ((error = Reroute(this, fd, *job))) {
          return error;
        }
      }
    }

    /* @NOTE: collect jobs from CHILD and route them to consumers */
    for (auto& child : _Children) {
      ErrorCodeE error;

      if (child->Find(fd)) {
        continue;
      } else if ((error = child->Scan(fd, ELooping, jobs)) &&
                  error != ENotFound) {
        return error;
      } else {
        passed = True;
      }

      for (auto& job: jobs) {
        if ((error = Reroute(child, fd, *job))) {
          return error;
        }
      }
    }

    return passed? ENoError: ENotFound;
  }

  ErrorCodeE OnRemoving(Int socket) {
    auto fd = Auto::As<Int>(socket);
    auto passed = False;

    if ((passed = !Find(fd))) {
      _Entries.erase(socket);
    }

    for (auto child: _Children) {
      if (!child->Find(fd)) {
        if (!child->Remove(fd)) {
          passed = True;
        }
      }
    }

    return passed? ENoError: ENotFound;
  }

  Map<Int, Auto> _Entries;
  Long _Tid;
  Pool _Pool;
  Run _Run;
};

namespace Internal {
namespace Fildes {
Shared<Monitor> Create(String name, Base::Monitor::TypeE type, Int system){
  return Shared<Monitor>(new class Fildes(name, type, system));
}

Int Trigger(Void* ptr, Int fd, Bool waiting) {
  Pool* pool = reinterpret_cast<Pool*>(ptr);

  if (pool) {
    if (waiting) {
      return (Int)(reinterpret_cast<class Fildes*>(pool->Pool))->OnWaiting(fd);
    } else {
      return (Int)(reinterpret_cast<class Fildes*>(pool->Pool))->OnLooping(fd);
    }
  } else {
    return (Int)EBadAccess;
  }
}

Int Heartbeat(Void* ptr, Int socket) {
  Pool* pool = reinterpret_cast<Pool*>(ptr);

  if (pool) {
    return (Int)(reinterpret_cast<class Fildes*>(pool->Pool))
      ->Heartbeat(Auto::As<Int>(socket));
  } else {
    return (Int)EBadAccess;
  }
}

Int Remove(Void* ptr, Int fd) {
  Pool* pool = reinterpret_cast<Pool*>(ptr);

  if (pool) {
    return (reinterpret_cast<class Fildes*>(pool->Pool))->OnRemoving(fd);
  } else {
    return (Int)EBadAccess;
  }
}
} // namespace Fildes
} // namespace Internal
} // namespace Base
