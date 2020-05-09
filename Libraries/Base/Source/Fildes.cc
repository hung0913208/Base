#include <Atomic.h>
#include <Exception.h>
#include <Macro.h>
#include <Monitor.h>
#include <Logcat.h>
#include <Lock.h>
#include <Vertex.h>

#include <unistd.h>

#define INIT 0
#define IDLE 1
#define RUNNING 2
#define INTERRUPTED 3
#define RELEASING 4
#define PANICING 5

using TimeSpec = struct timespec;

extern "C" {
typedef struct Pool {
  Void* Pool;
  Int Status, *Referral;

  struct {
    Void* Poll;

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
  Void* (*Build)(struct Pool* pool); 
} Pool;

enum Mode {
  EWaiting = 0,
  ELooping = 1,
  ERelease = 2
};

typedef Int (*Handler)(Pool*, Int, Int);

#if LINUX
Handler EPoll(Pool* pool, Int backlog);
Handler Poll(Pool* pool);
Handler Select(Pool* pool);
#elif MACOS || BSD
Handler KQueue(Pool* pool);
Handler Poll(Pool* pool);
Handler Select(Pool* pool);
#elif WINDOW
Handler IOStat(Pool* pool);
Handler Select(Pool* pool);
#endif
}

namespace Base {
namespace Internal {
Mutex* CreateMutex();

static Vertex<Mutex, True> Secure([](Mutex* mutex) { Locker::Lock(*mutex); },
                                  [](Mutex* mutex) { Locker::Unlock(*mutex); },
                                  CreateMutex());
void Idle(TimeSpec* spec);
Bool IsPipeAlive(Int pipe);
Bool IsPipeWaiting(Int pipe);
ULong GetUniqueId();

namespace Fildes {
Int Trigger(Void* ptr, Int fd, Bool reading);
Int Heartbeat(Void* ptr, Int* fd);
Int Remove(Void* ptr, Int fd);
Int Flush(Void* ptr, Int fd);
} // namespace Fildes
} // namespace Internal

class Fildes: public Monitor {
 public:
#if LINUX
  explicit Fildes(String name, UInt type, Int system, Int backlog = 100) :
#else
  explicit Fildes(String name, UInt type, Int system) :
#endif
      Monitor(name, type), _Tid{-1} {
    using namespace std::placeholders;  // for _1, _2, _3...

    Vertex<void> escaping{[&]() { 
    }, [&]() { 
      if (SwitchTo(EStarted)) {
        Bug(EBadLogic, "can\'t switch to EStarted"); 
      } 
    }};
    TimeSpec spec{.tv_sec=0, .tv_nsec=0};
    Bool retry{False};
    ULong timeout{1};

    while(!Attach()) {
      spec.tv_nsec = timeout = (timeout * 2) % ULong(1e9);

      Internal::Idle(&spec);
    }

    do {
      retry = False;

      memset(&_Pool, 0, sizeof(_Pool));

      _Pool.Heartbeat = Base::Internal::Fildes::Heartbeat;
      _Pool.Trigger = Base::Internal::Fildes::Trigger;
      _Pool.Remove = Base::Internal::Fildes::Remove;
      _Pool.Flush = Base::Internal::Fildes::Flush;

      Internal::Secure.Circle([&]() {
        auto a = Head();
        auto b = dynamic_cast<Monitor*>(this);

        if (Head() == dynamic_cast<Monitor*>(this)) {
          switch (system) {
          case 0:
#if LINUX
            _Pool.Run = EPoll(&_Pool, backlog);
#elif MACOS || BSD
            _Pool.Run = KQueue(&_Pool);
#elif WINDOW
            _Pool.Run = IOStat(&_Pool);
#else
            throw Except(ENoSupport, "");
#endif
            break;

          case 1:
#if LINUX
            _Pool.Run = Poll(&_Pool);
#elif MACOS || BSD
            _Pool.Run = Poll(&_Pool);
#elif WINDOW
            _Pool.Run = Select(&_Pool);
#else
            throw Except(ENoSupport, "");
#endif
            break;

          case 2:
#if UNIX
            _Pool.Run = Select(&_Pool);
#else
            throw Except(ENoSupport, "");
#endif
            break;
          }

          if (!_Pool.Run) {
            throw Except(EDrainMem, "can\'t allocate memory to polling system");
          }

          DEBUG(Format("Register lowlevel APIs pointer={}").Apply(
              ULong(_Pool.ll.Poll)));
      
          if (!_Pool.Referral) {
            _Pool.Referral = (Int*)ABI::Calloc(1, sizeof(Int));
          }
        } else {
          Monitor* head{Head()};

          if (head && head->Context() && head->State() == EStarted) {
            Pool* pool = (Pool*)head->Context();

            _Pool.Referral = pool->Referral;
            _Pool.ll = pool->ll;
            _Pool.Run = pool->Run;

            if (!_Pool.ll.Append || !_Pool.ll.Modify || !_Pool.ll.Probe) {
              retry = True;
            } else {
              DEBUG(Format("Mitigate lowlevel with referral {}").Apply(
                      ULong(dynamic_cast<Fildes*>(Head()))));
            }
          } else {
           retry = True;
          }
        }
      
        if (!retry) {
          _Shared = &_Pool;
          
          /* @NOTE: increase referral counter to prevent removing our pool
           * to soon */
          INC(_Pool.Referral);
        }

      });

      timeout = (timeout * 2) % ULong(1e9);

      if (retry) {
        spec.tv_nsec = timeout;

        /* @NOTE: wait the head to be activated so we can access Head's 
         * _Pool without locking. This is the good choice since Monitor 
         * can't communicate so it can't know if the head is switched or 
         * not to prevent accessing None in dynamic_cast */

        Internal::Idle(&spec);
      }
    } while(retry);

    /* @NOTE: the initing state is done, now switch to starting state to allow
     * children to be joined */
    if (SwitchTo(EStarting)) {
      Bug(EBadLogic, "can\'t switch to EStarting"); 
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

  ~Fildes() {
    TimeSpec spec{.tv_sec=0, .tv_nsec=0};
    ULong timeout{1};

    if (SwitchTo(Monitor::EStopping)) {
      Bug(EBadLogic, "can\'t switch to EStopping");
    }

    while (!Detach()) {
      spec.tv_nsec = timeout = (timeout * 2) % ULong(1e9);

      Internal::Idle(&spec);
    }
  }

 protected:
  friend Int Internal::Fildes::Trigger(Void* ptr, Int fd, Bool reading);
  friend Int Internal::Fildes::Heartbeat(Void* ptr, Int* fd);
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
          if ((error = _Append(Auto::As(fork.Error()), EWaiting))) {
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

  /* @NOTE: this function is used to enqueue callback into our pipeline, waiting
   * to be handled asynchronously */
  ErrorCodeE _Route(Auto fd, Perform& callback) {
    return ENoSupport;
  }

  /* @NOTE: this function is used to append a new fd to polling system */
  ErrorCodeE _Append(Auto fd, Int mode) {
    if (_Pool.Status == PANICING) {
      if ((_Pool.ll.Poll = _Pool.Build(&_Pool))) {
        _Pool.Status = IDLE;
      }
    }

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
    if (_Pool.Status == PANICING) {
      if ((_Pool.ll.Poll = _Pool.Build(&_Pool))) {
        _Pool.Status = IDLE;
      }
    }

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
        Vertex<void> escaping{[&]() { Monitor::Lock(_Type); },
                              [&]() { Monitor::Unlock(_Type); }};
        ErrorCodeE result = ENoError;
        Fildes* next = this;

        /* @NOTE: check from children, we wouldn't know it without checking
         * them because some polling system don't support checking how many
         * fd has waiting */

        while (!result) {
          next = dynamic_cast<Fildes*>(next->_Next);

          if (next) {
            Vertex<void> escaping{[&]() { 
              next->Claim();
              Monitor::Unlock(_Type);
            }, [&]() { 
              if (dynamic_cast<Fildes*>(next)->_Entries.size() > 0) {
                result = EInterrupted;
              }

              next->Done(); 
              Monitor::Lock(_Type); 
            }};
          } else {
            break;
          }
        }

        return result;
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
    Fildes* fildes = dynamic_cast<Fildes*>(child);

    if (!fildes) {
      return BadLogic("child should be Fildes").code();
    }

    if (fildes->_Tid < 0) {
      Vertex<void> escaping{[&](){ fildes->_Tid = Internal::GetUniqueId(); },
                            [&](){ fildes->_Tid = -1; }};

      if (fildes->_Pool.Status == PANICING) {
        if ((fildes->_Pool.ll.Poll = fildes->_Pool.Build(&fildes->_Pool))) {
          fildes->_Pool.Status = IDLE;
        }
      }
      
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

      return (ErrorCodeE) fildes->_Pool.Run(&fildes->_Pool, timeout, backlog);
    } else {
      return BadAccess("Monitor is still on  initing").code();
    }
  }

  ErrorCodeE _Handle(Monitor* child, Int timeout, Int backlog = 100) final {
    return ENoSupport;
  }

 private:
  Bool IsIdle(Monitor** next) {
    if (next) {
      *next = _Next;
    }

    if (_Entries.size() > 0) {
      return False;
    } else {
      return True;
    }
  }

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
    Vector<Pair<Monitor*, Monitor::Perform*>> jobs{};
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
        if ((error = Reroute(job.Left, fd, *job.Right))) {
          if (error == ENoSupport) {
            return (*job.Right)(Auto::As(socket), _Entries[socket]);
          } else {
            return error;
          }
        }
      }

      return ENoError;
    }

    return passed? ENoError: NotFound(Format{"fd {}"} << socket).code();
  }

  /* @NOTE: this function is called by callback Trigger when the fd is on the
   * Looping events */
  ErrorCodeE OnLooping(Int socket) {
    Vector<Pair<Monitor*, Monitor::Perform*>> jobs{};
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
        if ((error = Reroute(this, fd, *job.Right))) {
          return error;
        }
      }
    }

    return passed? ENoError: NotFound(Format{"fd {}"} << socket).code();
  }

  Void _Flush() final { }

  Bool _Clean() final {
    if (!DEC(_Pool.Referral)) {
      ABI::Free(_Pool.Referral);
      DEBUG(Format("Release lowlevel APIs pointer={}").Apply(ULong(_Pool.ll.Poll)));

      if (_Pool.ll.Poll) {
        if (_Pool.ll.Release) {
          if (_Pool.ll.Release(&_Pool, -1)) {
            WARNING << "It seems the lowlevel poll can't release itself. "
                        "It may cause memory leak it some point and we can't "
                        "control it propertly"
                    << Base::EOL;
            }
        }

        if (_Pool.ll.Poll) {
          ABI::Free(_Pool.ll.Poll);
        }
      }
    }

    return True;
  }

  ErrorCodeE OnRemoving(Int socket) {
    Vertex<void> escaping{[&]() { Monitor::Lock(_Type); },
                          [&]() { Monitor::Unlock(_Type); }};
    Auto fd{Auto::As<Int>(socket)};
    Bool passed{False};
    ErrorCodeE result{ENoError};
    Fildes* next{this};

    if ((passed = !Find(fd))) {
      _Entries.erase(socket);
    }

    /* @NOTE: check from children, we wouldn't know it without checking
     * them because some polling system don't support checking how many
     * fd has waiting */

    while (!result) {
      next = dynamic_cast<Fildes*>(next->_Next);

      if (next) {
        Vertex<void> escaping{[&]() { 
          next->Claim();
          Monitor::Unlock(_Type);
        }, [&]() { 
          if (!next->Find(fd)) {
            if (!next->Remove(fd)) {
              passed = True;
            }
          }

          next->Done(); 
          Monitor::Lock(_Type); 
        }};
      } else {
        break;
      }
    }

    return passed? ENoError: NotFound(Format{"fd {}"} << socket).code();
  }

  Map<Int, Auto> _Entries;
  Map<Int, Perform> _Read, _Write;
  Long _Tid;
  Pool _Pool;
};

namespace Internal {
namespace Fildes {
Bool Create(String name, UInt type, Int system, Monitor** result){
  if (system < 2 && result) {
    (*result) = new Base::Fildes(name, type, system);

    DEBUG(Format{"Allocate {}"}.Apply(name));
    if (*result) {
      return True;
    } else {
      return False;
    }
  } else {
    return False;
  }
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

Int Heartbeat(Void* ptr, Int* socket) {
  Pool* pool = reinterpret_cast<Pool*>(ptr);

  if (pool) {
    if (*socket > 0) {
      return (Int)(reinterpret_cast<class Fildes*>(pool->Pool))
        ->Heartbeat(Auto::As<Int>(*socket));
    } else {
      return ENoError;
    }
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
