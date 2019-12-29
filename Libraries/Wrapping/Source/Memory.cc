#include <Macro.h>
#include <Wrapping.h>
#include <Utils.h>
#include <Logcat.h>

#include <memory>

#if LINUX
#include <sys/mman.h>
#include <unistd.h>

namespace Base {
namespace Internal {
namespace Wrapping {
struct Status {
  Void* block;
  UInt current;
  UInt touchs;
  Bool readonly;

  Status(Void* block): block{block}, current{0}, touchs{0}, readonly{False} {}

  ~Status() {
    auto pagesize = sysconf(_SC_PAGE_SIZE);

    if (readonly) {
      if (Protect(block, pagesize, PROT_WRITE | PROT_READ)) {
        Bug(EBadLogic, "can't switch to READ/WRITE protection");
      }
    }

    ABI::Free(block);
  }
};

static Vector<Shared<Status>>* _Pages = None;

Vector<Shared<Status>>& Pages() {
  if (!_Pages) {
    _Pages = new Vector<Shared<Status>>();
  }

  return *_Pages;
}

void Clear() {
  if (_Pages->size() == 0) {
    if (_Pages) {
      delete _Pages;
    }

    _Pages = None;
  }
}
} // namespace Wrapping
} // namespace Internal


Void* Wrapping::Allocate(UInt size) {
  using namespace Internal::Wrapping;

  auto choose = Pages().size();
  auto pagesize = sysconf(_SC_PAGE_SIZE);

  for (UInt i = 0; i < Pages().size(); ++i) {
    if (pagesize - Pages()[i]->current <= size) {
      if (Pages()[i]->readonly) {
        continue;
      }

      choose = i;
      break;
    }
  }

  if (choose == Pages().size()) {
    Void* allocated = None;

    if (!(allocated = ABI::Memallign(pagesize, 1))) {
      return None;
    } else {
      if (Protect(allocated, pagesize, PROT_EXEC | PROT_READ | PROT_WRITE)) {
        Bug(EBadLogic, "can't switch to EXEC/READ/WRITE protection");
      }

      Pages().push_back(std::make_shared<Status>(allocated));
    }
  }

  Pages()[choose]->current += size;
  Pages()[choose]->touchs += 1;

  return (Void*)((ULong)Pages()[choose]->block + Pages()[choose]->current);
}

void Wrapping::Writeable(Void* address) {
  using namespace Internal::Wrapping;

  auto pagesize = sysconf(_SC_PAGE_SIZE);
  auto index = -1;

  for (UInt i = 0; i < Pages().size(); ++i) {
    auto begin = (ULong)Pages()[i]->block;
    auto end = begin + pagesize;

    if (begin <= (ULong)address && (ULong)address < end) {
      index = i;
      break;
    }
  }

  if (Pages()[index]->current == pagesize) {
    if (Pages()[index]->readonly) {
      if (Protect(Pages()[index]->block, pagesize, PROT_READ | PROT_WRITE)) {
        Bug(EBadLogic, "can't switch to READ/WRITE protection");
      }
    }

    Pages()[index]->readonly = False;
  }
}

void Wrapping::Readonly(Void* address) {
  using namespace Internal::Wrapping;

  auto pagesize = sysconf(_SC_PAGE_SIZE);
  auto index = -1;

  for (UInt i = 0; i < Pages().size(); ++i) {
    auto begin = (ULong)Pages()[i]->block;
    auto end = begin + pagesize;

    if (begin <= (ULong)address && (ULong)address < end) {
      index = i;
      break;
    }
  }

  if (Pages()[index]->current == pagesize) {
    if (!Pages()[index]->readonly) {
      if (Protect(Pages()[index]->block, pagesize, PROT_EXEC | PROT_READ)) {
        Bug(EBadLogic, "can't switch to READONLY protection");
      }
    }

    Pages()[index]->readonly = True;
  }
}

void Wrapping::Deallocate(Void* address) {
  using namespace Internal::Wrapping;

  auto pagesize = sysconf(_SC_PAGE_SIZE);
  auto index = -1;

  for (UInt i = 0; i < Pages().size(); ++i) {
    auto begin = (ULong)Pages()[i]->block;
    auto end = begin + pagesize;

    if (begin <= (ULong)address && (ULong)address < end) {
      index = i;
      break;
    }
  }

  if (index >= 0 && (--Pages()[index]->touchs) == 0) {
    Pages().erase(Pages().begin() + index);
  }

  Clear();
}
} // namespace Base
#endif
