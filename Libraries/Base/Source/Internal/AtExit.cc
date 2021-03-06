#include <Type.h>

namespace Base {
namespace Internal {
static Vector<Function<void()>>* _ExitCallbacks{None};

namespace Hook {
void ExitCallbackWrapper(void (*hook)()) {
  atexit(hook);
}
} // namespace Hook

void ExitCallbackWrapper() {
  for (UInt i = _ExitCallbacks->size(); i > 0; --i) {
    (*_ExitCallbacks)[i - 1]();
  }

  delete _ExitCallbacks;
}

void AtExit(Function<void()> callback) {
  if (_ExitCallbacks == None) {
    _ExitCallbacks = new Vector<Function<void()>>();
  
    Hook::ExitCallbackWrapper(ExitCallbackWrapper);
  }

  _ExitCallbacks->push_back(callback);
}
}  // namespace Internal
}  // namespace Base
