#include <Build.h>

namespace Build{
Bool Plugin::Exist(String function) {
  return _Functions.find(function) != _Functions.end();
}

Bool Plugin::Install(String name, Function& function) {
  if (Exist(name)) {
    return False;
  } else {
    _Functions[name] = &function;
    return True;
  }
}
} // namespace Build
