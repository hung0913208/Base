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
#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/Refcount.h>
#else
#include <Refcount.h>
#endif

using String = std::string;
#else
using String = Base::String;
#endif // BASE_TYPE_STRING_H_

#ifndef BASE_TYPE_FUNCTION_H_
template <typename F> using Function = std::function<F>;
#else
template <typename F> using Function = Base::Function<F>;
#endif // BASE_TYPE_FUNCTION_H_

#ifndef BASE_TYPE_TUPLE_H_
template <typename... Args> using Tuple = std::tuple<Args...>;
#else
template <typename... Args> using Tuple = Base::Tuple<Args...>;
#endif  // BASE_TYPE_TUPLE_H_
#endif  // __cplusplus

#if !__cplusplus
#include <string.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/Refcount.h>
#else
#include <Refcount.h>
#endif

#define String char*
#endif
#endif  // BASE_TYPE_H_
