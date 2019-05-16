#include <Lock.h>
#include <Logcat.h>
#include <Macro.h>
#include <Thread.h>
#include <Type.h>
#include <Vertex.h>
#include <Utils.h>
#include <signal.h>

#if LINUX
#include <sys/syscall.h>
#include <unistd.h>

#ifdef SYS_gettid
#define GetID() static_cast<ULong>(syscall(SYS_gettid))
#else
#error "SYS_gettid unavailable on this system"
#endif
#elif APPLE || BSD
#include <sys/syscall.h>
#include <sys/types.h>

ULong GetID() {
  uint64_t tid;

  ptinhread_threadid_np(None, &tid);
  return tid;
}

Int pthread_mutex_timedlock(Mutex* mutex, struct timespec* timeout) {
  pthread_mutex_trylock(mutex);
  nanosleep(timeout, None);

  if (!pthread_mutex_trylock(mutex)){
    return 0;
  } else if (errno == EBUSY){
    return ETIMEDOUT;
  } else {
    return errno;
  }
}

#elif WINDOW
#include <windows.h>

#define GetID GetCurrentThreadId
#else
#error "Unknow system, WatchStopper can't protect you"
#endif

#if USE_TID
#define GetTID() GetID()
#define GetMID() GetID()
#else
#define GetTID() pthread_self()
#define GetMID GetID
#endif

/* @NOTE: define built-in function SYNC_FETCH_ADD */
#if WINDOW
#define SYNC_FECTH_ADD(a, b) *a += b
#else
#define SYNC_FETCH_ADD __sync_fetch_and_add
#endif

#define MUTEX 0
#define THREAD 1
#define IOSYNC 2

/* @NOTE: this function will help to convert Timespec to String */
#define TIME_FMT sizeof("2012-12-31 12:59:59.123456789")

ErrorCodeE Timespec2String(CString buf, Int len, struct timespec *ts) {
  struct tm t;

  tzset();

  if (localtime_r(&(ts->tv_sec), &t) == NULL) {
    return EBadAccess;
  }

  if (!strftime(buf, len, "%F %T", &t)) {
    return EBadLogic;
  }

  len -= strlen(buf) - 1;
  if (snprintf(&buf[strlen(buf)], len, ".%09ld", ts->tv_nsec) >= len) {
    return EOutOfRange;
  }

  return ENoError;
}

namespace Base {
Mutex* GetMutex(Lock& lock);
Thread::StatusE GetThreadStatus(Thread& thread);

namespace Internal {
class Stopper {
 protected:
  enum LockT{ EUnknown = 0, EMutex = 1, EIOSync = 2, EJoin = 3 };

 public:
  virtual ~Stopper() {}

  /* @NOTE: these functions are used to check or update status of the
   * stopper */
  virtual Bool Unlocked() = 0;
  virtual Bool Expired() = 0;
  virtual Int Status() = 0;

  /* @NOTE: use to notify who has caused this stopper to be locked */
  Bool LockedBy(UInt name, ULong id);

 protected:
  UInt _Type;
  LockT _LockType;
  ULong _LockId;
};

namespace Implement {
class Thread : public Base::Internal::Stopper {
 public:
  Thread(Base::Thread& thread) : Stopper(), _Thread{&thread} {
    _Type = THREAD;
  }

  Thread(Base::Thread* thread) : Stopper(), _Thread{thread} {
    _Type = THREAD;
  }

  virtual ~Thread() {}

  Int Status() override { return GetThreadStatus(*_Thread); }

  /* @NOTE: unlock thread automatically */
  Bool Unlocked() override;
  Bool Expired() override;

 private:
  Base::Thread* _Thread;
};

class Lock : public Stopper {
 public:
  Lock(Base::Lock& lock) : Stopper() {
    _Type = MUTEX;
    _Mutex = Base::GetMutex(lock);
  }

  Lock(Mutex& mutex) : Stopper(), _Mutex{&mutex}{}

  /* @NOTE: these functions are used to check or update status of the
   * stopper */
  Bool Unlocked() final { return False; }
  Bool Expired() final { return True; }
  Int Status() final { return Locker::IsLocked(*_Mutex); }

 private:
  Vector<ULong> _Stoppers;
  Mutex* _Mutex;
};

class IOSync : public Stopper {
 public:
  IOSync() : Stopper() {
    _Type = IOSYNC;
  }

  Bool Unlocked() final {
    throw Except(EBadAccess, "Can\'t unlock a iosync");
  }

  Bool Expired() final {
    throw Except(EBadAccess, "Can\'t expire a iosync");
  }
};

class Main : public Implement::Thread {
 public:
  Main() : Thread(None) {}

  Int Status(){ return 0; }

  /* @NOTE: unlock main automatically */
  Bool Expired() final;
};
}  // namespace Implement

class Watch {
 private:
#ifdef BASE_TYPE_STRING_H_
  using Context = OMap<ULong, Unique<Stopper>>;
#else
  using Context = UMap<ULong, Unique<Stopper>>;
#endif

