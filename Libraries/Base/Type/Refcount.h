#if !defined(BASE_REFCOUNT_H_)
#define BASE_REFCOUNT_H_
#include <Common.h>

#if __cplusplus
namespace Base {
class Refcount {
 public:
  /* @NOTE: the destructor */
  virtual ~Refcount();

  /* @NOTE: the contrustors */
  Refcount(const Refcount& src);
  Refcount(Refcount&& src);
  Refcount();

  /* @NOTE: the assign operator */
  Refcount& operator=(const Refcount& src);

#if defined(BASE_UNITTEST_H_) || defined(BASE_REFCOUNT_CC_)
  Bool IsExist();
  Int Count();
#endif  // BASE_UNITTEST_H_

 protected:
  virtual void _Init() {}
  virtual void _Release() {}

  void Init();
  void Release();
  Bool Reference(const Refcount* src);

 private:
  void Release(Bool safed);

  Int* _Count;
};
} // namespace Base
#endif
#endif  // BASE_REFCOUNT_H_
