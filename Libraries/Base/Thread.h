#ifndef BASE_THREAD_H_
#define BASE_THREAD_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Property.h>
#include <Base/Type.h>
#else
#include <Property.h>
#include <Type.h>
#endif

#if __cplusplus
namespace Base {
class Thread {
 private:
  using PThread = pthread_t;

 public:
  enum StatusE {
    Unknown = 0,
    Initing = 1,
    Running = 2,
    Halting = 4,
    Expiring = 5
  };

  typedef Void(*Notify)(Void* context, Bool status);

  explicit Thread(bool daemon = False);
  explicit Thread(const Thread& src);
  explicit Thread(Thread& src);
  virtual ~Thread();

#if __cplusplus >= 201402L
  template <class... Args>
  void Prepare(Function<void(Args...)> delegate, Args... args) {
    _Delegate = [=]() { delegate(RValue(args)...); };
  }

  template <class... Args>
  void Start(Function<void(Args...)> delegate, Args... args) {
    Start([=]() { delegate(RValue(args)...); });
  }
#endif

  template <typename Type>
  void Prepare(Function<void(Type)> delegate, Type value) {
    _Delegate = [=]() { delegate(value); };
  }

  template <typename Type>
  Bool Start(Function<void(Type)> delegate, Type value) {
    return Start([=]() { delegate(value); });
  }

  Bool Start(Function<void()> delegate = None);
  Bool Daemonize(Bool value);

  ULong Identity(Bool uuid = True);
  Bool Daemon();
  StatusE Status();

 private:
  friend Void* Context(Thread& thread);
  friend void Register(Thread& thread, Void* context);
  friend void* Booting(void* thiz);
  friend StatusE GetThreadStatus(Thread& thread);
  friend void SetThreadStatus(Thread& thread, Int status);

  Function<void()> _Delegate;
  Notify _Watcher;
  PThread _ThreadId;
  StatusE _Status;
  ULong _UUID;
  Void* _Context;
  Bool _Registering, _Daemon;
  Int* _Count;
};
}  // namespace Base
#endif
#endif  // BASE_THREAD_H_
