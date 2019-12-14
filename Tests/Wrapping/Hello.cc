#include <Wrapping.h>
#include <Logcat.h>

PY_MODULE(Hello) {
  Base::Log::Level() = EDebug;

  Procedure("Print", []() {
      INFO << "Hello from native C++ through out the wrapping library ";
  });
}
