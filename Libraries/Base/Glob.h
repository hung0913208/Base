#ifndef BASE_GLOB_H_
#define BASE_GLOB_H_
#include <Type.h>

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
