#if USE_PYTHON
#include <IPython.h>
#include <Utils.h>

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

Bool Python::IsUpper(String UNUSED(function)) {
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

PyObject* Python::Up(Auto& input) {
  return Python::Up(std::move(input));
}

PyObject* Python::Up(Auto&& input) {
  if (input.Type() == typeid(Int)) {
#if PY_MAJOR_VERSION < 3
    return PyInt_FromLong(Long(input.Get<Int>()));
#else
    return PyLong_FromLong(Long(input.Get<Int>()));
#endif
  } else if (input.Type() == typeid(UInt)) {
#if PY_MAJOR_VERSION < 3
    return PyInt_FromLong(Long(input.Get<UInt>()));
#else
    return PyLong_FromUnsignedLong(ULong(input.Get<UInt>()));
#endif
  } else if (input.Type() == typeid(Short)) {
#if PY_MAJOR_VERSION < 3
    return PyInt_FromLong(Long(input.Get<Short>()));
#else
    return PyLong_FromLong(Long(input.Get<Short>()));
#endif
  } else if (input.Type() == typeid(UShort)) {
#if PY_MAJOR_VERSION < 3
    return PyInt_FromLong(Long(input.Get<UShort>()));
#else
    return PyLong_FromUnsignedLong(ULong(input.Get<UShort>()));
#endif
  } else if (input.Type() == typeid(Long)) {
    return PyLong_FromLong(input.Get<Long>());
  } else if (input.Type() == typeid(ULong)) {
    return PyLong_FromUnsignedLong(input.Get<ULong>());
  } else if (input.Type() == typeid(LLong)) {
    return PyLong_FromLongLong(input.Get<LLong>());
  } else if (input.Type() == typeid(ULLong)) {
    return PyLong_FromUnsignedLongLong(input.Get<ULLong>());
  } else if (input.Type() == typeid(Bool)) {
    return input.Get<Bool>()? Py_True: Py_False;
  } else if (input.Type() == typeid(Float)) {
    return PyFloat_FromDouble(Double(input.Get<Float>()));
  } else if (input.Type() == typeid(Double)) {
    return PyFloat_FromDouble(input.Get<Double>());
  } else {
    return Py_None;
  }
}

Auto Python::Down(PyObject* input) {
#if PY_MAJOR_VERSION < 3
  if (PyInt_Check(input)) {
    return Auto::As<Int>(PyInt_AsLong(input));
  }
#endif

  if (PyBool_Check(input)) {
    if (input == Py_True) {
      return Auto::As<Bool>(True);
    } else if (input == Py_False) {
      return Auto::As<Bool>(False);
    } else {
      throw Except(EBadLogic, "receive unexpected values");
    }
  } else if (PyLong_Check(input)) {
    return Auto::As<Long>(PyLong_AsLong(input));
  } else if (PyFloat_Check(input)) {
    return Auto::As<Float>(PyFloat_AsDouble(input));
  } else {
    return Auto();
  }
}
} // namespace Base
#endif  // USE_PYTHON
