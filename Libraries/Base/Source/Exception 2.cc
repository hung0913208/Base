#include <Config.h>
#include <Exception.h>
#include <Logcat.h>
#include <Macro.h>
#include <Utils.h>
#include <Vertex.h>
#include <signal.h>
#include <unistd.h>

namespace Base {
Exception::Exception(Error& error) : Stream{}, _Printed{True} {
  error.Print(True);

  _Message = error.message();
  _Code = error.code();
}

Exception::Exception(ErrorCodeE code)
  : Stream{}, _Printed{False}, _Message{""}, _Code{code} {}

Exception::Exception(ErrorCodeE code, const CString message, Bool autodel)
    : Stream{}, _Printed{False}, _Message{message}, _Code{code} {
  if (autodel) free(const_cast<CString>(message));
}

Exception::Exception(const CString message, Bool autodel, String function,
                     String file, Int line)
    : Stream{}, _Printed{False}, _Message{message}, _Code{EWatchErrno} {
  if (function.size() > 0 && file.size() > 0 && line >= 0) {
    _Message.append(" function ")
        .append(function)
        .append(" at ")
        .append(file)
        .append(":")
        .append(ToString(line))
        .append("\n");
  }
  if (autodel) free(const_cast<CString>(message));

#if defined(COVERAGE)
  Print();
#endif
}

Exception::Exception(String message, String function, String file, Int line)
    : Stream{}, _Printed{False}, _Message{message}, _Code{EWatchErrno} {
  if (function.size() > 0 && file.size() > 0 && line >= 0) {
    _Message.append(" function ")
        .append(function)
        .append(" at ")
        .append(file)
        .append(":")
        .append(ToString(line))
        .append("\n");
  }
#if defined(COVERAGE)
  Print();
#endif
}

Exception::Exception(ErrorCodeE code, String message, String function,
                     String file, Int line)
    : Stream{}, _Printed{False}, _Message{message}, _Code{code} {
  if (function.size() > 0 && file.size() > 0 && line >= 0) {
    _Message.append(" function ")
        .append(function)
        .append(" at ")
        .append(file)
        .append(":")
        .append(ToString(line))
        .append("\n");
  }
#if defined(COVERAGE)
  Print();
#endif
}

Exception::Exception(const Exception& src): Stream{src} {
  _Code = src._Code;
  _Printed = True;
  _Message = src._Message;
}

Exception::Exception(Exception& src): Stream{src} {
  _Code = src._Code;
  _Printed = src._Printed;
  _Message = src._Message;

  src.Ignore();
}

Exception::~Exception() { Print(); }

ErrorCodeE Exception::code() { return _Code; }

String&& Exception::message() { return RValue(_Message); }

void Exception::Print() {
  /* @NOTE: generate a locker who will protect this function */

  if (_Code != ENoError && !_Printed) {
    Vertex<void> excaping{[](){ LOCK(&Config::Locks::Error); },
                          [](){ UNLOCK(&Config::Locks::Error); }};

    /* @NOTE: print basic information about this error */
#if READABLE
    FATAL << "[ " <<  "Exception - " << ToString(_Code) << " ]: ";
#else
    if (_Message.size() > 0) {
      FATAL << "[ " << "Exception - " << ToString(_Code) << " ]: ";
    } else {
      FATAL <<  "[ Exception ]: " << ToString(_Code);
    }
#endif

    /* @NOTE: print human description of this error */
    if (_Message.size() > 0) VLOG(EError) << _Message;

    /* @NOTE: put enter to flush message thought to C++'s iostream */
    VLOG(EError) << "\n";

    /* @NOTE: mark this exception has been printed */
    _Printed = True;
  }
}

void Exception::Ignore() { _Printed = True; }
}  // namespace Base
