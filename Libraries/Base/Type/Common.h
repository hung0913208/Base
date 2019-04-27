#ifndef BASE_TYPE_COMMON_H_
#define BASE_TYPE_COMMON_H_
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
typedef unsigned int UInt;
typedef unsigned int* UInts;
typedef long Long;
typedef long* Longs;
typedef unsigned long ULong;
typedef unsigned long* ULongs;
typedef float Float;
typedef float* Floats;
typedef double Double;
typedef double* Doubles;

typedef pthread_mutex_t Mutex;
typedef pthread_cond_t Checker;

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
#endif  // BASE_TYPE_COMMON_H_
