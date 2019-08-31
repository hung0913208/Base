#if !defined(BASE_LOCK_H_) && __cplusplus
#define BASE_LOCK_H_
#include <Type.h>

namespace Base {
class Lock {
 public:
  enum StatusE {
    Unknown = 0,
    Locked = 3,
    Unlocked = 2,
    Unlocking = 4,
    Released = 5
  };

  explicit Lock(Bool locked = False);
  virtual ~Lock();

  Lock(const Lock& src);

  Lock& operator=(const Lock& src);
  Lock& operator=(Lock&& src);
  Lock& operator()();
  Lock& operator()(Bool lock);
  operator bool();

  const Lock& operator()() const;
  const Lock& operator()(Bool lock) const;
  operator bool() const;

  void Wait(Function<Bool()> event, Function<void()> react);
  void Wait(Function<Bool()> event);
  void Safe(Function<void()> callback);
  ULong Identity();
  StatusE Status();

 private:
  friend Void* Context(Lock& lock);
  friend Mutex* GetMutex(Lock& lock);
  friend void Register(Lock& lock, Void* context);

  Int* _Count;
  Void* _Context;
  Mutex* _Lock;
};
}  // namespace Base
#endif  // BASE_LOCK_H_
