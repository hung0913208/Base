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
   * - Acquire can pass through if we raise this would be a light-weight
   *   acquiring. This feature is designed to make the nodes don't die with a
   *   litter risk of raising somewhere, hope CPU's atomic works as it has been
   *     
   *     <Node> -------> <Node> -------> <Node> -------> <Node>
   *       ^               ^               ^
   *       |               |               |
   *   you're here   This node is    You're destination
   *                   detached
   *
   *   In the past, if the node is detached, even if we still see it onboard, 
   *   method Access can't reach to your destination. We would see several loop
   *   happen until the detached node is completedly remove. When we're facing
   *   with flooding threads, the kernel might stuck at each threads for very
   *   long time. What would we see here some threads might occupy a node and
   *   stuck at detaching it for very long time causing another faster threads
   *   to be hang and slowdown the whole process. Using light-weith Acquire
   *   may solve this issue and improve performance.
   *
   * - EAcquired should be accepted on parallel where multiple-nodes trying to
   *   access the node. The big problem is that the node might be detached
   *   unexpectedly and cause the list to be corrupted. There are 2 solution to
   *   deal with it: use light-weight lock to protect node or use referral
   *   counter to detect when the node is dead. 
   *
   * - The first solution is safe but slow when we face with so much threads 
   *   which causes everything await for so long time and make CPU got high 
   *   pitch.
   *
   * - The second one seems ok but we might see unexpected coredump happen at 
   *   malloc() even if everything should safe enough to deal with normal cases.
   *
   * - To make more safer, we should limit how much vcpu working on parallel so
   *   we can at least avoid unexpected behaviois which is caused by highload
   *   CPU or flooding threads.
   *
   * - By the time, it still too much stress to overcome highload CPU and
   *   flooding threads especially when we test our suites on parallel using
   *   bazel. Failing happens frequently on several racing cases and slowdown
   *   cases. Need to refactor higher layer to improve with better solution.
   * ______________________________________________________________________ */

  enum ActionE {
    EAcquired = 1,
    EDetached = 2,
    EDelete = 3,
    EFree = 0,
  };

 public:
#if !NDEBUG
  explicit List(Int retry = -1, Int limit = 0, Bool dumpable = False);
#else
  explicit List(Int retry = -1, Int limit = 0);
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

      if ((node = Access(EAcquired, number, is_index))) {
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
  Bool Del(ULong index, Void* pointer,
           ErrorCodeE* code = None, Bool failable = False);

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

    UInt Claim, Done;
    Void *Ptr;
    Node *Next, *Prev, **PNext, **PPrev, **PHead, **PLast;
    ULong Index;
    ActionE State;

    explicit Node(Void* pointer, Node** phead, Node** plast) {
      Ptr = pointer;
      Next = None;
      Prev = None;
      Done = 0;
      PPrev = None;
      PNext = None;
      PHead = phead;
      PLast = plast;
      State = EFree;
      Index = 0;
      Claim = 0;
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

 private:
  /* @NOTE: this method is used to acquire a priviledge to access the node,
   * this input is a tricky way to prenvent node to be deleted during request
   * priviledge */
  Bool Acquire(Node** node, Bool light = False);

  /* @NOTE: these methods are used to release the acquired state from the node
   * so it could be handle another requests */
  Bool Release(Node* node, ActionE from);
  Bool Release(Node* node);

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

  UInt _Retry, _Limit, _Online;
  Node *_Head, *_Last;
  ULong _Size, _Count;
  Mutex* _Lock;
};
} // namespace Base
#endif
#endif  // BASE_TYPE_H_ 
