#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Macro.h>
#else
#include <Macro.h>
#endif

#if !defined(BASE_TYPE_SET_H_) && (USE_BASE_SET || APPLE)
#define BASE_TYPE_SET_H_
#define BUILD_AS_ABI_TYPE 1

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Hashtable.h>
#include <Base/Tree.h>
#else
#include <Hashtable.h>
#include <Tree.h>
#endif

namespace Base {
template <typename Key>
class Set {
};
} // namespace Base
#undef BUILD_AS_ABI_TYPE
#endif  // BASE_TYPE_SET_H_
