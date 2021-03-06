#ifndef BASE_TYPE_H_
#define BASE_TYPE_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/ABI.h>
#include <Base/Type/Common.h>
#else
#include <Type/ABI.h>
#include <Type/Common.h>
#endif

#if __cplusplus
#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/String.h>
#else
#include <Type/String.h>
#endif

#ifndef BASE_TYPE_STRING_H_
#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/Refcount.h>
#else
#include <Type/Refcount.h>
#endif

using String = std::string;
#else
using String = Base::String;
#endif // BASE_TYPE_STRING_H_
#else
#include <string.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/Refcount.h>
#else
#include <Type/Refcount.h>
#endif

#define String char*
#endif
#endif  // BASE_TYPE_H_
