#ifndef BASE_QUEUE_H_
#define BASE_QUEUE_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Atomic.h>
#include <Base/Macro.h>
#include <Base/Type.h>
#include <Base/List.h>
#include <Base/Lock.h>
#include <Base/Vertex.h>
#include <Base/Logcat.h>
#else
#include <Atomic.h>
#include <Macro.h>
#include <Type.h>
#include <List.h>
#include <Lock.h>
#include <Vertex.h>
#include <Logcat.h>
#include <Utils.h>
#endif

namespace Base {
template <typename T>
class Queue {
 private:
  enum StatusE {
    ERejected = 0,
    EFinished = 1,
    EWaiting = 2
  };

  struct Node {
    Pair<T*, Bool> Slot;
    StatusE Status;
    Node *Next;

    Node(Pair<T*, Bool>&& slot): Slot{slot}, Status{ERejected}, Next{None} {}
    Node(Pair<T*, Bool>& slot): Slot{slot}, Status{ERejected}, Next{None} {}
  };

 public:
  /* @NOTE: as it suggests we can define our backlog size so if the qsize
   * reaches it, we will temporarily freeze the input so the worker-threads
   * will have enough time to reduce backlog a bit. With that we can control
   * the speed and keep balancing between input and output better */
  explicit Queue(UInt backlog = 0) : _Size{0}, _Cache{}, _Head{None},
                                     _Last{None} {
  }

  virtual ~Queue() {
  }

  /* @NOTE: This method is used to reject a task the the rejected task will put
   * back to the queue, waiting to be picked again by another threads */
  Bool Reject(UInt task) {
    Node* node = At(task);

    if (node) {
      if (!CMPXCHG(&node->Status, EWaiting, ERejected)) {
        return False;
      }

      if (!Put(node)) {
        return False;
      }

      return Del(task, node);
    }

    return False;
  }

  /* @NOTE: This method is used to confirm a task is done so we can remove it 
   * out from our cache */
  Bool Done(UInt task) {
    Node* node = At(task);

    if (node) {
      if (!CMPXCHG(&node->Status, EWaiting, EFinished)) {
        return False;
      }

      if (Del(task, node)) {
        Free(node);
      } else {
        return False;
      }
    } else {
      return False;
    }

    return True;
  }

  /* @NOTE: This method is used to lock input/output queue. 
   * - When an input is freeze, if any thread performs puting value to queue, it
   *   will lock this thread until another thread perform unfreeze.
   * - When an output is freeze, if any thread performs geting value from queue,
   *   it will lock this thread until another thread perform unfreeze.
   * - WatchStopper will ignore checking during this time or until the queue's
   *   backlog is emptied, it must depend what we would like to do and user can
   *   configure it manually */
  Bool Freeze(Bool input) {
    return _Locks[Int(input)](True);
  }

  /* @NOTE: This method is used to unlock input/output queue so the freezing
   * threads can work normally */
  Bool Unfreeze(Bool input) {
    return _Locks[Int(input)](False);
  }

  /* @NOTE: These methods are used to put a value to queue */
  Bool Put(const T& value, Double timeout = -1) {
    return Put(new Node{Pair<T*, Bool>(const_cast<T*>(&value), False)});
  }

  Bool Put(T&& value, Double timeout = -1) {
    return Put(new Node{Pair<T*, Bool>(new T{value}, True)});
  }

