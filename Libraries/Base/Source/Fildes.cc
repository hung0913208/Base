#include <Exception.h>
#include <Macro.h>
#include <Monitor.h>
#include <Logcat.h>
#include <Vertex.h>

#define INIT 0
#define IDLE 1
#define RUNNING 2
#define INTERRUPTED 3
#define RELEASING 4

extern "C" {
typedef struct Pool {
  Void* Pool;
  Int Status;

  struct {
    Void* Poll;

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
Bool IsPipeAlive(Int pipe);
Bool IsPipeWaiting(Int pipe);
ULong GetUniqueId();

namespace Fildes {
Int Trigger(Void* ptr, Int fd, Bool reading);
Int Heartbeat(Void* ptr, Int fd);
Int Remove(Void* ptr, Int fd);
Int Flush(Void* ptr, Int fd);
} // namespace Fildes
} // namespace Internal

class Fildes: public Monitor {
 public:
#if LINUX
  explicit Fildes(String name, Monitor::TypeE type, Int system, Int backlog = 100) :
#else
  explicit Fildes(String name, Monitor::TypeE type, Int system) :
#endif
      Monitor(name, type), _Tid{-1} {
    using namespace std::placeholders;  // for _1, _2, _3...

    _Pool.Heartbeat = Base::Internal::Fildes::Heartbeat;
    _Pool.Trigger = Base::Internal::Fildes::Trigger;
    _Pool.Remove = Base::Internal::Fildes::Remove;
    _Pool.Flush = Base::Internal::Fildes::Flush;

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

    if (type == Monitor::EPipe) {
      _Fallbacks.push_back([&](Auto fd) -> ErrorCodeE {
        if (Internal::IsPipeAlive(fd.Get<Int>())) {
          return ENoError;
        } else {
          return EBadAccess;
        }
      });
    }

    /* @NOTE: we should register our own Monitor to the polling system, since
     * the polling system only care about the HEAD */
    _Pool.Pool = this;

    /* @NOTE: build a pair check - indicate to help to translate
     * event -> callbacks */
    Registry(std::bind(&Fildes::OnChecking, this, _1, _2, _3),
             std::bind(&Fildes::OnSelecting, this, _1, _2));
  }

 protected:
  friend Int Internal::Fildes::Trigger(Void* ptr, Int fd, Bool reading);
  friend Int Internal::Fildes::Heartbeat(Void* ptr, Int fd);
  friend Int Internal::Fildes::Remove(Void* ptr, Int fd);
  friend Int Internal::Fildes::Flush(Void* ptr, Int fd);

  /* @NOTE: this function is used to register a triggering base on the event */
  ErrorCodeE _Trigger(Auto event, Monitor::Perform perform) {
    ErrorCodeE error;

    if (_Type == Monitor::EPipe) {
      /* @NOTE: if we are monitoring pipeline we should expect that event must
       * be Base::Fork or Int. In the case of Base::Fork, we must register
       * fallback to check if the child process is running or not in order to
       * manage our pipelines better */

      if (event.Type() == typeid(Fork)) {
        auto fork = event.Get<Fork>();

        if (fork.Status() != Fork::EBug) {
          if ((error =_Append(Auto::As(fork.Error()), EWaiting))) {
            return error;
          }

          if ((error = _Append(Auto::As(fork.Output()), EWaiting))) {
            return error;
          }

          _Read[fork.Output()] = perform;
          _Read[fork.Error()] = perform;
          return ENoError;
        } else {
          return BadAccess("fork can\'t create process as expected").code();
        }
      } else if (event.Type() == typeid(Int)) {
        if ((error = _Append(event, EWaiting))) {
          return error;
        }

        _Read[event.Get<Int>()] = perform;
        return ENoError;
      }
    } else if (_Type == Monitor::EIOSync) {
      /* @NOTE: if we are monitoring socket, we don't need to register anything
       * more to help to watch our sockets because the polling system should
       * detect when a socket is closed or not */

      if (event.Type() == typeid(Int)) {
        if ((error = _Append(event, EWaiting))) {
          return error;
        }

        _Read[event.Get<Int>()] = perform;
        return ENoError;
      }
    }

    return ENoSupport;
  }

  /* @NOTE: this function is used to append a new fd to polling system */
  ErrorCodeE _Append(Auto fd, Int mode) {
    try {
      if (fd.Get<Int>() < 0) {
        return BadLogic("fd shouldn\'t be negative").code();
      } else if (_Entries.find(fd) != _Entries.end()) {
        return BadLogic(Format{"duplicate fd {}"} << fd).code();
      } else {
        auto error = _Pool.ll.Append(_Pool.ll.Poll, fd.Get<Int>(), mode);

        if (error) {
          return (ErrorCodeE) error;
        }

        _Entries[fd.Get<Int>()] = Auto{};
        return ENoError;
      }
    } catch(Base::Exception& except) {
      return except.code();
    }
  }

  /* @NOTE: this function is used to change mode of fd by using polling
   * system callbacks */
  ErrorCodeE _Modify(Auto fd, Int mode) final {
    try {
      if (mode != ERelease) {
        Int error = _Pool.ll.Modify(_Pool.ll.Poll, fd.Get<Int>(), mode);

        if (error) {
          return (ErrorCodeE) error;
        }

        if (mode == ELooping && _Read.find(fd.Get<Int>()) != _Read.end()) {
          _Write[fd.Get<Int>()] = _Read[fd.Get<Int>()];
          _Read.erase(fd.Get<Int>());
        }

        if (mode == EWaiting && _Write.find(fd.Get<Int>()) != _Write.end()) {
          _Read[fd.Get<Int>()] = _Write[fd.Get<Int>()];
          _Write.erase(fd.Get<Int>());
        }

        return (ErrorCodeE) error;
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
      DEBUG(Format{"remove fd {}"}.Apply(fd));

      _Entries.erase(fd.Get<Int>());
      _Read.erase(fd.Get<Int>());
      _Write.erase(fd.Get<Int>());

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
        if (fildes->_Pool.Status == RELEASING) {
          return EDoNothing;
        } else if (timeout < 0) {
          fildes->_Pool.Status = RUNNING;
        } else if (fildes->_Pool.Status != INTERRUPTED) {
          fildes->_Pool.Status = INTERRUPTED;
        }

        if (fildes->_Pool.Status == INTERRUPTED) {
          if (_Entries.size() == 0) {
            fildes->_Pool.Status = IDLE;
          }
        }

        return (ErrorCodeE) _Run(&fildes->_Pool, timeout, backlog);
      } else {
        return BadLogic("child should be Fildes").code();
      }
    } else {
      return BadAccess("Monitor is still on  initing").code();
    }
  }

 private:
  Int IsWaiting(Int socket) {
    return _Read.find(socket) != _Read.end();
  }

  /* @NOTE: this method is called automatically to check if the event should be
   * going on or not. By default, i think we should accept everything since the
   * flow has been handle very good to prevent */
  ErrorCodeE OnChecking(Auto fd, Auto& UNUSED(context), Int mode) {
    auto sk = fd.Get<Int>();

    DEBUG(Format{"checking event {} of fd {}"}.Apply(mode, fd.Get<Int>()));

    switch(mode) {
    case EWaiting:
      if (_Read.find(sk) != _Read.end()) {
        return ENoError;
      }
      break;

    case ELooping:
      if (_Write.find(sk) != _Read.end()) {
        return ENoError;
      }
      break;

    default:
      break;
    }

    return ENoSupport;
  }

  /* @NOTE: this method is called automatically after passing the checking step,
   * it would use to pick handling callback which help to solve the specific events
   */
  Perform* OnSelecting(Auto& fd, Int mode) {
    return &(mode == EWaiting? _Read[fd.Get<Int>()]: _Write[fd.Get<Int>()]);
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
          if (error == ENoSupport) {
            return (*job)(Auto::As(socket), _Entries[socket]);
          } else {
            return error;
          }
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
          if (error == ENoSupport) {
            return (*job)(Auto::As(socket), _Entries[socket]);
          } else {
            return error;
          }
        }
      }
    }

    return passed? ENoError: NotFound(Format{"fd {}"} << socket).code();
  }

  /* @NOTE: this function is called by callback Trigger when the fd is on the
   * Looping events */
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

    return passed? ENoError: NotFound(Format{"fd {}"} << socket).code();
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

    return passed? ENoError: NotFound(Format{"fd {}"} << socket).code();
  }

  Map<Int, Auto> _Entries;
  Map<Int, Perform> _Read, _Write;
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
    return (Int)BadAccess("ptr is None").code();
  }
}

