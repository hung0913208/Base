#ifndef BASE_GLOB_H_
#define BASE_GLOB_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type.h>
#else
#include <Type.h>
#endif

namespace Base {
class Glob {
 public:
  explicit Glob(String pattern);

  /* @NOTE: export paths on serveral ways */
  Vector<String> AsList();

 private:
  String _Pattern;
};
} // namespace Base
#endif  // BASE_GLOB_H_
