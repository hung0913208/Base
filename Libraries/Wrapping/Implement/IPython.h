#if USE_PYTHON && !defined(BASE_PYTHON_WRAPPER_H_)
#define BASE_PYTHON_WRAPPER_H_

#include <Auto.h>
#include <Exception.h>
#include <Python.h>
#include <Type.h>

#ifndef BASE_WRAPPING_H_
#include <Wrapping.h>
#endif

namespace Base {
class Python : public Wrapping {
 private:
  typedef Function<PyObject*(PyObject*)> Wrapper;

 public:
  explicit Python(String name, UInt version) :
    Wrapping("Python", name, 100), _Version{version} {}

  virtual ~Python() {}

  /* @NOTE this function is used to define a standard procedure */
  template <typename... Args>
  ErrorCodeE Procedure(String name, void (*function)(Args...)) {
    if (_Wrappers.find(name) != _Wrappers.end()) {
      return (BadLogic << (Format{"redefine function {}"} << name)).code();
    }

    _Procedures[name] = reinterpret_cast<Void*>(function);
    _Wrappers[name] = [&](PyObject* pyargs) -> PyObject* {
      try {
        Vector<Void*> arguments{};
        Vector<Byte> types{};

        /* @NOTE: convert python's parameters to c/c++ parameters */
        ParseTuple<Args...>(pyargs, arguments, types);

        /* @NOTE: instruct CPU to perform the function with parameters
         * from Python */
        Wrapping::Instruct((void*)function, arguments, types);

        return Py_None;
      } catch (Error& error) {
        return Throw(error.code(), error.message());
      } catch (Exception& except) {
        return Throw(except.code(), except.message());
      } catch (std::exception& except) {
        return Throw(EWatchErrno, ToString(except.what()));
      } catch (...) {
        return Throw(EBadLogic, "Found unknown exception");
      }
    };

    return ENoError;
  }

  ErrorCodeE Procedure(String name, void (*function)()) {
    if (_Wrappers.find(name) != _Wrappers.end()) {
      return (BadLogic << (Format{"redefine function {}"} << name)).code();
    }

    _Procedures[name] = reinterpret_cast<Void*>(function);
    _Wrappers[name] = [&](PyObject* UNUSED(pyargs)) -> PyObject* {
      try {
        Vector<Void*> arguments{};
        Vector<Byte> types{};

        /* @NOTE: instruct CPU to perform the function with parameters
         * from Python */
        Wrapping::Instruct((void*)function, arguments, types);

        return Py_None;
      } catch (Error& error) {
        return Throw(error.code(), error.message());
      } catch (Exception& except) {
        return Throw(except.code(), except.message());
      } catch (std::exception& except) {
        return Throw(EWatchErrno, ToString(except.what()));
      } catch (...) {
        return Throw(EBadLogic, "Found unknown exception");
      }
    };

    return ENoError;
  }

  /* @NOTE this function is used to define a standard function */
  template <typename ResultT, typename... Args>
  ErrorCodeE Execute(String name, ResultT (*function)(Args...)) {
    if (_Wrappers.find(name) != _Wrappers.end()) {
      return (BadLogic << (Format{"redefine function {}"} << name)).code();
    }

    _Functions[name] = reinterpret_cast<Void*>(function);
    _Wrappers[name] = [&](PyObject* pyargs) -> PyObject* {
      try {
        Vector<Void*> arguments{};
        Vector<Byte> types{};
        ResultT result{};

        /* @NOTE: convert python's parameters to c/c++ parameters */
        ParseTuple<Args...>(pyargs, arguments, types);

        /* @NOTE: instruct CPU to perform the function with parameters
         * from Python */
        Wrapping::Instruct((void*)function, &result, arguments, types);

        /* @NOTE: convert ResultT to PyObjectT */
        return To<PyObject>(Auto::As(result));
      } catch (Error& error) {
        return Throw(error.code(), error.message());
      } catch (Base::Exception& except) {
        return Throw(except.code(), except.message());
      } catch (std::exception& except) {
        return Throw(EWatchErrno, ToString(except.what()));
      } catch (...) {
        return Throw(EBadLogic, "Found unknown exception");
      }
    };

    return ENoError;
  }

  template <typename ResultT>
  ErrorCodeE Execute(String name, ResultT (*function)()) {
    if (_Wrappers.find(name) != _Wrappers.end()) {
      return (BadLogic << (Format{"redefine function {}"} << name)).code();
    }

    _Functions[name] = reinterpret_cast<Void*>(function);
    _Wrappers[name] = [&](PyObject* UNUSED(pyargs)) -> PyObject* {
      try {
        Vector<Void*> arguments{};
        Vector<Byte> types{};
        ResultT result{};

        /* @NOTE: instruct CPU to perform the function with parameters
         * from Python */
        Wrapping::Instruct((void*)function, &result, arguments, types);

        /* @NOTE: convert ResultT to PyObjectT */
        return To<PyObject>(Auto::As(result));
      } catch (Error& error) {
        return Throw(error.code(), error.message());
      } catch (Base::Exception& except) {
        return Throw(except.code(), except.message());
      } catch (std::exception& except) {
        return Throw(EWatchErrno, ToString(except.what()));
      } catch (...) {
        return Throw(EBadLogic, "Found unknown exception");
      }
    };

    return ENoError;
  }

