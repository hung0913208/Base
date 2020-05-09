#include <Monitor.h>
#include <Vertex.h>
#include <Lock.h>
#include <Atomic.h>
#include <Utils.h>
#include <Vertex.h>

#include <unistd.h>

using TimeSpec = struct timespec;

namespace Base {
namespace Internal {
Mutex* CreateMutex();

static Map<UInt, Lock> MLocks;
static Map<UInt, Pair<Monitor*, Monitor*>> Monitors;
static Map<UInt, Bool (*)(String, UInt, Monitor**)> Builders;
static Vertex<Mutex, True> Secure([](Mutex* mutex) { Locker::Lock(*mutex); },
                                   [](Mutex* mutex) { Locker::Unlock(*mutex); },
                                   CreateMutex());

void CreateIfNeeded(UInt type) {
  Secure.Circle([&]() {
    if (Monitors.find(type) == Monitors.end()) {
      Monitors[type] = Pair<Monitor*, Monitor*>(None, None);
      MLocks[type] = Lock();
    }
  });
}

void Idle(TimeSpec* spec);

namespace Fildes {
Bool Create(String name, UInt type, Int system, Monitor** result);
} // namespace Fildes
} // namespace Internal

Monitor::Monitor(String name, UInt type): _Name{name}, _Type{type}, 
    _Shared{None}, _PNext{None}, _PLast{None}, _Head{None}, _Last{None}, 
    _Next{None}, _Prev{None},  _State{0}, _Using{0}, _Attached{False}, 
    _Detached{False} { 
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
}

ErrorCodeE Monitor::Append(Auto fd, Int mode) {
  using namespace Internal;

  if (_Find(fd)) {
    return _Append(fd, mode);
  } else {
    return DoNothing(Format{"fd {} are redefined"} << fd).code();
  }

  return ENoError;
}

ErrorCodeE Monitor::Modify(Auto fd, Int mode) {
  using namespace Internal;

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

  if (!_Find(fd)) {
    return _Remove(fd);
  } else {
    return NotFound(Format{"fd {}"} << fd).code();
  }
}

ErrorCodeE Monitor::Trigger(Auto event, Perform perform) {
  using namespace Internal;
  /* @NOTE: this method should be called by user to register a new
   * event-callback, the event will be determined by monitor itself and can
   * be perform automatically with the load-balancing system */

  return _Trigger(event, perform);
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
    return Head()->_Status(name, Auto::As<Int>(-1));
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

  if (heading && this != Head()) {
    return BadAccess("child shouldn\'t call method scan").code();
  } else if (_Find(fd)) {
    return ENotFound;
  }

  if (!ScanIter(fd, mode, &next, callbacks)) {
    passed = True;
  }

  MLocks[_Type].Safe([&]() {
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
  ErrorCodeE result{ENoError};

  if (_Find(fd)) {
    ForEach([&](Monitor* next) -> ErrorCodeE {
      if ((next)->_Find(fd)) {
        for (auto fallback: next->_Fallbacks) {
          result = fallback(fd);
        }
      }

      return result;
    });

    return result;
  } else {
    for (auto fallback: _Fallbacks) {
      if ((result = fallback(fd))) {
        return result;
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

ErrorCodeE Monitor::Claim(UInt retry) {
  ErrorCodeE result{EDoAgain};

  do {  
    BARRIER();

    if (CMP(&_State, EStarting)) { 
      retry--;
    } else if (!CMP(&_State, EStarted)) {
      result = EBadAccess;
    } else {
      INC(&_Using);
      goto done;
    }
  } while (!result && retry > 0);

  return result;

done:
  return ENoError;
}

ErrorCodeE Monitor::Done() {
  ErrorCodeE result{ENoError};

  if (CMP(&_State, EStopping) || CMP(&_State, EStarted)) {
    DEC(&_Using);
  } else {
    result = EBadAccess;
  }

  return result;
}

UInt Monitor::State() {
  return _State;
}

Void* Monitor::Context() {
  return _Shared;
}

Monitor* &Monitor::Head() {
  return Internal::Monitors[_Type].Right;
}

Bool Monitor::Attach(UInt retry) {
  using namespace Internal;

  auto thiz = this;
  auto plock = &Monitors[_Type].Left;
  auto touched = False;

  if (!CMP(&_State, EOffline)) {
    return True;
  }
  
  CreateIfNeeded(_Type);

  do {
    BARRIER();

    if (LIKELY(Head(), None)) {
      if (CMPXCHG(plock, None, this)) {
        _PLast = &_Last;

        MCOPY(&(Monitors[_Type].Right), &thiz, sizeof(this)); 
        MCOPY(&_Head, &thiz, sizeof(this));
        MCOPY(&_Last, &thiz, sizeof(this));

        touched = True;
        break;
      }
    } else if (CMPXCHG(plock, None, this)) {
      _Head = Head();

      if (!CMP(Head()->_PLast, None)) {
        /* @NOTE: always add to the end of the list, with that, we can trace which
         * one should be the next */

        _PNext = &((*Head()->_PLast)->_Next);
        _PLast = Head()->_PLast;

        MCOPY(_PLast, &thiz, sizeof(this));
        MCOPY(_PNext, &thiz, sizeof(this));
  
        /* @NOTE: this parameter is use to revert to the previous in the case the 
         * latest is released */

         MCOPY(&_Prev, _PLast, sizeof(this));

        /* @NOTE: this should be used only when the child became the head so we
         * should configure it as soon as we can so we can use it quick when 
         * the head is switched */

        MCOPY(&_Head, &thiz, sizeof(this));
        MCOPY(&_Last, &thiz, sizeof(this));

        touched = True;
        break;
      }
    }

    retry--;
  } while (retry > 0);

  CMPXCHG(plock, this, None);

  if (retry == 0 && !touched) {
    return False;
  }

finish:
  if (SwitchTo(EStarting)) {
    Bug(EBadLogic, "can\'t switch to EStarting");
  }

  return True;
}

Bool Monitor::Detach(UInt retry) {
  using namespace Internal;

  auto is_child = False;
  auto is_latest = False;
  auto is_locked = False;
  auto is_detached = False;
  auto plock = &(Monitors[_Type].Left);
  auto phead = &(Monitors[_Type].Right);
 
  if (!!CMP(&_State, EDetached)) {
    return True;
  } else if (!!CMP(&_State, EStarted)) {
    if (SwitchTo(EStopping)) {
      Bug(EBadLogic, "can\'t switch to EStopped"); 
    }
  }

  if (!CMP(&_Using, 0)) {
    return False;
  } else if (SwitchTo(EStopped)) {
    return False;
  }

  if (!CMP(phead, _Head)) {
    is_child = True;

detach:
    _Flush();
  
    if (is_detached) {
      goto finish;
    }
  }

  do {
    BARRIER();

    if (LIKELY(Head(), None)) {
      _Detached = True;

      if (is_child) {
        goto finish;
      } else if (CMPXCHG(plock, None, this)) {
        is_locked = True;
      }
    } else if (CMPXCHG(plock, None, this)) {
      is_locked = True;
    }

    if (is_locked) {
      auto next = _Next;
      auto pnext = _PNext;

      /* @NOTE: edit the next pointer of the previous node using pnext so we 
       * could optimize performance while keep everything safe */

      if (pnext && next) {
        MCOPY(pnext, &next, sizeof(next));
      }

      /* @NOTE: it's always safe since the next should be release inside a light
       * weith lock */

      if (next) {
        next->_PNext = pnext;
        next->_Prev = _Prev;
      } 
     
      /* @NOTE: check if we also the latest one so we should change head's 
       * parameter _Last with this pointer to make sure the later ones don't
       * touch the wrong pointer */ 

      if (CMP(_PLast, this)) {
        MCOPY(_PLast, &_Prev, sizeof(_Prev));
      }

      /* @NOTE: if the head is going to detach, we should migrate to the next
       * one so the system still works well even if the head is changing on
       * realtime */

      if (CMP(&_Head, this)) {
        MCOPY(phead, &_Next, sizeof(_Next));
        
        if (_Next == None) {
          is_latest = True;
        }
      }

      break;
    }

    retry--;
  } while (retry > 0);

  CMPXCHG(plock, this, None);

  if (retry == 0 && !is_locked) {
    return False;
  }

  if (is_child) {
    goto finish;
  }
    
  is_detached = True;
  goto detach;

finish:
  if (is_latest) {
    _Clean();
  }

  if (SwitchTo(EDetached)) {
    Bug(EBadLogic, "can\'t switch to EDetached");  
  }

  return True;
}

ErrorCodeE Monitor::ForEach(Function<ErrorCodeE(Monitor*)> callback) {
  Monitor **pnext{&Next()}, **pcurr{None}, *claiming{None};
  ErrorCodeE result{ENoError};
  Bool first{True};

  /* @NOTE: check from children, we wouldn't know it without checking
   * them because some polling system don't support checking how many
   * fd has waiting */

  do {
    TimeSpec spec{.tv_sec=0, .tv_nsec=1};
    ErrorCodeE error{EBadAccess};
    
    if (pcurr && !claiming) {
      /* @NOTE: this case only happens when we finish claiming a new job so
       * we can close the old job here */

      if ((error = (*pcurr)->Done())) {
        Bug(error, "can\'t close an unfinished job");
      } else {
        pcurr = None;
        error = pnext? ENoError: EBadAccess;
      }
    } else if (first) {
      goto init;
    }

    BARRIER();

    if (claiming && pnext && !result) {
      /* @NOTE: wait pnext switches to another Monitor since the current pnext
       * is pointing to a detaching Monitor object */

      while (CMP(pnext, claiming)) {
        BARRIER();

        spec.tv_nsec = (spec.tv_nsec * 2) % ulong(1e9);
        Internal::Idle(&spec);
      } 
    
init:
      BARRIER();

      /* @NOTE; everything is okay recently so we should try claiming again with
       * the new Monitor object */

      while ((*pnext) && (error = (*pnext)->Claim()) == EDoAgain) {
        BARRIER();

        spec.tv_nsec = (spec.tv_nsec * 2) % ulong(1e9);
        Internal::Idle(&spec);
      }

      /* @NOTE: at the begining, we should claim the next Monitor first, so we
       * can init the flow */

      first = False;
    }

    /* @NOTE: we have retried claiming again so if the issue happens, it
     * isn't out of caring recently, just finish the task we have claimed
     * to be done before */

    if (pcurr) {
      if ((*pcurr)->Done()) {
        Bug(EBadAccess, "can\'t close an unfinished job");
      } else {
        pcurr = None;
      }
    }
   
    if (!error) {
      /* @NOTE: we only enter here if we have claimed successfully a new job
       * so we will perform it on parallel while make sure that the node 
       * can't be detached */

      try {
        result = callback(*pnext);
        pcurr = pnext;
        pnext = &((*pnext)->Next());
        claiming = *pnext;
      } catch (Exception &except) {
        result = except.code();
      } catch (std::exception &except) {
        result = EBadAccess;
      } catch (...) {
        result = EBadAccess;
      }

      /* @NOTE: if we face any issue during testing just stop claiming new job
       * and mark the current job is `Done` */

      if (result) {
        pnext = None;
        claiming = None;

        continue;
      }

      BARRIER();

      if (*pnext) {
        error = EBadAccess;

        /* @NOTE: claim a new job so we could make sure that the loop can't 
         * be jump into unexpected states */

        while ((*pnext) && (error = (*pnext)->Claim()) == EDoAgain) {
          BARRIER();

          spec.tv_nsec = (spec.tv_nsec * 2) % ULong(1e9);
          Internal::Idle(&spec);
        }

        /* @NOTE; if we claim successfully the next job, we are in good flow
         * where we have threads lining on the right direction. If not, we 
         * should report that we can't claim a new job. This only happen
         * when we access the node too late and we should wait until the next
         * child emerges */

        if (!error) {
          claiming = None;
        }
      } else {
        claiming = None;
        pnext = None;
      }
    }
  } while (pcurr);

  return result;
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
    Internal::MLocks[type].Safe([&]() {
      result = Internal::Monitors[type].Right;
    });
  }

  return result;
}

void Monitor::Lock(UInt type) {
  if (Internal::MLocks.find(type) != Internal::MLocks.end()) {
    Internal::MLocks[type](True);
  }
}

void Monitor::Unlock(UInt type) {
  if (Internal::MLocks.find(type) != Internal::MLocks.end()) {
    Internal::MLocks[type](False);
  }
}

ErrorCodeE Monitor::_Append(Auto UNUSED(fd), Int UNUSED(mode)) {
  if (_State) {
    return ENoSupport;
  } else {
    Internal::MLocks[_Type].Safe([&]() {
      _Pipeline.push_back(Monitor::Action{EAppend, fd, mode});
    });
    return ENoError;
  }
}

ErrorCodeE Monitor::_Modify(Auto fd, Int mode) {
  if (_State) {
    return ENoSupport;
  } else {
    Internal::MLocks[_Type].Safe([&]() {
      _Pipeline.push_back(Monitor::Action{EModify, fd});
    });
    return ENoError;
  }
}

ErrorCodeE Monitor::_Find(Auto UNUSED(fd)) {
  if (_State) {
    return ENoSupport;
  } else {
    Internal::MLocks[_Type].Safe([&]() {
      _Pipeline.push_back(Monitor::Action{EFind, fd});
    });
    return ENoError;
  }
}

ErrorCodeE Monitor::_Remove(Auto UNUSED(fd)) {
  if (_State) {
    return ENoSupport;
  } else {
    Internal::MLocks[_Type].Safe([&]() {
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
