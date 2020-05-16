#if USE_PYTHON && !defined(BASE_PYTHON_WRAPPER_H_)
#define BASE_PYTHON_WRAPPER_H_
#include <Config.h>
#include <Python.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Auto.h>
#include <Base/Exception.h>
#include <Base/Utils.h>
#include <Base/Vertex.h>
#include <Base/Type.h>
#else
#include <Auto.h>
#include <Exception.h>
#include <Utils.h>
#include <Vertex.h>
#include <Type.h>
#endif

#ifndef BASE_WRAPPING_H_
#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Wrapping/Wrapping.h>
#else
#include <Wrapping.h>
#endif
#endif

namespace Base {
namespace IPython {
PyObject* Jump(Int index, PyObject* thiz, PyObject* args);
} // namespace IPython

class Python : public Wrapping {
 private:
  typedef Function<PyObject*(PyObject*, PyObject*)> Wrapper;

 public:
  explicit Python(String name, UInt version) :
#if PY_MAJOR_VERSION >= 3
    Wrapping("Python", name, sizeof(PyModuleDef)), _Size{0}, _Version{version}
#else
    Wrapping("Python", name, sizeof(PyMethodDef)), _Size{0}, _Version{version}
#endif
  {}

  virtual ~Python();

  /* @NOTE this function is used to define a standard procedure */
  template <typename... Args>
  ErrorCodeE Procedure(String name, void (*function)(Args...),
                       String document = "",
                       Int flags = METH_VARARGS) {
    if (_Wrappers.find(name) != _Wrappers.end()) {
      return (BadLogic << (Format{"redefine function {}"} << name)).code();
    }

    _Procedures[name] = reinterpret_cast<Void*>(function);
    _Documents[name] = document;
    _Flags[name] = flags;
    _Wrappers[name] = [=](PyObject* UNUSED(thiz),
                          PyObject* pyargs) -> PyObject* {
      Base::Vertex<void> escaping{[&](){ Enter(name, thiz); },
                                  [&](){ Exit(name, thiz); }};

      try {
        Vector<Auto> arguments{};

        /* @NOTE: convert python's parameters to c/c++ parameters */
        if (ParseTuple<Args...>(pyargs, arguments)) {
          Py_RETURN_NONE;
        }

        /* @NOTE: instruct CPU to perform the function with parameters
         * from Python */
        Wrapping::Instruct((void*)_Procedures[name], arguments);

        Py_RETURN_NONE;
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

  ErrorCodeE Procedure(String name, void (*function)(), String document = "",
                       Int flags = METH_VARARGS) {
    if (_Wrappers.find(name) != _Wrappers.end()) {
      return (BadLogic << (Format{"redefine function {}"} << name)).code();
    }

    _Procedures[name] = reinterpret_cast<Void*>(function);
    _Documents[name] = document;
    _Flags[name] = flags;
    _Wrappers[name] = [=](PyObject* UNUSED(thiz), 
                          PyObject* UNUSED(pyargs)) -> PyObject* {
      Base::Vertex<void> escaping{[&](){ Enter(name, thiz); },
                                  [&](){ Exit(name, thiz); }};

      try {
        Vector<Auto> arguments{};

        /* @NOTE: instruct CPU to perform the function with parameters
         * from Python */
        Wrapping::Instruct((void*)function, arguments);

        Py_RETURN_NONE;
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
  ErrorCodeE Execute(String name, ResultT (*function)(Args...),
                     String document = "",
                     Int flags = 0) {
    if (_Wrappers.find(name) != _Wrappers.end()) {
      return (BadLogic << (Format{"redefine function {}"} << name)).code();
    }

    _Functions[name] = reinterpret_cast<Void*>(function);
    _Documents[name] = document;
    _Flags[name] = flags;
    _Wrappers[name] = [=](PyObject* UNUSED(thiz),
                          PyObject* pyargs) -> PyObject* {
      Base::Vertex<void> escaping{[&](){ Enter(name, thiz); },
                                  [&](){ Exit(name, thiz); }};

      try {
        Vector<Auto> arguments{};
        ResultT result{};

        /* @NOTE: convert python's parameters to c/c++ parameters */
        if (ParseTuple<Args...>(pyargs, arguments)) {
          Py_RETURN_NONE;
        }

        /* @NOTE: instruct CPU to perform the function with parameters
         * from Python */
        Wrapping::Instruct((void*)function, &result, arguments);

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
  ErrorCodeE Execute(String name, ResultT (*function)(), String document = "",
                     Int flags = 0) {
    if (_Wrappers.find(name) != _Wrappers.end()) {
      return (BadLogic << (Format{"redefine function {}"} << name)).code();
    }

    _Functions[name] = reinterpret_cast<Void*>(function);
    _Documents[name] = document;
    _Flags[name] = flags;
    _Wrappers[name] = [=](PyObject* UNUSED(thiz),
                          PyObject* UNUSED(pyargs)) -> PyObject* {      
      Base::Vertex<void> escaping{[&](){ Enter(name, thiz); },
                                  [&](){ Exit(name, thiz); }};

      try {
        Vector<Auto> arguments{};
        ResultT result{};

        /* @NOTE: instruct CPU to perform the function with parameters
         * from Python */
        Wrapping::Instruct((void*)function, &result, arguments);

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
  virtual void Init();
  virtual void Enter(String function, PyObject* thiz);
  virtual void Exit(String function, PyObject* thiz);

  /* @NOTE: this function is used to compile the python wrapping functions to the
   * function table which are stored inside the variable _Config. */
  Bool Compile() final;

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
  static ResultT* To(PyObject* UNUSED(input)) {
    return None;
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

  UInt _Size;
  Map<String, Int> _Flags;
  Map<String, Void*> _Procedures;
  Map<String, Void*> _Functions;
  Map<String, String> _Documents;

 private:
  template <typename... Args>
  static ErrorCodeE ParseTuple(PyObject* pyargs, Vector<Auto>& output) {
    return TupleParser<Args...>{}.Parse(pyargs, 0u, output);
  }

  template <typename T, typename... Args>
  struct TupleParser {
      ErrorCodeE Parse(PyObject* pyargs, UInt begin, Vector<Auto>& output) {
      if (PyTuple_Check(pyargs)) {
        auto error = ParseTupleS<T>(PyTuple_GetItem(pyargs, begin), output);

        if (!error && sizeof...(Args) > 0) {
          return TupleParser<Args...>{}.Parse(pyargs, begin + 1, output);
        } else {
          return error;
        }
      } else {
        return (BadAccess << "pyargs must be a tuple").code();
      }
    }
  };

  template <typename T>
  struct TupleParser<T> {
      ErrorCodeE Parse(PyObject* pyargs, UInt begin, Vector<Auto>& output) {
      if (PyTuple_Check(pyargs)) {
        return ParseTupleS<T>(PyTuple_GetItem(pyargs, begin), output);
      } else {
        return (BadAccess << "pyargs must be a tuple").code();
      }
    }
  };

  template <typename T>
  static ErrorCodeE ParseTupleS(PyObject* arg, Vector<Auto>& output) {
    try {
      if (typeid(T) != typeid(void)) {
        auto converted = Python::Down(arg);

        if (typeid(T) == converted.Type()) {
          output.push_back(converted);
        } else {
          return BadLogic(Format{"{} != {}"}
              .Apply(Nametype<T>(), converted.Nametype())).code();
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

  friend PyObject* Base::IPython::Jump(Int index, PyObject* thiz, 
                                      PyObject* args);

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
    if (!Base::Wrapping::Create(module)) {                      \
      throw Except(EBadLogic, #Module);                         \
    } else if (module->IsLoaded()) {                            \
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
    if (!Base::Wrapping::Create(module)) {                      \
      throw Except(EBadLogic, #Module);                         \
    } else if (module->IsLoaded()) {                            \
      auto table = module->Configure<PyMethodDef>();            \
                                                                \
      if (table) {                                              \
        Py_InitModule(#Module, table);                          \
      } else {                                                  \
        Py_InitModule(#Module, table);                          \
      }                                                         \
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
