#include <Logcat.h>
#include <Utils.h>
#include <Vertex.h>
#include <iostream>

namespace Base {
namespace Internal {
ULong GetUniqueId();
Mutex* CreateMutex();

static UMap<ULong, Error*> Logcats{};
static Vertex<Mutex, True> Secure([](Mutex* mutex) { Locker::Lock(*mutex, True); },
                                  [](Mutex* mutex) { Locker::Unlock(*mutex); },
                                  CreateMutex());

void DumpPendingError();

namespace Logcat {
void Assign(Error* logcat) {
  auto UNUSED(guranteer) = Secure.generate();
  auto thread_id = GetUniqueId();

  if (Logcats.find(thread_id) != Logcats.end()) {
    /* @NOTE: it seems current thread has been ceased by another Logcat, force
     * print it now and overwrite with our new logcat */
    if (Logcats[thread_id]) {
      Logcats[thread_id]->Print(True);
    }
  } else {
    /* @NOTE: sometime we can use a tricky way like Assign same logcat on
     * different Thread and cause our system confuse and crash unexpectedly. So
     * we must check again to make sure everything is Okey.
     */
    auto found_it = thread_id;

    for (auto item : Logcats) {
      if (std::get<1>(item) == logcat) {
        found_it = std::get<0>(item);
        break;
      }
    }

    if (found_it != thread_id) {
      Logcats[found_it]->Print();
      Logcats[thread_id]->Print();
      Logcats[found_it] = None;
    }
  }

  Logcats[thread_id] = logcat;
}

void Decline(Error* logcat) {
  auto UNUSED(guranteer) = Secure.generate();
  auto thread_id = ULong{0};

  /* @NOTE: sometime we can use a tricky way like Assign same logcat on
   * different Thread and cause our system confuse and crash unexpectedly. So
   * we must check again to make sure everything is Okey.
   */

  for (auto item : Logcats) {
    thread_id = std::get<0>(item);

    if (std::get<1>(item) == logcat) {
      std::get<1>(item)->Print(True);
      break;
    }
  }

  if (thread_id > 0) {
    Logcats[thread_id] = None;
  }
}
}  // namespace Logcat
}  // namespace Internal

Error::Error(ErrorCodeE code, String message, String function, String file,
             int line, ErrorLevelE level)
    : Stream(),
      code{[&]() -> ErrorCodeE& { return _Code; },
           [](ErrorCodeE) { throw Exception(ENoSupport); }},
      message{[&]() -> String& { return _Message; },
              [](String) { throw Exception(ENoSupport); }} {
  _Datetime = Base::Datetime();
  _Function = function;
  _Message = message;
  _Level = level;
  _File = file;
  _Line = line;
  _Code = code;
  _IsRef = False;
  _IsPrinted = False;

  /* @NOTE: assign this logcat now since we finish its definition */
  Internal::Logcat::Assign(this);
}

Error::Error(String message)
    : Stream(),
      code{[&]() -> ErrorCodeE& { return _Code; },
           [](ErrorCodeE) { throw Exception(ENoSupport); }},
      message{[&]() -> String& { return _Message; },
              [](String) { throw Exception(ENoSupport); }} {
  _Datetime = Base::Datetime();
  _Message = message;
  _Function = "";
  _Code = ENoError;
  _IsRef = False;
  _IsPrinted = False;
  _File = "";
  _Line = -1;

  /* @NOTE: assign this logcat now since we finish its definition */
  Internal::Logcat::Assign(this);
}

Error::Error(Error&& error)
    : Stream(),
      code{[&]() -> ErrorCodeE& { return _Code; },
           [](ErrorCodeE) { throw Exception(ENoSupport); }},
      message{[&]() -> String& { return _Message; },
              [](String) { throw Exception(ENoSupport); }} {
  /* @NOTE: don't need tp assign this logcat since it's a reference  */

  _Function = error._Function;
  _Message = error._Message;
  _Code = error._Code;
  _File = error._File;
  _IsRef = True;
  _IsPrinted = True;
}

Error::Error(Error& error)
    : Stream(),
      code{[&]() -> ErrorCodeE& { return _Code; },
           [](ErrorCodeE) { throw Exception(ENoSupport); }},
      message{[&]() -> String& { return _Message; },
              [](String) { throw Exception(ENoSupport); }} {
  /* @NOTE: don't need tp assign this logcat since it's a reference  */

  _Function = error._Function;
  _Message = error._Message;
  _Code = error._Code;
  _File = error._File;
  _IsRef = True;
  _IsPrinted = True;
}

Error::~Error() { Internal::Logcat::Decline(this); }

Error& Error::operator<<(String message) {
  Bool print_now = False;

  /* @NOTE: check if message have the end line character */
  for (UInt enter_idx = 0; enter_idx < message.size(); ++enter_idx) {
    if ((print_now = (message[enter_idx] == '\n'))) {
      _Message.append(message, 0, enter_idx);
    }
  }

  /* @NOTE: if print_now flag is True, we must print everything now, if it's not
   * we just combine new message with the old message
   */
  if (print_now) {
    _IsPrinted = True;
  } else {
    _Message.append(message);
  }
  return *this;
}

Error& Error::operator()(String message) {
  _Message = message;

  Print(True);
  return *this;
}

Error& Error::operator=(Error& error) {
  /* @NOTE: remove this logcat now since we will link it to be a reference,
   * force to print message to console no matter it's finish or not
   */
  Internal::Logcat::Decline(this);

  _Function = error._Function;
  _Message = error._Message;
  _Code = error._Code;
  _File = error._File;

  /* @NOTE: set flags to */
  _IsPrinted = True;
  _IsRef = True;
  return *this;
}

Error& Error::operator=(Error&& error) {
  /* @NOTE: remove this logcat now since we will link it to be a reference,
   * force to print message to console no matter it's finish or not
   */
  Internal::Logcat::Decline(this);

  _Function = error._Function;
  _Message = error._Message;
  _Code = error._Code;
  _File = error._File;

  /* @NOTE: set flags to */
  _IsPrinted = True;
  _IsRef = True;
  return *this;
}

Error::operator bool() { return _Level == EError && _Code != ENoError; }

void Error::Print(bool force) {
  /* @NOTE: print basic information about this error */


  /* @NOTE: call lambda with or without safeguard */
  if (!force) {
    auto UNUSED(guranteer) = Internal::Secure.generate();

    _Print();
  } else {
    _Print();
  }

  /* @NOTE: since we are going to force to print */
  if (!force) Internal::Logcat::Decline(this);
}


Error& Error::Info() {
  _Level = EInfo;
  return *this;
}

Error& Error::Warn() {
  _Level = EWarning;
  return *this;
}

Error& Error::Fatal() {
  _Level = EError;
  return *this;
}

void Error::_Print() {
  if (!_IsPrinted) {
    if (_Code != ENoError) {
      auto color =
        _Level == EError ? RED : (_Level == EWarning ? YELLOW : GREEN);

      if (_IsPrinted == False) {
#if READABLE
        VLOG(_Level) << (color << ToString(_Level) << "[" << ToString(_Code)
                               << "]: ");
#else
        if (_Message.size() > 0) {
          VLOG(_Level) << (color << ToString(_Level) << "[ " << ToString(_Code)
                                 << " ]");
        } else {
          VLOG(_Level) << (color << ToString(_Level) << " " << ToString(_Code));
        }
#endif

        /* @NOTE: print human description of this error */
        if (_Message.size() > 0) {
          VLOG(_Level) << ": " << _Message;
        }

#if READABLE
        /* @NOTE: print information regarded about where the error happens
         */
        if (_Line >= 0 && _Function.size() > 0 && _File.size() > 0) {
          VLOG(_Level) << " at `" << _Function << "` " << _File << ":"
                       << ToString(_Line);
        }
#endif

        /* @NOTE: put enter to flush message thought to C++'s iostream */
        VLOG(_Level) << "\n";
      }
    }

    /* @NOTE: change flags _IsPrinted to prevent printing again */
    _IsPrinted = True;
  }
}
}  // namespace Base

extern "C" {
Int WriteLog0(enum ErrorCodeE code, enum ErrorLevelE level,
              const CString function, const CString file, Int line,
              const CString message) {
  return Base::Error{code, message, function, file, line, level}.code();
}
}
