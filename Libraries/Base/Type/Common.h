#ifndef BASE_TYPE_COMMON_H_
#define BASE_TYPE_COMMON_H_
#include <Macro.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

enum ErrorCodeE {
  ENoError = 0,
  EKeepContinue = 1,
  ENoSupport = 2,
  EBadLogic = 3,
  EBadAccess = 4,
  EOutOfRange = 5,
  ENotFound = 6,
  EDrainMem = 7,
  EWatchErrno = 8,
  EInterrupted = 9,
  EDoNothing = 10,
  EDoAgain = 11
};

enum ErrorLevelE { EError = 3, EWarning = 2, EInfo = 1, EDebug = 0 };

#if __cplusplus
#define True true
#define False false

typedef bool Bool;
#define None nullptr
#else
#include <stdlib.h>

#define None NULL
#define True 1
#define False 0

#define Bool int
#define None NULL
#endif

#define CString char*
#define Void void
#define Byte int8_t
#define Bytes int8_t*
#define Word int16_t
#define Words int16_t*
#define DWord int32_t
#define DWords int32_t*
#define LWord int64_t
#define LWords int64_t*

typedef char Char;
typedef int Int;
typedef int* Ints;
typedef short Short;
typedef short* Shorts;
typedef unsigned short UShort;
typedef unsigned short* UShorts;
typedef unsigned int UInt;
typedef unsigned int* UInts;
typedef long Long;
typedef long* Longs;
typedef long LLong;
typedef long* LLongs;
typedef unsigned long ULong;
typedef unsigned long* ULongs;
typedef unsigned long long ULLong;
typedef unsigned long long* ULLongs;
typedef float Float;
typedef float* Floats;
typedef double Double;
typedef double* Doubles;

#define LIKELY(x, y) ((x) == (y))
#define UNLIKELY(x, y) ((x) != (y))

typedef pthread_mutex_t Mutex;
typedef pthread_cond_t Checker;

#define MUTEX(mutex) pthread_mutex_init(mutex, None)
#define DESTROY(mutex) pthread_mutex_destroy(mutex)
#define LOCK(mutex) pthread_mutex_lock(mutex)
#define UNLOCK(mutex) pthread_mutex_unlock(mutex)
#define TRYLOCK(mutex) pthread_mutex_trylock(mutex)
#define WAIT(mutex)   \
  {                   \
    TRYLOCK(mutex);   \
    LOCK(mutex);      \
  }

#if __cplusplus
#define ISLOCKED(mutex) Base::Locker::IsLocked(mutex)
#else
# define ISLOCKED(mutex) IsLocked(mutex)
#endif

#if !__cplusplus
static Bool IsLocked(Mutex* mutex) {
#if USE_SPINLOCK
  Bool is_locked = (Bool)(ISLOCKED(mutex));
#else
  Bool is_locked = (Bool)pthread_mutex_trylock(mutex);

  if (!is_locked) pthread_mutex_unlock(mutex);
#endif
  return is_locked;
}
#endif

#if __cplusplus
namespace Base {
namespace Locker {
static Bool IsLocked(Mutex* mutex) {
#if USE_SPINLOCK
  Bool is_locked = (Bool)(ISLOCKED(mutex));
#else
  Bool is_locked = (Bool)pthread_mutex_trylock(mutex);

  if (!is_locked) pthread_mutex_unlock(mutex);
#endif
  return is_locked;
}

static Bool IsLocked(Mutex& mutex) {
  return IsLocked(&mutex);
}
} // namespace Locker
} // namespace Base
#endif

#if LINUX
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

typedef time_t TimeT;

#ifdef SYS_gettid
#define GetUUID() static_cast<ULong>(syscall(SYS_gettid))
#else
#error "SYS_gettid unavailable on this system"
#endif
#elif APPLE || BSD
#include <sys/syscall.h>
#include <sys/types.h>

ULong GetUUID() {
  uint64_t tid;

  pthread_threadid_np(None, &tid);
  return tid;
}

#elif WINDOW
#include <windows.h>

#define GetUUID GetCurrentThreadId
#else
#define GetUUID pthread_self
#endif
#endif  // BASE_TYPE_COMMON_H_
