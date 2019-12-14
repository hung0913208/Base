#include <Wrapping.h>

extern "C" {
void CallFunct(Void* callback, Void** params, Byte* types, UInt size,
               Void* result);
void CallProc(Void* callback, Void** params, Byte* types, UInt size);
}

namespace Base {
void Wrapping::Instruct(Void* callback, Void* result, Vector<Void*>& params,
                        Vector<Byte>& types) {
  if (types.size() != params.size()) {
    throw Except(EBadLogic, Format{"types({}) != params({})"}
        .Apply(types.size(), params.size()));
  }

#if __amd64__ || __x86_64__
  CallFunct(callback, const_cast<Void**>(params.data()),
            const_cast<Byte*>(types.data()), params.size(), result);
#elif __i386__
#elif __arm__
#error "No support your architecture"
#else
#error "No support your architecture, only run with gcc or clang"
#endif
}

void Wrapping::Instruct(Void* callback, Vector<Void*>& params,
                        Vector<Byte>& types) {
  if (types.size() != params.size()) {
    throw Except(EBadLogic, Format{"types({}) != params({})"}
        .Apply(types.size(), params.size()));
  }

#if __amd64__ || __x86_64__
  CallProc(callback, const_cast<Void**>(params.data()), 
           const_cast<Byte*>(types.data()), params.size());
#elif __i386__
#elif __arm__
#error "No support your architecture"
#else
#error "No support your architecture, only run with gcc or clang"
#endif
}

void Wrapping::Instruct(Void* callback, Void* result, Vector<Auto>& params){
  Vector<Void*> conv{};
  Vector<Byte> types{};

  for (auto& param: params) {
    if (param.Type() == typeid(Double) || param.Type() == typeid(Float)) {
      types.push_back(True);
    } else {
      types.push_back(False);
    }
      
    conv.push_back(*reinterpret_cast<Void**>(const_cast<Void*>(param.Raw())));
  }

  Wrapping::Instruct(callback, result, conv, types);
}

void Wrapping::Instruct(Void* callback, Vector<Auto>& params){
  Vector<Void*> conv;
  Vector<Byte> types{};

  for (auto& param: params) {
    if (param.Type() == typeid(Double) || param.Type() == typeid(Float)) {
      types.push_back(True);
    } else {
      types.push_back(False);
    }

    conv.push_back(*reinterpret_cast<Void**>(const_cast<Void*>(param.Raw())));
  }

  Wrapping::Instruct(callback, conv, types);
}
} // namespace Base
