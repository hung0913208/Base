#ifndef BASE_MEMMAP_H_
#define BASE_MEMMAP_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Exception.h>
#include <Base/Macro.h>
#include <Base/Type.h>
#else
#include <Exception.h>
#include <Macro.h>
#include <Type.h>
#endif

#if LINUX || BSD || APPLE || WINDOWS
/* @NOTE: only support WINDOWS, LINUX, BSD and APPLE */

#if LINUX || BSD || APPLE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#elif WINDOW
#include <windows.h>
#endif

namespace Base{
class Memmap{
 public:
#if WINDOW
  explicit Memmap(String file, UInt flags = GENERIC_READ | GENERIC_WRITE,
                  UInt offset = 0, UInt length = 0);
#else
  explicit Memmap(String file, UInt flags = O_RDWR, UInt offset = 0,
                  UInt length = 0);
#endif

  explicit Memmap(ULong size);
  virtual ~Memmap();

  template<typename Type>
  Type* Protrude(ULong begin, ULong size,
                 Function<Type*(Bytes, ULong)> convert = None,
                 Bool copy = False){
    Type* result;

    /* @NOTE: try to protrude mem page with the provided address and size */
    if (size % sizeof(Type)) {
      return None;
    } else if (convert) {
      result = convert(_Buffer + begin, size);
    } else {
      result = (Type*)(_Buffer + begin);
    }

    /* @NOTE: make a hard copy from the */
    if (copy) {
      Type* buffer = (Type*)ABI::Malloc(size);

      if (!buffer) {
        throw Except(EDrainMem, "causes by ABI::Malloc()");
      } else {
        memcpy(buffer, result, size);
        return buffer;
      }
    } else {
      return result;
    }
  }

  Bool Update(String file = "");

 private:
  Function<void()> _Close;
  Bytes _Buffer;
  ULong _Size;
};
}  // namespace Base
#endif
#endif  // BASE_MEMMAP_H_
