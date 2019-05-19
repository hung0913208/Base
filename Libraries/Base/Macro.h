#ifndef BASE_MACRO_H_
#define BASE_MACRO_H_

#ifndef SUPPRESS_UNUSED_ATTRIBUE
#if defined(__GNUC__) && defined(__clang__)
#define UNUSED(x) x __attribute__((__unused__))
#else
#define UNUSED(x) x
#endif  // __GNUC__

#if defined(__GNUC__) && defined(__clang__)
#define UNUSED_FUNCTION(x) __attribute__((__unused__)) x
#else
#define UNUSED_FUNCTION(x) x
#endif  // __GNUC__
#else
#define UNUSED(x) x
#define UNUSED_FUNCTION(x) x
#endif  // SUPPRESS_UNUSED_ATTRIBUE

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
#endif  // DETECT_OSX_DEVICE

#elif _WIN32
#ifdef _WIN64
#define WINDOWS 64

#else
#define WINDOWS 32
#endif  // DETECT_WINDOW_VERSION

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

#endif  // DETECT_BSD_DISTRIBUTE
#endif  // DETECT_OS

#ifndef __FUNCTION__
#define __FUNCTION__ __func__
#endif // __FUNCTION__

#endif  // BASE_MACRO_H_
