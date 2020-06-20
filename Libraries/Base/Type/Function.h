#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Macro.h>
#else
#include <Macro.h>
#endif

#if !defined(BASE_TYPE_FUNCTION_H_) && (USE_BASE_FUNCTION || APPLE)
#define BASE_TYPE_FUNCTION_H_

#if __cplusplus
#if USE_BASE_WITH_FULL_PATH_HEADER
//#include <Base/Type/ABI.h>
#include <Base/Type/Common.h>
#else
//#include <ABI.h>
#include <Common.h>
#endif

namespace Base {
template <typename T>
class Function;

template <typename R, typename... Args>
class Function<R(Args...)> {
 private:
  /* @NOTE: Function pointer types for the type-erasure behaviors all these
   * char* parameters are actually casted from some functor type */
  typedef R (*Invoke)(char*, Args&&...);
  typedef void (*Construct)(char*, char*);
  typedef void (*Destroy)(char*);

  template <typename F>
  static R InvokeFn(F* fn, Args&&... args) {
    return (*fn)(args...);
  }

  template <typename F>
  static void ConstructFn(F* dst, F* src) { new (dst) F(*src); }

  template <typename F>
  static void DestroyFn(F* f){ f->~F(); }

 public:
  Function(): _Invoke{None}, _Construct{None}, _Destroy{None},
              _Data{None}, _Size{0}
  {}

  template <typename F>
  Function(F f): _Invoke{reinterpret_cast<Invoke>(InvokeFn<F>)},
                 _Construct{reinterpret_cast<Construct>(ConstructFn<F>)},
                 _Destroy{reinterpret_cast<Destroy>(DestroyFn<F>)},
                 _Data{new char[sizeof(F)]},
                 _Size{sizeof(F)} {
    ConstructFn(_Data, reinterpret_cast<char*>(&f));
  }

  Function(Function const& rhs): _Invoke(rhs._Invoke),
                                 _Construct(rhs._Construct),
                                 _Destroy(rhs._Destroy),
                                 _Size(rhs._Size) {
    if (_Invoke) {
      delete _Data;
      _Data = new char[_Size];
      _Construct(_Data, rhs._Data);
    }
  }

  ~Function() {
    _Destroy(_Data);
    delete _Data;
  }

  R operator()(Args&&... args) {
    return _Invoke(_Data, RValue(args)...);
  }

  operator bool() {
    return _Size != 0;
  }

 protected:
  Invoke _Invoke;
  Construct _Construct;
  Destroy _Destroy;

  CString _Data;
  ULong _Size;
};
} // namespace Base
#else
#endif
#endif  // BASE_FUNCTION_H_
