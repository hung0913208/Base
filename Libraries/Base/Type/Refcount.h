#if !defined(BASE_REFCOUNT_H_)
#define BASE_REFCOUNT_H_
#include <Type/ABI.h>

#if __cplusplus
namespace Base {
class Refcount {
 public:
  /* @NOTE: the destructor */
  virtual ~Refcount();

  /* @NOTE: the contrustors */
  explicit Refcount(void (*release)(Refcount* thiz));
  explicit Refcount(void (*init)(Refcount* thiz),
                    void (*release)(Refcount* thiz));
  Refcount();
  Refcount(const Refcount& src);
  Refcount(Refcount&& src);

  /* @NOTE: the assign operator */
  Refcount& operator=(const Refcount& src);

#if defined(BASE_UNITTEST_H_) || defined(BASE_REFCOUNT_CC_)
  Bool IsExist();
  Int Count();
#endif  // BASE_UNITTEST_H_

  Void Secure(Int index, Void* address);
  Void* Access(Int index);

 protected:
  void (*_Init)(Refcount* thiz);
  void (*_Release)(Refcount* thiz);

  void Init();
  void Release();
  void Secure(Function<void()> callback);
  Bool Reference(const Refcount* src);

 private:
  void Release(Bool safed);

  Map<Int, Void*> _Secure;
  Bool _Status;
  Int* _Count;
  Void* _Context;
};
} // namespace Base
#endif
#endif  // BASE_REFCOUNT_H_
