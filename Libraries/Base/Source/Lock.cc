#include <Exception.h>
#include <Lock.h>
#include <Logcat.h>
#include <Macro.h>
#include <Utils.h>

namespace Base {
namespace Internal {
Bool WatchStopper(Base::Lock& lock);
Bool UnwatchStopper(Base::Lock& lock);
void RemoveMutex(Mutex* mutex);
Mutex* CreateMutex();
}  // namespace Internal

namespace Locker {
Bool Lock(Mutex& locker, Long timeout, Bool fail_if_autoresolve);
} // namespace Locker

Lock::Lock(Bool locked) : _Lock{Internal::CreateMutex()} {
  _Count = new Int{0};
  _Context = None;

  if (_Count) {
    if (!Internal::WatchStopper(*this)) {
      Bug(EBadLogic, "can\'t lease a lock as expected");
    }

    if (locked) Locker::Lock(*_Lock);
  }
}

Lock::~Lock() {
  if (Locker::IsLocked(*_Lock)) {
    Locker::Unlock(*_Lock);
  }

  if (*_Count == 0) {
    if (!Internal::UnwatchStopper(*this)) {
      Bug(EBadLogic, "can\'t unlease a lock as expected");
    }

    Internal::RemoveMutex(_Lock);
    delete _Count;
  } else {
    (*_Count)--;
  }
}

Lock::Lock(const Lock& src){
  _Lock = src._Lock;
  _Count = src._Count;
  _Context = src._Context;

  (*_Count)++;
}

Lock& Lock::operator=(const Lock& UNUSED(src)) {
  _Lock = src._Lock;
  _Count = src._Count;
  _Context = src._Context;

  (*_Count)++;
  return *this;
}

Lock& Lock::operator=(Lock&& UNUSED(src)) {
  _Lock = src._Lock;
  _Count = src._Count;
  _Context = src._Context;

  (*_Count)++;
  return *this;
}

Lock& Lock::operator()() {
  if (Locker::IsLocked(*_Lock)) {
    Locker::Unlock(*_Lock);
  } else {
    Locker::Lock(*_Lock);
  }

  return *this;
}

Lock::operator bool() { return Locker::IsLocked(*_Lock); }

Lock& Lock::operator()(Bool lock){
  if (lock) {
    Locker::Lock(*_Lock);
  } else {
    Locker::Unlock(*_Lock);
  }

  return *this;
}

const Lock& Lock::operator()() const{
  if (Locker::IsLocked(*_Lock)) {
    Locker::Unlock(*_Lock);
  } else {
    Locker::Lock(*_Lock);
  }

  return *this;
}

const Lock& Lock::operator()(Bool lock) const{
  if (lock) {
    Locker::Lock(*_Lock);
  } else {
    Locker::Unlock(*_Lock);
  }

  return *this;
}

Lock::operator bool() const { return Locker::IsLocked(*_Lock); }

Bool Lock::DoLockWithTimeout(Double timeout) {
  if (timeout < 0) {
    return Locker::Lock(*_Lock, -1, True);
  } else {
    return Locker::Lock(*_Lock, Long(timeout*1000000000), True);
  }
}

void Lock::Wait(Function<Bool()> event, Function<void()> react) {
  /* @NOTE: jump to lock's waiting state and wait another unlock it, at that
   * time check `event` -> if okey, release waiting state, otherwide, we will
   * do everything again */

  do {
    Locker::Lock(*_Lock);
  } while (!event());

  /* @NOTE: when an event has been trigger successful, we will call react
   * to perform a certain task */
  if (react) react();
}

void Lock::Wait(Function<Bool()> event) { Wait(event, None); }

void Lock::Safe(Function<void()> callback) {
  if (callback && _Lock) {
    Base::Exception error{ENoError};

    try {
      Locker::Lock(*_Lock);
      callback();
    } catch (Base::Exception& except) {
      error = except;
    } catch (std::exception& except) {
      error = Except(EWatchErrno, except.what());
    }

    Locker::Unlock(*_Lock);
    if (error.code()) throw error;
  }
}

ULong Lock::Identity() { return ULong(_Lock); }
}  // namespace Base
