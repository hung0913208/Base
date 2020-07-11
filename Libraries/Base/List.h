#ifndef BASE_LIST_H_
#define BASE_LIST_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type.h>
#include <Base/Utils.h>
#else
#include <Type.h>
#include <Utils.h>
#endif

#if __cplusplus
namespace Base {
namespace Debug {
void DumpWatch(String parameter);
} // namespace Debug

class List {
 protected:
  /* @NOTE: this enum defines how many action we can do with node. Normally.
   * we must try to acquire the node before doing anything. This is nature
   * way since the List is designed to handle with highload system where 
   * there are so many threads race to gain the right to control nodes. So
   * the control flow should be like this:
   *
   *            EFree ----> EAcquired ----> <Action> ----> EFree
   *                           |                             ^
   *                           |                             |
   *                           +-----------------------------+
   * ______________________________________________________________________ */

  enum ActionE {
    EFree = 0,
    EDelete = 1,
    EModify = 2,
    EAcquired = 3,
    EDetached = 4,
  };

 public:
#if !NDEBUG
  explicit List(Int retry = -1, Bool dumpable = False);
#else
  explicit List(Int retry = -1);
#endif
  virtual ~List();

  /* @NOTE: this method is used to get the pointer of a node, base on index or
   * its position number */
  template<typename T> 
  T* At(ULong number, Bool is_index = False) {
    T* result = None;
    Long nsec = 1;

    for (auto retry = _Retry; retry > 0; --retry) {
      Node* node = None;

      if ((node = Access(EModify, number, is_index))) {
        result = (T*)node->Ptr;

        Release(node);
        break;
      } 

      Idle((nsec = ((nsec * 2) % ULong(1e9))));
    }

    return result;
  }

  /* @NOTE: these methods is used to interact directly with our list to add or
   * delete pointers from it */
  Long Add(Void* pointer, Int retry = -1);
  Bool Del(ULong index, Void* pointer, ErrorCodeE* code = None);

  /* @NOTE: this method is used to acquire a node to prevent deleting actions */
  Bool Acquire(ULong index);

  /* @NOTE: this method is used to release a node out from state acquired */
  Bool Release(ULong index);

  /* @NOTE: */
  ULong Size();

 protected:
  struct Node {
    /* @NOTE: this is how our node is defined so with it, we can build a list to
     * work on parallel */

    Void *Ptr;
    Node *Next, *Prev, **PNext, **PPrev, **PHead, **PLast;
    ULong Index;
    ActionE State;

    explicit Node(Void* pointer, Node** phead, Node** plast) {
      Ptr = pointer;
      Next = None;
      Prev = None;
      PPrev = None;
      PNext = None;
      PHead = phead;
      PLast = plast;
      State = EFree;
      Index = 0;
    }
  };

  /* @NOTE: this method is used to attach a node into our list, the node could
   * be defined some-where else, waiting to be added into our list */
  Long Attach(Node* node);

  /* @NOTE: this method is used to detach a node out from our list so we could
   * modify it freely. To make sure that the node is within our list, the index
   * should be provided */
  Node* Detach(ULong index);

  /* @NOTE: these methods are used to access a node to grand an action to it
   * so we could control the behavior of each node on parallel */
  Node* Access(ActionE action, Node* node);
  Node* Access(ActionE action, ULong &number, Bool is_index = False,
               ErrorCodeE* code = None);

  /* @NOTE: this method is used to acquire a priviledge to access the node,
   * this input is a tricky way to prenvent node to be deleted during request
   * priviledge */
  Bool Acquire(Node** node);

  /* @NOTE: these methods are used to release the acquired state from the node
   * so it could be handle another requests */
  Bool Release(Node* node, ActionE from);
  Bool Release(Node* node);

 private:
  static void Idle(Long nanosec);

  /* @NOTE: this method is used to detach a node out from our list so we could
   * modify it freely. To make sure that the node is within our list, the index
   * should be provided */
  Node* Detach(Node* node);

  /* @NOTE: this function is used only when we need to debug watch-stopper
   * issues and we need to list everything inside the list */
  friend void Base::Debug::DumpWatch(String parameter);

#if !NDEBUG
  Bool _Dumpable;
#endif

  UInt _Retry, _Lock;
  Node *_Head, *_Last;
  ULong _Size, _Count;
};
} // namespace Base
#endif
#endif  // BASE_TYPE_H_ 
