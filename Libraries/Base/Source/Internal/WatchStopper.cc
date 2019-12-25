#include <Atomic.h>
#include <List.h>
#include <Lock.h>
#include <Logcat.h>
#include <Macro.h>
#include <Thread.h>
#include <Type.h>
#include <Vertex.h>
#include <Utils.h>

#include <signal.h>
#include <thread>

#define NUM_SPINLOCKS 2
#define TIMELOCK TimeLock

using TimeSpec = struct timespec;

namespace Base {
void Register(Base::Lock& lock, Void* context);
void Register(Base::Thread& thread, Void* context);

Void* Context(Base::Thread& thread);
Void* Context(Base::Lock& lock);

Int TimeLock(Mutex* mutex, Long timeout = -1, Bool* halt = None);
void SolveDeadlock();

Mutex* GetMutex(Lock& lock);
Thread::StatusE GetThreadStatus(Thread& thread);

void Capture (Void* ptr, Bool status);
void Cleanup(void* ptr);
void* Booting(void* ptr);

#if DEBUGING
namespace Debug {
void DumpThread(Base::Thread& thread, String parameter);
void DumpLock(Base::Lock& lock, String parameter);
void DumpWatch(String parameter);
} // namespace Debug
#endif // DEBUG

namespace Internal {
void UnwatchStopper(Base::Thread& thread);
Bool KillStoppers(UInt signal);
void Idle(TimeSpec* spec);
void AtExit(Function<void()> callback);

namespace Implement {
class Thread;
class Lock;
} // namespace Implement

class Stopper {
 public:
  virtual ~Stopper() {}

  /* @NOTE: this function is used to get the stopper's identify */
  virtual ULong Identity() = 0;

  /* @NOTE: this function is used to get the stopper's context */
  virtual void* Context() = 0;

  /* @NOTE: this function is used to unlock a stopper automatically */
  virtual Bool Unlock() = 0;

  /* @NOTE: this function is used to get status of this stopper */
  virtual Int Status() = 0;

  /* @NOTE: this function is used to verify the status of this stopper */
  virtual Bool IsStatus(Int status) = 0;

  /* @NOTE: this function is used to update status of the stopper */
  virtual ErrorCodeE SwitchTo(Int next) = 0;

  /* @NOTE: this function is used to lock a stoper from itself POV */
  virtual ErrorCodeE OnLocking(Implement::Thread* thread) = 0;

  /* @NOTE: this function is used to unlock a stoper from itself POV */
  virtual ErrorCodeE OnUnlocking(Implement::Thread* thread) = 0;

  /* @NOTE: this function is used to increase counters */
  virtual Bool Increase() = 0;

  /* @NOTE: this function is used to decrease counters */
  virtual Bool Decrease() = 0;
};

#if DEBUGING
namespace Debug {
void DumpLock(Implement::Lock& lock, String parameter);
void DumpLock(Implement::Thread& thread, String parameter);
} // namespace Debug
#endif  // DEBUG

namespace Implement {

class Thread : public Base::Internal::Stopper {
 public:
  explicit Thread(Base::Thread& thread) : Stopper(), _First{True},
           _Thread{&thread}, _Locker{None}{ }

  explicit Thread(Base::Thread* thread) : Stopper(), _First{True},
           _Thread{thread}, _Locker{None} { }

  ULong Identity() final;
  void* Context() final;
  Bool Unlock() final;
  Int Status() final;

  Bool IsStatus(Int status) final;

  ErrorCodeE SwitchTo(Int next) final;
  ErrorCodeE OnLocking(Implement::Thread* thread) final;
  ErrorCodeE OnUnlocking(Implement::Thread* thread) final;

  ErrorCodeE LockedBy(Stopper* locker);
  ErrorCodeE UnlockedBy(Stopper* locker);

  Bool Increase() final;
  Bool Decrease() final;

  Base::Thread* Super();

 private:
  Bool _First;
  Base::Thread* _Thread;
  Internal::Stopper* _Locker;
};

class Lock : public Base::Internal::Stopper {
 public:
#ifdef DEBUG
  explicit Lock(Base::Lock& lock) : Stopper(), _Mutex{Base::GetMutex(lock)},
           _Ticket{0}, _Count{0}, _Halt{False} {
#else
  explicit Lock(Base::Lock& lock) : Stopper(), _Lock{&lock},
          _Mutex{Base::GetMutex(lock)},  _Ticket{0}, _Count{0}, _Halt{False} {
#endif
  }

  explicit Lock(Mutex& mutex) : Stopper(), _Mutex{&mutex}, _Ticket{0},
           _Count{0}, _Halt{False} {
  }

  ULong Count();
  Bool* Halt();

  ULong Identity() final;
  void* Context() final;
  Bool Unlock() final;
  Int Status() final;

  Bool IsStatus(Int status) final;

  ErrorCodeE SwitchTo(Int next) final;
  ErrorCodeE OnLocking(Implement::Thread* thread) final;
  ErrorCodeE OnUnlocking(Implement::Thread* thread) final;

  Bool Increase() final;
  Bool Decrease() final;

 private:
#if DEBUGING
  friend void Base::Debug::DumpWatch(String parameter);
  friend void Debug::DumpLock(Implement::Lock& lock, String parameter);

  Base::Lock* _Lock;
#endif

  Mutex* _Mutex;
  ULong _Ticket;
  Long _Count;
  Bool _Halt;
};
}  // namespace Implement

class Watch {
 private:
  using Context = UMap<ULong, Unique<Stopper>>;

 public:
  enum StatusE {
    Unknown = 0,
    Initing = 1,
    Unlocked = 2,
    Locked = 3,
    Waiting = 4,
    Released = 5
  };

  Watch() {
    _Main = GetUUID();
    _Status = Unlocked;
    _Size = 0;
    _Solved = 0;
    _Rested = 0;
    _Spawned = 0;

    if (MUTEX(&_Lock[0])) {
      throw Except(EBadLogic, "Can\'t init _Lock[0]");
    }
    if (MUTEX(&_Lock[1])) {
      throw Except(EBadLogic, "Can\'t init _Lock[1]");
    }

    if (MUTEX(&_Global)) {
      throw Except(EBadLogic, "Can\'t init _Global");
    }
#if DEBUGING
    memset(_Owner, 0, sizeof(_Owner));
#endif
  }

  ~Watch() {
    /* @NOTE: try to resolve problems to prevent deadlock at the ending, we are
     * facing another issue with pthread_mutex_destroy. It seems on highloaded
     * system, we can't destroy the locks immediately */

    DESTROY(&_Lock[0]);
    DESTROY(&_Lock[1]);
  }