 public:
  enum StatusE{ Unlocked = 0, Locked = 1, Waiting = 2, Released = 3 };

  Atomic<Bool> _Crash;

  Watch(){
    UInt name = TypeId<Implement::Thread>();
    ULong thread_id = GetMID();

    _Stoppers[name] = Context{};
    _Stoppers[name][thread_id] = Unique<Stopper>(new Implement::Main());
    _Status[thread_id] = Unlocked;

    _Pending = 0;
    _Main = False;
    _Crash = False;

    pthread_mutex_init(&_Lock[0], None);
    pthread_mutex_init(&_Lock[1], None);
  }

  ~Watch() {
    /* @NOTE: try to resolve problems to prevent deadlock at the ending */

    pthread_mutex_destroy(&_Lock[0]);
    pthread_mutex_destroy(&_Lock[1]);
  }

  Bool Main(){ return _Main; }

  OMap<ULong, StatusE>& Status(){ return _Status; }

  Unique<Stopper>& Get(UInt name, ULong id, Bool safed = True) {
    if (_Stoppers.find(name) == _Stoppers.end()) {
      if (safed) UnlockMutex(0);

      /* @NOTE: not found this object type, should never reach */
      throw Except(EBadAccess,
                   Format{"Not found object type {}"}.Apply(name));
    } else if (_Stoppers[name].find(id) == _Stoppers[name].end()) {
      if (safed) UnlockMutex(0);

      /* @NOTE: not found the object, should never reach */
      throw Except(EBadAccess, Format{"Not found object {}"}.Apply(id));
    } else {
      return _Stoppers[name][id];
    }
  }

  Bool Spinlock(Int index) {
    if (index < 2) {
      return Locker::IsLocked(_Lock[index]);
    } else {
      return False;
    }
  }

  template <typename Type>
  Bool Reset(){
    UInt name = TypeId<Type>();

    _Crash = False;
    _Pending = 0;

    if (IsMain()){
      Vertex<void> escaping{[&](){ LockMutex(0); },
                            [&](){ UnlockMutex(0); }};

      /* @NOTE: clear everything now since we expect everything was done */
      if (typeid(Type) == typeid(Implement::Thread)){
        for (auto& item: _Status) {
          if (std::get<0>(item) == GetMID()) {
            continue;
          } else if (std::get<1>(item) != Released) {
            return False;
          } else {
            _Stoppers[name].erase(std::get<0>(item));
          }
        }
        _Status.clear();

        /* @NOTE: update status of main thread */
        _Status[GetMID()] = Unlocked;
        _Main = False;
      } else if (_Stoppers.find(name) != _Stoppers.end()) {
        _Stoppers[name].clear();
      }
      return True;
    }

    return False;
  }

  Bool IsMain(Bool safed = False){ return IsMain(GetMID(), safed); }

  ULong GetThreadId(Bool safed = False){
    return IsMain(safed)? GetMID(): ULong(GetTID());
  }

  void Summary(UInt type = 0) {
    UInt unlocked{0}, locked{0};

    for (auto& item: _Status) {
      auto tid = std::get<0>(item);
      auto status = std::get<1>(item);

      switch (type) {
      default:
        if (status) {
          INFO << Format{"thread {} has been locked"}.Apply(tid)
               << Base::EOL;
        }
        break;

      case 1:
        if (std::get<1>(item) == Unlocked) {
          unlocked++;
        } else if (std::get<1>(item) == Locked) {
          locked++;
        }
        break;
      }
    }

    switch(type) {
    case 1:
      INFO << Format{"Unlocked/Locked: {}/{}"}.Apply(unlocked, locked)
           << Base::EOL;
      break;

    default:
      break;
    }
  }

  ErrorCodeE OnWatching() {
    ULong thread_id = GetThreadId();;
    Vertex<void> escaping{[&](){ LockMutex(0); },
                          [&](){ UnlockMutex(0); }};

    if (!IsExist<Implement::Thread>(thread_id, True)) {
      return (NotFound << Format{"thread {}"}.Apply(thread_id)).code();
    }

    return ENoError;
  }

  ErrorCodeE OnExpiring() {
    ULong thread_id = GetThreadId();

    Vertex<void> escaping{[&](){ LockMutex(0); },
                          [&](){ UnlockMutex(0); }};

    if (!IsExist<Implement::Thread>(thread_id, True)) {
      return (NotFound << Format("thread {}").Apply(thread_id)).code();
    }

    /* @NOTE: remove current thread out of watching now */
    if (!Unregister<Implement::Thread>(thread_id, True, True)) {
      return DoAgain.code();
    }

    /* @NOTE: solve deadlock only happen on threads */
    if (!IsMain(True)){
      SolveDeadlock<Implement::Thread>(True);
    }

    return ENoError;
  }

