//<> Copyright (c) 2018,  All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.

// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.

// 3. Neither the name of ORGANIZATION nor the names of
//    its contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef BASE_UTILS_H_
#define BASE_UTILS_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Exception.h>
#include <Base/Macro.h>
#include <Base/Type.h>
#else
#include <Exception.h>
#include <Macro.h>
#include <Type.h>
#endif

#if WINDOWS
#include <windows.h>

inline void usleep(__int64 usec) {
  HANDLE timer;
  LARGE_INTEGER ft;

  /* @NOTE:Convert to 100 nanosecond interval, negative value indicates
   * relative time */
  ft.QuadPart = -(10*usec);

  timer = CreateWaitableTimer(NULL, TRUE, NULL);
  SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
  WaitForSingleObject(timer, INFINITE);
  CloseHandle(timer);
}
#else
#include <unistd.h>
#endif

/* @NOTE: work with string */
#if __cplusplus
CString BSFormat(String format, ...);
#else
CString BSCut(CString sample, Char sep, Int position);
CString BSFormat(CString format, ...);

Mutex* BSCreateMutex();
Void BSDestroyMutex(Mutex* locker);

Bool BSIsMutexLocked(Mutex* locker);
Bool BSLockMutex(Mutex* locker);
Bool BSUnlockMutex(Mutex* locker);
#endif

#if __cplusplus
extern "C" {
#endif

void BSLog(String message);
void BSError(UInt code, String message);

/* @NOTE: these functions are wrapping around I/O functions of specifict OS */
enum ErrorCodeE BSCloseFileDescription(Int fd);
Int BSReadFromFileDescription(Int fd, Bytes buffer, UInt size);
Int BSWriteToFileDescription(Int fd, Bytes buffer, UInt size);

#if __cplusplus
}
#endif

#if __cplusplus
#include <cxxabi.h>
#include <typeinfo>

/* @NOTE: these macros are used like a convenient tools to simplify my code */
#define RValue(variable) std::forward<decltype(variable)>(variable)
#define Cast(type, value) static_cast<decltype(type)>(value)

namespace Base {
class Auto;
class Error;
class Tie;

namespace Number {
template <typename Type>
Bool IsNumberic();

template <typename Type>
Type Min();

template <typename Type>
Type Max();
}  // namespace Number

namespace Locker {
bool IsLocked(Mutex& locker);
Bool Lock(Mutex& locker);
Bool Unlock(Mutex& locker);
}  // namespace Locker

void Wait(Mutex& locker);
String Datetime();

template<typename... Args>
Tie Bond(Args&... args);

template <typename LeftT, typename RightT>
struct Pair {
  LeftT Left;
  RightT Right;

  explicit Pair(LeftT left, RightT right): Left{left}, Right{right} {}