  Int Verison() { return _Version; }

 protected:
  virtual void Init() = 0;

  /* @NOTE: these funtions are used to throw an error fron C/C++ to Python */
  PyObject* Throw(ErrorCodeE error, String&& message);
  PyObject* Throw(ErrorCodeE error, String& message);

  /* @NOTE: this function is used to check if the object is existed or not */
  Bool IsExist(String object, String type) final;

  /* @NOTE: this function is used to check if the funtion is defined at wrapping
   * language */
  Bool IsUpper(String function) final;

  /* @NOTE: this function is to get address of object, it should be a function,
   * a procedure, a class or any kind of data */
  Auto Address(String object, String type = "") final;

  static Auto Down(PyObject* input);
  static PyObject* Up(Auto& input);
  static PyObject* Up(Auto&& input);

  template <typename ResultT>
  static ResultT* To(PyObject* input) {
    return &Python::Down(input).Get<ResultT>();
  }

  template <typename ResultT>
  static ResultT* To(Auto&& input) {
    if (typeid(ResultT) == typeid(PyObject)) {
      return (ResultT*)Python::Up(input);
    } else {
      return &input.Get<ResultT>();
    }
  }

  template <typename ResultT>
  static ResultT* To(Auto& input) {
    if (typeid(ResultT) == typeid(PyObject)) {
      return (ResultT*)Python::Up(input);
    } else {
      return &input.Get<ResultT>();
    }
  }

  Map<String, Void*> _Procedures;
  Map<String, Void*> _Functions;

 private:
  template <typename... Args>
  static ErrorCodeE ParseTuple(PyObject* pyargs, Vector<Void*>& output,
                               Vector<Byte>& types) {
    return ParseTuple<Args...>(pyargs, 0, output, types);
  }

  template <typename T, typename... Args>
  static ErrorCodeE ParseTuple(PyObject* pyargs, UInt begin,
                               Vector<Void*>& output, Vector<Byte>& types) {
    if (PyTuple_Check(pyargs)) {
      auto error = ParseTuple<T>(pyargs, begin, output);

      if (!error && sizeof...(Args) > 0) {
        return ParseTuple<Args...>(pyargs, begin + 1, output, types);
      } else {
        return error;
      }
    } else {
      return (BadAccess << "pyargs must be a tuple").code();
    }
  }

  template <typename T>
  static ErrorCodeE ParseTuple(PyObject* tuple, UInt index,
                               Vector<Void*>& output, Vector<Byte>& types) {
    try {
      if (typeid(T) != typeid(void)) {
        output.push_back(To<T>(PyTuple_GetItem(tuple, index)));

        if (typeid(T) == typeid(Double) || typeid(T) == typeid(Float)) {
          types.push_back(True);
        } else {
          types.push_back(False);
        }
      }
      return ENoError;
    } catch (Exception& except) {
      if (except.code() != ENoError) {
        except.Print();
      }
      return except.code();
    }
  }

  Map<String, Wrapper> _Wrappers;
  UInt _Version;
};
}  // namespace Base

/* @NOTE: this macro is used to init a python module */
#if PY_MAJOR_VERSION >= 3
#define PY_MODULE(Module)                                       \
  namespace Base {                                              \
  namespace Implement {                                         \
  namespace Python {                                            \
  class Module : public Base::Python {                          \
   public:                                                      \
    Module() : Python(#Module, 3) {}                            \
    ~Module() {}                                                \
                                                                \
    void Init() final;                                          \
  };                                                            \
  } /* namespace Python */                                      \
  } /* namespace Implement */                                   \
  } /* namespace Base */                                        \
                                                                \
  PyMODINIT_FUNC PyInit_##Module(void) {                        \
    using namespace Base::Implement::Python;                    \
    auto module = std::make_shared<Module>();                   \
                                                                \
    module->Init();                                             \
    if (module->IsLoaded()) {                                   \
      return PyModule_Create(module->Configure<PyModuleDef>()); \
    } else {                                                    \
      throw Except(ENoSupport, #Module);                        \
    }                                                           \
  }                                                             \
                                                                \
  void Base::Implement::Python::Module::Init()
#else
#define PY_MODULE(Module)                                       \
  namespace Base {                                              \
  namespace Implement {                                         \
  namespace Python {                                            \
  class Module : public Base::Python {                          \
   public:                                                      \
    Module() : Python(#Module, 2) {}                            \
    ~Module() {}                                                \
                                                                \
    void Init() final;                                          \
  };                                                            \
  } /* namespace Python */                                      \
  } /* namespace Implement */                                   \
  } /* namespace Base */                                        \
                                                                \
  PyMODINIT_FUNC init##Module(void) {                           \
    using namespace Base::Implement::Python;                    \
    auto module = std::make_shared<Module>();                   \
                                                                \
    module->Init();                                             \
    if (module->IsLoaded()) {                                   \
      Py_InitModule(#Module, module->Configure<PyMethodDef>()); \
    } else {                                                    \
      throw Except(ENoSupport, #Module);                        \
    }                                                           \
  }                                                             \
                                                                \
  void Base::Implement::Python::Module::Init()
#endif

#define PY_MODULE_CLI(Module) Base::Implement::Python::Module

#if INTERFACE == PYTHON
#define MODULE PY_MODULE
#endif
#endif  // BASE_PYTHON_WRAPPER_H_
