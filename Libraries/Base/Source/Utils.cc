#include <Auto.h>
#include <Logcat.h>
#include <Macro.h>
#include <Type.h>
#include <Utils.h>
#include <ctime>
#include <iomanip>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if UNIX
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

namespace Base {
namespace Internal {
Mutex *CreateMutex();
Void RemoveMutex(Mutex *locker);
} // namespace Internal
} // namespace Base

extern "C" Int BSReadFromFileDescription(Int fd, Bytes buffer, UInt size) {
  UInt total_size = 0;

  while (True) {
    Long read_size = read(fd, &buffer[total_size], size - total_size);

    if (read_size > 0) {
      total_size += read_size;
      if (total_size == size)
        return total_size;
    } else {
      switch (errno) {
      case EAGAIN:
        if (total_size >= size) {
          return -(Int)(
              (BadLogic << "there some bugs inside your system" << Base::EOL)
                  .code());
        }
        break;

      case EBADF:
        return -(Int)((BadLogic << "fd is not a valid file descriptor or is not"
                                << "open for writing" << Base::EOL)
                          .code());

      case EFAULT:
        return -(Int)((BadAccess << "buffer is outside your accessible"
                                 << "address space" << Base::EOL)
                          .code());

      case EINTR:
        return -(Int)((Interrupted
                       << "The call was interrupted by a signal before"
                       << "any data was written" << Base::EOL)
                          .code());

      case EINVAL:
        return -(Int)((BadLogic
                       << "fd is attached to an object which is unsuitable"
                       << "for writing; or the file was opened with the"
                       << "O_DIRECT flag, and either the address specified"
                       << "in buf, the value specified in count, or the"
                       << "file offset is not suitably aligned" << Base::EOL)
                          .code());

      case EIO:
        return -(Int)((WatchErrno << "A low-level I/O error occurred while"
                                  << "modifying the inode" << Base::EOL)
                          .code());

      case EISDIR:
        return -(Int)(
            (BadLogic << "fd refers to a directory" << Base::EOL).code());
      }
    }
  }

  return ENoError;
}

extern "C" Int BSWriteToFileDescription(Int fd, Bytes buffer, UInt size) {
  UInt total_size = 0;

  while (True) {
    Int written_size = write(fd, &buffer[total_size], size - total_size);

    if (written_size > 0) {
      total_size += written_size;
      if (total_size == size)
        return total_size;
    } else {
      switch (errno) {
      case EAGAIN:
        if (total_size >= size) {
          return -(Int)(
              (BadLogic << "there some bugs inside your system" << Base::EOL)
                  .code());
        }
        break;

      case EBADF:
        return -(Int)((BadLogic << "fd is not a valid file descriptor or is not"
                                << "open for writing" << Base::EOL)
                          .code());

      case EDESTADDRREQ:
        return -(Int)((BadAccess
                       << "fd refers to a datagram socket for which a "
                          "peer address has not been set using connect(2)"
                       << Base::EOL)
                          .code());

      case EDQUOT:
        return -(Int)((BadAccess
                       << "The user's quota of disk blocks on the"
                       << "filesystem containing the file referred to by"
                       << "fd has been exhausted" << Base::EOL)
                          .code());

      case EFAULT:
        return -(Int)((BadAccess << "buffer is outside your accessible"
                                 << "address space" << Base::EOL)
                          .code());
      case EFBIG:
        return -(Int)((OutOfRange
                       << "An attempt was made to write a file that"
                       << "exceeds the implementation-defined maximum"
                       << "file size or the process's file size limit,"
                       << "or to write at a position past the maximum"
                       << "allowed offset." << Base::EOL)
                          .code());

      case EINTR:
        return -(Int)((Interrupted
                       << "The call was interrupted by a signal before"
                       << "any data was written" << Base::EOL)
                          .code());

      case EINVAL:
        return -(Int)((BadLogic
                       << "fd is attached to an object which is unsuitable"
                       << "for writing; or the file was opened with the"
                       << "O_DIRECT flag, and either the address specified"
                       << "in buf, the value specified in count, or the"
                       << "file offset is not suitably aligned." << Base::EOL)
                          .code());

      case EIO:
        return -(Int)((WatchErrno << "A low-level I/O error occurred while"
                                  << "modifying the inode" << Base::EOL)
                          .code());

      case ENOSPC:
        return -(Int)((DrainMem
                       << "The device containing the file referred to by"
                       << "fd has no room for the data" << Base::EOL)
                          .code());

      case EPERM:
        return -(Int)((BadAccess << "The operation was prevented by a file seal"
                                 << Base::EOL)
                          .code());

      case EPIPE:
        return -(Int)((BadAccess
                       << "fd is connected to a pipe or socket whose"
                       << "reading end is closed.  When this happens the"
                       << "writing process will also receive a SIGPIPE"
                       << Base::EOL)
                          .code());

      default:
        return -(Int)(WatchErrno.code());
      }
    }
  }

  return ENoError;
}