  /* @NOTE: this function is used to add a stopper to ignoring list */
  template <typename Type> Bool Ignore(Stopper* stopper);

  /* @NOTE: this function is used to count thread with specific condition */
  template <typename Type> Int Count();

  /* @NOTE: this functions is used to get basic information of stoppers */
  template <typename Type> Pair<Int, ULong> Next();

  /* @NOTE: this function is used to access the stopper, this is non-safed
   * function and you should use it inside safed-areas */
  template <typename Type> Type* Get(ULong uuid);

  /* @NOTE: this function is used to check if the stopper is marked to be
   * ignored */
  template <typename T> Bool IsIgnored(Stopper* stopper);

  /* @NOTE: this function is used to register a new stopper's uuid and it
   * should run on main-thread to prevent race-condition */
  template <typename T> void OnRegister(ULong uuid);

  /* @NOTE: this function is used to unregister a new stopper's uuid and it
   * should run on main-thread to prevent race-condition */
  template <typename T> void OnUnregister(ULong uuid);

  /* @NOTE: this function is used to notify that the stopper is on locking */
  template <typename Type>
  Bool OnLocking(Stopper* stopper, Function<Void()> actor);

  /* @NOTE: this function is used to notify that the stopper is on unlocking */
  template <typename Type>
  Bool OnUnlocking(Stopper* stopper, Function<Void()> actor);

  /* @NOTE: this function is used to increase the counter of Watch */
  template <typename Type> Bool Increase(Stopper* stopper);

  /* @NOTE: this function is used to decrease the counter of Watch */
  template <typename Type> Bool Decrease(Stopper* stopper);

  /* @NOTE: this function is used to notify that we are killing threads and
   * and don't want to watch any one */
  ErrorCodeE OnWatching(Stopper* stopper);

  /* @NOTE: this function is used to notify that this current thread is on
   * expiring */
  ErrorCodeE OnExpiring(Stopper* stopper);

  /* @NOTE: this function is used to check status of Watch */
  StatusE& Status();

  /* @NOTE: this function is used to lock Watch to protect database during
   * updating or editing. Coredump might happen if it found that the double lock
   * happen which means you are calling this function more than one in the same
   * thread */
  void Lock(UInt index);

  /* @NOTE: this function is used to unlock Watch to run on multi-threads */
  void Unlock(UInt index);

  /* @NOTE: this function is used to get main thread's UUID */
  ULong Main();

  /* @NOTE: this function is used to get how many threads are managed by this
   * Watch object */
  ULong Size();

  /* @NOTE: this function is used to get how many threads are spawed */
  ULong Spawn();

  /* @NOTE: this function is used to get how many threads are done */
  ULong Rest();

  /* @NOTE: this function is used to get how many threads are solved */
  ULong Solved();

  /* @NOTE: this function is used to check if the thread is occupied by this
   * Watch object or not */
  Bool Occupy(Base::Thread* thread);

  /* @NOTE: this function is used to notice the Watch object to only accept exact
   * threads */
  void Join(Base::Thread* thread);

  /* @NOTE: this function is used to show short summary of the system */
  void Summary(){}

  /* @NOTE: this function is used to catculate and optimize timewait in order to
   * improve performance */
  ULong Optimize(Bool predict, ULong timewait);

  /* @NOTE: this function is used to set everything back the begining state */
  void Reset();

  /* @NOTE: this function is used to get type as number */
  template<typename T>
  static UInt Type() {
    if (typeid(T) == typeid(Implement::Thread)) {
      return 1;
    } else if (typeid(T) == typeid(Implement::Lock)) {
      return 2;
    } else {
      return 0;
    }
  }

  List Stucks;

 private:
  friend Bool Base::Internal::KillStoppers(UInt signal);
  friend void Base::Internal::UnwatchStopper(Base::Thread& thread);
  friend void Base::Capture (Void* ptr, Bool status);
  friend void Base::Cleanup(void* ptr);
  friend void* Base::Booting(void* ptr);

#if DEBUGING
  friend void Base::Debug::DumpWatch(String parameter); 

  ULong _Owner[NUM_SPINLOCKS];
  UMap<UInt, Set<ULong>> _Stoppers;
#endif

  /* @NOTE: this variable indicate if we are in safed zone or not */
  Bool _Safed[NUM_SPINLOCKS];

  /* @NOTE: internal locks of Watch */
  Mutex _Lock[NUM_SPINLOCKS], _Global;
  StatusE _Status;
  Long _Main;

  /* @NOTE: this variable is used to count exactly how many thread has been
   * generated */
  Long _Spawned;

  /* @NOTE: this variable is used to count exactly how many thread has been
   * released */
  Long _Rested;

  /* @NOTE: this variable is used to count exactly how many thread has been
   * solvedd */
  Long _Solved;

  /* @NOTE: this variable is used to count how many thread has been inited */
  Long _Size;