  template <typename Type>
  void OnLocking(ULong id, Function<void()> callback, Bool force = False,
                 Bool safed = False) {
    ULong tid = GetThreadId(safed);
    UInt name = TypeId<Type>();
    UInt thread_t = TypeId<Implement::Thread>();

    if (Find(_Ignores.begin(), _Ignores.end(), id) >= 0) {
      callback();
    } else if (!std::is_base_of<Base::Internal::Stopper, Type>::value) {
      throw Except(ENoSupport, typeid(Type).name());
    } else {
      if (_Stoppers.find(name) == _Stoppers.end()) {
        _Stoppers[name] = Context{};
      }

      Vertex<void> escaping{[&](){ if (!safed) LockMutex(0); },
                            [&](){ if (!safed) UnlockMutex(0); }};

      if (force || !IsDeadlock(name, id, True)){
        if (name == TypeId<Implement::Thread>()) {
          Stopper* stopper = _Stoppers[thread_t][tid].get();

          if (!IsMain(True)) {
            /* @NOTE: usually, this line will never reach, but i'm not sure
             * it will be true on the far future */

            if (!Update(tid, Waiting, True)) {
              Abort(EBadLogic);
            }
          }

          if (stopper) {
            /* @NOTE: usually, this callback must do nothing but it will not
             * true if i change my ideal, call it would be better than none
             * */
            Vertex<void> escaping{[&](){ UnlockMutex(0); },
                                  [&](){ LockMutex(0); }};

            callback();
          }
        } else {
          Bool keep = True;

          if (_Stoppers[name][id]->Status()) {
            Stopper* stopper = _Stoppers[thread_t][tid].get();

            if (Update(tid, Locked, True)) {
              if (!stopper || !stopper->LockedBy(name, id)){
                keep = False;
              }
            } else {
              keep = False;
            }
          }

          if (keep) {
            Vertex<void> escaping{
              [&](){ UnlockMutex(0); },
              [&](){
                LockMutex(0);

                if (!Update(tid, Unlocked, True)) {
                  VERBOSE << "Found an unknown bug caused by updating to Unlocked"
                          << Base::EOL;
                }
              }};

            callback();
          } else {
              VERBOSE << "Unknown error cause checking status fail"
                      << Base::EOL;
          }
        }
      }
    }
  }

  template <typename Type>
  Bool OnUnlocking(ULong id, Function<void()> callback,
                   Bool force = False, Bool safed = False) {
    UInt name = TypeId<Type>();

    if (Find(_Ignores.begin(), _Ignores.end(), id) >= 0){
      return True;
    } else if (!std::is_base_of<Base::Internal::Stopper, Type>::value) {
      throw Except(ENoSupport, typeid(Type).name());
    } else if (_Stoppers.find(name) == _Stoppers.end()) {
      throw Except(EBadLogic, ToString(name) + " must exist");
    }

    Vertex<void> escaping{[&](){ if (!safed) LockMutex(0); },
                          [&](){ if (!safed) UnlockMutex(0); }};

    if (IsMain(True) && name == TypeId<Implement::Thread>()){
      /* @NOTE: do release at here in safed place to make sure our database always
       * keep updating */

      callback();

      /* @NOTE: clear everything now since we expect everything was done */
      for (auto& item: _Status) {
        if (std::get<0>(item) == GetMID()) {
          continue;
        } else if (std::get<1>(item) != Released) {
          return False;
        } else {
          _Stoppers[name].erase(std::get<0>(item));
        }
      }
      _Status.clear();

      /* @NOTE: update status of main thread */
      _Status[GetMID()] = Unlocked;
      _Main = False;

      /* @NOTE: we might release now since we don't have anything to do more */
      return True;
    }

    /* @NOTE: perform update database */
    if (!IsExist<Type>(id, True)) {
      /* @NOTE: there still be an unharm race condition here, just warning
       * and ignore this issue */

      UnlockMutex(0);
      BadLogic.Warn() << Format{"{} must exist before doing "
                                "unlocking"}.Apply(id);
      return False;
    } else if (!force && IsDoubleUnlock(name, id, True)) {
      return False;
    } else {
      /* @NOTE: unlock will only affect to the thread that ownes the lock so we
       * will update the status of the remote threads from here */

      Unique<Stopper>& locker = _Stoppers[name][id];

      /* @NOTE: do release at here in safed place to make sure our database always
       * keep updating */
      do {
        callback();

        if (!force && locker->Status()) {
          return False;
        }
      } while(force && locker->Status());

      /* @NOTE: unlock completely a bundle of threads, for now, the locker should
       * be released and no thread was locked by this locker recently */
      return True;
    }
  }

