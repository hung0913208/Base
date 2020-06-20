#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Macro.h>
#include <Base/Tree.h>
#else
#include <Macro.h>
#include <Tree.h>
#endif

#if !defined(BASE_TYPE_MAP_H_) && (USE_BASE_MAP || APPLE)
#define BASE_TYPE_MAP_H_
namespace Base {
template<typename Key, typename Value>
class Map {
};
} // namespace Base
#endif  // BASE_TYPE_MAP_H_
