#if USE_PYTHON
#include <IPython.h>

namespace Base {
PyObject* Python::Throw(ErrorCodeE code, String& message) {
  return Throw(code, std::move(message));
}

PyObject* Python::Throw(ErrorCodeE code, String&& message) {
  PyObject* pycode;

  switch(code) {
  default:
  case ENoError:
    pycode = PyExc_Exception;
    break;

  case EDoAgain:
    pycode = PyExc_RuntimeError;
    break;

  case EKeepContinue:
    pycode = PyExc_StopIteration;
    break;

  case ENoSupport:
    pycode = PyExc_NotImplementedError;
    break;

  case EBadLogic:
    pycode = PyExc_AssertionError;
    break;

  case EBadAccess:
    pycode = PyExc_IOError;
    break;

  case EOutOfRange:
    pycode = PyExc_IndexError;
    break;

  case ENotFound:
    pycode = PyExc_LookupError;
    break;

  case EDrainMem:
    pycode = PyExc_MemoryError;
    break;

  case EWatchErrno:
    pycode = PyExc_OSError;
    break;

  case EInterrupted:
    pycode = PyExc_KeyboardInterrupt;
    break;

  case EDoNothing:
    pycode = PyExc_EnvironmentError;
    break;
  }

  PyErr_SetString(pycode, Format{"[ {} ] {}"}.Apply(ToString(code), message)
                                             .c_str());
  return (PyObject*)None;
}

Bool Python::IsExist(String object, String type) {
  if (type.size() == 0 || type == "procedure") {
    return _Procedures.find(object) != _Procedures.end();
  } else if (type.size() == 0 || type == "function") {
    return _Functions.find(object) != _Functions.end();
  } else {
    return False;
  }
}

Bool Python::IsUpper(String function) {
  /* @TODO */
  return False;
}

Auto Python::Address(String object, String type) {
  Auto result{};

  if (type.size() == 0 || type == "procedure") {
    if (_Procedures.find(object) != _Procedures.end()){
      result = Auto::As(_Procedures[object]);
    } else {
      DEBUG(Format{"Not found procedure {}"}.Apply(object));
    }
  } else if (type.size() == 0 || type == "function") {
    if (_Functions.find(object) != _Functions.end()){
      result = Auto::As(_Functions[object]);
    } else {
      DEBUG(Format{"Not found function {}"}.Apply(object));
    }
  } else {
    DEBUG(Format{"Don\'t support type {}"} << type);
  }

  return result;
}
} // namespace Base
#endif  // USE_PYTHON
