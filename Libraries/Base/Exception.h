#if !defined(BASE_EXCEPTION_H_) && __cplusplus
#define BASE_EXCEPTION_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Stream.h>
#include <Base/Type.h>
#else
#include <Stream.h>
#include <Type.h>
#endif

#define Except(code, message) \
  Base::Exception { code, message, __FUNCTION__, __FILE__, __LINE__ }

#define Abort(code)                 \
{                                   \
  FATAL << "Abort with code "       \
        << Base::ToString(code)     \
        << " at "                   \
        << __FUNCTION__             \
        << " "                      \
        << __FILE__                 \
        << ":"                      \
        << Base::ToString(__LINE__) \
        << Base::EOL;               \
  ABI::KillMe();                    \
  abort();                          \
}

#define Notice(code, message)        \
{                                    \
  FATAL << "SUSPECT - "              \
        << Base::ToString(code)      \
        << ": "                      \
        << Base::ToString((message)) \
        << " at "                    \
        << __FUNCTION__              \
        << " "                       \
        << __FILE__                  \
        << ":"                       \
        << Base::ToString(__LINE__)  \
        << Base::EOL;                \
}

#define Bug(code, message)           \
{                                    \
  FATAL << "BUG - "                  \
        << Base::ToString(code)      \
        << ": "                      \
        << Base::ToString((message)) \
        << " at "                    \
        << __FUNCTION__              \
        << " "                       \
        << __FILE__                  \
        << ":"                       \
        << Base::ToString(__LINE__)  \
        << Base::EOL;                \
  ABI::KillMe();                     \
  abort();                           \
}

namespace Base {
class Error;

class Exception : public Stream {
 public:
  explicit Exception(Error& error);
  explicit Exception(ErrorCodeE code = ENoError);
  explicit Exception(ErrorCodeE code, const CString message, Bool autodel);
  explicit Exception(const CString message, Bool autodel, String function,
                     String file, Int line);
  explicit Exception(String message, String function, String file, Int line);
  explicit Exception(ErrorCodeE code, String message, String function,
                     String file, Int line);
  virtual ~Exception();

  /* @NOTE: copy contructor */
  Exception(const Exception& src);
  Exception(Exception& src);

  /* @NOTE: return exception's infomation */
  ErrorCodeE code();
  String&& message();

  /* @NOTE: print the exception to console */
  void Print();

  /* @NOTE: ignore an exception */
  void Ignore();

 private:
  Bool _Printed;
  String _Message;
  ErrorCodeE _Code;
};
}  // namespace Base
#endif  // BASE_EXCEPTION_H_
