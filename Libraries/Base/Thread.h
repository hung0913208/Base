#ifndef BASE_THREAD_H_
#define BASE_THREAD_H_
#include <Property.h>
#include <Type.h>

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
    Waiting = 3,
    Expiring = 4
  };

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
  ULong Identity();

 private:
  friend void* Booting(void* thiz);
  friend StatusE GetThreadStatus(Thread& thread);

  Function<void(Bool)> _Watcher;
  Function<void()> _Delegate;
  PThread _ThreadId;
  StatusE _Status;
  Int* _Count;
};
}  // namespace Base
#endif
#endif  // BASE_THREAD_H_
