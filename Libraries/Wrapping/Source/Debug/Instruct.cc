#include <Wrapping.h>

namespace Base {
namespace Internal{
void Instruct(Void* UNUSED(callback), Void* result, Void** params, UInt size) {
#if __amd64__ || __x86_64__
  for (UInt i = 6; i < size; ++i){
    asm ("push    %0"::"r"(params[i]));
  }

  if (size > 5) asm ("mov    %0, %%r9" ::"r"(params[5]):"r9");
  if (size > 4) asm ("mov    %0, %%r8" ::"r"(params[4]):"r8");
  if (size > 3) asm ("mov    %0, %%rcx"::"r"(params[3]):"rcx");
  if (size > 2) asm ("mov    %0, %%rdx"::"r"(params[2]):"rdx");
  if (size > 2) asm ("mov    %0, %%rsi"::"r"(params[1]):"rsi");
  if (size > 0) asm ("mov    %0, %%rdi"::"r"(params[0]):"rdi");

  asm("callq    *-8(%rbp)");
  asm("mov     %%eax, %0":"=m"(result));
#elif __i386__
  for (UInt i = 6; i < size; ++i){
    asm ("push    %0"::"r"(params[i]));
  }

  asm("callq    *-8(%rbp)");
  asm("mov     %%eax, %0":"=m"(result));
#elif __arm__
#error "No support your architecture"
#else
#error "No support your architecture, only run with gcc or clang"
#endif
}

void Instruct(Void* UNUSED(callback), Void** params, UInt size) {
#if __amd64__ || __x86_64__
  for (UInt i = 6; i < size; ++i){
    asm ("push    %0"::"r"(params[i]));
  }

  if (size > 5) asm ("mov    %0, %%r9" ::"r"(params[5]):"r9");
  if (size > 4) asm ("mov    %0, %%r8" ::"r"(params[4]):"r8");
  if (size > 3) asm ("mov    %0, %%rcx"::"r"(params[3]):"rcx");
  if (size > 2) asm ("mov    %0, %%rdx"::"r"(params[2]):"rdx");
  if (size > 2) asm ("mov    %0, %%rsi"::"r"(params[1]):"rsi");
  if (size > 0) asm ("mov    %0, %%rdi"::"r"(params[0]):"rdi");

  asm("callq    *-8(%rbp)");
#elif __i386__
  for (UInt i = 6; i < size; ++i){
    asm ("push    %0"::"r"(params[i]));
  }

  asm("callq    *-8(%rbp)");
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
