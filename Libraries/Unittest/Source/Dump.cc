#include <Unittest.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <signal.h>

typedef void (*Throwing)(void*, void*, void (*)(void *));
typedef void (*Crashing)(int, siginfo_t*, void*);
typedef void (*Exiting)();
typedef void (*Finishing)();

void __cxa_throw (void* object, void *tinfo, void (*dest)(void *));
void __cxa_crash(int signal, siginfo_t* sinfo, void* context);
void __cxa_finish();
void __cxa_exit();

namespace Base {
namespace Internal {
Mutex* CreateMutex();
void AtExit(Function<void()> callback);
void CatchSignal(UInt signal, Function<void(siginfo_t *)> callback);

void SigCallbackWrapper(int signum, siginfo_t *siginfo, void* context); 
void ExitCallbackWrapper();

namespace Hook {
void SigCallbackWrapper(int signal, void (*hook)(int, siginfo_t*, void*));
void ExitCallbackWrapper(void (*hook)());
} // namespace Hook

namespace Dump {
thread_local Base::Unit::Dump* Current{None};
static Vertex<Mutex, True> Secure([](Mutex* mutex) { pthread_mutex_lock(mutex); },
                                  [](Mutex* mutex) { pthread_mutex_unlock(mutex); },
                                  CreateMutex());
static Map<UInt, Vector<Shared<Base::Unit::Dump>>> Dumpers{};
Finishing Finisher{None};
Crashing Crasher{None};
Throwing Thrower{None};
Exiting Exiter{None};
} // namespace Dump
} // namespace Internal

namespace Unit {
Dump::Dump(Case* testcase, UInt type): Trap{testcase}, _Type{type} {
  using namespace Internal;
  using namespace Internal::Dump;

  if (type == EExiting && !Exiter) {
    Hook::ExitCallbackWrapper(Exiter = __cxa_exit);
  } else if (type == ECrashing && !Crasher) {
    Hook::SigCallbackWrapper(SIGSEGV, Crasher = __cxa_crash);
  } else if (type == EFinishing && !Finisher) {
    Finisher = __cxa_finish;
  }
}

Dump::~Dump() { }

void Dump::operator<<(Function<void()> callback) {
  _Handler = callback;
}

void Dump::Assign(String name, Auto address) {
  _Variables[name] = address;  
}

void Dump::Preview() {
  (Internal::Dump::Current = this)->_Handler(); 
}

void Dump::Register(Shared<Base::Unit::Dump> dumper) {
  using namespace Internal;
  using namespace Internal::Dump;

  Secure.Circle([&]() {
    if (Dumpers.find(dumper->_Type) == Dumpers.end()) {
      Dumpers[dumper->_Type] = Vector<Shared<Base::Unit::Dump>>{};
    }

    Internal::Dump::Dumpers[dumper->_Type].push_back(dumper);
  });
}

void Dump::Clear() {
  Internal::Dump::Secure.Circle([]() {
    for (auto& contain: Internal::Dump::Dumpers) {
      std::get<1>(contain).clear();
    }
  });
}
} // namespace Unit
} // namespace Base

[[ noreturn ]] void __cxa_throw (void* object, void *tinfo,
                                 void (*dest)(void *)) {
  using namespace Base::Internal::Dump;
  using namespace Base;

  if (!Internal::Dump::Thrower) {
    Internal::Dump::Thrower = (Throwing) dlsym(RTLD_NEXT, "__cxa_throw");
  }

    if (Dumpers.find(Unit::Dump::EThrowing) != Dumpers.end()) {
      for (auto& dump : Dumpers[Unit::Dump::EThrowing]) {
        Secure.Circle([&]() {
          dump->Assign("object", Auto::As(object));
          dump->Assign("tinfo", Auto::As(tinfo));
          dump->Assign("dest", Auto::As(dest));
          dump->Preview();
        });
      }
    }

  Internal::Dump::Thrower(object, tinfo, dest);
  Bug(EBadLogic, "Thrower can't touch here");
}

void __cxa_crash(int signal, siginfo_t* sinfo, void* context) {
  using namespace Base::Internal::Dump;
  using namespace Base;

  if (Dumpers.find(Unit::Dump::ECrashing) != Dumpers.end() &&
      signal != SIGALRM) {

    for (auto& dump : Dumpers[Unit::Dump::ECrashing]) {
      Secure.Circle([&]() {
        dump->Assign("signal", Auto::As(signal));
        dump->Assign("sinfo", Auto::As(sinfo));
        dump->Preview();
      });
    }
  }

  if (sinfo) {
    Internal::SigCallbackWrapper(signal, sinfo, context);
  }
}

void __cxa_exit() {
  using namespace Base::Internal::Dump;
  using namespace Base;

  if (Dumpers.find(Unit::Dump::EExiting) != Dumpers.end()) {
    for (auto& dump : Dumpers[Unit::Dump::EExiting]) {
      dump->Preview();
    }
  }

  Internal::ExitCallbackWrapper();
}

void __cxa_finish() {
  using namespace Base::Internal::Dump;
  using namespace Base;

  if (Dumpers.find(Unit::Dump::EFinishing) != Dumpers.end()) {
    for (auto& dump : Dumpers[Unit::Dump::EFinishing]) {
      dump->Preview();
    }
  }
}