extern "C" ErrorCodeE BSCloseFileDescription(Int fd) {
  if (close(fd)) {
    switch (errno) {
    case EBADF:
      return (BadLogic << "fd is not an open file descriptor" << Base::EOL)
          .code();

    case EINTR:
      return (WatchErrno << "The close() call was interrupted by a signal; "
                            "see signal(7)."
                         << Base::EOL)
          .code();

    case EIO:
      return (WatchErrno << "A low-level I/O error occurred while"
                         << "modifying the inode" << Base::EOL)
          .code();
    case ENOSPC:
    case EDQUOT:
      return (BadAccess << "On NFS, these errors are not normally reported "
                           "against the first write which exceeds the "
                           "available storage space, but instead against a "
                           "subsequent write(2), fsync(2), or close(2)."
                        << Base::EOL)
          .code();

    default:
      return WatchErrno.code();
    }
  } else {
    return ENoError;
  }
}

#elif WINDOW
#include <windows.h>
#endif

extern "C" void BSError(UInt code, String message) {
  Base::Error{ErrorCodeE(code), message};
}

extern "C" void BSLog(String message) { Base::Error{message}; }

extern "C" CString BSCut(CString sample, Char sep, Int position) {
  return strdup(Base::Cut(String{sample}, sep, position).data());
}

extern "C" Bool BSIsMutexLocked(Mutex *locker) {
  return Base::Locker::IsLocked(*locker);
}

extern "C" Bool BSLockMutex(Mutex *locker) {
  return Base::Locker::Lock(*locker);
}

extern "C" Bool BSUnlockMutex(Mutex *locker) {
  return Base::Locker::Unlock(*locker);
}

extern "C" Mutex *BSCreateMutex() { return Base::Internal::CreateMutex(); }

extern "C" Void BSDestroyMutex(Mutex *locker) {
  Base::Internal::RemoveMutex(locker);
}

