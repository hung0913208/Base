#include <Type.h>

namespace Base {
namespace Internal {
static Vector<Function<void()>>* _ExitCallbacks{None};

static void ExitCallbackWrapper() {
  for (UInt i = 0; i < _ExitCallbacks->size(); ++i) {
    (*_ExitCallbacks)[i]();
  }

  delete _ExitCallbacks;
}

void AtExit(Function<void()> callback) {
  if (_ExitCallbacks == None) {
    _ExitCallbacks = new Vector<Function<void()>>();
  }

  if (_ExitCallbacks->size() == 0) atexit(ExitCallbackWrapper);
  _ExitCallbacks->push_back(callback);
}
}  // namespace Internal
}  // namespace Base
