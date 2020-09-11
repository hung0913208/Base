#include <Logcat.h>
#include <Macro.h>
#include <Type.h>

#if LINUX || MACOS
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>

namespace Base {
ErrorCodeE Protect(Void *address, UInt len, Int codes) {
#define MPROTECT_CODES                                                         \
  PROT_NONE | PROT_READ | PROT_WRITE | PROT_EXEC | PROT_GROWSUP | PROT_GROWSDOWN

  auto pagesize = sysconf(_SC_PAGE_SIZE);

  if (pagesize == -1) {
    return BadAccess("sysconf error").code();
  } else if (pagesize > len) {
    return BadLogic(Format{"len={} < pagesize={}"}.Apply(len, pagesize)).code();
  }

  switch (mprotect(address, pagesize, codes)) {
  case EACCES:
    return BadAccess("the memory cannot be given the specified access").code();

  case EINVAL:
    if ((PROT_GROWSUP & codes) && (PROT_GROWSDOWN & codes)) {
      return BadLogic("use both PROT_GROWSUP and PROT_GROWSDOWN").code();
    } else if (codes & ~(MPROTECT_CODES)) {
      return BadLogic("invalid flags specified in `codes`").code();
    } else {
      return BadAccess("address is not a valid pointer or not a multiple"
                       " of the system page size")
          .code();
    }

  case ENOMEM:
    return DrainMem("Kernel can't allocate or you're changing a part which"
                    " has already changed with the same codes")
        .code();

  case 0:
    return ENoError;

  default:
    return WatchErrno(Format{"Error: {}"} << strerror(errno)).code();
  }
}
} // namespace Base
#endif
