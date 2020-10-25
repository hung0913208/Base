#if !defined(BASE_LOGCAT_H_)
#define BASE_LOGCAT_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Property.h>
#include <Base/Stream.h>
#include <Base/Type.h>
#include <Base/Utils.h>
#else
#include <Property.h>
#include <Stream.h>
#include <Type.h>
#include <Utils.h>
#endif

#define LOGLEVEL 1
#define READABLE 1
#define USE_COLOR 1

#define ERROR_LABEL "[  ERROR  ]"
#define WARNING_LABEL "[ WARNING ]"
#define INFO_LABEL "[  INFO   ]"
#define DEBUG_LABEL "[  DEBUG  ]"

#if __cplusplus
#if READABLE
#define NoError                                                                \
  Base::Error {}
#define KeepContinue                                                           \
  Base::Error { EKeepContinue, "", __FUNCTION__, __FILE__, __LINE__ }
#define NoSupport                                                              \
  Base::Error { ENoSupport, "", __FUNCTION__, __FILE__, __LINE__ }
#define BadLogic                                                               \
  Base::Error { EBadLogic, "", __FUNCTION__, __FILE__, __LINE__ }
#define BadAccess                                                              \
  Base::Error { EBadAccess, "", __FUNCTION__, __FILE__, __LINE__ }
#define OutOfRange                                                             \
  Base::Error { EOutOfRange, "", __FUNCTION__, __FILE__, __LINE__ }
#define NotFound                                                               \
  Base::Error { ENotFound, "", __FUNCTION__, __FILE__, __LINE__ }
#define DrainMem                                                               \
  Base::Error { EDrainMem, "", __FUNCTION__, __FILE__, __LINE__ }
#define WatchErrno                                                             \
  Base::Error { EWatchErrno, "", __FUNCTION__, __FILE__, __LINE__ }
#define Interrupted                                                            \
  Base::Error { EInterrupted, "", __FUNCTION__, __FILE__, __LINE__ }
#define DoNothing                                                              \
  Base::Error { EDoNothing, "", __FUNCTION__, __FILE__, __LINE__ }
#define DoAgain                                                                \
  Base::Error { EDoAgain, "", __FUNCTION__, __FILE__, __LINE__ }
#else
#define NoError                                                                \
  Base::Error {}
#define KeepContinue                                                           \
  Base::Error { EKeepContinue }
#define NoSupport                                                              \
  Base::Error { ENoSupport }
#define BadLogic                                                               \
  Base::Error { EBadLogic }
#define BadAccess                                                              \
  Base::Error { EBadAccess }
#define OutOfRange                                                             \
  Base::Error { EOutOfRange }
#define NotFound                                                               \
  Base::Error { ENotFound }
#define DrainMem                                                               \
  Base::Error { EDrainMem }
#define WatchErrno                                                             \
  Base::Error { EWatchErrno }
#define Interrupted                                                            \
  Base::Error { EInterrupted }
#define DoNothing                                                              \
  Base::Error { EDoNothing }
#define DoAgain                                                                \
  Base::Error { EDoAgain }
#endif

#define FUNCTION __FUNCTION__
#define SOURCE                                                                 \
  Base::Format{"{}:{}"}.Apply(Base::Split(__FILE__, '/', -1), __LINE__)

#define SRC SOURCE

#define VLOG(level) VLOGC(level, WHITE)
#define VLOGC(level, color) Base::Log::Redirect(level, -1, color)

#define RED Base::Color(Base::Color::Red)
#define GREEN Base::Color(Base::Color::Green)
#define YELLOW Base::Color(Base::Color::Yellow)
#define BLUE Base::Color(Base::Color::Blue)
#define MAGNETA Base::Color(Base::Color::Magneta)
#define CYAN Base::Color(Base::Color::Cyan)
#define WHITE Base::Color(Base::Color::White)

#if DEBUG
#define ERROR VLOGC(EError, RED) << "[ " << Base::ToString(Base::PID()) << " ] "
#define WARNING                                                                \
  VLOGC(EWarning, YELLOW) << "[ " << Base::ToString(Base::PID()) << ") ] "

#define VERBOSE                                                                \
  VLOGC(EDebug, MAGNETA) << "[ " << Base::ToString(Base::PID()) << " - "       \
                         << FUNCTION << " ] "
#define INFO VLOGC(EInfo, WHITE) << "[ " << Base::ToString(Base::PID()) << " ] "
#define FATAL VLOGC(EError, RED) << "[ " << Base::ToString(Base::PID()) << " ] "

/* @NOTE: this macro will help to write debug log easier while prevent
 * performance issue */
#define DEBUG(message)                                                         \
  {                                                                            \
    if (Base::Log::Level() == EDebug) {                                        \
      VLOGC(EDebug, CYAN) << "[ " << Base::ToString(Base::PID()) << " - "      \
                          << FUNCTION << "] " << (message) << Base::EOL;       \
    }                                                                          \
  }
#else
#define ERROR VLOGC(EError, RED)
#define WARNING VLOGC(EWarning, YELLOW)
#define VERBOSE VLOGC(EDebug, MAGNETA) << "[  VERBOSE ] "
#define INFO VLOGC(EInfo, WHITE)
#define FATAL VLOGC(EError, RED)

/* @NOTE: this macro will help to write debug log easier while prevent
 * performance issue */
