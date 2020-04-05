#include <Auto.h>
#include <Vertex.h>
#include <cxxabi.h>
#include <string.h>

namespace Base {
namespace Internal {
Mutex* CreateMutex();

Map<ULong, Auto> Variables;
Vertex<Mutex, True> Secure([](Mutex* mutex) { Locker::Lock(*mutex); },
                           [](Mutex* mutex) { Locker::Unlock(*mutex); },
                           CreateMutex());
}  // namespace Internal

Auto::Auto()
    : Variable{[&]() -> Any& { return _Context; },
               [&](Any) { throw Except(ENoSupport, ""); }} {
  _Context.Context = None;
  _Context.Type = None;
  _Context.Reference = None;
  _Context.Del = None;
  _Context.Clone = None;
}

Auto::~Auto() { Clear(); }

Auto::Auto(std::nullptr_t)
    : Variable{[&]() -> Any& { return _Context; },
               [&](Any) { throw Except(ENoSupport, ""); }} {
  _Context.Context = None;
  _Context.Type = None;
  _Context.Reference = None;
  _Context.Del = None;
  _Context.Clone = None;
}

Auto::Auto(const Any& src)
    : Variable{[&]() -> Any& { return _Context; },
               [&](Any) { throw Except(ENoSupport, ""); }} {
  _Context.Context = None;
  _Context.Type = None;
  _Context.Reference = None;
  _Context.Del = None;
  _Context.Clone = None;

  if (&src != &_Context) {
    (*this) = const_cast<Any&>(src);
  }
}

Auto::Auto(const Auto& src)
    : Variable{[&]() -> Any& { return _Context; },
               [&](Any) { throw Except(ENoSupport, ""); }} {
  _Context.Context = None;
  _Context.Type = None;
  _Context.Reference = None;
  _Context.Del = None;
  _Context.Clone = None;

  if (&src != this) {
    (*this) = const_cast<Auto&>(src);
  }
}

Auto::Auto(Auto&& src)
    : Variable{[&]() -> Any& { return _Context; },
               [&](Any) { throw Except(ENoSupport, ""); }} {
  _Context.Context = None;
  _Context.Type = None;
  _Context.Reference = None;
  _Context.Del = None;
  _Context.Clone = None;

  if (&src != this) {
    (*this) = RValue(src);
  }
}

Auto& Auto::operator=(std::nullptr_t) {
  /* @NOTE: check Reference first, if counter is still on high, just reduce it
   */

  Clear();
  return *this;
}

Auto& Auto::operator=(Auto& src) {
  /* @NOTE: clear everything before assign a new variable */
  if (&src != this && memcmp(&src._Context, &_Context, sizeof(_Context))) {
    this->Clear();

    if (src._Context.Type != &typeid(None)) {
      _Context = src._Context;

      if (_Context.Reference) (*_Context.Reference)++;
    }
  }

  return *this;
}

Auto& Auto::operator=(Any& src) {
  /* @NOTE: clear everything before assign a new variable */
  if (&src != &_Context) {
    this->Clear();

    if (src.Type != &typeid(None)) {
      _Context = src;

      if (_Context.Reference) (*_Context.Reference)++;
    }
  }

  return *this;
}

Auto& Auto::operator=(Auto&& src) {
  /* @NOTE: clear everything before assign a new variable */
  if (&src != this && memcmp(&src._Context, &_Context, sizeof(_Context))) {
    this->Clear();

    if (src._Context.Type != &typeid(None)) {
      _Context = src._Context;

      if (_Context.Reference) (*_Context.Reference)++;
    }
  }

  return *this;
}

Auto& Auto::operator=(Any&& src) {
  /* @NOTE: clear everything before assign a new variable */
  if (&src != &_Context) {
    this->Clear();

    if (src.Type != &typeid(None)) {
      _Context = src;

      if (_Context.Reference) (*_Context.Reference)++;
    }
  }

  return *this;
}

Bool Auto::operator==(const Any& UNUSED(value)) {
  throw Except(EBadLogic, "can't compare Auto with Any");
}

Bool Auto::operator==(const Auto& UNUSED(value)) {
  throw Except(EBadLogic, "can't compare Auto with Auto");
}

Bool Auto::operator==(std::nullptr_t) { return _Context.Type == None; }

Bool Auto::operator!=(const Any& UNUSED(value)) {
  throw Except(EBadLogic, "can't compare Auto with Any");
}

Bool Auto::operator!=(const Auto& UNUSED(value)) {
  throw Except(EBadLogic, "can't compare auto with auto");
}

Bool Auto::operator!=(std::nullptr_t) { return _Context.Type != None; }

Bool Auto::operator==(Any&& UNUSED(value)) {
  throw Except(EBadLogic, "can't compare Auto with Any");
}

Bool Auto::operator==(Auto&& UNUSED(value)){
  throw Except(EBadLogic, "can't compare Auto with Auto");
}

Bool Auto::operator!=(Any&& UNUSED(value)) {
  throw Except(EBadLogic, "can't compare Auto with Any");
}

Bool Auto::operator!=(Auto&& UNUSED(value)) {
  throw Except(EBadLogic, "can't compare auto with auto");
}

Auto::operator bool() { return (*this != None); }

String Auto::Nametype() {
  if (_Context.Type) {
    auto type = reinterpret_cast<const std::type_info*>(_Context.Type);
    auto status = 0;

    std::unique_ptr<char[], void (*)(void*)> result(
        abi::__cxa_demangle(type->name(), 0, 0, &status), std::free);

    if (result.get()) {
#ifdef BASE_TYPE_STRING_H_
      return String(result.get()).copy();
#else
      return String(result.get());
#endif
    } else {
      throw Except(EBadAccess, "it seems c++ can\'t access name of the type");
    }
  } else {
    return "None";
  }
}

Auto::TypeD Auto::Type() {
  return *(reinterpret_cast<const std::type_info*>(_Context.Type));
}

const Void* Auto::Raw() { return _Context.Context; }

Tuple<Auto::RawD, Auto::DelD> Auto::Strip() {
  auto result = std::make_pair(_Context.Context, _Context.Del);

  if (_Context.Reference) {
    if (_Context.Del) {
      std::get<0>(result) = _Context.Clone(_Context.Context);

      if (*_Context.Reference > 0) {
        (*_Context.Reference)--;
      } else {
        ABI::Free(_Context.Reference);

        if (_Context.Reference) {
          _Context.Del(_Context.Context);
        } else {
          ABI::Free(_Context.Context);
        }
      }
    } else if (*_Context.Reference > 0) {
      (*_Context.Reference)--;
    } else {
      ABI::Free(_Context.Reference);
    }
  }

  ABI::Memset(&_Context, 0, sizeof(Any));
  return result;
}

void Auto::Clear() {
  Bool passed = False;

  if (_Context.Type && _Context.Reference) {
    if (*_Context.Reference > 0) {
      (*_Context.Reference)--;
      passed = True;
    }
  }

  if (!passed) {
    /* @NOTE: this is the lastest reference of this variable, remove it now */

    if (_Context.Del) _Context.Del(_Context.Context);
    if (_Context.Reference) ABI::Free(_Context.Reference);
  }
  ABI::Memset(&_Context, 0, sizeof(Any));
}

Auto::DelD& Auto::Delete() { return _Context.Del; }

Auto::CloneD& Auto::Clone() { return _Context.Clone; }

Bool Auto::Reff(Auto src) {
  return Reff(src._Context);
}

Bool Auto::Reff(Any src) {
  if (!src.Type) {
    return False;
  }

  /* @NOTE: clear the old thing */
  Clear();

  /* @NOTE: make new reference */
  ABI::Memcpy(&_Context, &src, sizeof(Any));
  (*_Context.Reference)++;

  return True;
}

Auto Auto::Copy() {
  if (_Context.Clone) {
    Auto result{};

    memset(&result._Context, 0, sizeof(_Context));

    /* @NOTE: do this to prevent removing the memory too soon duo to
     * the destruction of variable result before it clone a new guy to
     * us lately */

    if (!(result._Context.Reference = (Int*)ABI::Malloc(sizeof(Int)))) {
      throw Except(EDrainMem, "can\'t allocate Reference as expected");
    }

    /* @NOTE: config another parameters manually which bases on the current
     * Base::Auto::_Context */

    memset(result._Context.Reference, 0, sizeof(Int));

    result._Context.Clone = _Context.Clone;
    result._Context.Del = _Context.Del;
    result._Context.Type = _Context.Type;

    /* @NOTE: call the Clone API to make a new memory of the current
     * Context's data */

    result._Context.Context = _Context.Clone(_Context.Context);

    return result;
  } else {
    throw Except(EBadLogic, "can\'t copy an object without Clone callback");
  }
}
}  // namespace Base

