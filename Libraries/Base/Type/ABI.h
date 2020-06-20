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
#include <atomic>
#include <cstdlib>
#include <type_traits>

template <typename Type>
using Atomic = std::atomic<Type>;

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
#else
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <sstream>

template <typename Type>
using Atomic = std::atomic<Type>;

template <typename Key, typename Value>
using OMap = std::map<Key, Value>;

template <typename Key, typename Value>
using UMap = std::unordered_map<Key, Value>;

template <typename Key, typename Value>
using Map = OMap<Key, Value>;

template <typename Key>
using Set = std::set<Key>;

template <typename Type>
using Queue = std::queue<Type>;

template <typename Type>
using List = std::list<Type>;

template <typename Type>
using Stack = std::stack<Type>;

template <typename Type>
using Iterator = typename std::vector<Type>::iterator;

template <typename Type>
using Reference = std::reference_wrapper<Type>;

template <typename Type>
using Unique = std::unique_ptr<Type>;

template <typename Type>
using Shared = std::shared_ptr<Type>;

template <typename Type>
using Weak = std::weak_ptr<Type>;
#endif

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/Function.h>
#include <Base/Type/Tuple.h>
#include <Base/Type/Vector.h>
#else
#include <Function.h>
#include <Tuple.h>
#include <Vector.h>
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

/* @NOTE: calculate string's length */
UInt Strlen(const CString str);

/* @NOTE: kill myself if i got crash */
void KillMe();

/* @NOTE: exit program with provided exit code */
Int Exit(Int code, Bool force = True);
} // namespace ABI

#if APPLE
namespace {
inline void memcpy(void* dst, void* src, ULong size) {
  ABI::Memcpy(dst, src, size);
}
}
#endif
#endif
#endif  // BASE_TYPE_CXX_ABI_H_
