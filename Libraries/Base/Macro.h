#ifndef BASE_MACRO_H_
#define BASE_MACRO_H_

#if defined(COVERAGE) || !defined(NDEBUG)
#define DEBUGING 1
#endif

#ifndef SUPPRESS_UNUSED_ATTRIBUE
#if defined(__GNUC__) && defined(__clang__)
#define UNUSED(x) x __attribute__((__unused__))
#else
#define UNUSED(x) x
#endif // __GNUC__

#if defined(__GNUC__) && defined(__clang__)
#define UNUSED_FUNCTION(x) __attribute__((__unused__)) x
#else
#define UNUSED_FUNCTION(x) x
#endif // __GNUC__
#else
#define UNUSED(x) x
#define UNUSED_FUNCTION(x) x
#endif // SUPPRESS_UNUSED_ATTRIBUE

#ifdef __linux__
#define LINUX 1
#define UNIX 1

#elif __APPLE__
#include "TargetConditionals.h"

#define APPLE 1
#define UNIX 1

#if TARGET_IPHONE_SIMULATOR
#define IOS_SIMULATOR 1

#elif TARGET_OS_IPHONE
#define IOS_DEVICE 1

#elif TARGET_OS_MAC
#define OSX_DEVICE 1

#else
#error "Unknown Apple platform"
#endif // DETECT_OSX_DEVICE

#elif _WIN32
#ifdef _WIN64
#define WINDOWS 64

#else
#define WINDOWS 32
#endif // DETECT_WINDOW_VERSION

#elif !defined(BSD)
#define UNIX 1
#define BSD 1

#if __FreeBSD__
#define FREEBSD __FreeBSD__

#elif __NetBSD__
#define NETBSD __NetBSD__

#elif defined(__OpenBSD__)
#define OPENBSD __OpenBSD__

#elif defined(__DragonFly__)
#define DRAGONFLY __DragonFly__

#endif // DETECT_BSD_DISTRIBUTE
#endif // DETECT_OS

#ifndef __FUNCTION__
#define __FUNCTION__ __func__
#endif // __FUNCTION__

/*
 * __unqual_scalar_typeof(x) - Declare an unqualified scalar type, leaving
 *			       non-scalar types unchanged.
 */
/*
 * Prefer C11 _Generic for better compile-times and simpler code. Note: 'char'
 * is not type-compatible with 'signed char', and we define a separate case.
 */
#define __scalar_type_to_expr_cases(type)                                      \
  unsigned type : (unsigned type)0, signed type : (signed type)0

#if __cplusplus
#define __unqual_scalar_typeof(x) __typeof__(x)
#else
#define __unqual_scalar_typeof(x)                                              \
  typeof(_Generic((x), char                                                    \
                  : (char)0, __scalar_type_to_expr_cases(char),                \
                    __scalar_type_to_expr_cases(short),                        \
                    __scalar_type_to_expr_cases(int),                          \
                    __scalar_type_to_expr_cases(long),                         \
                    __scalar_type_to_expr_cases(long long), default            \
                  : (x)))
#endif

#define READ_ONCE(x) (*(const volatile __unqual_scalar_typeof(x) *)&(x))

#if __cplusplus
#define WRITE_ONCE(x, val)                                                     \
  do {                                                                         \
    *((volatile __typeof__(x) *)&(x)) = (val);                                 \
  } while (0)
#else
#define WRITE_ONCE(x, val)                                                     \
  do {                                                                         \
    *(volatile typeof(x) *)&(x) = (val);                                       \
  } while (0)
#endif
#endif // BASE_MACRO_H_