  template <typename Type>
  void Register(ULong id, Type* value, Bool safed = False) {
    UInt name = TypeId<Type>();

    if (!std::is_base_of<Base::Internal::Stopper, Type>::value) {
      throw Except(ENoSupport, typeid(value).name());
    } else {
      Vertex<void> escaping{[&](){ if (!safed) LockMutex(0); },
                            [&](){ if (!safed) UnlockMutex(0); }};

      if (_Stoppers.size() == 0 || _Stoppers.find(name) == _Stoppers.end()) {
        _Stoppers[name] = Context{};
      }

      if (_Stoppers[name].find(id) == _Stoppers[name].end()) {
        _Stoppers[name][id] = Unique<Base::Internal::Stopper>(value);

        if (name == TypeId<Implement::Thread>()) {
          _Status[id] = Unlocked;

          /* @NOTE: increase pending if a new thread applies */
          SYNC_FETCH_ADD(&_Pending, 1);
        }
      } else {
        /* @TODO: duplicated installation, what do we do next? */
      }
    }
  }

  template <typename Type>
  Bool Unregister(ULong id, Bool safed = False, Bool force = False) {
    UInt name = TypeId<Type>();

    if (!std::is_base_of<Base::Internal::Stopper, Type>::value) {
      throw Except(ENoSupport, typeid(Type).name());
    } else if (_Stoppers.find(name) != _Stoppers.end()) {
      Vertex<void> escaping{[&](){ if (!safed) LockMutex(0); },
                            [&](){ if (!safed) UnlockMutex(0); }};
      Int ignored = Find(_Ignores.begin(), _Ignores.end(), id);

      if (ignored >= 0){
        _Ignores.erase(_Ignores.begin() + ignored);
      }

      if (typeid(Type) == typeid(Implement::Thread)){
        if (Update(id, Released, True, force)){
          _Stoppers[name][id] = None;

          /* @NOTE: decrease pending if a thread is closed */
          SYNC_FETCH_ADD(&_Pending, -1);
        } else {
          UnlockMutex(0);
          safed = True;

          return !(BadLogic << Format{"Can\'t unregister "
                                      "the thread {}"}.Apply(id));
        }
      } else if (_Stoppers[name].find(id) != _Stoppers[name].end()){
        _Stoppers[name].erase(id);
      }

      return True;
    } else {
      return !(BadLogic << Format{"Can\'t unregister the stopper {}"}.Apply(id));
    }
  }

  template <typename Type>
  Bool IsExist(ULong id, Bool safed = False) {
    if (!std::is_base_of<Base::Internal::Stopper, Type>::value) {
      throw Except(ENoSupport, typeid(Type).name());
    } else {
      Vertex<void> escaping{[&](){ if(!safed) LockMutex(0); },
                            [&](){ if(!safed) UnlockMutex(0); }};
      UInt name = TypeId<Type>();

      if (_Stoppers.size() == 0) {
        return False;
      } else if (_Stoppers.find(name) == _Stoppers.end()) {
        return False;
      } else {
        return _Stoppers[name].find(id) != _Stoppers[name].end();
      }
    }
  }

  template <typename Type>
  UInt Count() {
    UInt name = TypeId<Type>();
    UInt result = 0;

    if (!std::is_base_of<Base::Internal::Stopper, Type>::value) {
      throw Except(ENoSupport, typeid(Type).name());
    }

    for (auto& item: _Stoppers[name]){
      if (std::get<1>(item)) result++;
    }

    return result;
  }

  template <typename Type>
  Bool Ignore(ULong id){
    UInt name = TypeId<Type>();

    if (Find(_Ignores.begin(), _Ignores.end(), id) >= 0){
      return True;
    } else {
      _Ignores.push_back(id);
    }

    if (IsExist<Type>(id)){
      if (name == TypeId<Implement::Thread>()){
        /* @NOTE: it very dangerous to ignore a stopper recklessly since
         * it would lead our system into Deadlock state or corruption
         * so we must simmulate the whole system to make sure everything
         * works fine */

        if (!IsDeadlock()){
          _Ignores.erase(_Ignores.begin() + _Ignores.size());
          return False;
        }
      }

      if (!Unregister<Type>(id)) {
        return False;
      }
    }

    return True;
  }

  template <typename Type>
  Type* GetStopper(ULong id){
    UInt name{};

    if (typeid(Type) == typeid(Base::Thread)){
      name = TypeId<Implement::Thread>();
    } else if (typeid(Type) == typeid(Base::Lock)){
      name = TypeId<Implement::Lock>();
    } else {
      return None;
    }

    if (IsMain(id)){
      return None;
    } else if (_Stoppers[name].find(id) != _Stoppers[name].end()){
      return dynamic_cast<Type*>(_Stoppers[name][id].get());
    } else {
      return None;
    }
  }

  void LockMutex(UInt index) {
    if (index < 2 && !_Crash.load()) {
      /* @NOTE: DState will indicate that we are on internal deadlock
       * because of using OnLocking */

      pthread_mutex_lock(&_Lock[index]);
    } else {
      _Crash = True;
      throw Except(ENotFound, Format{"inner mutex {}"} << index);
    }
  }

