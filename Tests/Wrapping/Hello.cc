#include <Wrapping.h>
#include <Logcat.h>

void PrintText(CString text) {
  INFO << "the wrapping library recieve \'" << text << "\'"
        << Base::EOL;
}

PY_MODULE(Hello) {
  Base::Log::Level() = EDebug;

  Procedure("Print", []() {
      INFO << "Hello from native C++ through out the wrapping library "
           << Base::EOL;
  });

  Procedure<CString>("PrintText", PrintText);
}
