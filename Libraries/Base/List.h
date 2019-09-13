#include <Type.h>
#include <Utils.h>

namespace Base {
namespace Debug {
void DumpWatch(String parameter);
} // namespace Debug

class List {
 private:
  enum ActionE { ERead = 0, ELModify, ERModify, ERemove, EEnd };
  using Barrier = Pair<ULong*, Mutex>;

 public:
  List(Int parallel = 0);
  ~List();

  template<typename T> 
  T* At(ULong index) {
    Item* item = Access(ERead, index, True);
    T* result = (T*) (item? item->_Ptr : None);

    if (Unlock(item)) {
      return result;
    } else {
      return None;
    }
  }

  ULong Size();
  ULong Add(Void* ptr);
  Bool  Del(ULong index, Void* ptr);

 private:
  struct Item {
    Void* _Ptr;
    ULong _Index;
    Item *_Next, *_Prev;

    Item(): _Ptr{None}, _Index{0}, _Next{None}, _Prev{None} {}
  };

#if DEBUGING
  friend void Base::Debug::DumpWatch(String parameter);
#endif

  ULong Add(Item* prev, Void* ptr);
  Item* Access(ActionE action, Item* node);
  Item* Access(ActionE action, ULong index, Bool counting = False);
  Bool Unlock(Item* item);
  Bool Unlock(ULong ticket);

  Bool _Global, _Sweep;
  Item *_Head, *_Curr;
  Barrier* _Barriers;
  ULong _Size[2], _Parallel, _Count;
};
} // namespace Base
