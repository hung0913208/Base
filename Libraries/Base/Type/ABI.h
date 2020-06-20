#ifndef BASE_TYPE_CXX_ABI_H_
#define BASE_TYPE_CXX_ABI_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/Common.h>
#include <Base/Macro.h>
#else
#include <Common.h>
#include <Macro.h>
#endif

#if __cplusplus
#ifndef BUILD_AS_ABI_TYPE
#define BUILD_AS_ABI_TYPE 1
#endif

#include <atomic>
#include <cstdlib>
#include <type_traits>

namespace ABI {
/* @NOTE: memory allocation */
void* Memallign(ULong alignment, Float size);
void* Realloc(Void* buffer, ULong resize);
void* Calloc(ULong count, ULong size);
void* Malloc(ULong size);

/* @NOTE: memory deallocation */
void Free(void* buffer);
void Free(void** buffer);

/* @NOTE: memory writer  */
void Memset(void* mem, Char sample, UInt size);
void Memcpy(void* dst, void* src, UInt size);
void* Memmove(void* dst, const void* src, UInt size);

/* @NOTE: calculate string's length */
UInt Strlen(const CString str);

/* @NOTE: kill myself if i got crash */
void KillMe();

/* @NOTE: exit program with provided exit code */
Int Exit(Int code, Bool force = True);
} // namespace ABI

#ifdef APPLE
template <typename Type>
using Iterator = Type*;

#ifndef USE_BASE_FUNCTION
#define USE_BASE_FUNCTION 1
#endif  // USE_BASE_FUNCTION

#ifndef USE_BASE_TUPLE
#define USE_BASE_TUPLE 1
#endif  // USE_BASE_TUPLE

#ifndef USE_BASE_VECTOR
#define USE_BASE_VECTOR 1
#endif  // USE_BASE_TUPLE

#ifndef USE_BASE_MAP
#define USE_BASE_MAP 1
#endif  // USE_BASE_MAP

#ifndef USE_BASE_SET
#define USE_BASE_SET 1
#endif  // USE_BASE_SET

#ifndef USE_BASE_TABLE
#define USE_BASE_TABLE 1
#endif  // USE_BASE_TABLE

#ifndef USE_BASE_STRING
#define USE_BASE_STRING 1
#endif  // USE_BASE_STRING

#if APPLE
typedef decltype(nullptr) nullptr_t;

namespace {
inline void memcpy(void* dst, void* src, ULong size) {
  ABI::Memcpy(dst, src, size);
}

inline void* memmove (void* dest, const void* src, ULong len) {
  return ABI::Memmove(dest, src, len);
}
}
#endif
#else
#include <functional>
#include <tuple>
#include <vector>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <sstring>

template <typename Type>
using Iterator = typename std::vector<Type>::iterator;

template <typename Type>
using Atomic = std::atomic<Type>;

template <typename Type>
using Queue = std::queue<Type>;

template <typename Type>
using List = std::list<Type>;

template <typename Type>
using Stack = std::stack<Type>;

template <typename Type>
using Unique = std::unique_ptr<Type>;

template <typename Type>
using Shared = std::shared_ptr<Type>;

template <typename Type>
using Weak = std::weak_ptr<Type>;
#endif

template <typename Type>
using Reference = std::reference_wrapper<Type>;

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/Function.h>
#include <Base/Type/Tuple.h>
#include <Base/Type/Vector.h>
#include <Base/Type/Map.h>
#include <Base/Type/Table.h>
#include <Base/Type/String.h>
#else
#include <Function.h>
#include <Tuple.h>
#include <Vector.h>
#include <Map.h>
#include <Set.h>
#endif  // USE_BASE_WITH_FULL_PATH_HEADER

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

#ifndef BASE_TYPE_VECTOR_H_
template <typename Type> using Vector = std::vector<Type>;
#else
template <typename Type> using Vector = Base::Vector<Type>;
#endif  // BASE_TYPE_TUPLE_H_

#ifndef BASE_TYPE_MAP_H_
template <typename Key, typename Value>
using OMap = std::map<Key, Value>;
#else
template <typename Key, typename Value>
using OMap = Base::Map<Key, Value>;
#endif  // BASE_TYPE_MAP_H_

#ifndef BASE_TYPE_SET_H_
template <typename Key>
using Set = std::set<Key>;
#else
template <typename Key>
using Set = Base::Set<Key>;
#endif

#if USE_BASE_TABLE
#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Hashtable.h>
#else
#include <Hashtable.h>
#endif

template <typename Key, typename Value>
using UMap = Base::Hashtable<Key, Value, Int>;
#else
template <typename Key, typename Value>
using UMap = std::unordered_map<Key, Value, Int>;
#endif  // USE_BASE_TABLE

template <typename Key, typename Value>
using Map = OMap<Key, Value>;

#undef BUILD_AS_ABI_TYPE
#endif
#endif  // BASE_TYPE_CXX_ABI_H_
