#ifndef BASE_TYPE_CXX_ABI_H_
#define BASE_TYPE_CXX_ABI_H_
#include <Type/Common.h>

#if __cplusplus
#include <atomic>
#include <cstdlib>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

template <typename Type> using Atomic = std::atomic<Type>;

template <typename Key, typename Value> using OMap = std::map<Key, Value>;

template <typename Key, typename Value>
using UMap = std::unordered_map<Key, Value>;

template <typename Key, typename Value> using Map = OMap<Key, Value>;

template <typename Key> using Set = std::set<Key>;

template <typename... Args> using Tuple = std::tuple<Args...>;

template <typename Type> using Vector = std::vector<Type>;

template <typename Type> using List = std::list<Type>;

template <typename Type> using Stack = std::stack<Type>;

template <typename Type> using Iterator = typename std::vector<Type>::iterator;

template <typename Type> using Reference = std::reference_wrapper<Type>;

template <typename F> using Function = std::function<F>;

template <typename Type> using Unique = std::unique_ptr<Type>;

template <typename Type> using Shared = std::shared_ptr<Type>;

template <typename Type> using Weak = std::weak_ptr<Type>;

namespace ABI {
/* @NOTE: memory allocation */
void *Memallign(ULong alignment, Float size);
void *Realloc(Void *buffer, ULong resize);
void *Calloc(ULong count, ULong size);
void *Malloc(ULong size);

/* @NOTE: memory deallocation */
void Free(void *buffer);
void Free(void **buffer);

/* @NOTE: memory writer  */
void Memset(void *mem, Char sample, UInt size);
void Memcpy(void *dst, void *src, UInt size);

/* @NOTE: calculate string's length */
UInt Strlen(const CString str);

/* @NOTE: kill myself if i got crash */
void KillMe();

/* @NOTE: exit program with provided exit code */
Int Exit(Int code, Bool force = True);
} // namespace ABI
#endif
#endif // BASE_TYPE_CXX_ABI_H_
