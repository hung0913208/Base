#include <Exception.h>
#include <Macro.h>
#include <Glob.h>
#include <Type.h>

#if WINDOW
#else
#include <glob.h>
#include <string.h>
#endif

namespace Base {
Glob::Glob(String pattern): _Pattern{pattern}{}

Vector<String> Glob::AsList() {
  Vector<String> result;

#if WINDOW
#else
  glob_t buff;

  ABI::Memset(&buff, 0, sizeof(glob_t));

  /* @NOTE: scan directory using the pattern */
  switch(glob(_Pattern.c_str(), GLOB_DOOFFS | GLOB_APPEND, None, &buff)) {
  case 0:
    break;

  case GLOB_ABORTED:
    throw Except(EBadAccess, _Pattern);

  case GLOB_NOMATCH:
    throw Except(ENotFound, _Pattern);

  case GLOB_NOSPACE:
    throw Except(EDrainMem, "");

  default:
    throw Except(EWatchErrno, strerror(errno));
  }

  /* @NOTE: load result from buff */
  for (UInt i = 0; i < buff.gl_pathc; ++i) {
#ifdef BASE_TYPE_STRING_H_
    result.push_back(String{buff.gl_pathv[i]}.copy());
#else
    result.push_back(String{buff.gl_pathv[i]});
#endif

  }

  /* @NOTE: release glob_t */
  globfree(&buff);
  return result;
#endif
}
} // namespace Base