  void UnlockMutex(UInt index) {
    if (index < 2 && !_Crash.load()) {
      pthread_mutex_unlock(&_Lock[index]);
    } else {
      _Crash = True;
      throw Except(ENotFound, Format{"inner mutex {}"} << index);
    }
  }

  template<typename Type>
  static UInt TypeId(){
    if (typeid(Type) == typeid(Implement::Thread)) {
      return 1;
    } else if (typeid(Type) == typeid(Implement::Lock)) {
      return 2;
    } else if (typeid(Type) == typeid(Implement::IOSync)) {
      return 3;
    } else if (typeid(Type) == typeid(Implement::Main)) {
      return 4;
    } else {
      return 0;
    }
  }

 private:
  Bool Update(ULong id, StatusE status, Bool safed = False, Bool force = False) {
    UInt name{TypeId<Implement::Thread>()};
    Vertex<void> escaping{[&](){ if (!safed) LockMutex(0); },
                          [&](){ if (!safed) UnlockMutex(0); }
      };

    /* @NOTE: if the thread don't exist, don't do anything */
    if (_Status.find(id) == _Status.end()){
      return False;
    }

    /* @NOTE: don't update if the thread is released */
    if (_Status[id] == Released){
      return True;
    }

    /* @NOTE: don't lock/unlock if the thread is on waiting */
    if (_Status[id] == Waiting && status < Waiting) {
      return False;
    }

    if (id == GetMID()) {
      if (status == Waiting) {
        _Main = True;
      }
    } else if (status == Unlocked && _Status[id] == Locked) {
      UnlockMutex(1);
    }

    if (status == Released && !force) {
      if (!_Stoppers[name][id]->Expired()) {
        return False;
      }
    }

    _Status[id] = status;
    return True;
  }

  Bool Wait(Bool safed, ULong sec = 0, ULong nsec = 10){
    while(True) {
      struct timespec timeout;

      /* @NOTE: set the next time wiil be used to check */
      clock_gettime(CLOCK_REALTIME, &timeout);
      timeout.tv_sec += sec;
      timeout.tv_nsec += nsec;

      /* @NOTE: show on debug log the time when we recheck the locking
       * system */
      if (Log::Level() == EDebug) {
        Char buff[TIME_FMT];

        memset(buff, 0, sizeof(buff));
        if (!Timespec2String(buff, sizeof(buff), &timeout)) {
          // DEBUG(Format{"check again at {}"}.Apply(buff));
        }
      }

      if (IsMain(safed)){
        switch(pthread_mutex_timedlock(&_Lock[1], &timeout)){
          case EAGAIN:
            break;

          case EINVAL:
            return False;

          case ETIMEDOUT:
            return False;

          default:
            return True;
        }
      } else {
        VERBOSE << (Format{"Wait() run on thread {}"} << GetThreadId(safed))
                << Base::EOL;
        return False;
      }
    }
  }

  Bool IsMain(ULong id, Bool safed = False){
    UInt name = TypeId<Implement::Thread>();
    Vertex<void> escaping{[&](){ if (!safed) LockMutex(0); },
                          [&](){ if (!safed) UnlockMutex(0); }
      };

    if (_Stoppers[name].find(id) != _Stoppers[name].end()){
      if (dynamic_cast<Implement::Main*>(_Stoppers[name][id].get())){
        return True;
      }
    }

    return False;
  }

  UInt IsDeadlock(Bool safed = False){
    using namespace Implement;

    /* @NOTE: now we must check on the whole system again to make sure that
     * we don't on deadlock crisis */

    UInt locked{0}, unlocked{0};
    UInt name{TypeId<Implement::Thread>()};
    Vertex<void> escaping{[&](){ if (!safed) LockMutex(0); },
                          [&](){ if (!safed) UnlockMutex(0);}
      };

    for (auto& item: _Status){
      auto& tid = std::get<0>(item);

      if (_Stoppers[name].find(tid) == _Stoppers[name].end()) {
        continue;
      } else if (_Stoppers[name][tid] == None) {
        continue;
      } else if (std::get<1>(item) == Unlocked) {
        unlocked++;
      } else if (std::get<1>(item) == Locked) {
        locked++;
      } else if (std::get<1>(item) == Waiting) {
        locked++;
      }
    }

    if (unlocked > 0) {
      return 0;
    } else if (locked == 1 && IsMain(True)){
      return 0;
    }

    return locked;
  }

