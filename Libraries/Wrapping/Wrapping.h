#ifndef BASE_WRAPPING_H_
#define BASE_WRAPPING_H_
#include <Auto.h>
#include <Type.h>

namespace Base {
class Wrapping {
 public:
  explicit Wrapping(String language, String module, UInt szconfig);
  virtual ~Wrapping();

  /* @NOTE: this function will be redefined to init a specific wrapper */
  virtual void Init() = 0;

  /* @NOTE: these functions are used to invoke a function. There are 2 types
   * of function:
   * - Upper functions: these funtions are defined wrapping language
   * - Extend function: these functions are defined with the same level with
   * Wrapping and required to be compiled with Wrapping */
  ErrorCodeE Invoke(String function, Vector<Auto> arguments,
                    Bool testing = False);
  ErrorCodeE Invoke(String function, Vector<Auto> arguments, Auto& result,
                    Bool testing = False);

  /* @NOTE: this function is used to check if the module is loaded */
  Bool IsLoaded();

  /* @NOTE: show what language this wrapper is supporting */
  String Language();

  /* @NOTE: show the name of this wrapper */
  String Name();

  /* @NOTE: get Configure of a wrapper */
  template<typename ResultT>
  ResultT* Configure() { return (ResultT*)_Config; }

  /* @NOTE: access modules of specific languages */
  static Wrapping* Module(String language, String module);

  /* @NOTE: wrap around to help create module without saving any trace */
  static Bool Create(Wrapping* module);

  /* @NOTE: these funtions are used to test Wrappers without installing Upper
   * languages */
  template<typename ...Args>
  static Bool Test(String language, String module, String function,
                   Args... args){
    auto wrapper = Wrapping::Module(language, module);

    if (wrapper) {
      return wrapper->Invoke(function, Stack(args...), True) == ENoError;
    }

    return !(NotFound(Format("{}::{}.{}").Apply(language, module, function)));
  }

  static Bool Test(String language, String module, String function) {
    auto wrapper = Wrapping::Module(language, module);

    if (wrapper) {
      return wrapper->Invoke(function, Vector<Auto>{}, True) == ENoError;
    } else {
      return !(NotFound(Format("{}.{} of {}").Apply(module, function, language)));
    }
  }

  /* @NOTE: these functions will help to instruct CPU to run a callback */
  static void Instruct(Void* callback, Void* result, Vector<Auto>& params);
  static void Instruct(Void* callback, Vector<Auto>& params);
  static void Instruct(Void* callback, Void* result, Vector<Void*>& params);
  static void Instruct(Void* callback, Vector<Void*>& params);

 protected:
  /* @NOTE: this function is used to check if the object is existed or not */
  virtual Bool IsExist(String object, String type = "") = 0;

  /* @NOTE: this function is used to check if the funtion is defined at wrapping
   * language */
  virtual Bool IsUpper(String function) = 0;

  /* @NOTE: this function is to get address of object, it should be a function,
   * a procedure, a class or any kind of data */
  virtual Auto Address(String object, String type = "") = 0;

  template<typename ...Args>
  Vector<Auto> Stack(Args... args) {
    Vector<Auto> result;

    Stack(result, args...);
    return result;
  }

 private:
  template<typename T, typename ...Args>
  void Stack(Vector<Auto>& result, T value, Args... args) {
    Stack<T>(result, value);

    if (sizeof...(args) > 0) {
      Stack(result, args...);
    }
  }

  template<typename T>
  void Stack(Vector<Auto>& result, T value) {
    result.push_back(Auto::As(value));
  }

  Void* _Config;
  String _Language, _Module;
};
} // namespace Base

/* @NOTE: implement bridges */
#include <IPython.h>
#include <IRuby.h>
#endif  // BASE_WRAPPING_H_
