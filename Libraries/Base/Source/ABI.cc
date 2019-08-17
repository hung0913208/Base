#include <Type.h>
#include <signal.h>
#include <stdlib.h>

namespace Base {
namespace Internal {
namespace Dump {
extern void (*Crasher)(int, siginfo_t*, void*);
} // namespace Dump
} // namespace Internal
} // namespace Base

namespace ABI{
void* Realloc(Void* buffer, ULong resize) { return realloc(buffer, resize); }

void* Calloc(ULong count, ULong size) { return calloc(count, size); }

void* Malloc(ULong size) { return malloc(size); }

void Free(void* buffer) { free(buffer); }

void KillMe() {
  if (Base::Internal::Dump::Crasher) {
    Base::Internal::Dump::Crasher(SIGSEGV, None, None);
  }

#if COVERAGE
  exit(-1);
#else
  abort(); 
#endif
}

void Memset(void* mem, Char sample, UInt size) {
  if (mem) {
    for (UInt i = 0; i < size; ++i) {
      ((Char*)mem)[i] = sample;
    }
  }
}

void Memcpy(void* dst, void* src, UInt size) {
  if (dst && src) {
    for (UInt i = 0; i < size; ++i) {
      ((Char*)dst)[i] = ((Char*)src)[i];
    }
  }
}

UInt Strlen(const CString str) {
  UInt result = 0;

  if (str) {
    for (; str[result]; ++result) { }
  }
  return result;
}
} // namespace ABI