Int Heartbeat(Void* ptr, Int socket) {
  Pool* pool = reinterpret_cast<Pool*>(ptr);

  if (pool) {
    return (Int)(reinterpret_cast<class Fildes*>(pool->Pool))
      ->Heartbeat(Auto::As<Int>(socket));
  } else {
    return (Int)BadAccess("ptr is None").code();
  }
}

Int Remove(Void* ptr, Int fd) {
  Pool* pool = reinterpret_cast<Pool*>(ptr);

  if (pool) {
    return (reinterpret_cast<class Fildes*>(pool->Pool))->OnRemoving(fd);
  } else {
    return (Int)BadAccess("ptr is None").code();
  }
}

Int Flush(Void* ptr, Int fd) {
  Pool* pool = reinterpret_cast<Pool*>(ptr);

  if (pool) {
    auto fildes = (reinterpret_cast<class Fildes*>(pool->Pool));

    if (pool->ll.Probe) {
      if (pool->ll.Probe(pool->ll.Poll, fd, EWaiting) > 0) {
        return fildes->OnWaiting(fd);
      }
    } else if (fildes->IsWaiting(fd) > 0) {
      return fildes->OnWaiting(fd);
    }

    return 0;
  } else {
    return (Int)BadAccess("ptr is None").code();
  }
}
} // namespace Fildes
} // namespace Internal
} // namespace Base
