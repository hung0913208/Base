#include "Utils.h"
#include <stdarg.h>
#include <stdio.h>

extern "C" CString BSFormat(CString format, ...) {
  CString result = None;
  va_list ap;
  Int size;

  va_start(ap, format);
  if ((size = vsnprintf(None, 0, format, ap)) <= 0) {
    return None;
  } else if (!(result = (CString)ABI::Calloc(size + 1, sizeof(Byte)))) {
    return None;
  } else if (vsnprintf(result, size + 1, format, ap) <= 0) {
    free(result);
    return None;
  } else {
    return result;
  }
}
CString BSFormat(String format, ...) {
  CString result = None;
  va_list ap;
  Int size;

  va_start(ap, format);
  if ((size = vsnprintf(None, 0, format.c_str(), ap)) <= 0) {
    return None;
  } else if (!(result = (CString)ABI::Calloc(size + 1, sizeof(Byte)))) {
    return None;
  } else if (vsnprintf(result, size + 1, format.c_str(), ap) <= 0) {
    free(result);
    return None;
  } else {
    return result;
  }
}

namespace Base{
Tuple<Int, Format::SupportE> Format::Control(String& result, String&& format,
                                             Int begin){
  /* @NOTE: this static function is used to control how to format result from
   * arguments */

  for (UInt i = begin; i < format.size(); ++i) {
    if (format[i] == '{' && format[i + 1] == '}') {
      /* @NOTE: use Python format here, {} -> String */

        return std::make_tuple<Int, Format::SupportE>(i + 2, ERaw);
    } else if (format[i] == '%'){
      /* @NOTE: define a limited version of C format */

      switch(format[i + 1]){
      case 'd':
      case 'i':
        return std::make_tuple<Int, Format::SupportE>(i + 2, EInteger);

      case 'l':
        return std::make_tuple<Int, Format::SupportE>(i + 2, ELong);

      case 'u':
        return std::make_tuple<Int, Format::SupportE>(i + 2, EUInteger);

      case 'f':
        return std::make_tuple<Int, Format::SupportE>(i + 2, EFloat);

      case 's':
        return std::make_tuple<Int, Format::SupportE>(i + 2, EString);

      default:
        result += format[i];
        break;
      }
    } else {
      result += format[i];
    }
  }

  return std::make_tuple<Int, Format::SupportE>(-1, Format::ERaw);
}
}
