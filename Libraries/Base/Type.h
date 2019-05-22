#ifndef BASE_TYPE_H_
#define BASE_TYPE_H_
#include <ABI.h>
#include <Common.h>

#if __cplusplus
#include <String.h>

#ifndef BASE_TYPE_STRING_H_
#include <Refcount.h>

using String = std::string;
#else
using String = Base::String;
#endif // BASE_TYPE_STRING_H_
#else
#include <string.h>
#include <Refcount.h>

#define String char*
#endif
#endif  // BASE_TYPE_H_
