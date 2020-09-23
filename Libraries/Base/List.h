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
  /* @NOTE:
   * - Node only have 2 state True/False which dedicates the node is dead or
   *   alive.
   * - Acquire can pass through if we raise this would be a light-weight
   *   acquiring. This feature is designed to make the nodes don't die with a
   *   litter risk of raising somewhere, hope CPU's atomic works as it has
   *   been
   *
   *     <Node> -------> <Node> -------> <Node> -------> <Node>
   *       ^               ^               ^
   *       |               |               |
   *   you're here   This node is    You're destination
   *                   detached
   *
   *   In the past, if the node is detached, even if we still see it onboard,
   *   method Access can't reach to your destination. We would see several
   *   loop
   *   happen until the detached node is completedly remove. When we're facing
   *   with flooding threads, the kernel might stuck at each threads for very
   *   long time. What would we see here some threads might occupy a node and
   *   stuck at detaching it for very long time causing another faster threads
   *   to be hang and slowdown the whole process.
   * ______________________________________________________________________ */

public:
#if !NDEBUG
  explicit List(Int retry = -1, Bool dumpable = False);
#else
  explicit List(Int retry = -1);
#endif
  virtual ~List();

  inline void Dump() {
    Node *node = _Head[0]->Prev[0];
    ULong t = _Size[1];

    while (node && (--t > 0)) {
      if (node == _Head[0])
        printf("%p -> (%lu : [%p:%p]) [head]\n", node, node->Index,
               node->Prev[0], node->Next);
      else if (node == _Last)
        printf("%p -> (%lu : [%p:%p]) [last]\n", node, node->Index,
               node->Prev[0], node->Next);
      else
        printf("%p -> (%lu : [%p:%p) \n", node, node->Index, node->Prev[0],
               node->Next);

      node = node->Prev[0];
    }
  }

  /* @NOTE: this method is used to get the pointer of a node, base on index or
   * its position number */
  template <typename T> T *At(ULong number, Bool is_index = False) {
    T *result = None;
    Long nsec = 1;

    for (auto retry = _Retry; retry > 0; --retry) {
      Node *node = None;

      if ((node = Access(number, is_index))) {
        result = (T *)node->Ptr;

        break;
      }

      Idle((nsec = ((nsec * 2) % ULong(1e9))));
    }

    return result;
  }

  /* @NOTE: these methods are used to detect if a node existes or not */
  Int Exist(ULong key);
  Int Exist(Void *pointer);

  /* @NOTE: this method is used to get an id from a provided pointer */
  ErrorCodeE IndexOf(Void *pointer, ULong *result);

  /* @NOTE: these methods are used to interact directly with our list to add or
   * delete pointers from it */
  ErrorCodeE Add(ULong index, Void *pointer, Int retry = -1);
  Long Add(Void *pointer, Int retry = -1);
  Bool Del(ULong index, Void *pointer, ErrorCodeE *code = None,
           Bool failable = False);

  /* @NOTE: this method is used to get List's size */
  ULong Size(Bool type = False);

protected:
  struct Node {
    /* @NOTE: this is how our node is defined so with it, we can build a list to
     * work on parallel */

    Void *Ptr;
    Node *Next, *Prev[2], **PNext, **PPrev, **PHead, **PLast;
    ULong Index;
    Bool State;

    explicit Node(Void *pointer, Node **phead, Node **plast) {
      Ptr = pointer;
      Next = None;
      Prev[0] = None;
      Prev[1] = None;
      PPrev = None;
      PNext = None;
      PHead = phead;
      PLast = plast;
      State = True;
      Index = 0;
    }

    void Reset(Void *pointer = None, Bool state = True) {
      Ptr = pointer;
      Next = None;
      Prev[0] = None;
      Prev[1] = None;
      PPrev = None;
      PNext = None;
      State = state;
      Index = 0;
    }
  };

  /* @NOTE: this method is used to attach a node into our list, the node could
   * be defined some-where else, waiting to be added into our list */
  Long Attach(Node *node, ErrorCodeE *error = None);

  /* @NOTE: this method is used to detach a node out from our list so we could
   * modify it freely. To make sure that the node is within our list, the index
   * should be provided */
  Node *Detach(ULong index);

  /* @NOTE: these methods are used to access a node to grand an action to it
   * so we could control the behavior of each node on parallel */
  Node *Access(Void *pointer, ErrorCodeE *code = None);
  Node *Access(ULong &number, Bool is_index = False, ErrorCodeE *code = None);

  /* @NOTE: this method is used to allocate a new node. By default, it should
   * use allocated nodes which are stored inside preserved queue */
  Node *Allocate(Void *pointer, Bool immutable = False);

private:
  static void Idle(Long nanosec);

  /* @NOTE: this method is used to detach a node out from our list so we could
   * modify it freely. To make sure that the node is within our list, the index
   * should be provided */
  Node *Detach(Node *node);

  /* @NOTE: this function is used only when we need to debug watch-stopper
   * issues and we need to list everything inside the list */
  friend void Base::Debug::DumpWatch(String parameter);

#if !NDEBUG
  Bool _Dumpable;
#endif

  UInt _Retry;
  Node *_Head[2], *_Last;
  ULong _Size[2], _Count;
};
} // namespace Base
#endif
#endif // BASE_TYPE_H_