  /* @NOTE: This method is used to get a value from the queue. When a value is
   * picked, it will be moved to an internal cache and waiting in it to be 
   * claimed done or be rejected back to our queue */
  ULong Get(T& value, Double timeout = -1.0) {
    TimeT begin{time(None)};
    Double passing{0.0};

    while (timeout < 0.0 || (passing = difftime(time(None), begin)) > timeout) {
      Node *head{_Head}, *next{head? head->Next: None}, *last{_Last};
      ULong result{0};

      /* @NOTE: detect a freezing request so we will see the queue is hanging 
       * here until the timeout happen and this will break the queue from the
       * locked state */

      if (!Enter(False, timeout - passing)) {
        return 0;
      } else if (!CMPXCHG(&_Head, head, next)) {
        goto again;
      }
      
      if (!next) {
        CMPXCHG(&_Last, last, None);
      }

      /* @NOTE: so we successfully fetch a node out of the Queue's head and now
       * we will process this node like adding it to our cache to remember it
       * in the case user reject this node */

      if (head && head->Slot.Left) {
        if ((result = _Cache.Add(head))) {
          Vertex<void> escaping{[](){}, [&]() { Exit(False); }};
          value = *(head->Slot.Left);
        }

        return result;
      }

      break;
again:
      BARRIER();
    }

    return 0;
  }

  /* @NOTE: This method shows the actual backlog size of our queue */
  Int QSize() {
    return _Size;
  }

  /* @NOTE: This method shows the acture waiting node which are claimed to be
   * processing and it's stucked at our cache */
  Int WSize() {
    return _Cache.Size();
  }

 protected:
  /* @NOTE: this method is used to mock the new node into our serial  */
  Bool Put(Node* node, Double timeout = -1) {
    Vertex<void> escaping{[](){}, [&]() { Exit(True); }};
    TimeT begin{time(None)};
    Double passing{0.0};

    if (!CMPXCHG(&node->Status, ERejected, EWaiting)) {
      return False;
    }

    /* @NOTE: we must make sure that the inserted node doesn't connect to any node
     * to prevent unexpected behavior */
    node->Next = None;

    while (timeout < 0.0 || (passing = difftime(time(None), begin)) > timeout) {
      Node *last{_Last}, *head{_Head};


      /* @NOTE: detect a freezing request so we will see the queue is hanging 
       * here until the timeout happen and this will break the queue from the
       * locked state */

      if (!Enter(True, timeout - passing)) {
        goto fail;
      }

      if (!CMPXCHG(&_Head, None, node)) {
        if (head == None) {
          goto again;
        }
      }

      if (!CMPXCHG(&_Last, last, node)) {
        goto again;
      }

      if (last) {
        last->Next = node;
      }

      return True;
again:
      BARRIER();
    }

fail:
    if (node->Slot.Right) {
      delete node->Slot.Left;
    }

    delete node;

    return False;
  }

  /* @NOTE: this method is used to release the node after we fetch it from the
   * queue. The slot should be moved to cache, waiting to be claimed by users */
  Bool Free(Node* node) {
    if (node && node->Slot.Left) {
      if (node->Slot.Right) {
        delete node->Slot.Left;
      }

      delete node;
    } else {
      return False;
    }

    return True;
  }

  /* @NOTE: this method is used to check if we enter a freezed channel or not
   * If it's true, we should lock it and wait until it's unlocked by another
   * threads. If we set timeout, it should break after a certain time */
  Bool Enter(Bool input, Double timeout) {
    if (_Locks[Int(input)]) {
      if (_Locks[Int(input)].DoLockWithTimeout(timeout > 0.0? timeout: 0.0)) {
        _Locks[Int(input)](False);
      } else {
        return False;
      }
    }

    return True;
  }

  /* @NOTE: this method is used to increase/decrease the queue's size, depends
   * on which channel is used */
  void Exit(Bool input) {
    if (input) {
      INC(&_Size);
    } else {
      DEC(&_Size);
    }
  }

  Node* At(ULong index) {
    Node* result{None};

    //_Locks[2].Safe([&]() {
        result = _Cache.At<Node>(index, True);
    //  });

    return result;
  }

  Bool Del(ULong index, Node* node) {
    Bool result{False};

    //_Locks[2].Safe([&]() {
        result = _Cache.Del(index, node);
    // });

    return result;
  }

 private:
  Int _Size;
  Lock _Locks[3];
  List _Cache;
  Node *_Head, *_Last;
};
} // namespace Base
#endif // BASE_QUEUE_H_
