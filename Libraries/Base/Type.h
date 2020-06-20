#ifndef BASE_TYPE_H_
#define BASE_TYPE_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/ABI.h>
#include <Base/Type/Common.h>
#else
#include <ABI.h>
#include <Common.h>
#endif

#if __cplusplus
#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/String.h>
#else
#include <String.h>
#endif

#ifndef BASE_TYPE_STRING_H_
using String = std::string;
#else
using String = Base::String;
#endif // BASE_TYPE_STRING_H_
#else
#include <string.h>

#define String char*
#endif

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/Refcount.h>
#else
#include <Refcount.h>
#endif

#endif  // BASE_TYPE_H_
