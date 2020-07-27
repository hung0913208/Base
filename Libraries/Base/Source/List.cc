#include <Atomic.h>
#include <List.h>
#include <Logcat.h>
#include <Vertex.h>

#include <iostream>
#include <thread>

using TimeSpec = struct timespec;

namespace Base {
namespace Internal {
void Idle(TimeSpec* spec);
Int Random(Int min, Int max);
} // namespace Internal

#if NDEBUG
List::List(Int retry, Int limit) {
#else
List::List(Int retry, Int limit, Bool dumpable) {
  _Dumpable = dumpable;
#endif
  if (limit) {
    _Lock = new Mutex();

    if (limit < 0) {
      _Limit = std::thread::hardware_concurrency();
    }

    MUTEX(_Lock);
  } else {
    _Lock = None;
    _Limit = 0;
  }

  _Online = 0;
  _Count = 0;
  _Size = 0;
  _Head = new Node{None, &_Head, &_Last};
  _Last = new Node{None, &_Head, &_Last};

  _Head->Next = _Last;
  _Last->Prev = _Head;

  /* @NOTE: here is the explanation of how the nodes connect each-other:
   * - We have 2 node A and B, A->Next --> B while B->Prev --> A while
   *   A->PNext --> &(B->Next) and B->PPrev --> &(A->Prev). For more detail
   *   please check the image below.
   *
   *      +-------------+     +-------------+
   *      |      A      |     |      B      |
   *      |             |     |             |
   *  ----|Prev&        |<----|Prev&        |
   *      |    ^        |     |    ^        |
   *  --->|    |   &Next|---->|    |   &Next|----
   *      |    ++  ^    |     |    ++  ^    |
   *  -------+  |  +-------------+  |  +---------
   *      |  |  |       |     |  |  |       |
   *      |PNext|  PPrev|     |PNext|  PPrev|
   *      +-----|----|--+     +-----|----|--+
   *            |    |              |    |
   *  ----------+    +--------------+    +-------
   *
   * - With this designation, it can be easily to remove a node on parallel
   *   using our atomic operators. To do that, we just use MCOPY to write Next
   *   to PNext and Prev to PPrev. Doing that would cause rewrite the parameters
   *   Next of the previous node and Prev of the next node respectively and the
   *   middle node will be detached before we do removing it.
   * ------------------------------------------------------------------------ */

  _Head->PPrev = &_Last->Prev;
  _Last->PNext = &_Head->Next;

  if (retry > 0) {
    _Retry = retry;
  } else {
    _Retry = 10*std::thread::hardware_concurrency();
  }
}

List::~List() {
  /* @NOTE: this destructor is unsafe since it's call by os and we can't
   * control it. The user should know that their thread should be done 
   * before this method is called. Everything is handled easily by WatchStopper
   * but i can't control daemon threads since they are out of my scope */

  Node* node{_Head};

  for (ULong i = 0; i < _Size; ++i) {
    Node* temp{node};

    node = node->Next;

    if (temp != _Head && temp != _Last) {
      delete temp;
    }
  }

  if (_Lock) {
    delete _Lock;
  }

  delete _Head;
  delete _Last;
}

void List::Idle(Long nanosec) {
  TimeSpec spec{.tv_sec=0, .tv_nsec=nanosec};
    
  Internal::Idle(&spec);

}

ULong List::Size() { return _Size; }

Long List::Add(Void* pointer, Int retry) {
  Node* node{new Node(pointer, &_Head, &_Last)};
  Long nsec{1};

  do {
    ULong result = Attach(node);

    if (result) {
      INC(&_Size);
      return result;
    }

    if (retry > 0) {
      retry--;
    }

    /* @NOTE: normally, we don't need this solution to cooldown our CPU but i
     * put it here to make everything slow abit so the CPU will have enough
     * time to handle everything with the hope that adding will work if we wait
     * enough time, when everything slow enough */

    List::Idle((nsec = (nsec * Internal::Random(1, 10) % ULong(1e9))));
  } while (retry != 0);

  delete node;

#if !NDEBUG
  if (_Dumpable) {
    ABI::KillMe();
  }
#endif

  return 0;
}

Long List::Attach(Node* node) {
  Int retry = _Retry;
  ULong result = 0;

  /* @NOTE: as i have said before, the node could be detached and reattach again
   * using this function. During this time, the node's index is defined so we 
   * don't need to check it. We should make sure that its head and last are same
   * with our list to make sure it's created by our list. */

  if (node->Index == 0) {
    node->Index = INC(&_Count);
  } else if (node->PHead != &_Head) {
    return -1; // <-- We detect a wrong node which isn't created by this list
  }

  result = node->Index;

  /* @NOTE: request adding our node to the end of our list, this should be done
   * after several attempts or we should return 0 so we could request user wait
   * a bit more since the CPU is on highload and couldn't handle our request 
   * gently in a limit time */

  while (retry > 0) {
    Node* last = node;

    retry--;

    /* @NOTE: here is the theory of how to build this list with the RCU 
     * mechanism:
     *
     * +------+  +------+
     * | head |  | last | <- While this guy will become the normal node
     * +------+  +------+
     *   ^  |      ^  |  
     *   |  |      |  |     +------+
     *   |  v      |  |     | wait | <- this guy will become the `last` node
     * ........    |  |     +------+
     *   ^  |      |  |
     *   |  |      |  |
     *   |  v      |  |
     * +------+    |  |
     * | node | ---+  |
     * +------+       |
     *   ^            |
     *   |            |
     *   +------------+
     *
     * - The node `wait` will become the new `last` node while the old `last`
     *   node will turn into a normal node with small swaping here. This would
     *   make everything run faster while maintaining the chain stable.
     * - The _Head and _Last can't be deleted so we could use them as starting
     *   point easily. They will be deleted by the List's destructor only
     *
     * ---------------------------------------------------------------------- */

    /* @NOTE: build our new `last` node on parallel, since it's still a local
     * node so we don't need to care anyone touch it when we are prepare this
     * node  */

    last->Index = 0;
    last->Next = None;
    last->Prev = _Last;
    last->PPrev = None;
    last->PNext = &_Last->Next;
      
    if (CMPXCHG(&_Last->Index, 0, result)) {
      Node* curr = _Last;

      /* @NOTE: replicate data of our new node to the current `last` node so we
       * could easy control adding new node to our list while keep our chain
       * stable, even if we are dealing with the high-loaded systems */

      _Last->Ptr = last->Ptr;
      _Last->Next = last;
      _Last->PPrev = &last->Prev; 

      /* @NOTE: everything is done now, we instruct to every node to know that
       * now we are applying the new `last` node while the old one will be used
       * as a normal node */

      if (!CMPXCHG(&_Last, curr, last)) {
        Bug(EBadLogic, "Can't announce a new `last` node");
      }
    } else {
      goto again;
    }

    return result;

again:
    BARRIER();
  }

  return 0;
}

List::Node* List::Detach(Node* node) {
  if (node && !CMP(&node, _Head) && !CMP(&node, _Last)) {
    auto prev = node->Prev;
    auto next = node->Next;
    auto pprev = node->PPrev;
    auto pnext = node->PNext;

    if (next) {
      MCOPY(pnext, &next, sizeof(next));
    }

    if (prev) {
      MCOPY(pprev, &prev, sizeof(prev));
    }

    next->PNext = pnext;
    prev->PPrev = pprev;

    Release(node);
  }

  return node;
}

List::Node* List::Detach(ULong index) {
  Node* node = None;
  Long nsec = 1;

  for (auto retry = _Retry; retry > 0; --retry) {
    if ((node = Access(EDelete, index, true))) {
      return Detach(node);
      break;
    } 

    Idle((nsec = (nsec * Internal::Random(1, 10) % ULong(1e9))));
  }

  return None;
}

Bool List::Del(ULong index, Void* ptr, ErrorCodeE* code, Bool failable) {
  Node *node = None;
  Long nsec = 1;

  for (auto retry = _Retry; retry > 0; --retry) {
    node = None;

    if ((node = Access(List::EDelete, index, True, code))) {
      if (ptr == node->Ptr) {
        if (!Detach(node)) {
          if (code) {
            *code = BadAccess("can't detach node as it should be").code();
          }

          goto reattach;
        } else {
          DEC(&_Size);
        }

        return True;
      } else {
        Release(node);
      }

      break;
    } else {
      Idle((nsec = ((nsec * Internal::Random(2, 10)) % ULong(1e9))));
    }
  }

fail:
#if !NDEBUG
  if (_Dumpable) {
    ABI::KillMe();
  }
#endif

  return False;

reattach:
  Release(node);

  while (UNLIKELY(Attach(node), Long(node->Index))) {
    Idle((nsec = (nsec * Internal::Random(1, 10) % ULong(1e9))));
  }

  goto fail;
}

List::Node* List::Access(List::ActionE action, ULong& number, Bool is_index,
                         ErrorCodeE* code) {
  Node *node{_Head};
  Node *next{None};
  Bool touched{False};

  /* @NOTE: acquire our head node first so we could prevent access to deadly
   * and cause crash to application */

  if (action == EFree) {
    return None;
  }

  while ((!is_index && number > 0) || (is_index && number != node->Index)) {
    /* @NOTE: try to consult the permision to the next node within a certain
     * attempts, hopfully we can gain it before the*/

    for (Int retry = _Retry; retry > 0; --retry) {
      if (node->Next && !CMP(&node->Next, _Last)) {
        /* @NOTE: try to acquire the next one, we are inquired to do that
         * so we could gain the priviledge to manipulate this node before
         * the timeout excess */

        if (Acquire(&node->Next, True)) {
          goto done_1;
        } else {
          BARRIER();
        }
      } else {
        if (code) {
          *code = NotFound("Reach end of list but can't see your node").code();
        }

        goto fail_1;
      }
    }

    /* @NOTE: unluckily, we reachs the timeout now so we have to drop it here
     * and return the current node so the higher level function can use it
     * for the next attempts */

    for (auto retry = _Retry; retry > 0; --retry) {
      if (Release(node)) {
        goto done_2;
      }
    }

    Bug(EBadAccess, Format{"state shouldn't be {}"}.Apply(node->State));

done_2:
    if (code) {
      *code = DoAgain("Can't acquire the next node").code();
    }

    return None;

done_1:
    /* @NOTE: got it, i have catched the right permission to do with the 
     * next node so we will release the current node to */

    next = node->Next;
    touched = True;

    if (Release(node)) {
      node = next;
    } else {
      /* @NOTE: we don't expect this block happen since we are the owner of
       * this node recently so we should have fully priviledge to this node
       * and we must be the only one to be able to release this node or we
       * are facing race condition some-where else */

      if (code) {
        *code = BadAccess("Can't release node as it should be").code();
      }

      goto fail_1;
    }
 
    if (!is_index) {
      --number;
    }
  }

  /* @NOTE: we have the right node recently so now able update the real
   * priviledge of this node */

  return Access(action, node);

fail_1:
  if (node != _Head && node != _Last) {
    for (auto retry = _Retry; retry > 0; --retry) {
      if (Release(node, EAcquired)) {
        goto fail_2;
      }
    }

    /* @TODO: this is just the workaround to prevent unexpected behavious we
     * should find a better solution or trick to debug which thread has free
     * this node */

    if (!CMP(&node->State, EFree)) {
      Bug(EBadAccess, "can't release node from state EAcquired");
    }
  }

fail_2:
  return None;
}

List::Node* List::Access(List::ActionE action, Node* node) {
  if (CMPXCHG(&node->State, EAcquired, action)) {
    return node;
  } else {
    return None;
  }
}

Bool List::Acquire(Node** node, Bool light) {
  /* @NOTE: use _Lock to maintain how many vcore supports to acquire/release 
   * nodes on parallel. This would help to prevent stressing to much to any node
   * during delete/append so much on parallel */

  if (_Lock && INC(&_Online) > _Limit) {
    LOCK(_Lock);
  }

  INC(&(*node)->Claim);

  if (node != &_Head && node != &_Last) {
    if (CMPXCHG(&(*node)->State, EFree, EAcquired)) {
      return True;
    } else if (light || CMP(&(*node)->State, EAcquired)) {
      return True;
    } else {
      goto fail;
    }
  } 
    
  return True;

fail:
  if (_Lock && DEC(&_Online) > _Limit) {
    UNLOCK(_Lock);
  }
      
  return False;
}

Bool List::Release(Node* node) {
  return Release(node, node? node->State: EFree);
}

Bool List::Release(Node* node, List::ActionE state) {
  /* @NOTE: use _Lock to maintain how many vcore supports to acquire/release 
   * nodes on parallel. This would help to prevent stressing to much to any node
   * during delete/append so much on parallel */

  if (_Lock && DEC(&_Online) > _Limit) {
    UNLOCK(_Lock);
  }

  if (!node) {
    return True;
  } 

  INC(&node->Done);

  if (!CMP(&node->Claim, node->Done)) {
    return True;
  } else if (node == _Head || node == _Last) {
    return True;
  } else if (CMPXCHG(&node->State, state, EFree)) {
    if (state == EDelete) {
      delete node;
    }

    return True;
  } else {
    return False;
  }
}
} // namespace Base
