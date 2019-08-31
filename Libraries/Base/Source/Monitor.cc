#include <Monitor.h>
#include <Utils.h>
#include <Vertex.h>

namespace Base {
namespace Internal {
Mutex* CreateMutex();

static Map<Monitor::TypeE, Pair<Monitor*, Monitor*>> Monitors;
static Vertex<Mutex, True> Secure([](Mutex* mutex) { Locker::Lock(*mutex); },
                                  [](Mutex* mutex) { Locker::Unlock(*mutex); },
                                  CreateMutex());

namespace Fildes {
Shared<Monitor> Create(String name, Base::Monitor::TypeE type, Int system);
} // namespace Fildes
} // namespace Internal

Monitor::Monitor(String name, TypeE type): _Name{name}, _Type{type} {
  using namespace Internal;
  Vertex<Mutex, False> UNUSED(guranteer) = Secure.generate();

  if (Monitors.find(type) == Monitors.end() || Monitors[type].Right == None) {
    Monitors[type] = Pair<Monitor*, Monitor*>(None, this);
  } else {
    Monitors[type].Right->_Children.push_back(this);
  }
}

Monitor::~Monitor() {
  using namespace Internal;
  /* @NOTE: if this is the latest monitor of the branch, we should remove it.
   * Otherwide, we will choose a child to become the head of this brand */

  auto UNUSED(guranteer) = Secure.generate();

  if (Monitor::Head(_Type) != this) {
    Int index = Base::Find<Monitor*>(Head(_Type)->_Children.begin(),
                                     Head(_Type)->_Children.end(),
                                     (Monitor*)this);

    /* @NOTE: we should detect the child and remove it, if not, throw an
     * exception because this is a bug */
    if (index < 0) {
      Bug(EBadLogic, Format{"Not found {} from its parent"} << _Name);
    } else {
      Head(_Type)->_Children.erase(Head(_Type)->_Children.end() + index);
    }
  } else if (_Children.size() > 0) {
    auto head = (Monitors[_Type].Right = _Children.back());

    /* @NOTE: reconfigure children to point to the new parent */
    for (auto& child: _Children) {
      if (child == head) {
        continue;
      }

      head->_Children.push_back(child);
    }
  } else {
    Monitors[_Type].Right = None;
  }
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

  Vertex<Mutex, False> UNUSED(guranteer) = Secure.generate();
  Vertex<void> escaping{[&]() { Monitors[_Type].Left = this; },
                        [&]() { Monitors[_Type].Left = None; }};

  if (!_Find(fd)) {
    return _Remove(fd);
  } else {
    return NotFound(Format{"fd {}"} << fd).code();
  }

  return ENoError;
}

ErrorCodeE Monitor::Trigger(Auto event, Perform perform) {
  using namespace Internal;
  /* @NOTE: this method should be called by user to register a new
   * event-callback, the event will be determined by monitor itself and can
   * be perform automatically with the load-balancing system */

  Vertex<Mutex, False> UNUSED(guranteer) = Secure.generate();
  Vertex<void> escaping{[&]() { Monitors[_Type].Left = this; },
                        [&]() { Monitors[_Type].Left = None; }};
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
    return Monitors[_Type].Left == this? ENoError: EInterrupted;
  } else {
    return Head(_Type)->_Status(name, Auto::As<Int>(-1));
  }
}

ErrorCodeE Monitor::Scan(Auto fd, Int mode, Vector<Perform*>& callbacks) {
  using namespace Internal;

  Bool passed = False;

  /* @NOTE: scan throught events and collect callbacks, i assume that scan
   * functions should run very fast but callbacks may run quiet slow so we
   * don't invoke it here. Instead, i will do them later on differnet */

  if (this != Head(_Type)) {
    return BadAccess("CHILD shouldn\'t call method Scan").code();
  } else if (_Find(fd)) {
    return ENotFound;
  }

  for (auto& check: _Checks) {
    if (!check(fd, _Access(fd), mode)) {
      auto callback = _Indicators[GetAddress(check)](fd, mode);

      if (callback) {
        callbacks.push_back(callback);
        passed = True;
      }
    }
  }

  return passed? ENoError: ENotFound;
}

ErrorCodeE Monitor::Heartbeat(Auto fd) {
  ErrorCodeE error;

  if (_Find(fd)) {
    Vertex<Mutex, False> UNUSED(guranteer) = Internal::Secure.generate();

    for (auto child: Monitor::Head(_Type)->_Children) {
      if (child->_Find(fd)) {
        continue;
      }

      for (auto fallback: child->_Fallbacks) {
        if ((error = fallback(fd))) {
          return error;
        }
      }
    }
  } else {
    for (auto fallback: _Fallbacks) {
      if ((error = fallback(fd))) {
        return error;
      }
    }
  }

  return ENoError;
}

ErrorCodeE Monitor::Loop(Function<Bool(Monitor&)> status, Int timeout) {
  ErrorCodeE error = EBadAccess;

  do {
    while ((error = Head(_Type)->_Interact(this, timeout)) == EDoAgain) {}
  } while (status(*this) || error == EBadAccess);

  return error;
}

ErrorCodeE Monitor::Loop(UInt steps, Int timeout) {
  return Loop([&](Monitor&) -> Bool { return (--steps) > 0; }, timeout);
}

Shared<Monitor> Monitor::Make(String name, Monitor::TypeE type) {
  switch(type) {
  case EIOSync:
    return Internal::Fildes::Create(name, type, 0);

  case EPipe:
    return Internal::Fildes::Create(name, type, 1);

  default:
    throw Except(ENoSupport, name);
  }
}

Monitor* Monitor::Head(Monitor::TypeE type) {
  if (Internal::Monitors.find(type) == Internal::Monitors.end()) {
    return None;
  }

  return Internal::Monitors[type].Right;
}
} // namespace Base