  Bool IsDeadlock(UInt name, ULong causer, Bool safed = False) {
    Vertex<void> escaping{[&](){ if (!safed) LockMutex(0); },
                          [&](){ if (!safed) UnlockMutex(0); }};
    ULong thread_id = GetThreadId(True);
    UInt thread_t = TypeId<Implement::Thread>();

    /* @NOTE: the first screenario is when we reuse a locked locker and it
     * cause this issue */
    if (_Stoppers[thread_t].find(thread_id) == _Stoppers[thread_t].end()){
      return False;
    }

    if (name == TypeId<Implement::Lock>()){
      /* @NOTE: the reason why i have to check it because some of the lock can
       * be called again event if a thread has been expired and cause a big
       * issue above since Map will re-add thread_id again and missmatch with
       * the flow */

      if (_Stoppers[name].find(causer) == _Stoppers[name].end()) {
        return False;
      } else if (_Stoppers[name][causer]->Status()) {
        auto saved = _Status[thread_id];

        if (Update(thread_id, Locked, True)) {
          if (IsDeadlock(True)) {
            return Update(thread_id, saved, True);
          } else {
            return False;
          }
        }
      }
    } else if (name == TypeId<Implement::Thread>()){
      /* @NOTE: this should happen on main thread */

      if (IsMain(True)){
        /* @NOTE: solve deadlock problem now since main thread call
         * `pthread_join` here after */

        if (_Stoppers[thread_t][thread_id]->LockedBy(thread_t, thread_id)) {
          if (Update(thread_id, Waiting, True)) {
            SolveDeadlock<Implement::Thread>(True);
          } else {
            UnlockMutex(0);
            throw Except(EBadLogic, "can\'t switch to waiting stage of "
                                    "main thread");
          }
        } else {
          UnlockMutex(0);
          throw Except(EBadLogic, "can\'t register locking itself on "
                                  "main thread");
        }

        return True;
      } else {
        UnlockMutex(0);
        throw Except(EBadLogic, "`pthread_join` must be run on main thread");
      }
    }

    return False;
  }

  Bool IsDoubleUnlock(UInt name, ULong causer, Bool safed = False) {
    /* @NOTE: this is a conventional tool which aims to detect double unlock
     * and avoid it to reduce wasting resource */

    Vertex<void> escaping{[&](){ if (!safed) LockMutex(0); },
                          [&](){ if (!safed) UnlockMutex(0);}};
    Bool result = False;

    if (name == TypeId<Implement::Lock>()){
      /* @NOTE: on lock perspective, unlocking an unlocked mutex cause
       * performance issue somehow so we must detect and avoid this minor issue
       * */

      if (_Stoppers[name].find(causer) != _Stoppers[name].end()){
        result = _Stoppers[name][causer]->Status() == False;
      }

      if (result) {
        VERBOSE << Format{"detect double unlock {} on thread {}"}
                         .Apply(causer, GetThreadId(safed))
                << Base::EOL;
      }
    }

    return result;
  }

  template<typename Type>
  void SolveDeadlock(Bool safed = False){
    UInt name{TypeId<Type>()}, count{0};

    do {
      struct timespec start, end;
      Vertex<void> escaping{[&]() { if (!safed) LockMutex(0);},
                            [&]() { if (!safed) UnlockMutex(0); }};

      /* @NOTE: get start time */
      clock_gettime(CLOCK_REALTIME, &start);

      /* @NOTE: check deadlock and solve the issue */
      if ((count = IsDeadlock(True))) {
        auto& threads = _Stoppers[name];

        for (auto it = threads.begin(); it != threads.end(); it++) {
          if (!it->second) {
            /* @NOTE: unexpected error happen, killing the app is the best
             * solution here */

            if (_Status[it->first] == Waiting) {
              continue;
            } else if (_Status[it->first] == Released) {
              continue;
            } else if (_Status.find(it->first) != _Status.end()) {
              /* @FIXME: thread_id change status, i can't show message
               * here dua to fluctuated locks */
              continue;
            }
          } else {
            it->second->Unlocked();
          }
        }
      }

      /* @NOTE: get end time */
      clock_gettime(CLOCK_REALTIME, &end);
      safed = True;

      if (IsMain(True)) {
        /* @NOTE: on very busy system, main thread may be locked before
         * some thread and cause stuck when they turn to locked */

        Vertex<void> escaping{[&](){ UnlockMutex(0); },
                              [&](){ LockMutex(0); }};

        /* @TODO: i will implement another solution to reduce checking so much
         * on main thread */
        if (Wait(False, 2*_Pending*(end.tv_sec - start.tv_sec),
                        2*_Pending*(end.tv_nsec - start.tv_nsec))) {
          DEBUG("Solving deadlock's locker has been unlocked by an unknown thread");
        }
      }
    } while(IsMain(True) &&  _Pending > 0);
  }

  /* @NOTE: database of Watch */
  OMap<UInt, Context> _Stoppers;
  OMap<ULong, StatusE> _Status;
  Vector<ULong> _Ignores;

  /* @NOTE: internal locks of Watch */
  Mutex _Lock[2];

  /* @NOTE: save status of Watch */
  Bool _Main;
  Int _Pending;
};
}  // namespace Internal
}  // namespace Base