  Pair() : Left{}, Right{} {}
  Pair(const Pair& src) : Left{src.Left}, Right{src.Right} {}
};

template<typename Type>
String Nametype() {
  auto& type = typeid(Type);
  auto status = 0;
  auto result = abi::__cxa_demangle(type.name(), 0, 0, &status);

  if (result) {
#ifdef BASE_TYPE_STRING_H_
    return String(result).copy();
#else
    return String(result);
#endif
  } else {
    throw Except(EBadAccess, "it seems c++ can\'t access name of the type");
  }
}

template <typename Ret, typename... Args>
Ret Wait(Mutex& locker, Function<Ret(Args...)> run_after_wait, Args... args) {
  if (Locker::IsLocked(locker)) Locker::Lock(locker);
  return run_after_wait(args...);
}

template <typename T, typename... U>
ULong GetAddress(Function<T(U...)> f) {
  typedef T(fnType)(U...);

  fnType **fnPointer = f.template target<fnType *>();
  return (fnPointer)? (size_t)*fnPointer: 0;
}

#ifdef BASE_TYPE_STRING_H_
template <typename Type>
String ToString(Type UNUSED(value)) {
  throw Except(ENoSupport, Nametype<Type>());
}
#else
template <typename Type>
String ToString(Type value) {
  return std::to_string(value);
}

template <>
inline String ToString<std::nullptr_t>(std::nullptr_t UNUSED(value)) {
  return "None";
}

template<>
inline String ToString<Void*>(Void* value) {
  std::stringstream stream;

  stream << std::hex << reinterpret_cast<intptr_t>(value);;
  return std::string(stream.str());
}
#endif

template <typename Type>
Int ToInt(Type UNUSED(value)) { throw Except(ENoSupport, Nametype<Type>()); }

template <>
Int ToInt<UInt>(UInt value);

template <>
Int ToInt<Int>(Int value);

template <>
Int ToInt<String>(String value);

template <>
Int ToInt<CString>(CString value);

template <typename Type>
Float ToFloat(Type UNUSED(value)) { throw Except(ENoSupport, Nametype<Type>()); }

template <>
Float ToFloat<UInt>(UInt value);

template <>
Float ToFloat<Int>(Int value);

template <>
Float ToFloat<String>(String value);

template <>
Float ToFloat<CString>(CString value);

template<unsigned Index, typename ...Ts>
#ifndef BASE_TYPE_TUPLE_H_
auto Get(Tuple<Ts...>& tuple) -> decltype(Base::Get<Index>(tuple)) {
  return Base::Get<Index>(tuple);
}
#else
auto Get(Tuple<Ts...>& t) -> decltype(t.template Get<Index>(t)){
  return t.template Get<Index>(t);
}
#endif

template <typename Type>
Int Find(Iterator<Type> begin, Iterator<Type> end, Type value){
  for (UInt i = 0; i < end - begin; i += 1){
    if (value == *(begin + i)){
      return i;
    }
  }

  return -1;
}

template <typename Type>
Int Find(Type value, Tuple<Iterator<Type>, Iterator<Type>> range){
  for (UInt i = 0; i < (Base::Get<1>(range) - Base::Get<0>(range)); i += 1){
    if (value == *(Base::Get<0>(range) + i)){
      return i;
    }
  }

  return -1;
}

template <>
String ToString<String>(String value);

template <>
String ToString<char*>(char* value);

template <>
String ToString<const char*>(const char* value);

template <>
String ToString<ErrorCodeE>(ErrorCodeE value);

template <>
String ToString<ErrorLevelE>(ErrorLevelE value);

template <typename Return>
Return ReturnWithError(Return& ret, Error&& UNUSED(error)) {
  return ret;
}

#if !USE_STD_STRING
template <>
String ToString<Byte>(Byte value);

template <>
String ToString<Int>(Int value);

template <>
String ToString<UInt>(UInt value);

template <>
String ToString<Long>(Long value);

template <>
String ToString<ULong>(ULong value);

template <>
String ToString<Float>(Float value);

template <>
String ToString<Double>(Double value);
#endif

template <typename Return>
Return ReturnWithError(Return&& ret, Error&& UNUSED(error)) {
  return ret;
}

class Format{
 public:
  enum SupportE{
    ERaw,
    EFloat,
    EFloat32,
    EFloat64,
    EInteger,
    EUInteger,
    ELong,
    EString
  };

  explicit Format(String format): _Template{format}{}

  template<typename ...Args>
  String Apply(Args... args){
    auto result = String{};
    auto next = Control(result, RValue(_Template), 0);
    auto index = Base::Get<0>(next);
    auto type = Base::Get<1>(next);

    if (index >= 0){
      if (sizeof...(args) > 1) {
        return result +
          Format::_Apply(
#ifndef BASE_TYPE_TUPLE_H_
              std::make_tuple(this, _Template, index, type),
#else
              Tuple<>::Make(this, _Template, index, type),
#endif
                         args...);
      } else {
        return result +
          Format::_Apply(
#ifndef BASE_TYPE_TUPLE_H_
              std::make_tuple(this, _Template, index, type),
#else
              Tuple<>::Make(this, _Template, index, type),
#endif
                         args...) + _Template.substr(index);
      }
    } else {
      return result;
    }
  }

  template<typename Type>
  inline String operator<<(Type value){
    return Apply(value);
  }

 private:
  static Tuple<Int, Format::SupportE> Control(String& result, String&& format,
                                              Int begin);

  template<typename T, typename ...Args>
  static String _Apply(Tuple<Format*, String, Int, Format::SupportE>&& config,
                       const T &value, const Args&... args){
    auto thiz = Base::Get<0>(config);
    auto format = Base::Get<1>(config);
    auto index = Base::Get<2>(config);
    auto type = Base::Get<3>(config);

    /* @NOTE: this is a tricky way to redirect smartly our arguments to
     * specific apply function using c++'s overload */

    if (sizeof...(args) >= 1){
      auto result = String{};
      auto next = Format::Control(result, RValue(format), index);

      result = Format::_Apply(RValue(config), value) + result;
      index = Base::Get<0>(next);
      type = Base::Get<1>(next);

      /* @NOTE: decide whether of not of applying argument to the
       * next part */
      if (index < 0){
        return result;
      } else {
        return result + Format::_Apply(Tuple<>::Make(thiz, format, index, type), args...);
      }
    } else {
      return Format::_Apply(RValue(config), value);
    }
  }

