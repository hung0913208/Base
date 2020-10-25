#ifndef BASE_TYPE_H_
#define BASE_TYPE_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/ABI.h>
#include <Base/Type/C++.h>
#include <Base/Type/C.h>
#include <Base/Type/Common.h>
#else
#include <Type/ABI.h>
#include <Type/C++.h>
#include <Type/C.h>
#include <Type/Common.h>
#endif

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/Refcount.h>
#else
#include <Type/Refcount.h>
#endif

#if !__cplusplus
#define String char *
#endif
#endif // BASE_TYPE_H_
