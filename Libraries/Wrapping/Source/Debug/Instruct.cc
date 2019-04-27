#include <Wrapping.h>

namespace Base {
namespace Internal{
void Instruct(Void* callback, Void* result, Void** params, UInt size) {
#if __amd64__ || __x86_64__
  register void* UNUSED(addr)  asm("rax") = 0;
  register void* UNUSED(arg0)  asm("rdi") = 0;
  register void* UNUSED(arg1)  asm("rsi") = 0;
  register void* UNUSED(arg2)  asm("rdx") = 0;
  register void* UNUSED(arg3)  asm("rcx") = 0;
  register void* UNUSED(arg4)  asm("r8")  = 0;
  register void* UNUSED(arg5)  asm("r9")  = 0;

  for (UInt i = 6; i < size; ++i){
    arg4 = params[i];
    asm("push    %r8");
  }

  if (size > 5) arg5 = params[5];
  if (size > 4) arg4 = params[4];
  if (size > 3) arg3 = params[3];
  if (size > 2) arg2 = params[2];
  if (size > 1) arg1 = params[1];
  if (size > 0) arg0 = params[0];

  arg0 = (void*)result;
  addr = callback;
  asm("call    %rax");
#elif __i386__
  register void* UNUSED(addr) asm("rax") = 0;
  register void* UNUSED(rtmp) asm("rdi") = 0;

  for (UInt i = 6; i < size; ++i){
    rtmp = params[i];
    asm("push    %rdi");
  }

  rtmp = (void*)result;
  addr = callback;
  asm("call    %rax");
#elif __arm__
#error "No support your architecture"
#else
#error "No support your architecture, only run with gcc or clang"
#endif
}

void Instruct(Void* callback, Void** params, UInt size) {
#if __amd64__ || __x86_64__
  register void* UNUSED(addr)  asm("rax") = 0;
  register void* UNUSED(arg0)  asm("rdi") = 0;
  register void* UNUSED(arg1)  asm("rsi") = 0;
  register void* UNUSED(arg2)  asm("rdx") = 0;
  register void* UNUSED(arg3)  asm("rcx") = 0;
  register void* UNUSED(arg4)  asm("r8")  = 0;
  register void* UNUSED(arg5)  asm("r9")  = 0;

  for (UInt i = 6; i < size; ++i){
    arg4 = params[i];
    asm("push    %r8");
  }

  if (size > 5) arg5 = params[5];
  if (size > 4) arg4 = params[4];
  if (size > 3) arg3 = params[3];
  if (size > 2) arg2 = params[2];
  if (size > 1) arg1 = params[1];
  if (size > 0) arg0 = params[0];

  addr = callback;
  asm("call    %rax");
#elif __i386__
  register void* UNUSED(addr) asm("rax") = 0;
  register void* UNUSED(rtmp) asm("rdi") = 0;

  for (UInt i = 6; i < size; ++i){
    rtmp = params[i];
    asm("push    %rdi");
  }

  addr = callback;
  asm("call    %rax");
#elif __arm__
#error "No support your architecture"
#else
#error "No support your architecture, only run with gcc or clang"
#endif
}
} // namespace Internal

void Wrapping::Instruct(Void* callback, Void* result, Vector<Void*>& params) {
  Internal::Instruct(callback, result, const_cast<Void**>(params.data()),
                     UInt(params.size()));
}

void Wrapping::Instruct(Void* callback, Vector<Void*>& params) {
  Internal::Instruct(callback, const_cast<Void**>(params.data()),
                     UInt(params.size()));
}

void Wrapping::Instruct(Void* callback, Void* result, Vector<Auto>& params){
  Vector<Void*> tmp;

  for (auto& param: params) {
    tmp.push_back(*reinterpret_cast<Void**>(const_cast<Void*>(param.Raw())));
  }

  Wrapping::Instruct(callback, result, tmp);
}

void Wrapping::Instruct(Void* callback, Vector<Auto>& params){
  Vector<Void*> tmp;

  for (auto& param: params) {
    tmp.push_back(*reinterpret_cast<Void**>(const_cast<Void*>(param.Raw())));
  }

  Wrapping::Instruct(callback, tmp);
}
} // namespace Base
