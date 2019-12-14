#if USE_PYTHON
#include <IPython.h>
#include <Utils.h>
#include <stdlib.h>

namespace Base {
namespace Internal {
namespace IPython {
static Map<Int, Pair<String, String>> Entries;
} // namespace IPython
} // namespace Internal

namespace IPython {
typedef PyObject* (*Bait)(Int, PyObject*, PyObject*);

PyObject* Jump(Int index, PyObject* thiz, PyObject* args) {
  auto& module = Internal::IPython::Entries[index].Left;
  auto& entry = Internal::IPython::Entries[index].Right;
  auto wrapper = Base::Wrapping::Module("Python", module);

  if (wrapper && dynamic_cast<Python*>(wrapper)) {
    return dynamic_cast<Python*>(wrapper)->_Wrappers[entry](thiz, args);
  } else {
    Py_RETURN_NONE;
  }
}

PyCFunction Compile(String module, String method) {
  Char* result = None;
  Bait bait = Jump;
  Int index = -1;
  Int size = 0, pos = 0;

#if __amd64__ || __x86_64__ || __i386__
#define X86_CALL_RAX 2
#define X86_REQ 1

#if __amd64__ || __x86_64__
#define X86_ENTER_FUNCTION 4
#define X64_MOVE_RSI_TO_RDX 3
#define X64_MOVE_RDI_TO_RSI 3
#define X64_XOR_RDI_WITH_RDI 3
#define X64_MOVE_BAIT_TO_RAX 10
#define X64_MOVE_INDEX_TO_EDI 5
#endif
#endif

  /* @NOTE: detect the index number base on the couple 'module', 'method' and
   * generate a binary code to call the bait function. */

  for (auto i = 0; i < 2; i++) {
    for (auto& item : Internal::IPython::Entries) {
      if (item.second.Left != module) {
        continue;
      } else if (item.second.Right != method) {
        continue;
      } else {
        index = item.first;
        break;
      }
    }

    if (index < 0) {
      auto latest = 0;

      if (Internal::IPython::Entries.size() > 0) {
        for (auto item : Internal::IPython::Entries) {
          if (item.first > latest) {
            latest = item.first;
          }
        }

        latest += 1;
      }

      Internal::IPython::Entries[latest] = Pair<String, String>(module, method);
    }
  }

  if (index < 0) {
    return None;
  }

#if __amd64__ || __x86_64__ || __i386__
  size += X86_ENTER_FUNCTION;

#if __amd64__ || __x86_64__
  size += X64_MOVE_RSI_TO_RDX;
  size += X64_MOVE_RDI_TO_RSI;
  size += X64_XOR_RDI_WITH_RDI;
  size += X64_MOVE_INDEX_TO_EDI;
  size += X64_MOVE_BAIT_TO_RAX;
#else
#error "No support your architecture, only run with gcc or clang"
#endif

  size += X86_CALL_RAX;
  size += X86_REQ;
#elif __arm__
#error "No support your architecture, only run with gcc or clang"
#else
#error "No support your architecture, only run with gcc or clang"
#endif

  if (!(result = (Char*) Wrapping::Allocate(size))) {
    return None;
  } else {
    Wrapping::Writeable(result);
  }

#if __amd64__ || __x86_64__
  /* @NOTE: the function's begining from the objdump
   * 55                      push   %rbp
   * 50                      push   %rax
   * 52                      push   %rdx
   * 56                      push   %rsi
   * 57                      push   %rdi
   */

  result[pos++] = 0x55;
  result[pos++] = 0x50;
  result[pos++] = 0x52;
  result[pos++] = 0x56;
  result[pos++] = 0x57;

#elif __i386__
#error "No support your architecture, only run with gcc or clang"
#elif __arm__
#error "No support your architecture, only run with gcc or clang"
#else
#error "No support your architecture, only run with gcc or clang"
#endif

#if __amd64__ || __x86_64__
  /* @NOTE: generate variable bait to number move it to rax
   *  48 a1 88 77 66 55 44    movabs 0x1122334455667788,%rax
   *  33 22 11
   */

  result[pos++] = 0x48; result[pos++] = 0xb8;
  for (auto i = 0; i < 8; ++i) {
    result[pos++] = (Char) (0xff & (((ULong) bait) >> 8*i));
  }

  /* @NOTE: move rsi to rdx
   * 48 89 f2                mov    %rsi, %rdx
   */

  result[pos++] = 0x48; result[pos++] = 0x89; result[pos++] = 0xf2;

  /* @NOTE: move rdi to rsi
   * 48 89 fe                mov    %rdi, %rsi
   */

  result[pos++] = 0x48; result[pos++] = 0x89; result[pos++] = 0xfe;

  /* @NOTE: xor rdi with itself to turn its value to 0
   * 48 31 ff               xor     %rdi, %rdi
   */

  result[pos++] = 0x48; result[pos++] = 0x31; result[pos++] = 0xff;

  /* @NOTE: generate variable index to number and move it to edi
   * bf 0a 00 00 00          mov    $0xa,%edi
   */

  result[pos++] = 0xbf;
  for (auto i = 0; i < 4; ++i) {
    result[pos++] = (Char) (0xff & (((UInt) index) >> 8*i));
  }

#elif __i386__
#error "No support your architecture, only run with gcc or clang"
#elif __arm__
#error "No support your architecture, only run with gcc or clang"
#else
#error "No support your architecture, only run with gcc or clang"
#endif

#if __amd64__ || __x86_64__ || __i386__
  /* @NOTE: call the register rax
   * ff d0                   callq  *%rax
   */

  result[pos++] = 0xff; result[pos++] = 0xd0;

  /* @NOTE: the function's ending from the objdump
   * 5f                      pop   %rdi
   * 58                      pop   %rsi
   * 5a                      pop   %rdx
   * 5e                      pop   %rax
   * 5d                      pop   %rbp
   * c3                      retq
   */

  result[pos++] = 0x5f;
  result[pos++] = 0x58;
  result[pos++] = 0x5a;
  result[pos++] = 0x5e;
  result[pos++] = 0x5d;
  result[pos++] = 0xc3;
#elif __arm__
#error "No support your architecture, only run with gcc or clang"
#else
#error "No support your architecture, only run with gcc or clang"
#endif

  if (result) {
    Wrapping::Readonly(result);
  }

  return (PyCFunction) result;
}
} // namespace IPython

Python::~Python() {
  auto keep = ((PyModuleDef*)_Config)->m_methods != None;
  auto i = 0;

  if (_Size > 0) {
    Vector<Int> abandon{};

    for (auto& item : Internal::IPython::Entries) {
      if (item.second.Left != Name()) {
        continue;
      }

      abandon.push_back(item.first);
    }

    for (auto& item : abandon) {
      Internal::IPython::Entries.erase(item);
    }
  }

  while (keep) {
    keep = ((PyModuleDef*)_Config)->m_methods[i].ml_meth != None;

    if (keep) {
      Wrapping::Deallocate((Void*)((PyModuleDef*)_Config)->m_methods[i].ml_meth);
    }

    i++;
  }
}

Bool Python::Compile() {
  UInt i = 0;
  String name = Name();
  PyMethodDef* methods = None;

  if (_Size == 0 || _Config == None) {
    _Size = _Procedures.size() + _Functions.size();

    if ((_Config = ABI::Calloc(sizeof(PyModuleDef), 1))) {
      methods = (PyMethodDef*) ABI::Calloc(sizeof(PyMethodDef), _Size + 1);
    } else {
      return False;
    }
  } else if (_Size < _Procedures.size() + _Functions.size()) {
    return False;
  }

  /* @NOTE: the ending should be a NULL method definition */
  methods[_Size].ml_doc = None;
  methods[_Size].ml_name = None;
  methods[_Size].ml_meth = None;
  methods[_Size].ml_flags = 0;

  /* @NOTE: by default, we always set procedures first */
  for (auto it = _Procedures.begin(); it != _Procedures.end(); ++it) {
    auto method = it->first;
    auto bait = IPython::Compile(Name(), method);

    if (bait) {
      methods[i].ml_name = method.c_str(), method.size();
      methods[i].ml_meth = bait;
      methods[i].ml_flags = _Flags[method];

      if (_Documents[method].size() == 0) {
        methods[i].ml_doc = None;
      } else {
        methods[i].ml_doc = _Documents[method].c_str();
      }

      ++i;
    }
  }

  /* @NOTE: after finishing seting the procedures, we will set functions */
  for (auto it = _Functions.begin(); it != _Functions.end(); ++it) {
    auto method = it->first;
    auto bait = IPython::Compile(Name(), method);

    if (bait) {
      methods[i].ml_name = method.c_str();
      methods[i].ml_meth = IPython::Compile(Name(), method);
      methods[i].ml_flags = _Flags[method];

      if (_Documents[method].size() == 0) {
        methods[i].ml_doc = None;
      } else {
        methods[i].ml_doc = _Documents[method].c_str();
      }

      ++i;
    }
  }

  ((PyModuleDef*)_Config)->m_base = PyModuleDef_HEAD_INIT;
  ((PyModuleDef*)_Config)->m_name = _Module.c_str();
  ((PyModuleDef*)_Config)->m_doc = "";
  ((PyModuleDef*)_Config)->m_size = -1;
  ((PyModuleDef*)_Config)->m_methods = methods;

  return True;
}

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
  } else if (input.Type() == typeid(String)) {
#if PY_MAJOR_VERSION < 3
    return PyString_FromStringAndSize(input.Get<String>().c_str(),
                                      input.Get<String>().size());
#else
    return PyUnicode_FromStringAndSize(input.Get<String>().c_str(),
                                       input.Get<String>().size());
#endif
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
#if PY_MAJOR_VERSION < 3
  } else if (PyString_Check(input)) {
    return Auto::As(String{PyString_AsString(input), PyString_Size(input)});
#endif
  } else if (PyUnicode_Check(input)) {
    Py_ssize_t size = 0;
    CString data = PyUnicode_AsUTF8AndSize(input, &size);

    return Auto::As(String{data, ULong(size)});
  } else {
    return Auto();
  }
}

void Python::Init() {}

void Python::Enter(String UNUSED(function), PyObject* UNUSED(thiz)) {}

void Python::Exit(String UNUSED(function), PyObject* UNUSED(thiz)) {}
} // namespace Base
#endif  // USE_PYTHON
