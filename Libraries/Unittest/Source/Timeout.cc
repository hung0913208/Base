#include <Thread.h>
#include <Type.h>
#include <Unittest.h>
#include <Vertex.h>
#include <signal.h>
#include <unistd.h>

namespace Base {
namespace Internal {
void CatchSignal(UInt signal, Function<void(siginfo_t*)> callback);
void DemolishSignal(UInt signal, Function<void(siginfo_t*)> callback);
Bool KillStoppers(UInt signal = SIGTERM);
}  // namespace Internal

namespace Unit {
Timeout::Timeout(Case* testcase, ULong seconds, UInt line, String file)
    : Trap{testcase}, _Seconds{seconds}, _Bypass{False}{
  _Handler = std::bind(&Timeout::HandleSignal, this, std::placeholders::_1,
                       file, line);
}

Timeout::Timeout(Case* testcase, Function<void()> watchdog, UInt line,
                 String file)
    : Trap{testcase}, _Seconds{0}, _Bypass{False} {
  if (std::thread::hardware_concurrency() == 1){
    throw Except(ENoSupport, "`watchdog` only supports with multi-core systems");
  }

  _Handler = [&](siginfo_t*) {
    watchdog();
    if (!_Bypass) {
      testcase->Expect("doesn\'t spend " + ToString(_Seconds) + " sec",
                      "the code ", file, line);
      testcase->Fatal(True);
    }
  };
}

void Timeout::HandleSignal(siginfo_t* UNUSED(siginfo), String file, UInt line){
  if (!_Bypass) {
    _Case->Expect("doesn\'t spend " + ToString(_Seconds) + " sec",
                  "the code ", file, line);
    _Case->Fatal(!Internal::KillStoppers());
  }
}

void Timeout::operator<<(Function<void()> callback) {
  auto watchdog = [&] {
    if (_Seconds > 0) {
      /* @NOTE: if _Seconds > 0, we must wait until the
       * signal has triggered */

      Internal::CatchSignal(SIGALRM, _Handler);
      alarm(_Seconds);
    } else {
      /* @NOTE: with this case, we must wait on parallel
       * and perform decision */

      Thread{True}.Start([&]() {
        sleep(_Seconds); 
        if (!_Bypass) _Handler(None);
      });
    }
  };
  auto bypass = [&] {
    _Bypass = True;

    if (_Seconds > 0) {
      Internal::DemolishSignal(SIGALRM, _Handler);
    }
  };

  Vertex<void> escaping{watchdog, bypass};
  callback();
}
}  // namespace Unit
}  // namespace Base
