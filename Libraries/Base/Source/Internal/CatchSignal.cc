#include "Exception.h"
#include "Logcat.h"
#include "Lock.h"
#include "Macro.h"
#include "Type.h"
#include "Utils.h"
#include "Vertex.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>

namespace Base {
namespace Internal {
Mutex* CreateMutex();

static Map<UInt, Vector<Function<void(siginfo_t *)>>> *_SignalCallbacks{None};
static Vertex<Mutex, True> Secure([](Mutex* mutex) { LOCK(mutex); },
                                  [](Mutex* mutex) { UNLOCK(mutex); },
                                  CreateMutex());
namespace Hook {
void SigCallbackWrapper(int signal, void (*hook)(int, siginfo_t*, void*)) {
  struct sigaction act;

  memset(&act, '\0', sizeof(act));

  act.sa_sigaction = hook;
  act.sa_flags = SA_SIGINFO;

  if (sigaction(signal, &act, None) < 0) {
    throw Exception(
          EWatchErrno,
          BSFormat("Can\'t register signal %d with sigaction", signal), True);
  } else if (_SignalCallbacks->find(signal) == _SignalCallbacks->end()) {
    (*_SignalCallbacks)[signal] = Vector<Function<void(siginfo_t *)>>();
  }
}
} // namespace Hook

void SigCallbackWrapper(int signum, siginfo_t *siginfo, void *UNUSED(context)) {
  Map<UInt, Vector<Function<void(siginfo_t*)>>> callbacks;
 
  Secure.Circle([&]() { 
    callbacks = *_SignalCallbacks;;
  });

  if (callbacks.find(signum) != callbacks.end()) {
    for (UInt i = 0; i < (*_SignalCallbacks)[signum].size(); ++i) {
      (*_SignalCallbacks)[signum][i](siginfo);
    }

    if (signum != SIGALRM) {
      /* @NOTE: this is the trick: it will trigger the core dump */

      if (signum == SIGKILL || signum == SIGSEGV) {
        ABI::KillMe();
      } else {
        exit(signum);
      }
    }
  } else {
    exit(signum);
  }
}

void AtExit(Function<void()> callback);

void CatchSignal(UInt signal, Function<void(siginfo_t *)> callback) {
  if (_SignalCallbacks == None) {
    _SignalCallbacks = new Map<UInt, Vector<Function<void(siginfo_t *)>>>();

    AtExit([]() {
      auto callbacks = _SignalCallbacks;

      Secure.Circle([]() { _SignalCallbacks = None; });
      delete callbacks;
    });

  }

  if (_SignalCallbacks->find(signal) == _SignalCallbacks->end()) {
    Hook::SigCallbackWrapper(signal, SigCallbackWrapper);
  }

  for (UInt i = 0; i < (*_SignalCallbacks)[signal].size(); ++i) {
    if (GetAddress((*_SignalCallbacks)[signal][i]) == GetAddress(callback)) {
      return;
    }
  }

  (*_SignalCallbacks)[signal].push_back(callback);
}

void DemolishSignal(UInt signal, Function<void(siginfo_t *)> callback) {
  Int index = -1;

  for (UInt i = 0; i < (*_SignalCallbacks)[signal].size(); ++i) {
    if (GetAddress((*_SignalCallbacks)[signal][i]) == GetAddress(callback)) {
      index = i;
    }
  }

  if (index >= 0) {
    (*_SignalCallbacks)[signal].erase((*_SignalCallbacks)[signal].begin() +
                                      index);
  }
}
} // namespace Internal
} // namespace Base