namespace Base {
namespace Internal {
static Watch Watcher{};

ULong GetUniqueId() { return Watcher.GetThreadId(); }

Bool Stopper::LockedBy(UInt name, ULong id){
  _LockId = id;

  if (name == Watch::TypeId<Implement::Thread>()){
    /* @NOTE: happen when we call pthread_join */

    _LockType = EJoin;
  } else if (name == Watch::TypeId<Implement::IOSync>()){
    /* @NOTE: when a I/O system-call was triggered */

    _LockType = EIOSync;
  } else if (name == Watch::TypeId<Implement::Lock>()){
    auto& locker = Watcher.Get(name, id, True);

    /* @NOTE: happen when we call pthread_mutex_lock */
    if (!locker) {
      return False;
    } else {
      /* @NOTE: this line mustn't be reached be cause we never see a stopper
       * has been locked more than one with the same locker */

      _LockType = EMutex;
    }
  }

  return True;
}

Bool RunAsDaemon(Base::Thread& thread) {
  /* @NOTE: by default, run as daemon means we don't manage this thread
   * any more even if we add again this thread to*/

  return Watcher.Ignore<Implement::Thread>(thread.Identity());
}

Function<void(Bool)> WatchStopper(Base::Thread& thread) {
  Vertex<void> escaping{[](){ Watcher.LockMutex(0); },
                        [](){ Watcher.UnlockMutex(0); }};

#if USE_TID
  ULong id = GetTID();
#else
  ULong id = thread.Identity();
  Bool reused = False;

  if ((ULong)(GetTID()) == id){
    /* @NOTE: it should be race condition here but it's not so important, so
     * we don't need to care about it too much */

    if (Watcher.Status().find(id) != Watcher.Status().end()){
      reused = Watcher.Status()[id] == Watch::Released;
    }
  }

  if (reused) {
    return [](Bool UNUSED(status)) {};
  }
#endif

  if (!Watcher.IsExist<Implement::Thread>(id, True)) {
    Watcher.Register(id, new Implement::Thread(thread), True);
  }

  return [](Bool status) {
    ULong id = GetMID();

    if (status) {
      Watcher.OnLocking<Implement::Thread>(id, [&](){});
    } else {
      Watcher.OnUnlocking<Implement::Thread>(id, [&](){});
    }
  };
}

void WatchStopper(Base::Lock& lock) {
  ULong id = lock.Identity();

  if (!Watcher.IsExist<Implement::Lock>(id)) {
    Watcher.Register(id, new Implement::Lock(lock));
  }
}

Bool UnwatchStopper(Base::Thread& thread){
  ULong id = thread.Identity();
  Bool reused = False;

  if ((ULong)(GetTID()) == id){
    /* @NOTE: it should be race condition here but it's not so important, so
     * we don't need to care about it too much */

    Vertex<void> escaping{
      []() {
        if (!Watcher.Spinlock(0)) {
          Watcher.LockMutex(0);
        }
      },
      []() { Watcher.UnlockMutex(0); }
    };

    if (Watcher.Status().find(id) != Watcher.Status().end()){
      reused = Watcher.Status()[id] == Watch::Released;
    }
  }

  if (!reused) {
    if (Watcher.IsExist<Implement::Thread>(id)) {
      return Watcher.Unregister<Implement::Thread>(id);
    }
  }

  return True;
}

void UnwatchStopper(Base::Lock& lock){
  ULong id = lock.Identity();
  ULong tid = (ULong)GetTID();
  Bool dying = False;

  if (Watcher.Status().find(tid) != Watcher.Status().end()) {
    Vertex<void> escaping{
      []() {
        if (Watcher.Spinlock(0)) {
          Watcher.LockMutex(0);
        }
      },
      []() { Watcher.UnlockMutex(0); }
    };

    dying = Watcher.Status()[tid] == Watch::Released;
  }

  if (Watcher.IsExist<Implement::Lock>(id, dying)) {
    Watcher.Unregister<Implement::Lock>(id, dying);
  }
}

Bool KillStoppers(UInt signal = SIGTERM) {
  if (Watcher.Spinlock(0)) {
    return False;
  } else if (!Watcher.IsMain(True)) {
    return False;
  } else {
    auto count = 0;

    for (auto& thread: Watcher.Status()){
      auto& tid = std::get<0>(thread);
      auto& status = std::get<1>(thread);

      /* @FIXME: send sigterm to the whole threads, by default I might use
       * SIGTERM but I'm not sure it's good or not */
      if (status != Watch::Released) {
        auto thread = Watcher.GetStopper<Base::Thread>(tid);

        if (thread) {
          pthread_kill((pthread_t)thread->Identity(), signal);
          count++;
        }
      }
    }

    Watcher.Reset<Implement::Thread>();
    Watcher.Reset<Implement::Lock>();
    Watcher.Reset<Implement::IOSync>();

    return count != 0;
  }
}

namespace Summary{
void WatchStopper(){ Watcher.Summary(); }
}  // namespace Summary
}  // namespace Internal

namespace Locker {
void Lock(ULong id, Bool force) {
  using namespace Internal;
  Mutex* locker = (Mutex*)(id);

  /* @NOTE: we must register this locker if it didn't exist */
  if (!Watcher.IsExist<Implement::Lock>(id)){
    Watcher.Register(id, new Implement::Lock(*locker));
  }

  /* @NOTE: now, we going to use this locker now */
  Watcher.OnLocking<Internal::Implement::Lock>(id, [&]() {
    auto delay = 0;

    if (Log::Level() == EDebug && IsLocked(*locker)) {
      if (!force) {
        throw Except(EBadLogic, "lock a locked mutex without forcing");
      } else {
        Vertex<void> escaping{[&](){ Watcher.LockMutex(0); },
                              [&](){ Watcher.UnlockMutex(0); }};

        DEBUG(Format{"force locking a locked mutex({})"}.Apply(id));
      }
    }

    do {
      /* @NOTE: fix race condition cause by unknown error inside pthread */

      delay += 1;

      if (locker->__data.__owner) {
        usleep(delay);
        continue;
      } else {
        pthread_mutex_lock(locker);
      }
    } while(False);
  }, force);
}

void Lock(Mutex& locker, Bool force) {
  using namespace Internal;
  ULong id = ULong(&locker);

  /* @NOTE: we must register this locker if it didn't exist */
  if (!Watcher.IsExist<Implement::Lock>(id)){
    Watcher.Register(id, new Implement::Lock(locker));
  }

  /* @NOTE: now, we going to use this locker now */
  Watcher.OnLocking<Implement::Lock>(id, [&]() {
    auto delay = 0;

    do {
      /* @NOTE: fix race condition cause by unknown error inside pthread */

      delay += 1;

      if (locker.__data.__owner) {
        usleep(delay);
        continue;
      } else {
        pthread_mutex_lock(&locker);
      }
    } while(False);
  }, force);
}

void Unlock(ULong id, Bool force) {
  using namespace Internal;
  auto locker = (Mutex*)(id);
  auto perform = [&]() {
    do {
      pthread_mutex_unlock(locker);
    } while(force && IsLocked(*locker));
  };

  if (!Watcher.OnUnlocking<Implement::Lock>(id, perform, force, True)) {
    if (force) {
      Abort(EBadAccess);
    } else {
      DEBUG("Fail to unlock a locker");
    }
  }
}

void Unlock(Mutex& locker) {
  using namespace Internal;
  auto perform = [&]() {
    pthread_mutex_unlock(&locker);
  };

  if (!Watcher.OnUnlocking<Implement::Lock>(ULong(&locker), perform)) {
    DEBUG("Fail to unlock a locker");
  }
}
}  // namespace Locker

Mutex* GetMutex(Lock& lock){
  return lock._Lock;
}

Thread::StatusE GetThreadStatus(Thread& thread){
  return thread._Status;
}

void* Booting(void* thiz) {
  Base::Thread* thread = (Base::Thread*)thiz;

  using namespace Base::Internal;
  using namespace Base::Internal;

  if ((thread->_Watcher = Internal::WatchStopper(*thread))) {
    ErrorCodeE error{Internal::Watcher.OnWatching()};

    if (!error) {
      Base::Vertex<void> escaping{
        [&]() {
          Vertex<void> escaping{[&](){ Watcher.LockMutex(0); },
                                [&](){ Watcher.UnlockMutex(0); }};

          ((Base::Thread*)thiz)->_Status = Base::Thread::Running;
        },
        [&]() {
          Vertex<void> escaping{[&](){ Watcher.LockMutex(0); },
                                [&](){ Watcher.UnlockMutex(0); }};

          if (Watcher.IsExist<Implement::Thread>((ULong)GetTID(), True)) {
            ((Base::Thread*)thiz)->_Status = Base::Thread::Expiring;
          } else {
            /* @NOTE: unlock this to prevent deadlock inside WatchStopper */
            Watcher.UnlockMutex(0);

            /* @FIXME: report an error to fix latter, consider how to show
             * log here */
            error = EDoNothing;
          }
        }};

      thread->_Delegate();
      do {
        if (error == EDoAgain) {
          usleep(1);
        }

        error = Internal::Watcher.OnExpiring();
      } while (error == EDoAgain);
    }

    if (error && error != EDoNothing) {
      Except(error, "A system error happens");
    }
  }

  return None;
}

namespace Internal{
namespace Implement{
Bool Thread::Unlocked(){

  if (_LockType == EMutex){
    Mutex* mutex = (Mutex*)_LockId;

    if (Locker::IsLocked(*mutex)) {
      Locker::Unlock(_LockId, True);

      /* @NOTE: mutex may lock permantly caused by unexpected error */
      return !Locker::IsLocked(*mutex);
    } else {
      return False;
    }
  } else {
    return False;
  }
}

Bool Thread::Expired(){
  if (_Thread) {
    return GetThreadStatus(*_Thread) == Base::Thread::Running;
  } else {
    return False;
  }
}

Bool Main::Expired(){
  return False;
}
}  // namespace Implement
}  // namespace Internal
}  // namespace Base
