#include <Macro.h>
#include <Wrapping.h>
#include <Utils.h>

#if LINUX
#include <sys/mman.h>
#include <unistd.h>

namespace Base {
namespace Internal {
namespace Wrapping {
struct Status {
  UInt current;
  UInt touchs;
  Bool readonly;

  Status(): current{0}, touchs{0}, readonly{False}{}
};

static Vector<Pair<Void*, Status>> Pages;
} // namespace Wrapping
} // namespace Internal


Void* Wrapping::Allocate(UInt size) {
  using namespace Internal::Wrapping;

  auto choose = Pages.size();
  auto pagesize = sysconf(_SC_PAGE_SIZE);

  for (UInt i = 0; i < Pages.size(); ++i) {
    if (pagesize - Pages[i].Right.current <= size) {
      if (Pages[i].Right.readonly) {
        continue;
      }

      choose = i;
      break;
    }
  }

  if (choose == Pages.size()) {
    Void* allocated = None;

    if (!(allocated = ABI::Memallign(pagesize, 1))) {
      return None;
    } else {
      if (Protect(allocated, pagesize, PROT_EXEC | PROT_READ | PROT_WRITE)) {
        Bug(EBadLogic, "can't switch to EXEC/READ/WRITE protection");
      }

      Pages.push_back(Pair<Void*, Status>(allocated, Status()));
    }
  }

  Pages[choose].Right.current += size;
  Pages[choose].Right.touchs += 1;

  return (Void*)((ULong)Pages[choose].Left + Pages[choose].Right.current);
}

void Wrapping::Writeable(Void* address) {
  using namespace Internal::Wrapping;

  auto pagesize = sysconf(_SC_PAGE_SIZE);
  auto index = -1;

  for (UInt i = 0; i < Pages.size(); ++i) {
    auto begin = (ULong)Pages[i].Left;
    auto end = begin + pagesize;

    if (begin <= (ULong)address && (ULong)address < end) {
      index = i;
      break;
    }
  }

  if (Pages[index].Right.current == pagesize) {
    if (Pages[index].Right.readonly) {
      if (Protect(Pages[index].Left, pagesize, PROT_READ | PROT_WRITE)) {
        Bug(EBadLogic, "can't switch to READ/WRITE protection");
      }
    }

    Pages[index].Right.readonly = False;
  }
}

void Wrapping::Readonly(Void* address) {
  using namespace Internal::Wrapping;

  auto pagesize = sysconf(_SC_PAGE_SIZE);
  auto index = -1;

  for (UInt i = 0; i < Pages.size(); ++i) {
    auto begin = (ULong)Pages[i].Left;
    auto end = begin + pagesize;

    if (begin <= (ULong)address && (ULong)address < end) {
      index = i;
      break;
    }
  }

  if (Pages[index].Right.current == pagesize) {
    if (!Pages[index].Right.readonly) {
      if (Protect(Pages[index].Left, pagesize, PROT_EXEC | PROT_READ)) {
        Bug(EBadLogic, "can't switch to READONLY protection");
      }
    }

    Pages[index].Right.readonly = True;
  }
}

void Wrapping::Deallocate(Void* address) {
  using namespace Internal::Wrapping;

  auto pagesize = sysconf(_SC_PAGE_SIZE);
  auto index = -1;

  for (UInt i = 0; i < Pages.size(); ++i) {
    auto begin = (ULong)Pages[i].Left;
    auto end = begin + pagesize;

    if (begin <= (ULong)address && (ULong)address < end) {
      index = i;
      break;
    }
  }

  if (index >= 0 && (--Pages[index].Right.touchs) == 0) {
    if (Pages[index].Right.readonly) {
      if (Protect(Pages[index].Left, pagesize, PROT_READ | PROT_WRITE)) {
        Bug(EBadLogic, "can't switch to READ/WRITE protection");
      }
      
      Pages[index].Right.readonly = False;
    }

    if (Pages[index].Left) {
      ABI::Free(Pages[index].Left);
    }

    Pages.erase(Pages.begin() + index);
  }
}
} // namespace Base
#endif
