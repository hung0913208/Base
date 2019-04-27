#include <Memmap.h>
#include <Utils.h>

#if LINUX || BSD || APPLE || WINDOWS
/* @NOTE: only support WINDOWS, LINUX, BSD and APPLE */

#if !WINDOWS
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace Base {
namespace FileN {
#if WINDOWS
ULong Size(HANDLE fd) {
  ULong current = SetFilePointer(fd, 0, 0, FILE_CURRENT);
  ULong result = SetFilePointer(fd, 0, 0, FILE_END);

  SetFilePointer(fd, current >> 32, current & 0xffffffff, FILE_BEGIN);
  return result;
}
#else
ULong Size(Int fd) {
  UInt current = lseek(fd, 0, SEEK_CUR);
  UInt result = lseek(fd, 0, SEEK_END);

  lseek(fd, current, SEEK_SET);
  return result;
}
#endif
} // namespace FileN

Memmap::Memmap(String file, UInt flags, UInt offset, UInt length) {
  auto page_size = Pagesize();
  auto aligned_offset = (offset / page_size) * page_size;
  
  _Size = offset - aligned_offset + length;

#if WINDOWS
  if (!(flags & GENERIC_READ)) {
    throw Except(EBadLogic, "Memmap can\'t work without reading flag");
  }
#else
  if (!(flags & O_RDONLY) && !(flags & O_RDWR)) {
    throw Except(EBadLogic, "Memmap can\'t work without reading flag");
  }
#endif

#if WINDOWS
  HANDLE fd = CreateFileA(file.c_str(),
                          flags,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          0,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, 0)
  HANDLE fdmap;
  Int prot, access;

  /* @NOTE: update approviated flags */
  if (flags & GENERIC_READ) {
    prot = PAGE_READONLY;
    access = FILE_MAP_READ;
  }

  if (flags & GENERIC_WRITE) {
    prot = PAGE_READWRITE;
    access = FILE_MAP_WRITE;
  }

  /* @NOTE: create file mapping */
  fdmap = CreateFileMapping(fd, 0, prot, aligned_offset >> 32,
                            aligned_offset & 0xffffffff, 0);

  _Close = [=](){ CloseHandle(fd); };
  _Buffer = MapViewOfFile(fdmap, access, aligned_offset >> 32,
                          aligned_offset & 0xffffffff,
                          _Size);
#else
  Int fd = open(file.c_str(), flags);
  Int prot;

  /* @NOTE: update approviated flags */
  if (flags & O_RDONLY) {
    prot = PROT_READ;
  }

  if (flags & O_RDWR) {
    prot = PROT_READ | PROT_WRITE;
  }

  if (fd < 0) {
    throw Except(EBadAccess, file);
  }

  /* @NOTE: create file mapping */
  _Close = [=](){ close(fd); };
  _Buffer = (Bytes) mmap(None, _Size, prot, MAP_SHARED, fd, aligned_offset);
#endif
}

Memmap::Memmap(ULong size) {
  _Buffer = (Bytes) ABI::Calloc(size, sizeof(Byte));

  if (_Buffer) {
    _Close = [&](){ free(_Buffer); };
  } else {
    throw Except(EDrainMem, "cause by ABI::Calloc()");
  }
}

Memmap::~Memmap() { if (_Close) _Close(); }
}  // namespace Base
#endif
