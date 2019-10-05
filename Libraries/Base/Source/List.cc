#include <Atomic.h>
#include <List.h>
#include <Logcat.h>
#include <Vertex.h>

#include <thread>

namespace Base {
List::List(Int parallel): _Global{False}, _Sweep{False}, _Head{None},
      _Curr{None}, _Parallel{0}, _Count{0} {
  if (parallel <= 0) {
    parallel = std::thread::hardware_concurrency();
  }

  _Barriers = new Barrier[3 * parallel];
  _Size[1] = 3 * parallel;
  _Size[0] = 0;
  
  for (auto i = 0; i < 3 * parallel; ++i) {
    _Barriers[i].Left = new ULong[EEnd];

    ABI::Memset(_Barriers[i].Left, 0, EEnd * sizeof(ULong));
    MUTEX(&_Barriers[i].Right);
  }
}

List::~List() {
  _Sweep = True;

  for (auto item = _Curr; item;) {
    auto tmp = item;

    item = item->_Prev;
    delete tmp;
  }
}

ULong List::Size() { return _Size[0]; }

ULong List::Add(Void* ptr) {
  return Add(_Curr, ptr);
}

ULong List::Add(Item* prev,  Void* ptr) { 
  ULong index{INC(&_Count)};
  Item* next{prev ? prev->_Next : None};
  Item* item{new Item{}};
  Item* curr{_Curr};

  if (_Sweep) {
    return False;
  }

  item->_Index = index;
  item->_Ptr = ptr;

  if (!_Sweep) {
    Bool done{False};

    if (Access(ELModify, prev)) {
      prev->_Next = item;
      item->_Prev = prev;
    }

    if (Access(ERModify, next)) {
      next->_Prev = item;
      item->_Next = prev;
    }

    do {
      if ((done = ACK(&_Global, True))) {
        CMPXCHG(&_Head, None, item);

	if (!CMPXCHG(&_Curr, None, item)) {
          if (!next && !CMPXCHG(&_Curr, curr, item)) {
            curr = _Curr;
            item->_Prev = curr;
            curr->_Next = item;
          }
        }

        INC(&_Size[0]);
        REL(&_Global);
      }

      BARRIER();
    } while(!done);

    if (prev) {
      Unlock(prev);
    }

    if (next) {
      Unlock(next);
    }
  }

  return item->_Index;
}

Bool List::Del(ULong expected, Void* ptr) {
  Item* curr{None};
  Bool result{False};

  if (_Sweep) {
    return False;
  }

  curr = Access(ERemove, expected);
  result = curr && curr->_Ptr == ptr;

  if (!_Sweep && result) {
    Item* prev{Access(ERModify, curr->_Prev)};
    Item* next{Access(ELModify, curr->_Next)};
    Bool done{False};

    if (prev) {
      prev->_Next = next;
    }

    if (next) {
      next->_Prev = prev;
    }

    if (prev && !Unlock(prev)) {
      Bug(EBadAccess, Format{"Can unlock node {}"}.Apply(prev->_Index));
    }

    if (next && !Unlock(next)) {
      Bug(EBadAccess, Format{"Can unlock node {}"}.Apply(next->_Index));
    }

    if (_Head == curr) {
      do {
        if ((done = ACK(&_Global, True))) {
          CMPXCHG(&_Head, curr, curr->_Next);
          REL(&_Global);
        }

        BARRIER();
      } while(!done);
    }

    if (_Curr == curr) {
      do {
        if ((done = ACK(&_Global, True))) {
          CMPXCHG(&_Curr, curr, curr->_Prev);
          REL(&_Global);
        }

        BARRIER();
      } while(!done);
    }

    DEC(&_Size[0]);
  }

  if (result) {
    delete curr;
  }

  if (curr) {
    if (Unlock(expected)) {
      return result;
    } else if (_Curr && _Head) {
      Bug(EBadAccess, Format{"Can delete node {}"}.Apply(expected));
    }
  }

  return 0;
}

List::Item* List::Access(ActionE action, ULong index, Bool counting) {
  Item* node{None};

  if (counting) {
    Vertex<void> escaping{[&](){ while(!ACK(&_Global, True)) { BARRIER(); } },
                          [&](){ REL(&_Global); }};

    for (node = _Head; node; node = node->_Next) {
      if ((--index) == 0) {
        break;
      }
    }
  } else {
    Vertex<void> escaping{[&](){ while(!ACK(&_Global, True)) { BARRIER(); } },
                          [&](){ REL(&_Global); }};

    for (node = _Head; node; node = node->_Next) {
      if (node->_Index == index) {
        break;
      }
    }
  }

  return Access(action, node);
}

List::Item* List::Access(ActionE action, Item* node) {
  Item* prev{node? node->_Prev: None};
  Item* next{node? node->_Next: None};
  Item* result{None};
  UInt i;

  if (!node) {
    return None;
  }

  for (i = 0; i < _Size[1]; ++i) {
    switch (action) {
    default:
      break;

    case ERead:
      if (CMP(&_Barriers[i].Left[ERemove], node->_Index)) {
        goto finish;
      }
      break;

    case ERModify:
      if (CMP(&_Barriers[i].Left[ELModify], node->_Index)) {
        goto checking;
      } else if (CMP(&_Barriers[i].Left[ERModify], node->_Index)) {
        goto pending;
      }
      break;

    case ELModify:
      if (CMP(&_Barriers[i].Left[ERModify], node->_Index)) {
        goto checking;
      } else if (CMP(&_Barriers[i].Left[ELModify], node->_Index)) {
        goto pending;
      }
      break;

    case ERemove:
      if (CMP(&_Barriers[i].Left[ERemove], node->_Index)) {
        goto finish;
      } else if (prev && CMP(&_Barriers[i].Left[ERModify], prev->_Index)) {
        goto finish;
      } else if (next && CMP(&_Barriers[i].Left[ELModify], next->_Index)) {
        goto finish;
      }
      break;
    } 
  }

checking:
  if (_Size[1] <= _Parallel) {
    while (!ACK(&_Global, True)) {
      BARRIER();
    }
  }
  
  for (i = 0; i < _Size[1]; ++i) {
    Int j = 0;

    for (; j < EEnd; ++j) {
      if (!CMP(&_Barriers[i].Left[j], 0)) {
        break;
      }
    }

    if (j == EEnd) {
      break;
    }
  }

  LOCK(&_Barriers[i].Right);
  INC(&_Parallel);
  goto registering;

pending:
  LOCK(&_Barriers[i].Right);

registering:
  XCHG64(&_Barriers[i].Left[action], node->_Index);
  result = node;

finish:
  return result;
}

Bool List::Unlock(ULong index) {
  Bool locked = _Global && _Parallel >= _Size[1];
  Bool result = False;

  for (ULong i = 0; i < _Size[1] && !result; ++i) {
    for (Int j = 0; j < Int(EEnd) && !result; ++j) {
      if ((result = CMPXCHG(&_Barriers[i].Left[j], index, 0))) {
        DEC(&_Parallel);
        UNLOCK(&_Barriers[i].Right);
      }
    }
  }

  if (result && locked) {
    _Global = False;
  }
  return result;
}

Bool List::Unlock(Item* item) {
  if (item) {
    return Unlock(item->_Index);
  } else {
    return False;
  }
}
} // namespace Base
