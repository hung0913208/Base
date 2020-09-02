#include <Atomic.h>
#include <List.h>
#include <Logcat.h>
#include <Vertex.h>

#include <iostream>
#include <thread>

#define DEV 0

#define IMMUTABLE 0
#define SWITCHING -1
#define REUSEABLE -2
#define ALLOCATED -3

using TimeSpec = struct timespec;

namespace Base {
namespace Internal {
void Idle(TimeSpec *spec);
Int Random(Int min, Int max);
} // namespace Internal

List::List
#if NDEBUG
    (Int retry) {
#else
    (Int retry, Bool dumpable) {
  _Dumpable = dumpable;
#endif
  ABI::Memset(_Head, 0, 2 * sizeof(Node *));

  _Count = 0;
  _Size[0] = 0;
  _Size[1] = 0;
  _Head[1] = Allocate(None);
  _Head[0] = Allocate(None);
  _Last = Allocate(None);

  /* @NOTE: since these nodes are immutable so it should be best to set their
   * index as zero to avoid accessing */

  _Head[0]->Index = 0;
  _Last->Index = 0;

  /* @NOTE: these nodes are set to be immutable so we could avoid
   * unexpected behaviour causes */

  _Head[0]->State = False;
  _Last->State = False;

  /* @NOTE: do this would help our list to be linkeable and easy to detach or
   * attach without consuming so much CPU */

  _Head[0]->Next = _Last;
  _Last->Prev[0] = _Head[0];

  /* @NOTE: here is the explanation of how the nodes connect each-other:
   * - We have 2 node A and B, A->Next --> B while B->Prev[0] --> A while
   *   A->PNext --> &(B->Next) and B->PPrev --> &(A->Prev[0]). For more detail
   *   please check the image below.
   *
   *      +---------------+     +---------------+
   *      |       A       |     |       B       |
   *      |               |     |               |
   *      |               |     |               |
   *  ----|Prev&<-+       |<----|Prev&<-+       |<---
   *      |       |       |     |       |       |
   *  --->|       |  &Next|---->|       |  &Next|----
   *      |       |  ^    |     |       |  ^    |
   *  -------+    |  +-------------+    |  +---------
   *      |  |    |       |     |  |    |       |
   *      |PNext  |  PPrev|     |PNext  |  PPrev|
   *      +-------|----|--+     +-------|----|--+
   *              |    |                |    |
   *  ------------+    +----------------+    +-------
   *
   * - With this designation, it can be easily to remove a node on parallel
   *   using our atomic operators. To do that, we just use MCOPY to write Next
   *   to PNext and Prev[0] to PPrev. Doing that would cause rewrite the
   * parameters Next of the previous node and Prev[0] of the next node
   * respectively and the middle node will be detached before we do removing it.
   *
   * - When a node is deleted, it doesn't actually remove from our list. Instead
   *   this node will be moved to deleted stack so we could reuse it later. When
   *   it happens, we will see the node's structure is changed like this.
   *
   *      +-------------+     +--------------+
   *      |       A     |     |       B      |
   *      |             |     |              |
   *      |             |     |              |
   *  ----|Prev[1]      |<----|Prev[1]       |<---
   *      |             |     |              |
   *      |             |     |              |
   *      |             |     |              |
   *      |             |     |              |
   *      |             |     |              |
   *      |             |     |              |
   *      +-------------+     +--------------+
   *
   * - Since we use dynamic queue to manage reuseable nodes, we only need one
   *   pointer to indicate where is the queue's head and one pointer for queue's
   *   tail. Queue should be a better solution since we can reuse dead nodes
   *   so we could reduce using malloc/free too much. This design intends to
   *   overcome the race condition issue when a node is going to be deleted, it
   *   could be accessed by so many thread at the same time. If we delete this
   *   node during handling accessing on another threads.
   * ------------------------------------------------------------------------ */

  _Head[0]->PPrev = &_Last->Prev[0];
  _Last->PNext = &_Head[0]->Next;

  if (retry > 0) {
    _Retry = retry;
  } else {
    _Retry = 1; // 10 * std::thread::hardware_concurrency();
  }
}

List::~List() {
  /* @NOTE: this destructor is unsafe since it's call by os and we can't
   * control it. The user should know that their thread should be done
   * before this method is called. Everything is handled easily by WatchStopper
   * but i can't control daemon threads since they are out of my scope */

  Node *node = _Head[1]->Prev[0];

  while (node != _Head[1]) {
    Node *temp{node};

    node = node->Prev[0];
    delete temp;
  }

  delete _Head[1];
}

void List::Idle(Long nanosec) {
  TimeSpec spec{.tv_sec = 0, .tv_nsec = nanosec};

  Internal::Idle(&spec);
}

ULong List::Size(Bool type) { return _Size[type]; }

Int List::Exist(ULong key) {
  ErrorCodeE error = ENoError;
  Node *node = None;

  if ((node = Access(key, True, &error))) {
    goto done;
  }

  return error == ENotFound ? 0 : -error;

done:
  return 1;
}

Int List::Exist(Void *pointer) {
  ErrorCodeE error = IndexOf(pointer, None);

  if (error) {
    return error == ENotFound ? 0 : -error;
  } else {
    return 1;
  }
}

ErrorCodeE List::IndexOf(Void *pointer, ULong *result) {
  ErrorCodeE error = ENoError;
  Node *node = None;

  if ((node = Access(pointer, &error))) {
    if (result) {
      *result = node->Index;
    }
  } else {
    node = None;
  }

  return error;
}

Long List::Add(Void *pointer, Int retry) {
  Node *node{None};
  ULong result{0};
  Long nsec{1};

  node = Allocate(pointer);

#if !DEV
  do {
#endif
    if ((result = Attach(node))) {
      INC(&_Size[0]);
      goto finish;
    }

#if !DEV
    if (retry > 0) {
      retry--;
    }

    /* @NOTE: normally, we don't need this solution to cooldown our CPU but i
     * put it here to make everything slow abit so the CPU will have enough
     * time to handle everything with the hope that adding will work if we wait
     * enough time, when everything slow enough */

    List::Idle((nsec = (nsec * Internal::Random(1, 10) % ULong(100))));
  } while (retry != 0);
#endif

#if !NDEBUG
  if (_Dumpable) {
    ABI::KillMe();
  }
#endif

  WRITE_ONCE(node->State, False);

finish:
  return result;
}

ErrorCodeE List::Add(ULong index, Void *pointer, Int retry) {
  ErrorCodeE error{EDoNothing};
  Node *node{None};
  Long nsec{1};

  node = Allocate(pointer);
  node->Index = index;

#if !DEV
  do {
#endif
    ULong result = Attach(node, &error);

    if (result) {
      INC(&_Size[0]);
      goto finish;
    }
#if !DEV
    if (error != EDoAgain) {
      break;
    }

    if (retry > 0) {
      retry--;
    }

    /* @NOTE: normally, we don't need this solution to cooldown our CPU but i
     * put it here to make everything slow abit so the CPU will have enough
     * time to handle everything with the hope that adding will work if we wait
     * enough time, when everything slow enough */

    List::Idle((nsec = (nsec * Internal::Random(1, 10) % ULong(100))));
  } while (retry != 0);
#endif

#if !NDEBUG
  if (_Dumpable) {
    ABI::KillMe();
  }
#endif

  WRITE_ONCE(node->State, False);

finish:
  return error;
}

Long List::Attach(Node *node, ErrorCodeE *error) {
  Int retry = _Retry;
  ULong result = 0;

  /* @NOTE: as i have said before, the node could be detached and reattach again
   * using this function. During this time, the node's index is defined so we
   * don't need to check it. We should make sure that its head and last are same
   * with our list to make sure it's created by our list. */

  if (node->Index == 0) {
    node->Index = INC(&_Count);
  } else if (node->PHead != &_Head[0]) {
    return -1; // <-- We detect a wrong node which isn't created by this list
  } else if (!error || *error != ENotFound) {
    /* @NOTE: this tricky way helps to reduce the number of using method Access
     * which might causes heavy load to the List itself with the asumption that
     * we never add same key to the list or add same key but on different
     * threads which might cause unexpected behavious.
     * Anyway, don't do thing without thinking since i can't cover everything
     * while keep everything works with high performance */

    if (Access(node->Index, True, error)) {
      return 0;
    }
  }

  result = node->Index;

  /* @NOTE: request adding our node to the end of our list, this should be done
   * after several attempts or we should return 0 so we could request user wait
   * a bit more since the CPU is on highload and couldn't handle our request
   * gently in a limit time */
#if !DEV
  while (retry > 0) {
#endif
    Node *last = node;

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
    last->Prev[0] = _Last;
    last->State = False;
    last->PPrev = None;
    last->PNext = &_Last->Next;

    if (CMPXCHG(&_Last->Index, 0, result)) {
      Node *curr = _Last;

      /* @NOTE: replicate data of our new node to the current `last` node so we
       * could easy control adding new node to our list while keep our chain
       * stable, even if we are dealing with the high-loaded systems */

      WRITE_ONCE(_Last->Ptr, last->Ptr);
      WRITE_ONCE(_Last->PPrev, &last->Prev[0]);
      WRITE_ONCE(_Last->Next, last);
      WRITE_ONCE(_Last->State, True);

      /* @NOTE: everything is done now, we instruct to every node to know that
       * now we are applying the new `last` node while the old one will be used
       * as a normal node */

      if (!CMPXCHG(&_Last, curr, last)) {
        Bug(EBadLogic, "Can't announce a new `last` node");
      }
#if !DEV
    } else {
      goto again;
#endif
    }

    if (error) {
      *error = ENoError;
    }

    return result;

#if !DEV
  again:
    BARRIER();
  }
#endif

  return 0;
}

List::Node *List::Detach(Node *node) {
  if (node && !CMP(&node, _Head[0]) && !CMP(&node, _Last)) {
    auto next = node->Next;
    auto prev = node->Prev[0];
    auto pprev = node->PPrev;
    auto pnext = node->PNext;

    if (next) {
      WRITE_ONCE(*pnext, next);
    }

    if (prev) {
      WRITE_ONCE(*pprev, prev);
    }

    WRITE_ONCE(next->PNext, pnext);
    WRITE_ONCE(prev->PPrev, pprev);
  }

  return node;
}

List::Node *List::Detach(ULong index) {
  Node *node = None;
  Long nsec = 1;

#if !DEV
  for (auto retry = _Retry; retry > 0; --retry) {
#endif
    if ((node = Access(index, true))) {
      return Detach(node);

#if !DEV
      break;
    }

    Idle((nsec = (nsec * Internal::Random(1, 10) % ULong(100))));
#endif
  }

  return None;
}

Bool List::Del(ULong index, Void *ptr, ErrorCodeE *code, Bool failable) {
  ErrorCodeE error = ENoError;
  Node *node = None;

  if (index == 0) {
    return False;
  }

  /* @NOTE: This is my theory of how to delete a node. When a node is deleted,
   * this node will be moved to the second queue. We will use backing pointers
   * to handle this.
   *
   * +------+
   * | head | <-- This node should be created but we don't use it.
   * +------+
   *    | ^
   *    | .         +------+
   *    v +.........| wait |
   * ........       +------+
   *    |               ^
   *    |               .
   *    v               .
   * +------+           .
   * | last |...........+ <-- This link happens during a deleted node is mocked
   * +------+                 to this list. This node is effected when we update
   *                          the pointer _Head[1]
   *
   * - These deleted nodes still allow another threads to be pass throught since
   *   we only use pointer Prev[0] while method `Access` only use pointer Next
   * ------------------------------------------------------------------------ */

  if ((node = Access(index, True, failable ? code : (&error)))) {
    if (ptr == node->Ptr) {
      if (!Detach(node)) {
        if (code) {
          *code = BadAccess("can't detach node as it should be").code();
        }

        goto reattach;
      } else {
        goto preserve;
      }
    }
  }

fail:
#if !NDEBUG
  if (_Dumpable || failable) {
    if (error || (code && *code) || failable) {
      ABI::KillMe();
    }
  }
#endif

  return False;

reattach:
  if (UNLIKELY(Attach(node), Long(node->Index))) {
    Bug(EBadLogic, "Can't reattach node");
  }

  goto fail;

preserve:
  DEC(&_Size[0]);
  return True;
}

List::Node *List::Access(ULong &number, Bool is_index, ErrorCodeE *code) {
  Node *node{_Head[0]};

  while ((!is_index && number > 0) ||
         (is_index && number != READ_ONCE(node->Index))) {
    /* @NOTE: try to consult the permision to the next node within a certain
     * attempts, hopfully we can gain it before the */

    node = READ_ONCE(node->Next);

    if (CMP(&node, _Last)) {
      if (code) {
        *code = ENotFound;
      }

      goto fail;
    }

    if (!is_index) {
      --number;
    }
  }

  /* @NOTE: we have the right node recently so now able update the real
   * priviledge of this node */

  return node;

fail:
  return None;
}

List::Node *List::Access(Void *pointer, ErrorCodeE *code) {
  Node *node{_Head[0]};

  while (node && node->Ptr != pointer) {
    /* @NOTE: try to consult the permision to the next node within a certain
     * attempts, hopfully we can gain it before the*/

    if (node->Next && !CMP(&node->Next, _Last)) {
      /* @NOTE: try to acquire the next one, we are inquired to do that
       * so we could gain the priviledge to manipulate this node before
       * the timeout excess */

      goto done;
    } else {
      if (code) {
        *code = ENotFound;
      }

      goto fail;
    }

  done:
    node = node->Next;
  }

  /* @NOTE: we have the right node recently so now able update the real
   * priviledge of this node */

  if (node) {
    return node;
  }

fail:
  return None;
}

List::Node *List::Allocate(Void *pointer) {
  Node *result = _Head[1] ? _Head[1]->Prev[1] : None;

  if (result) {
    while (!CMP(&result, _Head[1])) {
      Node *next = result ? result->Prev[1] : None;

      if (!next) {
        break;
      } else if (CMPXCHG(&result->State, False, True)) {
        goto finish;
      } else {
        BARRIER();
      }

      result = result->Prev[1];
    }
  }

  result = new Node(pointer, &_Head[0], &_Last);
  INC(&_Size[1]);

  /* @NOTE: even if we face high-loaded issue, we can't face any crash
   * because we are maintaining a circuling queue so we might at least
   * hiting to the _Head before the node is fully switch to the newest
   * dead node. Like this:
   *
   * +------+
   * | head |<-+  <- This node should be created but we don't use it.
   * +------+  |
   *     |     |
   *     |     |
   *     v     |
   * ........  |  <- Even if switching is slow, we still safe becasue
   *     |     |     these threads will hit back to the head which is
   *     |     |     never killed, i call it a backing link.
   *     v     |
   * +------+  |
   * | last |--+
   * +------+
   *     .          +-----------+
   *     +.........>| switching |
   *                +-----------+
   *
   * - The backing link will be configured after the last node is fully
   *   switched so in theory there is no exception here and we would see
   *   the circular queue is expanded, even if we are dealing with high
   *   loaded issues.
   *
   * -------------------------------------------------------------------- */

  if (_Head[1]) {
    result->Prev[1] = _Head[1];

    WRITE_ONCE(*_Head[1]->PNext, result);
  }

finish:
  result->Ptr = pointer;
  result->PHead = &_Head[0];
  result->PLast = &_Last;
  result->Index = 0;

  if (!_Head[1]) {
    _Head[1] = result;

    /* @NOTE: by default, this node should be Imutable so user can't use it */
    _Head[1]->Index = 0;
    _Head[1]->State = False;

    /* @NOTE: do this would make a circular queue so we can prevent from
     * touching undefined addresses */

    _Head[1]->Prev[0] = _Head[1];
    _Head[1]->Next = _Head[1];

    _Head[1]->PPrev = &_Head[1]->Prev[0];
    _Head[1]->PNext = &_Head[1]->Next;
  }

  return result;
}
} // namespace Base
