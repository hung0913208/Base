#include <Monitor.h>
#include <Vertex.h>
#include <Lock.h>
#include <Atomic.h>
#include <Utils.h>
#include <Vertex.h>

namespace Base {
namespace Internal {
Mutex* CreateMutex();

static Map<UInt, Shared<Lock>> MLocks;
static Map<UInt, Monitor*> Currents;
static Map<UInt, Pair<UInt, UInt>> Counters;
static Map<UInt, Pair<Monitor*, Monitor*>> Monitors;
static Map<UInt, Bool (*)(String, UInt, Monitor**)> Builders;
static Vertex<Mutex, True> Secure([](Mutex* mutex) { Locker::Lock(*mutex); },
                                   [](Mutex* mutex) { Locker::Unlock(*mutex); },
                                   CreateMutex());

namespace Fildes {
Bool Create(String name, UInt type, Int system, Monitor** result);
} // namespace Fildes
} // namespace Internal

Monitor::Monitor(String name, UInt type): _Name{name}, _Type{type}, 
    _Shared{None}, _PPrev{None}, _Next{None}, _State{0}, _Using{0},
    _Attached{False}, _Detached{False} { 
  using namespace Internal;

  Secure.Circle([&]() {
    if (Counters.find(_Type) == Counters.end()) {
      Counters[_Type] = Pair<UInt, UInt>(0, 0);
    }

    INC(&Counters[_Type].Left);
  });
}

Monitor::~Monitor() {
  using namespace Internal;

  if (_Attached) {
    if (_Detached) {
      if (_State != EStopped) {
        if (SwitchTo(EStopped)) {
          Bug(EBadLogic, "can\'t switch to EStopped"); 
        }
      }
    } else {
      Detach();
    }

    if (!_Detached) {
      Bug(EBadLogic, "can\'t detach our Monitor as expected");
    }
  }    

  /* @NOTE: we decrease this counter because this is used to statistic how many
   * Monitor are existed inside the system */

  DEC(&Counters[_Type].Left);
}

ErrorCodeE Monitor::Append(Auto fd, Int mode) {
  using namespace Internal;

  Vertex<Mutex, False> UNUSED(guranteer) = Secure.generate();
  Vertex<void> escaping{[&]() { Monitors[_Type].Left = this; },
                        [&]() { Monitors[_Type].Left = None; }};

  if (_Find(fd)) {
    return _Append(fd, mode);
  } else {
    return DoNothing(Format{"fd {} are redefined"} << fd).code();
  }

  return ENoError;
}

ErrorCodeE Monitor::Modify(Auto fd, Int mode) {
  using namespace Internal;

  Vertex<Mutex, False> UNUSED(guranteer) = Secure.generate();
  Vertex<void> escaping{[&]() { Monitors[_Type].Left = this; },
                        [&]() { Monitors[_Type].Left = None; }};

  if (!_Find(fd)) {
    return _Modify(fd, mode);
  } else {
    return NotFound(Format{"fd {}"} << fd).code();
  }
}

ErrorCodeE Monitor::Find(Auto fd) {
  return _Find(fd);
}

ErrorCodeE Monitor::Remove(Auto fd) {
  using namespace Internal;

  auto result = ENoError;

  MLocks[_Type]->Safe([&]() {
    Vertex<void> escaping{[&]() { Monitors[_Type].Left = this; },
                          [&]() { Monitors[_Type].Left = None; }};

    if (!_Find(fd)) {
      result = _Remove(fd);
    } else {
      result = NotFound(Format{"fd {}"} << fd).code();
    }
  });

  return result;
}

ErrorCodeE Monitor::Trigger(Auto event, Perform perform) {
  using namespace Internal;
  /* @NOTE: this method should be called by user to register a new
   * event-callback, the event will be determined by monitor itself and can
   * be perform automatically with the load-balancing system */

  auto result = ENoError;

  MLocks[_Type]->Safe([&]() {
    Vertex<void> escaping{[&]() { Monitors[_Type].Left = this; },
                          [&]() { Monitors[_Type].Left = None; }};

    result = _Trigger(event, perform);
  });

  return result;
}

void Monitor::Registry(Check check, Indicate indicate) {
  if (_Indicators.find(GetAddress(check)) == _Indicators.end()) {
    _Checks.push_back(check);
  }

  _Indicators[GetAddress(check)] = indicate;
}

void Monitor::Heartbeat(Fallback fallback) {
  _Fallbacks.push_back(fallback);
}

ErrorCodeE Monitor::Status(String name) {
  using namespace Internal;

  if (name == "active") {
    auto UNUSED(guranteer) = Secure.generate();

    return Monitors[_Type].Left == this? ENoError: EInterrupted;
  } else {
    return Head(_Type)->_Status(name, Auto::As<Int>(-1));
  }
}

ErrorCodeE Monitor::Scan(Auto fd, Int mode,
                         Vector<Pair<Monitor*, Perform*>>& callbacks) {
  return ScanImpl(fd, mode, True, callbacks);
}

ErrorCodeE Monitor::ScanImpl(Auto fd, Int mode, Bool heading,
                         Vector<Pair<Monitor*, Perform*>>& callbacks) {
  using namespace Internal;

  ErrorCodeE error = ENoError;
  Monitor* next = None;
  Bool passed = False;

  /* @NOTE: scan throught events and collect callbacks, i assume that scan
   * functions should run very fast but callbacks may run quiet slow so we
   * don't invoke it here. Instead, i will do them later on differnet */

  if (heading && this != Head(_Type)) {
    return BadAccess("child shouldn\'t call method scan").code();
  } else if (_Find(fd)) {
    return ENotFound;
  }

  if (!ScanIter(fd, mode, &next, callbacks)) {
    passed = True;
  }

  MLocks[_Type]->Safe([&]() {
    MCOPY(&next, &_Next, sizeof(_Next));

    try {
      while (next) {
        if (!next->ScanIter(fd, mode, &next, callbacks)) {
          passed = True;
        }
      }
    } catch (Base::Exception& except) {
      error = except.code();
    }
  });

  return passed? error: ENotFound;
}

ErrorCodeE Monitor::ScanIter(Auto fd, Int mode, Monitor** next,
                             Vector<Pair<Monitor*, Perform*>>& callbacks) {
  Bool passed = False;

  /* @NOTE: i assume we are in safe zone, or we might face core dump somewhere
   * here or when we remove the child while the head access it on parallel */

  if (_State != EStarted) {
    return BadAccess(Format{"_State({}) != EStarted"}.Apply(_State)).code();
  }

  for (auto& check: _Checks) {
    if (!check(fd, _Access(fd), mode)) {
      auto callback = _Indicators[GetAddress(check)](fd, mode);

      if (callback) {
        if (Claim()) {
          Bug(EBadLogic, "can\'t claim a new job as expected");
        }

        callbacks.push_back(Pair<Monitor*, Perform*>(this, callback));
        passed = True;

        Done();
      }
    }
  }

  if (next) {
    *next = _Next;
  }

  return passed? ENoError: ENotFound;
}

ErrorCodeE Monitor::Heartbeat(Auto fd) {
  ErrorCodeE error{ENoError};

  if (_Find(fd)) {
    Vertex<void> escaping{[&]() { Monitor::Lock(_Type); },
                          [&]() { Monitor::Unlock(_Type); }};
    Monitor* next = this;

    /* @NOTE: check from children, we wouldn't know it without checking
     * them because some polling system don't support checking how many
     * fd has waiting */

    while (!error) {
      next = next->_Next;

      if (next) {
        Vertex<void> escaping{[&]() { 
          next->Claim();
          Monitor::Unlock(_Type);
        }, [&]() { 
          if (next->_Find(fd)) {
            for (auto fallback: next->_Fallbacks) {
              if ((error = fallback(fd))) {
                break;
              }
            }
          }

          next->Done(); 
          Monitor::Lock(_Type); 
        }};
      } else {
        break;
      }
    }

    return error;
  } else {
    for (auto fallback: _Fallbacks) {
      if ((error = fallback(fd))) {
        return error;
      }
    }
  }

  return ENoError;
}

ErrorCodeE Monitor::Reroute(Monitor* child, Auto fd, Perform& callback) {
  auto error = child->_Route(fd, callback);

  if (error) {
#if DEV
    /* @TODO: try to use idle threads to handle callback asynchronously in
     * background. With that, i believe we can reuse WatchStopper better with
     * provided resource */

#endif
  }

  return error;
}

ErrorCodeE Monitor::Claim(Bool safed) {
  ErrorCodeE result{ENoError};
  
  if (safed) {
    if (!_Detached && _State < EStopping) {
      _Using++;
    } else {
      result = BadAccess(Format{"_State != {}"}.Apply(_State)).code();
    }
  } else {
    Internal::MLocks[_Type]->Safe([&]() {
      if (!_Detached && _State < EStopping) {
        _Using++;
      } else {
        result = BadAccess(Format{"_State != {}"}.Apply(_State)).code();
      }
    });
  }

  return result;
}

Bool Monitor::Done() {
  Bool result = True;

  Internal::MLocks[_Type]->Safe([&]() {
    if (_Using > 0) {
      _Using--;
    } else {
      result = False;
    }
  });

  return result;
}

UInt Monitor::State() {
  return _State;
}

Void* Monitor::Context() {
  return _Shared;
}

Void Monitor::Devote() {
  using namespace Internal;

  auto head = Monitor::Head(_Type);
  auto thiz = this;

  MLocks[_Type]->Safe([&]() {
    if (head == None) {
      Secure.Circle([&]() { 
        Monitors[_Type].Right = this; 
        Currents[_Type] = this;

        _Next = None;
        _PPrev = None;
        _Children = new List();
      });

      _Index = _Children->Add(this);
    } else if (head->_Children != _Children) {
      _Children = head->_Children;
      _Index = _Children->Add(this);

      Secure.Circle([&]() { 
        _PPrev = &Currents[_Type];
        Currents[_Type] = this;

        MCOPY(_PPrev, &thiz, sizeof(this));
      });
    }
  });
}

Void Monitor::Attach() {
  using namespace Internal;

  auto UNUSED(guranteer) = Secure.generate();
  auto is_new_one = Monitors.find(_Type) == Monitors.end();
  auto thiz = this;

  if (_Attached) {
    goto finish;
  }

  /* @NOTE: C++ use a vtable to control which function is called according the
   * class so it might have time when the Monitor is creating and we would hit
   * virtual methods while it isn't defined and cause corrupted. We should store
   * these requests to a list and solve them later */

  if (is_new_one || Monitors[_Type].Right == None) {
    Monitors[_Type] = Pair<Monitor*, Monitor*>(None, this);

    if (is_new_one) {
      MLocks[_Type] = std::make_shared<Base::Lock>();
    }

    _Children = new List();
  } else {
    /* @NOTE: always add to the end of the list, with that, we can trace which
     * one should be the next */

    _PPrev = &Currents[_Type];
    _Children = Head(_Type)->_Children;

    MCOPY(_PPrev, &thiz, sizeof(this));
  }

  if ((_Index = Head(_Type)->_Children->Add(this))) {
    Currents[_Type] = this;
  } else {
    if (is_new_one) {
      delete _Children;
    }

    throw Except(EBadAccess, "can\'t create new Monitor");
  }

  /* @NOTE: increase the counter to detech how many children are attaching so
   * we can use this value to consider when we remove our Monitor system */

  INC(&Counters[_Type].Right);

finish:
  _Attached = True;
}

Void Monitor::Detach() {
  using namespace Internal;

  auto UNUSED(guranteer) = Secure.generate();
  auto is_child = False;
  auto is_latest = DEC(&Counters[_Type].Right) == 0;

  /* @NOTE: if this is the latest monitor of the branch, we should remove it.
   * Otherwide, we will choose a child to become the head of this brand */

  if (!_Detached) {
    auto head = Head(_Type);

    if (!CMP(&head, this)) {
      /* @NOTE: only CHILD can reach here and we should turn on this flag bellow
       * to apply action which only happen in child Monitor */

      is_child = True;

detach: 
      /* @TODO: we should finish jobs before removing our child to prevent race
       * condition here */

      if (CMP(&_State, EStopping)) {
        if (SwitchTo(EStopped)) {
          Bug(EBadLogic, "can\'t switch to EStopped"); 
        }
      } else {
        _Detached = True;
      }

      MLocks[_Type]->Safe([&]() {
        _Flush();
      });

      if (CMP(&_Using, 0)) {
        head = Head(_Type);

        /* @NOTE: we should detect the child and remove it, if not, throw an
         * exception because this is a bug */

        if (head) {
          if (!head->_Children->Del(_Index, this)) {
            Notice(EBadAccess, Format{"Race condition, this={}"}.Apply(ULong(this)));
          }
        }

        /* @NOTE: this is abit trick from Linux kernel and we should follow
         * this structure to improve performance, consider to build a struct
         * later to unify this one better */

        MLocks[_Type]->Safe([&]() {
          auto next = _Next;
          auto pprev = _PPrev;

          if (pprev && next) {
            MCOPY(pprev, &next, sizeof(next));
          }
        
          if (next) {
            next->_PPrev = pprev;
          }
        });

        if (!is_child || !Head(_Type)) {
          /* @NOTE: if it's not CHILD, we should stop here and jump to the
           * tag finish since everything is done now */

          goto finish;
        }

        if (!is_latest) {
          /* @NOTE: we only do this code if we aren't the lastest one on the
           * group recently to prevent defer to a NULL pointer here which is
           * caused by the step `clean` bellow */

          MLocks[_Type]->Safe([&]() {
            if (CMP(&(Head(_Type)->_Next), this)) {
              MCOPY(&(Head(_Type)->_Next), &_Next, sizeof(_Next));
            }
          });
        }
      } else {
        Notice(EBadLogic, Format{"Still have {} pending job(s)"}.Apply(_Using));
      }
    } else if (!is_latest) {
      /* @NOTE: head is always choosen as the next one so we can keep track the
       * status of monitors with-in the same type */
 
      MLocks[_Type]->Safe([&]() {
        if (CMP(&head, this)) {
          MCOPY(&Monitors[_Type].Right, &_Next, sizeof(_Next));
        }
      });

      goto detach;
    } else {
      head = (Monitor*)None;

      /* @NOTE: we are safe here, just secure our head only or we might face core
       * dump here or the place we create a new Monitor type */

      if (_Clean()) {
        MLocks[_Type]->Safe([&]() {
          MCOPY(&Monitors[_Type].Right, &head, sizeof(Monitor*));
        });
      }
      goto detach;
    }

finish:
    _Detached = True;

    if (is_latest) {
      /* @NOTE: remove everything if we are the last one. To prevent any
       * unexpected behavior, we should check None using Atomic before delete
       * this list */

      delete _Children;
    }
  }
}

ErrorCodeE Monitor::SwitchTo(UInt state) {
  if (state > _State) {
    _State = state;
  } else if (state == _State) {
    return DoNothing("switch to the same state").code();
  } else {
    return BadLogic(Format{"can\'t switch {} -> {}"}.Apply(_State, state))
            .code();
  }

  return ENoError;
}

ErrorCodeE Monitor::Loop(Function<Bool(Monitor&)> status, Int timeout) {
  ErrorCodeE error = EBadAccess;

  do {
    if (IsHead(_Type, dynamic_cast<Monitor*>(this))) {
      error = _Interact(this, timeout);
    } else {
      error = _Handle(this, timeout);
    }
  } while (status(*this) || error == EBadAccess || error == EDoAgain);

  return error;
}

ErrorCodeE Monitor::Loop(UInt steps, Int timeout) {
  return Loop([&](Monitor&) -> Bool { return (--steps) > 0; }, timeout);
}

Shared<Monitor> Monitor::Make(String name, UInt type) {
  Monitor* result{None};

  switch(type) {
  case EIOSync:
    if (Internal::Fildes::Create(name, type, 0, &result)) {
      return Shared<Monitor>(result);
    } else {
      return None;
    }

  case EPipe:
    if (Internal::Fildes::Create(name, type, 1, &result)) {
      return Shared<Monitor>(result);
    } else {
      return None;
    }

  default:
    if (Internal::Builders.find(type) == Internal::Builders.end()) {
      throw Except(ENoSupport, name);
    } else if (Internal::Builders[type](name, type, &result)) {
      return Shared<Monitor>(result);
    } else {
      return None;
    }
  }
}

Bool Monitor::Sign(UInt type, Bool (*builder)(String, UInt, Monitor**)) {
  using namespace Base::Internal;

  if (type == EIOSync || type == EPipe || type == ETimer) {
    return False;
  } else if (Builders.find(type) != Builders.end()) {
    return False;
  }
    
  Builders[type] = builder;
  return True;
}

Bool Monitor::IsSupport(UInt type) {
  switch(type) {
  case EIOSync:
  case EPipe:
    return True;

  case ETimer:
    return False;

  default:
    return Internal::Builders.find(type) != Internal::Builders.end();
  }
}

Bool Monitor::IsHead(UInt type, Monitor* sample) {
  if (Internal::Monitors.find(type) == Internal::Monitors.end()) {
    return False;
  } else {
    return CMP(&Internal::Monitors[type].Right, sample);
  }
}

Monitor* Monitor::Head(UInt type) {
  Monitor* result{None};

  if (Internal::Monitors.find(type) != Internal::Monitors.end()) {
    Internal::MLocks[type]->Safe([&]() {
      result = Internal::Monitors[type].Right;
    });
  }

  return result;
}

void Monitor::Lock(UInt type) {
  if (Internal::MLocks.find(type) != Internal::MLocks.end()) {
    (*Internal::MLocks[type])(True);
  }
}

void Monitor::Unlock(UInt type) {
  if (Internal::MLocks.find(type) != Internal::MLocks.end()) {
    (*Internal::MLocks[type])(False);
  }
}

ErrorCodeE Monitor::_Append(Auto UNUSED(fd), Int UNUSED(mode)) {
  if (_State) {
    return ENoSupport;
  } else {
    Internal::MLocks[_Type]->Safe([&]() {
      _Pipeline.push_back(Monitor::Action{EAppend, fd, mode});
    });
    return ENoError;
  }
}

ErrorCodeE Monitor::_Modify(Auto fd, Int mode) {
  if (_State) {
    return ENoSupport;
  } else {
    Internal::MLocks[_Type]->Safe([&]() {
      _Pipeline.push_back(Monitor::Action{EModify, fd});
    });
    return ENoError;
  }
}

ErrorCodeE Monitor::_Find(Auto UNUSED(fd)) {
  if (_State) {
    return ENoSupport;
  } else {
    Internal::MLocks[_Type]->Safe([&]() {
      _Pipeline.push_back(Monitor::Action{EFind, fd});
    });
    return ENoError;
  }
}

ErrorCodeE Monitor::_Remove(Auto UNUSED(fd)) {
  if (_State) {
    return ENoSupport;
  } else {
    Internal::MLocks[_Type]->Safe([&]() {
      _Pipeline.push_back(Monitor::Action{ERemove, fd});
    });
    return ENoError;
  }
}

Monitor::Action::Action(ActTypeE type, Auto fd, Int mode) {
  this->type = type;

  switch(type) {
  case Monitor::EAppend:
  case Monitor::EModify:
    this->fd = fd;
    this->mode = mode;
    break;

  default:
    throw Except(ENoSupport, "can't recognine your action");
  }
}

Monitor::Action::Action(ActTypeE type, Auto fd) {
  this->type = type;

  switch(type) {
  case Monitor::EFind:
  case Monitor::ERemove:
    this->fd = fd;
    break;

  default:
    throw Except(ENoSupport, "can't recognine your action");
  }
}
} // namespace Base