extern "C" Any* BSAssign2Any(Any* any, void* context, void* type,
                             void (*del)(void*)) {
  using namespace Base;
  using namespace Base::Internal;

  auto index = (ULong)context;
  auto var = Any{};

  var.Context = context;
  var.Type = type;
  var.Reference = None;
  var.Del = del;
  var.Clone = None;

  /* @NOTE: init variable */
  if (!(var.Reference = (Int*)ABI::Malloc(sizeof(Int)))) {
    return ReturnWithError(RValue(None), DrainMem);
  } else {
     memset(var.Reference, 0, sizeof(Int));
  }

  if (!any) {
    if (Variables.find(index) == Variables.cend()) {
      Variables[index] = Auto{var};
    } else {
      Variables[index] = RValue(var);
    }

    any = &(Variables[index].Variable.ref());
  } else {
    *any = var;
  }

  return any;
}

extern "C" void BSNone2Any(Any* any) {
  using namespace Base;
  using namespace Base::Internal;

  auto index = (ULong)any->Context;

  if (Variables.find(index) != Variables.end()) {
    Variables[index] = None;
  } else if (any->Reference) {
    if (*(any->Reference) == 0) {
      ABI::Free(any->Reference);

      if (any->Del) {
        any->Del(any->Context);
      }

      any->Reference = None;
      any->Context = None;
    } else {
      (*any->Reference)--;
    }
  }
}
