#if !defined(BASE_REFCOUNT_H_)
#define BASE_REFCOUNT_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/Common.h>
#include <Base/Type/Function.h>
#else
#include <Common.h>
#include <Function.h>
#endif

#if __cplusplus
namespace Base {
template<typename KeyT, typename ValueT, typename IndexT=Int>
class Hashtable;

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

  Hashtable<Int, Void*>* _Secure;
  Bool _Status;
  Int* _Count;
  Void* _Context;
};
} // namespace Base
#endif
#endif  // BASE_REFCOUNT_H_