#define DEBUG(message)                                                         \
  {                                                                            \
    if (Base::Log::Level() == EDebug) {                                        \
      VLOGC(EDebug, CYAN) << "[   DEBUG  ] " << (message) << Base::EOL;        \
    }                                                                          \
  }
#endif

namespace Base {
class Color {
public:
  enum ColorCodeE {
    Reset = 0,
    Red = 31,
    Green = 32,
    Yellow = 33,
    Blue = 34,
    Magneta = 35,
    Cyan = 36,
    White = 37
  };

  explicit Color(ColorCodeE code);
  virtual ~Color();

  /* @NOTE: these operators support how to wrapp a message with  color */
  virtual Color &operator()(String &message);
  virtual Color &operator()(String &&message);

  /* @NOTE: these operators support how to communicate with Color object */
  virtual Color &operator<<(String &message);
  virtual Color &operator<<(String &&message);
  virtual Color &operator>>(String &message);

  Property<String> Message;
  ColorCodeE &Code();

  /* @NOTE: control how to present message manually*/
  void Print();
  void Clear();

protected:
  void Print(String &&message);

private:
  ColorCodeE _Code;
  String _Message;
  Bool _Printed;
};

class Log : public Stream {
public:
  explicit Log(Int device = -1, Color color = WHITE, Log *previous = None);
  virtual ~Log();

  Color &Apperance();
  Bool &Lock();

  Log &WithDevice(Int device = -1);
  ErrorCodeE ActiveDevice(Int device = -1, Bool status = True);
  ErrorCodeE CloseDevice(Int device);

  Log &operator<<(Color &&value);
  Log &operator<<(Color &value);

  Log &operator<<(const String &value) final;
  Log &operator<<(String &&value) final;
  Log &operator<<(Int &&value) final;
  Log &operator<<(UInt &&value) final;
  Log &operator<<(Float &&value) final;
  Log &operator<<(Double &&value) final;

  Log &operator>>(String &value) final;
  Log &operator>>(Byte &value) final;
  Log &operator>>(Int &value) final;
  Log &operator>>(UInt &value) final;
  Log &operator>>(Float &value) final;
  Log &operator>>(Double &value) final;

  static Log &Redirect(UInt level, Int device = -1, Color color = WHITE);
  static Void Enable(UInt level, Int device);
  static Void Disable(UInt level, Int device);
  static UInt &Level();

protected:
  virtual ErrorCodeE WriteToDevice(Bytes &&buffer, UInt *size);
  virtual Bool AllowChangingStatus(Int device, Bool expected);
  virtual Bool AllowChangingStatus(Bool expected);

private:
  ErrorCodeE WriteToColorConsole(Bytes &&buffer, UInt *size);

  Map<Int, Log *> _Loggers;
  Log *_Previous;
  Bool _Status, _Lock;
  ;
  Color _Color;
  Int _Device;
};

class Error : public Stream {
public:
  explicit Error(ErrorCodeE code = ENoError, String message = "",
                 String function = "", String file = "", Int line = 0,
                 ErrorLevelE level = EError);
  explicit Error(String message);

  Error(Error &error);
  Error(Error &&error);
  ~Error();

  /* @NOTE: usually, we will use Print to export manually message from Error */
  void Print(Bool force = False);
  Error &Info();
  Error &Warn();
  Error &Fatal();

  /* @NOTE: operator to make reference among Error objects */
  Error &operator=(Error &error);
  Error &operator=(Error &&error);

  /* @NOTE: opeartor to update message */
  Error &operator<<(String message);
  Error &operator()(String message);

  operator bool();

  /* @NOTE: properties which are corelated to internal parameters */
  Property<ErrorCodeE> code;
  Property<String> message;

private:
  void _Print();

  Bool _IsRef, _IsPrinted;

  Int _Line;
  String _Function, _File, _Message, _Datetime;

  ErrorCodeE _Code;
  ErrorLevelE _Level;
};

Int PID();
} // namespace Base
#else
#define Error(code, message)                                                   \
  WriteLog0(code, EError, __FUNCTION__, __FILE__, __LINE__, message)
#define Warning(code, message)                                                 \
  WriteLog0(code, EWarning, __FUNCTION__, __FILE__, __LINE__, message)
#define Info(code, message)                                                    \
  WriteLog0(code, EInfo, __FUNCTION__, __FILE__, __LINE__, message)

Int WriteLog0(enum ErrorCodeE code, enum ErrorLevelE level,
              const CString function, const CString file, Int line,
              const CString message);
#endif

#if __cplusplus
template <typename... Args> inline void D(String format, Args... args) {
  VERBOSE << Base::Format{format}.Apply(args...);
}

template <typename... Args> inline void I(String format, Args... args) {
  INFO << Base::Format{format}.Apply(args...);
}

template <typename... Args> inline void E(String format, Args... args) {
  ERROR << Base::Format{format}.Apply(args...);
}

template <typename... Args>
inline void D(String tag, String format, Args... args) {
  VERBOSE << tag << ": " << Base::Format{format}.Apply(args...);
}

template <typename... Args>
inline void I(String tag, String format, Args... args) {
  INFO << tag << ": " << Base::Format{format}.Apply(args...);
}

template <typename... Args>
inline void E(String tag, String format, Args... args) {
  ERROR << tag << ": " << Base::Format{format}.Apply(args...);
}
#endif
#endif // BASE_LOGCAT_H_