  /* @NOTE: database of Watch */
  UMap<UInt, Long> _Counters;
  Set<Base::Thread*> _Threads;
  UMap<UInt, Set<ULong>> _Ignores;
};
}  // namespace Internal
}  // namespace Base

/* @NOTE: this is a place to define our API */
namespace Base {
namespace Internal {
thread_local Implement::Thread* Thiz{None};
static Shared<UMap<ULong, Implement::Lock>> Mutexes{None};
static Watch* Watcher{None};

namespace Summary {
void WatchStopper() { Watcher->Summary(); }
}  // namespace Summary

void WatchStopper(Base::Thread& thread) {
  Watcher->Join(&thread);
}

Bool WatchStopper(Base::Lock& lock) {
  try {
    if (Context(lock)) {
      return False;
    }

    if (!Watcher) {
      try {
        Watcher = new Watch();
        AtExit([]() { delete Watcher; });
      } catch (std::bad_alloc&) {
        return False;
      }
    }

    Register(lock, (Void*)(new Implement::Lock(lock)));
    Watcher->OnRegister<Implement::Lock>(lock.Identity());
  } catch (std::exception& except) {
    return False;
  }

  return True;
}

void UnwatchStopper(Base::Thread& thread) {
  Watcher->OnUnregister<Implement::Thread>(thread.Identity(False));

#if DEBUGING
  Watcher->_Threads.erase(&thread);
#endif
}

Bool UnwatchStopper(Base::Lock& lock) {
  if (Context(lock)) {
    Watcher->OnUnregister<Implement::Lock>(lock.Identity());
    delete reinterpret_cast<Implement::Lock*>(Context(lock));

    Register(lock, None);
    return True;
  }

  return False;
}

Bool KillStoppers(UInt signal) {
  Bool result{False};

  for (auto thread: Watcher->_Threads) {
    if (!pthread_kill(thread->Identity(), signal)) {
      result = False;
    }
  }

  return result;
}

/* @NOTE: this function is used to get UUID of the current thread */
ULong GetUniqueId() { return GetUUID(); }
}  // namespace Internal

namespace Locker {
/* @NOTE: this function is used to lock a mutex with the watching to prevent
 * deadlocks happen */

Bool Lock(Base::Lock& locker) {
  auto context = reinterpret_cast<Internal::Implement::Lock*>(Context(locker));

  /* @NOTE: lock may fail and cause coredump because we allow to another side
   * to do that, we can't guarantee that everything should be in safe all the
   * time if the user tries to trick the highloaded systems */

  return Internal::Watcher->OnLocking<Internal::Implement::Lock>(
    context,
    [&]() {
      TIMELOCK((Mutex*)locker.Identity(), -1, context->Halt());
    });
}

Bool Lock(Mutex& locker) {
  auto context = &(Internal::Mutexes->at(ULong(&locker)));

  /* @NOTE: lock may fail and cause coredump because we allow to another side
   * to do that, we can't guarantee that everything should be in safe all the
   * time if the user tries to trick the highloaded systems */

  return Internal::Watcher->OnLocking<Internal::Implement::Lock>(
    context,
    [&]() {
      TIMELOCK(&locker, -1, context->Halt());
    });
}

/* @NOTE: this function is used to unlock a mutex with the watching to prevent
 * double unlock */
Bool Unlock(Base::Lock& locker) {
  using namespace Internal;

  /* @NOTE: unlock may fail and cause coredump because we allow to another side
   * to do that, we can't guarantee that everything should be in safe all the
   * time if the user tries to trick the highloaded systems */

  return Watcher->OnUnlocking<Implement::Lock>(
    reinterpret_cast<Implement::Lock*>(Context(locker)),
    [&]() {
      do {
        UNLOCK((Mutex*)locker.Identity());
      } while (IsLocked(*((Mutex*)locker.Identity())));
    });
}

Bool Unlock(Mutex& locker) {
  using namespace Internal;

  /* @NOTE: unlock may fail and cause coredump because we allow to another side
   * to do that, we can't guarantee that everything should be in safe all the
   * time if the user tries to trick the highloaded systems */

  return Watcher->OnUnlocking<Implement::Lock>(
    &(Mutexes->at(ULong(&locker))),
    [&]() {
      do {
        UNLOCK(&locker);
      } while (IsLocked(locker));
    });
}
}  // namespace Locker

/* @NOTE: this function is used to get mutex from a Base::Lock object */
Mutex* GetMutex(Lock& lock) { return lock._Lock; }

/* @NOTE: this function is used to get status of Base::Thread objects */
Thread::StatusE GetThreadStatus(Thread& thread) {
  return thread._Status;
}

/* @NOTE: this function is used to set status of Base::Thread objects */
void SetThreadStatus(Thread& thread, Int status) {
  thread._Status = (Thread::StatusE)status;
}

void Cleanup(void* ptr) {
  using namespace Base::Internal;
  using namespace Base;

  if (ptr) {
    Thread* thiz = reinterpret_cast<Thread*>(ptr);
    ErrorCodeE error = ENoError;
    Implement::Thread* impl =
      reinterpret_cast<Implement::Thread*>(Context(*thiz));

    do {
      if (error == EDoAgain) {
        RELAX();
      }

      error = Base::Internal::Watcher->OnExpiring(impl);
    } while (error == EDoAgain);

    INC(&Watcher->_Rested);
  }

  pthread_exit((void*) -1);
}

/* @NOTE: this function is used to capture a status from Base::Thread */
void Capture (Void* ptr, Bool status) {
  using namespace Base::Internal;

  Implement::Thread* thread = reinterpret_cast<Implement::Thread*>(ptr);

  if (thread) {
    if (status) {
      /* @NOTE: this thread is going to switch to waiting state and will
       * perform solving deadlock if we see it's necessary */

      Watcher->OnLocking<Implement::Thread>(thread, SolveDeadlock);
    } else {
      /* @NOTO: ending state of the thread, there is not much thing to do here
       * so at least it should be used to notice that we are done the thread
       * here */

      if (!Watcher->OnUnlocking<Implement::Thread>(thread, None)) {
        Bug(EBadAccess, Format{"unidentify thread {}"} << thread->Identity());
      }

      Watcher->OnUnregister<Implement::Thread>(thread->Super()->Identity(False));
      Watcher->_Threads.erase(thread->Super());

      /* @NOTE: when the thread reaches here, it would means that this is the
       * ending of this thread and it's safe to clean it from WatchStopper POV
       * */
      delete thread;

      if (Watcher->Solved() == Watcher->Size()) {
        Watcher->Reset();
      }
    }
  }
}

/* @NOTE: this function is used to boot a thread which was created by
 * Base::Thread */
void* Booting(void* ptr) {
  using namespace Base::Internal;

  Base::Thread* thread = (Base::Thread*) ptr;
  ErrorCodeE error = ENoError;
  Bool locked = False;
  ULong id = GetUUID();

  try {
    Thiz = new Implement::Thread(thread);

    if (LIKELY(thread->_Registering, False)) {
      delete Thiz;
    } else if (thread->Daemon()) {
      XCHG16(&(thread->_Status), Watch::Initing);
    } else {
      /* @NOTE: we should register the implementer to our thread since i don't
       * want this object manage this automatically. This role should be from
       * the thread only */

      if ((locked = (Watcher->Status() == Watch::Locked))) {
        delete Thiz;
      } else if (Watcher->Status() == Watch::Waiting) {
        if ((locked = !Watcher->Occupy(thread))) {
          delete Thiz;
        }
      } else {
        while (!Watcher->Occupy(thread)) {
          RELAX();
        }
      }

      if (!locked) {
        Watcher->OnRegister<Implement::Thread>(thread->Identity(False));
        Register(*thread, Thiz);
      }

      /* @NOTE: register specific parameters to identify our thread */
      thread->_UUID = id;
      thread->_Watcher = Capture;

      if (locked) {
        XCHG16(&(thread->_Status), Watch::Released);
      } else {
        XCHG16(&(thread->_Status), Watch::Initing);
      }

      /* @NOTE: after finish register our thread, we should release this
       * lock to notify main-thread to check and verify the status */
      REL(&(thread->_Registering));
    }
  } catch (Base::Exception& except) {
    error = except.code();
  } catch (std::bad_alloc&) {
    error = EDrainMem;
  } catch (...) {
    error = EBadLogic;
  }

  if (!Thiz || error) {
    /* @NOTE: this line shouldn't happen or we might face another issues
     * related to OS and stdlib or logic code, bases on the error code we
     * have here */

    goto bugs;
  } else {
    INC(&Watcher->_Spawned);
  }

  pthread_cleanup_push(Cleanup, ptr);
  if (thread->Daemon()) {
    Vertex<void> escaping{[&]() { XCHG16(&thread->_Status, Watch::Unlocked); },
                          [&]() { XCHG16(&thread->_Status, Watch::Waiting); }};

    try {
      thread->_Delegate();
    } catch (Base::Exception& except) {
    } catch (std::exception& except) {
    } catch (...) {
    }
  } else if (!locked && !(error = Internal::Watcher->OnWatching(Thiz))) {
    /* @NOTE: invoke the main body of this thread and secure them with
     * try-catch to protect the whole system from crashing causes by
     * exceptions */

    try {
      thread->_Delegate();
    } catch (Base::Exception& except) {
    } catch (std::exception& except) {
    } catch (...) {
    }

    /* @NOTE: notify that this thread is on expiring from Watch POV and we
     * can't guarantee about if crashes happen here. Any crashes would produce
     * coredump and it's used for mantaining class Watch */

    do {
      if (error == EDoAgain) {
        RELAX();
      }

      error = Internal::Watcher->OnExpiring(Thiz);
    } while (error == EDoAgain);
  }

  pthread_cleanup_pop(0);
  if (error && error != EDoNothing) {
bugs:
    /* @NOTE: we should unlock this mutex in the case we facing any issue from
     * thread side in order to unlock main-thread */

    thread->_Registering = False;

    /* @NOTE: the implementer got ending state and we should remove it now to
     * prevent memory leak */

    if (Thiz) {
      delete Thiz;
    }

    INC(&Watcher->_Rested);

#if defined(COVERAGE)
    Except(error, Format{"Thread {}: A system error happens"}.Apply(id));
    exit(-1);
#else
    throw Except(error, Format{"Thread {}: A system error happens"}.Apply(id));
#endif
  } else {
    INC(&Watcher->_Rested);
  }

  return None;
}

namespace Internal {
Mutex* CreateMutex() {
#if UNIX
  auto result = (Mutex*)ABI::Calloc(1, sizeof(Mutex));

  if (!result) {
    throw Except(EDrainMem, "`ABI::Malloc` place to contain `Mutex`");
  } else if (MUTEX(result)) {
    switch (errno) {
      case EAGAIN:
        throw Except(EDoAgain, "");

      case ENOMEM:
        throw Except(EDrainMem, "");

      case EPERM:
        throw Except(EWatchErrno,
                     "The caller does not have the provilege to"
                     " perform the operation.");
    }
  }

  if (!Mutexes) {
    Mutexes = std::make_shared<UMap<ULong, Implement::Lock>>();
  }

  Mutexes->insert(std::make_pair(ULong(result), Implement::Lock(*result)));
  return result;
#elif WINDOW
  throw Except(ENoSupport, "");
#else
  throw Except(ENoSupport, "");
#endif
}

void RemoveMutex(Mutex* mutex) {
  if (!DESTROY(mutex)) {
    Mutexes->erase(ULong(mutex));
    free(mutex);
  }
}

/* @NOTE: this is a place to implement Watch's methods */

template <typename T>
Bool Watch::Ignore(Stopper* stopper) {
  /* @NOTE: check if the stopper is existing or not. We can't ignore anything
   * we never see or manage */

  if (stopper->IsStatus(Released)) {
    return False;
  } else {
    Vertex<void> escaping{[&](){ Lock(0); }, [&](){ Unlock(0); }};

    /* @NOTE: check and register a stopper to the exact place of _Ignores. This
     * list should be created and manage automatically by Watch and there is no
     * way to modify this list from outside, this is the only API to help to add
     * a stopper to this list and it will remain here until it's released */

    if (_Ignores.find(Type<T>()) != _Ignores.end()) {
      if (_Ignores[Type<T>()].find(stopper->Identity()) != _Ignores.end()) {
        return False;
      }
    } else {
      _Ignores[Type<T>()] = Set<ULong>{};
    }


    _Ignores[Type<T>()].insert(stopper->Identity());
    return True;
  }
}

template <typename T>
Int Watch::Count() {
  if (_Counters.find(Type<T>()) == _Counters.end()) {
    return 0;
  } else {
    return _Counters[Type<T>()];
  }
}

template <typename T>
Pair<Int, ULong> Watch::Next() {
  return Pair<Int, ULong>{0, 0};
}

template <typename T>
Bool Watch::IsIgnored(Stopper* stopper) {
  Vertex<void> escaping{[&](){ Lock(0); }, [&](){ Unlock(0); }};

  if (_Ignores.find(Type<T>()) != _Ignores.end()) {
    return _Ignores[Type<T>()].find(stopper->Identity()) !=
        _Ignores[Type<T>()].end();
  }

  return False;
}

template <typename T>
void Watch::OnRegister(ULong uuid) {
  Vertex<void> escaping{[&](){ Lock(0); }, [&](){ Unlock(0); }};

#if DEBUGING
  if (_Stoppers.find(Type<T>()) == _Stoppers.end()) {
    _Stoppers[Type<T>()] = Set<ULong>{};
  }

  _Stoppers[Type<T>()].insert(uuid);
#endif
}

template <typename T>
void Watch::OnUnregister(ULong uuid) {
  Vertex<void> escaping{[&](){ Lock(0); }, [&](){ Unlock(0); }};

#if DEBUGING
  _Stoppers[Type<T>()].erase(uuid);
#endif

  if (Type<T>() == Type<Implement::Thread>()) {
    INC(&_Solved);

    if (_Size == _Solved) {
      _Status = Unlocked;
    }
  }
}

template <typename T>
Bool Watch::OnLocking(Stopper* stopper, Function<Void()> actor) {
  Bool ignored{False};

  /* @NOTE: check if this stopper is on the ignoring list and jump to the place
   * to perform actor */

  if ((ignored = IsIgnored<T>(stopper))) {
    goto passed;
  }

  /* @NOTE: invoke stoppers to check if it's ok to call actor to perform locking
   * or not. I assumes that actor may cause crash with exceptions so we need to
   * prevent it instead at here. We will force close this thread since it has
   * crashed by unexpected reasons */

  if (stopper->IsStatus(Released)) {
    return False;
  } else if (!stopper->OnLocking(Thiz)) {
 passed:
    /* @NOTE: we must know clearly the current thread's next status after
     * calling `actor` and keep update it first before doing it. This would
     * used to prevent any deadlock from Watch POV */

    Vertex<void> escaping {
      [&](){
        /* @NOTE: increase number of locking threads */
        if (!ignored) {
          Increase<Implement::Lock>(stopper);
        }
      },
      [&](){
        /* @NOTE: decrease number of locking threads */
        if (!ignored) {
          Decrease<Implement::Lock>(stopper);
        }
      }};

    try {
      /* @NOTE: run `actor` here now and secure it with try-catch */
      if (actor) actor();

      /* @NOTE: good ending and we will recovery the function's status here
       * after returning the status code */
      return stopper->IsStatus(Watch::Locked);
    } catch(Base::Exception& except) {
      /* @NOTE: we may face an exception related to the lock is locked even if
       * this lock is free. This happens on highloaded system when so many lock
       * apply at the same time and causes this issue. This is expected
       * behavious and we just return false at this time. */
    } catch(std::exception& except) {
    } catch(...) {
    }
  } else {
    RELAX();
  }

  /* @TODO: bad ending, thinking about how to process this situation */
  return False;
}

template <typename T>
Bool Watch::OnUnlocking(Stopper* stopper, Function<Void()> actor) {
  ErrorCodeE error;

  /* @NOTE: notify a stopper to switch to state Unlocked. This should be defined
   * specifically by each stopper and the only thing we can provide is the
   * database, stoppers will use it to decide it should start locking this or
   * not. We also need to check if the stoppers is on ignoring database to avoid
   * doing unecessary things */

  if (IsIgnored<T>(stopper)) {
    goto passed;
  }

  /* @NOTE: invoke stoppers to check if it's ok to call actor to perform
   * unlocking or not. I assumes that actor may cause crash with exceptions so
   * we need to prevent it instead at here. We will force close this thread
   * since it has crashed by unexpected reasons */

  if (stopper->IsStatus(Released)) {
    return False;
  }

  if (!(error = stopper->OnUnlocking(Thiz))) {
 passed:
    try {
      /* @NOTE: run `actor` here now and secure it with try-catch */
      if (actor) actor();

      /* @NOTE: check the status after calling `actor`, this may fail for some
       * unknown reasons */
      if (Type<T>() == Type<Implement::Lock>()) {
        return stopper->IsStatus(Watch::Unlocked) ||
          stopper->IsStatus(Watch::Waiting);
      } else {
        return stopper->IsStatus(Watch::Unlocked);
      }
    } catch(Base::Exception& except) {
    } catch(std::exception& except) {
    } catch(...) {
    }
  } else {
    RELAX();
  }

  /* @TODO: bad ending, thinking about how to process this situation */
  return False;
}

template <typename T>
Bool Watch::Increase(Stopper* stopper) {
  Vertex<void> escaping{[&](){ Lock(0); }, [&](){ Unlock(0); }};

   if (_Counters.find(Type<T>()) == _Counters.end()) {
    _Counters[Type<T>()] = 1;
  }
   
  if (stopper->Increase()) {
    _Counters[Type<T>()] += 1;
  }

  if (_Main == (Long)GetUUID() || Type<T>() == Type<Implement::Thread>()) {
    return True;
  } else {
    return Thiz? Thiz->SwitchTo(Unlocked) == 0: True;
  }
}

template <typename T>
Bool Watch::Decrease(Stopper* stopper) {
  Vertex<void> escaping{[&](){ Lock(0); }, [&](){ Unlock(0); }};

  if (_Counters.find(Type<T>()) == _Counters.end()) {
#if DEBUGING
    return False;
#else
    Bug(EBadLogic, "decrease an undefined type");
#endif
  }
  
  if (stopper->Decrease()) {
    _Counters[Type<T>()] -= 1;
  }

  if (_Main == (Long)GetUUID() || Type<T>() == Type<Implement::Thread>()) {
    return True;
  } else {
    return Thiz? Thiz->SwitchTo(Unlocked) == 0: True;
  }
}

ErrorCodeE Watch::OnWatching(Stopper* stopper) {
  ErrorCodeE error{ENoError};

  /* @NOTE: this function is used to switch status of a thread to Running state
   * this status only effects on Thread */

  if (!(error = stopper->SwitchTo(Unlocked))) {
    if (!Increase<Implement::Thread>(stopper)) {
      return EBadLogic;
    }
  } else if (stopper->SwitchTo(Initing)) {
    Bug(EBadLogic, "fail to revert to Initing");
  }
  
  return error;
}

ErrorCodeE Watch::OnExpiring(Stopper* stopper) {
  ErrorCodeE error{ENoError};

  /* @NOTE: this function is used to switch status of a thread to Released state
   * this status should be the ending phrase of any stopper object. When it's
   * marked as `Released`, this object shouldn't be reused without specific
   * configurations */

  if ((error = stopper->SwitchTo(Released))) {
    return error;
  } else {
    auto status = stopper->Status();

    if (Decrease<Implement::Thread>(stopper)) {
      /* @NOTE: the thread is on going to be clear from Watch POV and will be
       * released soon */

      return ENoError;
    } else {
      return stopper->SwitchTo(status);
    }
  }
}

void Watch::Lock(UInt index) {
#if DEBUGING
  if (sizeof(_Owner)/sizeof(index) <= index) {
    Bug(EBadLogic, "use unknown spinlock");
  }
#endif

  /* @NOTE: lock the spinlock */
  _Safed[index] = !LOCK(&_Lock[index]);

#if DEBUGING
  /* @NOTE: update the new owner of this lock, this only indicate that which one
   * is locking this spinlock */
  _Owner[index] = GetUUID();
#endif
}

void Watch::Unlock(UInt index) {
#if DEBUGING
  if (sizeof(_Owner)/sizeof(index) <= index) {
    Bug(EBadLogic, "use unknown spinlock");
  }

  /* @NOTE: release onwer before unlock this to prevent race condition */
  _Owner[index] = -1;
#endif

  /* @NOTE: unlock the spinlock */
  _Safed[index] = UNLOCK(&_Lock[index]);
}

ULong Watch::Main() { return _Main; }

ULong Watch::Size() {
  return _Size;
}

ULong Watch::Spawn() {
  return _Spawned;
}


ULong Watch::Rest() {
  return _Rested;
}

ULong Watch::Solved() {
  return _Solved;
}

Bool Watch::Occupy(Base::Thread* thread) {
  return _Threads.find(thread) != _Threads.end();
}

void Watch::Join(Base::Thread* thread) {
  _Threads.insert(thread);
  _Size++;
}

Watch::StatusE& Watch::Status() { return _Status; }

ULong Watch::Optimize(Bool UNUSED(predict), ULong timewait) {
  /* @TODO: design a new algothrim to optmize timewait */
  return timewait;
}

void Watch::Reset() {
#if DEBUGING
  _Stoppers.clear();
#endif

  _Threads.clear();
  _Counters.clear();
  _Status = Unlocked;

  _Size = 0;
  _Solved = 0;
  _Rested = 0;
  _Spawned = 0;

  for (UInt i = 0; i < sizeof(_Lock)/sizeof(Mutex); ++i) {
    UNLOCK(&_Lock[i]);
  }
  UNLOCK(&_Global);
}

/* @NOTE: this is a place to implement Stopper's methods */
namespace Implement {
Int Thread::Status() {
  return GetThreadStatus(*_Thread);
}

Bool Thread::IsStatus(Int status) {
  if (_Thread) {
    return LIKELY(status, (Int) GetThreadStatus(*_Thread));
  }

  return Watcher->Status() == status;
}

ULong Thread::Identity() {
  if (_Thread) {
    return _Thread->Identity();
  }

  return 0;
}

void* Thread::Context() {
  return _Thread;
}

Bool Thread::Unlock() {
  /* @TODO: */

  return True;
}

ErrorCodeE Thread::SwitchTo(Int next) {
  if (IsStatus(Watch::Released)) {
    return EBadAccess;
  } else if (IsStatus(next)) {
    if (LIKELY(next, Watch::Waiting) && LIKELY(Watcher->Main(), GetUUID())) {
      return ENoError;
    }

    return EDoNothing;
  }

  /* @NOTE: switch to the correct state */
  if (LIKELY(next, Watch::Locked) && LIKELY(ULong(GetUUID()), Watcher->Main())) {
    next = Watch::Waiting;
  }

  switch (next) {
  case Watch::Initing:
    if (_Thread) {
      Int status = (Int)GetThreadStatus(*_Thread);

      if (UNLIKELY(status, Watch::Unlocked) &&
          UNLIKELY(status,Watch::Unknown)) {
        return EBadAccess;
      }
    } else if (UNLIKELY(Watcher->Status(), Watch::Unlocked)) {
      return EBadAccess;
    }
    break;

  case Watch::Unlocked:
    /* @NOTE: Locked -> Unlocked: only happen when a dealock is solved by main
     * or it has been unlocked by another threads during running */

    if (_Thread) {
      auto status = (Int)GetThreadStatus(*_Thread);

      if (LIKELY(status, Watch::Locked) &&
          LIKELY(GetUUID(), _Thread->Identity())) {
        Bug(EBadLogic, "auto-unlock causes by itself");
      } else if (UNLIKELY(status, Watch::Locked) &&
                 UNLIKELY(status, Watch::Waiting) &&
                 UNLIKELY(status, Watch::Initing)) {
        return EBadAccess;
      } else if (UNLIKELY(GetUUID(), Watcher->Main()) &&
                 LIKELY(status, Watch::Waiting)) {
        return ENoError;
      }
    } else if (UNLIKELY(Watcher->Status(), Watch::Waiting)) {
      return EBadAccess;
    }
    break;

  case Watch::Locked:
    /* @NOTE: Unlocked -> Locked: only happen when mutex locks itself more than
     * one time and this thread will switch to this stage automatically */

    if (_Thread) {
      Int status;

      if (UNLIKELY(GetUUID(), _Thread->Identity())) {
        return BadLogic.code();
      } else if (UNLIKELY((status = (Int)GetThreadStatus(*_Thread)),
                          Watch::Unlocked)) {
        return BadAccess.code();
      }
    } else if (UNLIKELY(Watcher->Status(), Watch::Unlocked)) {
      return BadAccess.code();
    }
    break;

  case Watch::Waiting:
    /* @NOTE: Unlocked -> Waiting: only happen at a certain time on main thread
     * only, and we will trigger SolveDeadlock to watch and reslove any deadlock
     * situaltions */

    if (UNLIKELY(Watcher->Main(), GetUUID())) {
      return BadLogic.code();
    } else if (_Thread) {
      Int status;

      if (UNLIKELY((status = (Int)GetThreadStatus(*_Thread)), Watch::Unlocked)) {
        if (GetUUID() != _Thread->Identity(True)) {
          Bug(EBadAccess, Format{"thread {} doesn\'t exist"} << GetUUID());
        } else {
          return BadAccess(Format{"Unlocked != {}"} << status).code();
        }
      }
    } else if (LIKELY(Watcher->Status(), Watch::Locked)) {
      return BadAccess.code();
    }

    Watcher->Status() = Watch::Waiting;
    break;

  case Watch::Released:
    /* @NOTE: happen automatically we the thread finish and exit completedly */

    if (_Thread) {
      return ENoError;
    }
    break;

  default:
    return ENoSupport;
  }

  if (_Thread) {
    SetThreadStatus(*_Thread, next);
  } else {
    Watcher->Status() = (Watch::StatusE) next;
  }
  return ENoError;
}

ErrorCodeE Thread::OnLocking(Implement::Thread* UNUSED(thread)) {
  return SwitchTo(Watch::Waiting);
}

ErrorCodeE Thread::OnUnlocking(Implement::Thread* UNUSED(thread)) {
  return SwitchTo(Watch::Unlocked);
}

ErrorCodeE Thread::LockedBy(Stopper* locker) {
  if (!CMPXCHG(&_Locker, locker, _Locker)) {
    /* @NOTE: maybe on high-loading systems, we would see this command repeate
     * a certain time before we are sure that everything is copy completedly.
     * We should consider about the timeout at this stage to prevent hanging
     * issues */

    while (!CMPXCHG(&_Locker, None, locker)) {
      BARRIER();
    }
  }

  return ENoError;
}

ErrorCodeE Thread::UnlockedBy(Stopper* UNUSED(stopper)) {
  /* @NOTE: maybe on high-loading systems, we would see this command repeate
   * a certain time before we are sure that everything is copy completedly.
   * We should consider about the timeout at this stage to prevent hanging
   * issues */

  if (_Locker) {
    _Locker =  None;
  } else if (_First) {
    _First = False;
  } else {
    return BadAccess("_Locker shouldn\'t be None").code();
  }

  return ENoError;
}

Bool Thread::Increase() { return True; }

Bool Thread::Decrease() { return True; }

Base::Thread* Thread::Super() { return _Thread; }

ULong Lock::Count() {
  return _Count;
}

Bool* Lock::Halt() {
  return &_Halt;
}

Bool Lock::Unlock() {
  if (_Mutex) {
    return Watcher->OnUnlocking<Implement::Lock>(this,
      [&]() {
        _Halt = True;
      });
  } else {
    return False;
  }

  return !ISLOCKED(_Mutex);
}

Bool Lock::IsStatus(Int status) {
  return LIKELY(Status(), status);
}

Int Lock::Status() {
  return ISLOCKED(_Mutex) ? Watch::Locked :
         (CMP(&_Count, 0) ? Watch::Unlocked: Watch::Waiting);
}

void* Lock::Context() {
  return _Mutex;
}

ULong Lock::Identity() {
  if (_Mutex) {
    return (ULong)_Mutex;
  }

  return 0;
}

ErrorCodeE Lock::SwitchTo(Int next) {
  if (Status() == Watch::Released) {
    return EBadAccess;
  } else if (Status() == next) {
    return EDoNothing;
  }

  switch (next) {
  case Watch::Initing:
    /* @NOTE: init the first state of this lock we will choose according the
     * current status of this lock */

    next = ISLOCKED(_Mutex) ? Watch::Locked : Watch::Unlocked;
    break;

  case Watch::Unlocked:
    /* @NOTE: Locked -> Unlocked: happens when the mutex switch to unlocked
     * state */

    if (!IsStatus(Watch::Waiting)) {
      return BadAccess.code();
    }
    break;

  case Watch::Locked:
    /* @NOTE: Unlocked -> Locked: happens when the mutex switch to locked
     * state */

    break;

  case Watch::Released:
    /* @NOTE: happen when the lock is removed */

    if (UNLIKELY(Count(), 0)) {
      return EBadLogic;
    }
    break;

  case Watch::Waiting:
    if (!IsStatus(Watch::Locked)) {
      return EBadLogic;
    }
    break;

  default:
    return ENoSupport;
  }

  return ENoError;
}

ErrorCodeE Lock::OnLocking(Implement::Thread* thread) {
  ErrorCodeE error{ENoError};

  /* @NOTE: update relationship between this lock and threads which are locked
   * by this mutex */

  if ((error = SwitchTo(Watch::Locked))) {
    if (error == EDoNothing) {
      goto finish;
    }
    
    return error;
  }

  if (CMP(&_Count, 1)) {
lock:
    if (thread && (error = thread->LockedBy(this))) {
      return error;
    }
  }

  goto done;

finish:
  if (CMP(&_Count, 1)) {
    goto lock;
  }


done:
  return ENoError;
}

ErrorCodeE Lock::OnUnlocking(Implement::Thread* thread) {
  ErrorCodeE error{ENoError};

  /* @NOTE: the thread will be unlocked by another threads, when it happens
   * more than one thread is released at this time, so the lock should jump to
   * unlocking state */

  if (!ISLOCKED(_Mutex)) {
    return BadAccess.code();
  }
    
  if ((error = SwitchTo(Watch::Waiting))) {
    if (error == EDoNothing) {
      goto finish;
    }
    
    return error;
  }

  /* @NOTE: update relationship between this lock and threads which are locked
   * by this mutex */

  if (thread && (error = thread->UnlockedBy(this))) { 
    if (SwitchTo(Watch::Locked)) {
      Bug(EBadAccess, "can\'t revert case `Locked -> Waiting`");
    }

    return error;
  }

finish:
  return ENoError;
}

Bool Lock::Increase() {
  if (LIKELY(INC(&_Count), 1)) {
    _Ticket = Watcher->Stucks.Add(this);
  }

  return _Count > 0;
}

Bool Lock::Decrease() {
  auto ticket = _Ticket;

  if (DEC(&_Count) > 0) {
    goto finish;
  }

  /* @NOTE: remove the ticket here since we are reaching to the latest node.
   * This should be deleted here because we only touch here when _Count
   * reaches to 0 */

  if (!Watcher->Stucks.Del(ticket, this)) {
    if (Watcher->Stucks.Size()) {
      Notice(EBadAccess, Format{"Can\'t delete ticket {}"} << ticket);
    }
  }

  /* @NOTE: wow, i can detect the silence state here. Of course, this state is very
   * unstable but i believe that it should be very useful in the future */

  if (CMP(&_Count, 0)) {
  }

finish:
  return _Count >= 0;
}
}  // namespace Implement
}  // namespace Internal

Lock::StatusE Lock::Status() {
  auto lock = reinterpret_cast<Internal::Implement::Lock*>(_Context);

  if (lock) {
    if (ISLOCKED(_Lock)) {
      return Locked;
    } else if (lock->Count()) {
      return Unlocking;
    } else {
      return Unlocked;
    }
  } else {
    return Released;
  }
}

Void* Context(Base::Thread& thread) {
  return thread._Context;
}

Void* Context(Base::Lock& lock) {
  return lock._Context;
}

void Register(Base::Thread& thread, Void* context) {
  thread._Context = context;
}

void Register(Base::Lock& lock, Void* context) {
  lock._Context = context;
}

ULong Time() {
  TimeSpec spec{.tv_sec=0, .tv_nsec=0};

  if (!clock_gettime(CLOCK_REALTIME, &spec)) {
    return spec.tv_sec*1000000 + spec.tv_nsec;
  }

  return 0;
}

Int TimeLock(Mutex* mutex, Long timeout, Bool* halt) {
  TimeSpec spec{.tv_sec=0, .tv_nsec=10};
  Bool passed{False};
  Long clock{0};
  Int result{0};

  while ((!halt || !(passed = *halt)) && (timeout < 0 || clock < timeout)) { 
    if (timeout >= 0) {
      clock += spec.tv_nsec;
    }

    switch (TRYLOCK(mutex)) {
      case 0:
        goto finish;

      case EBUSY:
        result = ETIMEDOUT;
        break;

      default:
        return result;
    }

    Internal::Idle(&spec);
  }

finish:
  if (passed) {
    UNLOCK(mutex);
  }
  return passed? 0: result;
}

void SolveDeadlock() {
  using namespace Base::Internal;

  ULong marks[2], timeout{1000};
  TimeSpec spec{.tv_sec=0, .tv_nsec=0};

  /* @NOTE: when we reach here, there is no way to re-create a new thread until
   * the number of thread reduces to 0. It would mean that any thread creates
   * later will cause panicing or being rejected */

  memset(marks, 0, sizeof(marks));

  while (Watcher->Spawn() != Watcher->Size() || 
         Watcher->Rest() != Watcher->Size()) {
    if (LIKELY(Watcher->Count<Implement::Thread>(), 0)) {
      goto again;
    }

    if (Watcher->Count<Implement::Lock>() >
               Watcher->Count<Implement::Thread>()) {
      Bool tracking = False, checking = False;

      marks[1] = marks[0];
      marks[0] = Time();

      if (marks[1] != 0) {
        /* @NOTE: we found a certain time has pass since the last deadlock is
         * found and this place is used as a hook to help to optimize how many
         * time we should wait for the next probing */

        // timeout = Watcher->Optimize(True, marks[1] - marks[0]);
      }

      for (ULong i = 0; i < Watcher->Stucks.Size(); ++i) {
        Implement::Lock* locker = Watcher->Stucks.At<Implement::Lock>(i + 1);

        if (!locker || !(checking = locker->Unlock())) {
          tracking = True;
        } else {
          break;
        }
      }

      if (tracking) {
        /* @NOTE: We could able to detect D-state issues and kill it directly
         * but we should provide plugins to do it since we can't detect what
         * really the root-cause of D-state issues recently */

        goto again;
      }

      if (!checking) {
        /* @NOTE: do nothing so we should increase timeout in order to prevent
         * performance impact causes by running this function too much*/

        timeout *= 10;
      } else {
        timeout /= 10;
      }
    } else {
      timeout *= 10;
    }

again:
    /* @NOTE: take a litter bit of time before checking again */

    spec.tv_sec = timeout / ULong(1e9);
    spec.tv_nsec = timeout % ULong(1e9);

    Internal::Idle(&spec);
  }
}

#if DEBUGING
namespace Internal {
namespace Debug {
void DumpLock(Base::Internal::Implement::Lock& lock, String parameter) {
  if (parameter == "Count") {
    VERBOSE << Format{"Lock {}'s Count is {}"}
                  .Apply(ULong(lock._Lock), lock._Count)
            << EOL;
  } else if (parameter == "Ticket") {
    VERBOSE << Format{"Lock {}'s Ticket is {}"}
                  .Apply(ULong(lock._Lock), lock._Ticket)
            << EOL;
  } else if (parameter == "Status") {
    VERBOSE << Format{"Lock {}'s Status is {}"}
                  .Apply(ULong(lock._Lock), ISLOCKED(lock._Mutex))
            << EOL;
  }
}

void DumpLock(Base::Internal::Implement::Thread& UNUSED(thread),
              String UNUSED(parameter)) {
}
} // namespace Debug
} // namespace Internal

namespace Debug {
void DumpThread(Base::Thread& UNUSED(thread), String UNUSED(parameter)) {
}

void DumpLock(Base::Lock& UNUSED(lock), String UNUSED(parameter)) {
}

void DumpWatch(String parameter) {
  using namespace Internal;
  using namespace Internal::Debug;

  if (parameter == "Stucks") {
    auto curr = Watcher->Stucks._Head;

    VERBOSE << "Dump information of list Watcher->Stucks:" << EOL;
    VERBOSE << (Format{" - List's count is {}"} << Watcher->Stucks._Count)
            << EOL;
    VERBOSE << (Format{" - List's head is {}"} << ULong(Watcher->Stucks._Head))
            << EOL;
    VERBOSE << (Format{" - List's tail is {}"} << ULong(Watcher->Stucks._Curr))
            << EOL;
    VERBOSE << (Format{" - List has {} node(s):"} << Watcher->Stucks.Size())
            << EOL;

    for (UInt i = 0; i < Watcher->Stucks.Size() && curr; ++i) {
      VERBOSE << Format{"   + {} -> {}"}.Apply(curr->_Index, (ULong)curr->_Ptr)
              << EOL;
      curr = curr->_Next;
    }

    VERBOSE << (Format{" - Have {} running job(s)"} << Watcher->Stucks._Parallel)
            << EOL;

    for (ULong i = 0; i < Watcher->Stucks._Size[1]; ++i) {
      auto& barrier = Watcher->Stucks._Barriers[i];

      for (auto j = 0; j < Int(List::EEnd); ++j) {
        if (barrier.Left[j] == 0) {
          continue;
        }

        VERBOSE << Format{"   + {} is doing job {}"}.Apply(barrier.Left[j], j)
                << EOL;
        break;
      }
    }
  } else if (parameter == "Counters") {
    VERBOSE << "Dump Watcher's counters:" << EOL;
    VERBOSE << Format{" - Count<Implement::Thread>() = {}"}
                  .Apply(Watcher->Count<Implement::Thread>()) << EOL;
    VERBOSE << Format{" - Count<Implement::Lock>() = {}"}
                  .Apply(Watcher->Count<Implement::Lock>()) << EOL; 
  } else if (parameter == "Stucs.Unlock") {
    VERBOSE << Format{" - Spawn() = {}"}.Apply(Watcher->Spawn()) << EOL;
    VERBOSE << Format{" - Size() = {}"}.Apply(Watcher->Size()) << EOL;
    VERBOSE << Format{" - Solved() = {}"}.Apply(Watcher->Solved()) << EOL;
    VERBOSE << Format{" - Rest() = {}"}.Apply(Watcher->Rest()) << EOL;
  } else if (parameter == "Stucks.Unlock") {
    for (ULong i = 0; i < Watcher->Stucks._Size[1]; ++i) {
      for (auto j = 0; j < Int(List::EEnd); ++j) {
        auto index = Watcher->Stucks._Barriers[i].Left[j];
        auto curr = Watcher->Stucks._Head;

        if (index == 0) {
          continue;
        }

        for (UInt i = 0; i < Watcher->Stucks.Size() && curr; ++i) {
          if (curr->_Index == index) {
            break;
          }

          curr = curr->_Next;
        }

        if (curr) {
          auto lock = (Implement::Lock*)curr->_Ptr;

          Internal::Debug::DumpLock(*lock, "Count");
          Internal::Debug::DumpLock(*lock, "Ticket");
          Internal::Debug::DumpLock(*lock, "Status");
        }
        break;
      }
    }
  }
}
} // namespace Debug
#endif
} // namespace Base