  template<typename T>
  static String _Apply(Tuple<Format*, String, Int, Format::SupportE>&& config,
                       const T& value){
    auto type = Base::Get<3>(config);

    switch(type){
    case ERaw:
      break;

    case EFloat:
      if (typeid(T) != typeid(Float) && typeid(T) != typeid(Double)){
        throw Except(EBadLogic, "");
      }
      break;

    case EFloat32:
      if (typeid(T) != typeid(Float)){
        throw Except(EBadLogic, "");
      }
      break;

    case EFloat64:
      if (typeid(T) != typeid(Double)){
        throw Except(EBadLogic, "");
      }
      break;

    case EInteger:
      if (typeid(T) != typeid(Int)){
        throw Except(EBadLogic, "");
      }
      break;

    case EUInteger:
      if (typeid(T) != typeid(UInt)){
        throw Except(EBadLogic, "");
      }
      break;

    case ELong:
      if (typeid(T) != typeid(Long) && typeid(T) != (typeid(ULong))){
        throw Except(EBadLogic, "");
      }
      break;

    case EString:
      if (typeid(T) != typeid(String) && (typeid(T) == typeid(CString))){
        throw Except(EBadLogic, "");
      }
    }
    return Base::ToString(value);
  }

  String _Template;
};

class Fork: Refcount {
 public:
  enum StatusE {
    EBug = -1,
    ERunning = 0,
    EExited = 1,
    EInterrupted = 2,
    EContSigned = 4,
    EStopSigned = 6,
    EStopped = 8,
    ESegmented = 10,
    ETerminated = 12
  };

  explicit Fork(Function<Int()> callback, Bool redirect = True);
  virtual ~Fork();

  /* @NOTE: copy constructors */
  Fork(const Fork& src);
  Fork(Fork&& src);

  /* @NOTE: the assign operator */
  Fork& operator=(const Fork& src);

  /* @NOTE: get PID of the child processes */
  Int PID();

  /* @NOTE: access I/O fd of child processes */
  Int Input();
  Int Error();
  Int Output();

  /* @NOTE: get status of child processes */
  StatusE Status();
  Int ECode();

 private:
  static void Release(Refcount* ptr);

  Int _PID, _Input, _Output, _Error, *_ECode;
};

class Tie {
 public:
  template<typename Input>
  Tie& operator=(Input& input) {
    return (*this) << input;
  }

  template<typename Input>
  Tie& operator<<(Input& input) {
    throw Except(ENoSupport, 
                 Format{"No support type {}"}.Apply(Nametype<Input>()));
  }

  template<typename Output>
  Tie& operator>>(Output& output) {
    throw Except(ENoSupport, 
                 Format{"No support type {}"}.Apply(Nametype<Output>()));
  }

 private:
  Tie();

  Bool Mem2Cache(Void* context, const std::type_info& type, UInt size);

  template<typename T, typename ...Args>
  Bool Prepare(const T &value, const Args&... args) {
    if (sizeof...(args) >= 1) {
      if (Mem2Cache((Void*)&value, typeid(T), sizeof(T))) {
        return Prepare(args...);
      } else {
        return False;
      }
    } else {
      return Mem2Cache((Void*)&value, typeid(T), sizeof(T));
    }
  }

  template<typename T>
  Bool Prepare(const T &value) {
      return Mem2Cache((Void*)&value, typeid(T), sizeof(T));
  }

  template<typename... Args>
  friend Base::Tie Base::Bond(Args&... args);

  Vector<Auto> _Cache;
  Vector<UInt> _Sizes;
};

template<typename... Args>
Tie Bond(Args&... args) {
  Tie result{};
  
  if (result.Prepare(args...)) {
    return result;
  } else {
    throw Except(EBadLogic, "Can't generate a Tie object");
  }
}

String Cut(String sample, Char sep, Int posiiton);
Vector<String> Split(String sample, Char sep);
UInt Pagesize();
Int PID();
ErrorCodeE Protect(Void* address, UInt len, Int codes);
}  // namespace Base
#endif
#endif  // BASE_UTILS_H_