namespace Base {
template <> String ToString<Auto>(Auto value) {
  if (value.Type() == typeid(String)) {
    return value.Get<String>();
  }

  return "";
}

template <> String ToString<ErrorCodeE>(ErrorCodeE value) {
  switch (value) {
  case ENoError:
    return "NoError";

  case EKeepContinue:
    return "KeepContinue";

  case ENoSupport:
    return "NoSupport";

  case EBadLogic:
    return "BadLogic";

  case EBadAccess:
    return "BadAccess";

  case EOutOfRange:
    return "OutOfRange";

  case ENotFound:
    return "NotFound";

  case EDrainMem:
    return "DrainMem";

  case EWatchErrno:
    return "WatchErrno";

  case EInterrupted:
    return "Interrupted";

  case EDoNothing:
    return "DoNothing";

  case EDoAgain:
    return "DoAgain";

  default:
    return "";
  }
}

template <> String ToString<ErrorLevelE>(ErrorLevelE value) {
  switch (value) {
  case EError:
    return "[    ERROR ]";

  case EWarning:
    return "[  WARNING ]";

  case EInfo:
    return "[    INFOR ]";

  case EDebug:
    return "[    DEBUG ]";

  default:
    return "";
  }
}

template <> String ToString<String>(String value) { return value; }

template <> String ToString<char *>(char *value) {
  UInt len = 0;

  /* @NOTE: strlen causes issues on Release mode */
  for (; value[len] != '\0'; ++len) {
  }

  if (len > 0) {
    String result{};

    result.resize(len);
    memcpy((void *)result.c_str(), value, len);
    return result;
  } else {
    return String("");
  }
}

template <> String ToString<const char *>(const char *value) {
  return ToString<char *>(const_cast<char *>(value));
}

template <> Int ToInt<Auto>(Auto value) {
  if (value.Type() == typeid(UInt)) {
    return value.Get<UInt>();
  } else if (value.Type() == typeid(Int)) {
    return value.Get<Int>();
  } else {
    throw Except(EBadLogic,
                 Format{"can\'t convert {}"}.Apply(value.Nametype()));
  }
}

template <> Int ToInt<UInt>(UInt value) { return value; }

template <> Int ToInt<Int>(Int value) { return value; }

template <> Int ToInt<String>(String value) { return std::stoi(value); }

template <> Int ToInt<CString>(CString value) { return atoi(value); }

template <> Float ToFloat<UInt>(UInt value) { return value; }

template <> Float ToFloat<Int>(Int value) { return value; }

template <> Float ToFloat<String>(String value) { return std::stof(value); }

template <> Float ToFloat<CString>(CString value) { return atof(value); }
namespace Locker {} // namespace Locker

void Wait(Mutex &locker) {
  if (Locker::IsLocked(locker))
    Locker::Lock(locker);
}

String Datetime() {
  time_t t = std::time(nullptr);
  char buffer[80];

  /* @NOTE: convert std::time_t to String */

  strftime(buffer, 80, "%d-%m-%Y %H-%M-%S", localtime(&t));
  return String{buffer};
}

String Cut(String sample, Char sep, Int position) {
  Vector<String> result = Split(sample, sep);
  Int i = 0;

  if (position < 0) {
    for (i = position; i >= 0; i += result.size()) {
    }
  } else {
    for (i = 0; i < position; i += result.size()) {
    }
  }

  return result[i];
}

Vector<String> Split(String sample, Char sep) {
  Vector<String> result;

  for (UInt e = 0, b = 0; e < sample.size(); ++e) {
    if (sample[e] == sep) {
      result.push_back(String(sample.data() + b, e - b));
      b = e;
    }
  }

  return result;
}

UInt Pagesize() {
#if WINDOW
  SYSTEM_INFO SystemInfo;
  GetSystemInfo(&SystemInfo);

  return static_cast<UInt>(SystemInfo.dwAllocationGranularity);
#else
  return static_cast<UInt>(sysconf(_SC_PAGE_SIZE));
#endif
}

Int PID() {
#if UNIX
  return getpid();
#else
  return -1;
#endif
}

Tie::Tie() {}

Tie::~Tie() {}

Bool Tie::Mem2Cache(Void *context, const std::type_info &type, UInt size) {
  try {
    Any temp;

    _Cache.push_back(
        Auto::FromAny(*BSAssign2Any(&temp, context, (Void *)&type, None)));
    _Sizes.push_back(size);
    return True;
  } catch (...) {
    return False;
  }
}

template <> Tie &Tie::put<Vector<Auto>>(Vector<Auto> &input) {
  for (UInt i = 0; i < input.size(); ++i) {
    if (i >= _Cache.size()) {
      break;
    } else if (input[i].Type() == _Cache[i].Type()) {
      ABI::Memcpy((Void *)_Cache[i].Raw(), (Void *)input[i].Raw(), _Sizes[i]);
    }
  }
  return *this;
}

template <> Tie &Tie::get<Vector<Auto>>(Vector<Auto> &output) {
  for (UInt i = 0; i < output.size(); ++i) {
    if (i >= _Cache.size()) {
      break;
    } else if (output[i].Type() == _Cache[i].Type()) {
      ABI::Memcpy((Void *)output[i].Raw(), (Void *)_Cache[i].Raw(), _Sizes[i]);
    }
  }
  return *this;
}

Sequence::Sequence()
    : Stream{[&](Bytes &&buffer, UInt *buffer_size) -> ErrorCodeE {
               _Cache += String((CString)buffer, *buffer_size);
               return ENoError;
             },
             [&](Bytes &, UInt *) -> ErrorCodeE {
               return NoSupport("Sequence can't produce string").code();
             }} {}

Sequence::~Sequence() {}

String Sequence::operator()() {
  auto result = _Cache;

  _Cache = "";
  return result;
}
} // namespace Base
