#if !defined(BASE_TYPE_CXX_ABI_H_) && __cplusplus
#define BASE_TYPE_CXX_ABI_H_
#include <Type/Common.h>
#include <cstdlib>

namespace ABI {
/* @NOTE: memory allocation */
void *Memallign(ULong alignment, Float size);
void *Realloc(Void *buffer, ULong resize);
void *Calloc(ULong count, ULong size);
void *Malloc(ULong size);

/* @NOTE: memory deallocation */
void Free(void *buffer);
void Free(void **buffer);

/* @NOTE: memory writer  */
void Memset(void *mem, Char sample, UInt size);
void Memcpy(void *dst, void *src, UInt size);

/* @NOTE: calculate string's length */
UInt Strlen(const CString str);

/* @NOTE: kill myself if i got crash */
void KillMe();

/* @NOTE: exit program with provided exit code */
Int Exit(Int code, Bool force = True);
} // namespace ABI

#define New(type) ABI::Calloc(1, sizeof(type))
#define Array(size, type) ABI::Calloc(size, sizeof(type))
#define Zero(ptr) ABI::Memset(ptr, 0, sizeof(*ptr))
#define Compare(a, b) ABI::Memcmp(a, b, sizeof(*a))
#define Length(s) ABI::Strlen(s)
#endif // BASE_TYPE_CXX_ABI_H_
