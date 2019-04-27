#include <Wrapping.h>

namespace Base {
namespace Internal {
static Map<String, Map<String, Wrapping*>> Modules;
} // namespace Internal

Wrapping::Wrapping(String language, String module, UInt sz_config):
    _Language{language}, _Module{module} {
  using namespace Internal;

  if (!(_Config = ABI::Calloc(sz_config, 1))) {
      throw Except(EDrainMem, "");
  } else if (Modules.find(language) == Modules.end()) {
    Modules[language] = Map<String, Wrapping*>{};
  } else if (Modules[language].find(module) != Modules[language].end()) {
    throw Except(EBadLogic, Format{"duplicate module {} of "
                                   "language {}"}.Apply(module, language));
  }

  Modules[language][module] = this;
}

Wrapping::~Wrapping() {
  Internal::Modules[_Language][_Module] = None;
}

Bool Wrapping::IsLoaded() { return _Config != None; }

String Wrapping::Language() { return _Language; }

String Wrapping::Name() { return _Module; }

ErrorCodeE Wrapping::Invoke(String function, Vector<Auto> arguments,
                            Bool testing) {
  if (!IsExist(function, "procedure")) {
    return NotFound(function).code();
  } else if (IsUpper(function)) {
    /* @TODO */
  } else {
    auto callback = Address(function, "procedure");

    if (!callback) {
      return NotFound(function).code();
    } else if (callback.Type() != typeid(Void*)) {
      return BadLogic(Format{"callback shouldn\'t be "
                             "{}"}.Apply(callback.Nametype())).code();
    } else {
      Instruct(callback.Get<Void*>(), arguments);
    }
  }

  return ENoError;
}

ErrorCodeE Wrapping::Invoke(String function, Vector<Auto> arguments,
                            Auto& result, Bool testing) {
  if (result == None) {
    return EDoNothing;
  } else if (!IsExist(function, "function")) {
    return NotFound(function).code();
  } else if (IsUpper(function)) {
    /* @TODO */
  } else {
    auto callback = Address(function, "procedure");

    if (!callback || callback.Type() != typeid(Void*)) {
      return NotFound(function).code();
    } else {
      Instruct(callback.Get<Void*>(), const_cast<Void*>(result.Raw()),
               arguments);
    }
  }

  return ENoError;
}

Bool Wrapping::Create(Wrapping* module) {
  using namespace Base::Internal;

  if (Modules.find(module->Language()) == Modules.end()) {
    return !(NotFound(module->Language()));
  } else if (Modules[module->Language()].find(module->Name()) ==
             Modules[module->Language()].end()) {
    return !(NotFound(module->Name()));
  } else {
    module->Init();
  }

  return True;
}

Wrapping* Wrapping::Module(String language, String module) {
  using namespace Base::Internal;

  if (Modules.find(language) == Modules.end()) {
    return None;
  } else if (Modules[language].find(module) == Modules[language].end()) {
    return None;
  }

  return Modules[language][module];
}
} // namespace Base
