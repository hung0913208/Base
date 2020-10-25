#if !defined(BASE_TYPE_CPLUSPLUS_H_) && __cplusplus
#define BASE_TYPE_CPLUSPLUS_H_
#include <Type/Common.h>

#include <atomic>
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

template <typename T> using Atomic = std::atomic<T>;
template <typename K, typename V> using OMap = std::map<K, V>;
template <typename K, typename V> using UMap = std::unordered_map<K, V>;
template <typename K, typename V> using Map = OMap<K, V>;
template <typename K> using Set = std::set<K>;
template <typename... Args> using Tuple = std::tuple<Args...>;
template <typename T> using Vector = std::vector<T>;
template <typename T> using Queue = std::queue<T>;
template <typename T> using List = std::list<T>;
template <typename T> using Stack = std::stack<T>;
template <typename T> using Iterator = typename std::vector<T>::iterator;
template <typename T> using Reference = std::reference_wrapper<T>;
template <typename F> using Function = std::function<F>;
template <typename T> using Unique = std::unique_ptr<T>;
template <typename T> using Shared = std::shared_ptr<T>;
template <typename T> using Weak = std::weak_ptr<T>;

// @NOTE: these types are usually using during developing anything new
using OStream = std::ostream;
using IStream = std::istream;
using ostream = std::ostream;
using istream = std::istream;

using String = std::string;
using str = String;

using IVec = Vector<Int>;
using FVec = Vector<Double>;
using ivec = Vector<Int>;
using fvec = Vector<Double>;

using IMat = Vector<Vector<Int>>;
using imat = Vector<Vector<Int>>;
using FMat = Vector<Vector<Double>>;
using fmat = Vector<Vector<Double>>;
#endif // BASE_TYPE_CPLUSPLUS_H_
