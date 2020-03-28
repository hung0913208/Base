#ifndef BASE_HASHTABLE_H_
#define BASE_HASHTABLE_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Exception.h>
#include <Base/Logcat.h>
#include <Base/Type.h>
#include <Base/Utils.h>
#else
#include <Exception.h>
#include <Logcat.h>
#include <Type.h>
#include <Utils.h>
#endif

/* @NOTE: this header will be used to calculate log */
#include <math.h>

namespace Base {
template<typename KeyT, typename ValueT, typename IndexT=Int>
class Hashtable {
 protected:
  /* @NOTH: this tricky way will help to build a manual Hashtable with specific
   * memory addresses */
  Hashtable(){}

 public:
  struct Mapping {
    Int KType, VType;
    UInt Size;
    Void *Keys, *Values;
    Bool *Levels;
    IndexT *Roots, *Lefts, *Rights;
  };

  explicit Hashtable(Function<IndexT(KeyT*)> hashing, 
                     Bool use_bintree = False):
      _Hash{hashing}, _Style{use_bintree}, _Bitwise{True}, _Count{0} {
    memset(&_Map, 0, sizeof(_Map));

    _Map.Lefts = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
    _Map.Rights = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
    _Map.Roots = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
    _Map.Levels = (Bool*) ABI::Malloc(sizeof(Bool)*2);
    _Map.Size = 2;
    Clear();
  }

  explicit Hashtable(IndexT(*hashing)(KeyT*), Bool use_bintree = False):
      _Hash{hashing}, _Style{use_bintree}, _Bitwise{True}, _Count{0} {
    memset(&_Map, 0, sizeof(_Map));

    _Map.Lefts = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
    _Map.Rights = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
    _Map.Roots = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
    _Map.Levels = (Bool*) ABI::Malloc(sizeof(Bool)*2);
    _Map.Size = 2;
    Clear();
  }

  explicit Hashtable(UInt size, IndexT(*hashing)(KeyT*),
                     Bool use_bintree = False):
      _Hash{hashing}, _Style{use_bintree}, _Count{0} {
    auto n = log(size)/log(2);

    _Bitwise = (n == UInt(n));
    memset(&_Map, 0, sizeof(_Map));

    _Map.Lefts = (IndexT*) ABI::Malloc(sizeof(IndexT)*size);
    _Map.Rights = (IndexT*) ABI::Malloc(sizeof(IndexT)*size);
    _Map.Roots = (IndexT*) ABI::Malloc(sizeof(IndexT)*size);
    _Map.Levels = (Bool*) ABI::Malloc(sizeof(Bool)*size);
    _Map.Size = size;
    Clear();
  }

  virtual ~Hashtable() { Clear(True); }

  /* @NOTE: put key-value to our Hashtable */
  inline ErrorCodeE Put(KeyT&& key, ValueT&& value) {
    UInt size = _Map.Size;
    KeyT* keys = (KeyT*)_Map.Keys;
    Bool* levels = _Map.Levels;
    IndexT* roots = _Map.Roots;
    IndexT* lefts = _Map.Lefts;
    IndexT* rights = _Map.Rights;
    IndexT hashing = _Hash(&key), mask = size;
    IndexT index = Mod(hashing), next = 0;

    /* @NOTE: check if we need migration or not */
    if (_Count >= size) {
      if (levels) {
        auto error = Migrate();

        if (error) {
          return error;
        } else {
          return Put(RValue(key), RValue(value));
        }
      } else {
        return OutOfRange.code();
      }
    }

    /* @NOTE: analyze table and pick the approviated index */
    if (_Style) {
      return NoSupport("still on developing").code();
    } else {
      IndexT latest = index;
      IndexT root = index;

      next = index;

      if ((mask = RightAt(rights, index)) >= 0) {
        if (KeyAt(keys, index) == key) {
          goto update_root_and_level;
        } else if (RootAt(roots, index) != index) {
          /* @NOTE: it seems another key-val are using this slot and this key
           * doesn'have same hash-key (it's not within the same root with the
           * key-val we would like to add */
        
          auto prev = LeftAt(lefts, index);
          auto slot = FindEmpty(index);
          auto next = RightAt(rights, index);

          if (slot < 0) {
            return OutOfRange(Format{"size = {}"}.Apply(size)).code();
          } else if (!Move(index, slot)) {
            Bug(EBadLogic, Format{"can\'t move {} to {}"}.Apply(index, slot));
          } else {
            if (prev >= 0) {
              RightAt(rights, prev) = slot;
            }

            if (slot >= 0) {
              LeftAt(lefts, slot) = prev;

              if (next >= 0 && next < Cast(next, size)) {
                RightAt(rights, slot) = next;
                LeftAt(lefts, next) = slot;
              } else {
                RightAt(rights, slot) = size;
              }
            }

            LevelAt(levels, slot) = LevelAt(levels, index);
            RootAt(roots, slot) = RootAt(roots, index);

            mask = size;
            goto update_root_and_level;
          }
        }

        /* @NOTE: maybe we will found the key if we keep checking indexes 
         * so we only change the key's value */

        for (auto i = 0; 0 <= latest && latest < Cast(latest, size); ++i) {
          auto r = RightAt(rights, latest);

          if (i > Cast(i, size)) {
            Bug(EBadLogic, "It seem we are in an infinitive loop");
          } else if (size > Cast(size, r) && r >= 0 && KeyAt(keys, r) == key) {
            index = latest;
            goto update_root_and_level;
          } else if (0 <= r && Cast(size, r) < size) {
            latest = r;
            continue;
          }

          index = latest;
          break;
        }

        /* @NOTE: not found the key, going to find a new slot to insert this
         * key-value */

find_new_slot:
        if ((next = FindEmpty(index)) < 0) {
          return OutOfRange(Format{"size = {}"}.Apply(size)).code();
        } else {
          mask = size;
        }

        /* @NOTE: make the current index to point to the end of Hashtable,
         * with that we can find the key-value better */

        if (UInt(next) != UInt(index)) {
          RightAt(rights, latest) = next;
          LeftAt(lefts, next) = latest;
        }

        index = next;
      } else if (KeyAt(keys, index) == key) {
        goto update_root_and_level;
      } else {
        goto find_new_slot;
      }
    } 

    /* @NOTE: save level and root values of each item to use on migrating
     * pharse */

update_root_and_level:
    LevelAt(levels, index) = Bool(1 & (hashing / size));
    RootAt(roots, index) = hashing%size;

    /* @NOTE: increase couter */
    _Count++;

    if (mask < 0) {
      mask = size;
    }

    if (Insert(key, value, index)) {
      RightAt(rights, index) = mask;
    } else {
      return EBadAccess;
    }

    return ENoError;
  }

  /* @NOTE: put a pair of Key-Value to Hashtable */
  inline ErrorCodeE Put(KeyT& key, ValueT& value) {
    return Put(std::move(key), std::move(value));
  }

  inline ErrorCodeE Put(Pair<KeyT, ValueT> instance) {
    return Put(instance.Left, instance.Right);
  }

  /* @NOTE: get value from existence key */
  template<typename ResultT=ValueT>
  inline ResultT Get(KeyT& key) {
    return Get<ResultT>(std::move(key));
  }

  template<typename ResultT=ValueT>
  inline ResultT Get(KeyT&& key) {
    ResultT output;
    Error error{};

    if (Get(RValue(key), output, error)) {
      return output;
    } else {
      throw Exception(error);
    }
  }

  template<typename ResultT=ValueT>
  inline Error Get(KeyT&& key, ResultT& output) {
    Error error{};
    
    Get(RValue(key), output, error);
    return error;
  }
  template<typename ResultT=ValueT>
  inline Bool Get(KeyT&& key, ResultT& output, Error& error) {
    ValueT* values{None};
    Void* pointer{None};
    KeyT* keys{None};
    UInt size = _Map.Size, select{0};
    IndexT hashing{_Hash(&key)};

    /* @NOTE: check result type */
    if (typeid(ResultT) == typeid(ValueT)) {
      select = 1;
    } else if (typeid(ResultT) == typeid(ValueT*)) {
      select = 2;
    } else if (typeid(ResultT) == typeid(ValueT&)) {
      select = 3;
    } else if (typeid(ResultT) == typeid(ValueT&&)) {
      select = 4;
    } else {
      error = NoSupport(Nametype<ResultT>());
      return False;
    }

    /* @NOTE: select value base on key value */
    if (_Style) {
      error = NoSupport("Hashtable with binary tree is still "
                        "on developing");
      return False;
    } else {
      keys = (KeyT*)_Map.Keys;
      values = (ValueT*)_Map.Values;

      if (size == 0) {
        error = DoNothing("Hashtable is empty recently");
      } else if (!keys) {
        error = BadLogic("Hastable::Keys is empty recently");
      } else if (!values) {
        error = BadLogic("Hastable::Values is empty recently");
      } else {
        for (UInt index = Mod(hashing), i = 0; index < size;
                  index = RightAt(_Map.Rights, index), ++i) {
          if (i > Cast(i, size)) {
            Bug(EBadLogic, "It seem we are in an infinitive loop");
          } else if (KeyAt(keys, index) != key) {
            continue;
          }

          pointer = (Void*)(&ValueAt(values, index));
          break;
        }
      }
    }

    /* @NOTE: verify the exception */
    if (!pointer && select) {
      if (!error){
        error = NotFound(Format{"key with hash {}"}.Apply(hashing));
      }
      return False;
    }

    /* @NOTE: return according result */
    switch(select) {
    case 2:
    {
      ResultT result{};

      if (ToPointer(result, pointer)) {
        output = result;
      } else {
        error = BadLogic(Format{"is {} pointer?"}.Apply(Nametype<ResultT>()));
        return False;
      }
    }
    break;

    case 1:
    case 3:
    case 4:
      output = *((ResultT*)(pointer));
      break;

    default: /* @NOTE: we shouldn't reach to here */
      error = BadLogic << "bug from compiler, please report it";
    }

    return True;
  }

  /* @NOTE: clear everything with this method */
  virtual void Clear(Bool all = False) {
    if (all) {
      if (_Map.Lefts) free(_Map.Lefts);
      if (_Map.Rights) free(_Map.Rights);
      if (_Map.Levels) free(_Map.Levels);
      if (_Map.Roots) free(_Map.Roots);
    } else {
      if (_Map.Levels) {
        memset(_Map.Levels, 0, _Map.Size*sizeof(Bool));
      }

      if (_Map.Roots) {
        memset(_Map.Roots, -1, _Map.Size*sizeof(IndexT));
      }

      if (_Map.Lefts) {
        memset(_Map.Lefts, -1, _Map.Size*sizeof(IndexT));
      }

      if (_Map.Rights) {
        memset(_Map.Rights, -1, _Map.Size*sizeof(IndexT));
      }
    }

    if (_Map.Keys) free(_Map.Keys);
    if (_Map.Values) free(_Map.Values);

    if (!all) {
      _Map.Keys = ABI::Malloc(sizeof(KeyT)*_Map.Size);
      _Map.Values = ABI::Malloc(sizeof(ValueT)*_Map.Size);

      if (_Map.Keys) {
        memset(_Map.Keys, 0, sizeof(KeyT)*_Map.Size);
      }

      if (_Map.Values) {
        memset(_Map.Values, 0, sizeof(ValueT)*_Map.Size);
      }
    }

    if (all) memset(&_Map, 0, sizeof(_Map));
  }

 protected:
  inline virtual KeyT& KeyAt(KeyT* array, UInt index) { return array[index]; }

  inline virtual ValueT& ValueAt(ValueT* array, UInt index) { return array[index]; }

  inline virtual IndexT& RootAt(IndexT* array, UInt index) { return array[index]; }

  inline virtual Bool& LevelAt(Bool* array, UInt index) { return array[index]; }

  inline virtual IndexT& LeftAt(IndexT* array, UInt index) { return array[index]; }

  inline virtual IndexT& RightAt(IndexT* array, UInt index) { return array[index]; }

  inline virtual ErrorCodeE Expand() {
    UInt size = _Map.Size;
    Bool* levels = _Map.Levels;
    IndexT* roots = _Map.Roots;
    IndexT* lefts = _Map.Lefts;
    IndexT* rights = _Map.Rights;
    KeyT* keys = (KeyT*)_Map.Keys;
    ValueT* values = (ValueT*)_Map.Values;
    ErrorCodeE error = ENoError;

    /* @NOTE: realocate variable keys */
    if (!(keys = (KeyT*)ABI::Realloc(keys, 2*size*sizeof(KeyT)))) {
      error = DrainMem(Format{"ABI::Realloc() `keys` with {} "
                              "bytes"}.Apply(2*size*sizeof(KeyT))).code();
    } else {
      _Map.Keys = keys;
    }

    /* @NOTE: realocate variable values */
    if (error || !(values = (ValueT*)ABI::Realloc(values, 2*size*sizeof(ValueT)))) {
      if (!error) {
        error = DrainMem(Format{"ABI::Realloc() `values` with {} "
                                "bytes"}.Apply(2*size*sizeof(ValueT))).code();
      }
    } else {
      _Map.Values = values;
    }

    /* @NOTE: realocate variable levels */
    if (error || !(levels = (Bool*)ABI::Realloc(levels, 2*size*sizeof(Bool)))) {
      if (!error) {
        error = DrainMem(Format{"ABI::Realloc() `levels` with {} "
                                "bytes"}.Apply(2*size*sizeof(Bool))).code();
      }
    } else {
      _Map.Levels = levels;
    }

    /* @NOTE: realocate variable roots */
    if (error || !(roots = (IndexT*)ABI::Realloc(roots, 2*size*sizeof(IndexT)))) {
      if (!error) {
        error = DrainMem(Format{"ABI::Realloc() `roots` with {} "
                                "bytes"}.Apply(2*size*sizeof(IndexT))).code();
      }
    } else {
      _Map.Roots = roots;
    }

    /* @NOTE: realocate variable rights */
    if (error || !(rights = (IndexT*)ABI::Realloc(rights, 2*size*sizeof(IndexT)))) {
      if (!error) {
        error = DrainMem(Format{"ABI::Realloc() `rights` with {} "
                                "bytes"}.Apply(2*size*sizeof(IndexT))).code();
      }
    } else {
      memset(&rights[size], -1, size*sizeof(IndexT));
      _Map.Rights = rights;
    }

    /* @NOTE: realocate variable lefts */
    if (error || !(lefts = (IndexT*)ABI::Realloc(lefts, 2*size*sizeof(IndexT)))) {
      if (!error) {
        error = DrainMem(Format{"ABI::Realloc() `lefts` with {} "
                                "bytes"}.Apply(2*size*sizeof(IndexT))).code();
      }
    } else {
      memset(&lefts[size], -1, size*sizeof(IndexT));
      _Map.Lefts = lefts;
    }

    return ENoError;
  }

  /* @NOTE: access value according index, type must be KeyT or ValueT */
  template<typename ResultT>
  inline ResultT* At(UInt index) {
    if (typeid(ResultT) == typeid(ValueT)) {
      return (ResultT*)(&ValueAt((ValueT*)_Map.Values, index));
    } else if (typeid(ResultT) != typeid(KeyT)) {
      return (ResultT*)(&KeyAt((KeyT*)_Map.Keys, index));
    } else {
      throw Except(ENoSupport, Nametype<ResultT>());
    }
  }

  /* @NOTE: convert pointer, it would be a tricky way to copy Void pointer to
   * any pointer when we are working on templating functions */
  template<typename Type>
  static Bool ToPointer(Type& result, Void* pointer) {
    auto name = Nametype<Type>();

    if (name[name.size() - 1] == '*') {
      memcpy(&result, (Void*)(&pointer), sizeof(Type));
      return True;
    } else {
      return False;
    }
  }

  /* @NOTE: this helper will support to insert a key-value to slot index-th */
  inline Bool Insert(KeyT& key, ValueT& value, UInt index) {
    if (_Style) {
      if (LeftAt(_Map.Lefts, index) >= 0 ||
          RightAt(_Map.Rights, index) >= 0) {
        return False;
      }
    } else {
      if (RightAt(_Map.Rights, index) >= 0) {
        return False;
      }
    }

    KeyAt((KeyT*)_Map.Keys, index) = key;
    ValueAt((ValueT*)_Map.Values, index) = value;

    return True;
  }

  /* @NOTE: this helper will support how to move a key-value from place
   *  to place */
  inline Bool Move(UInt from, UInt to) {
    auto key = KeyAt((KeyT*)_Map.Keys, from);
    auto val = ValueAt((ValueT*)_Map.Values, from);

    if (_Style) {
    } else {
    }

    if (!Insert(KeyAt((KeyT*)_Map.Keys, from),
                ValueAt((ValueT*)_Map.Values, from),
                to)) {
      return False;
    }

    LeftAt(_Map.Lefts, from) = -1;
    RightAt(_Map.Rights, from) = -1;

    return True;
  }

  /* @NOTE: this helper will support how to deprecate a slot */
  inline ErrorCodeE Deprecate(Int index, Int size){
    if (_Style) {
      LeftAt(_Map.Lefts, index) = -1;
      RightAt(_Map.Rights, index) = -1;
    } else {
      auto root = RootAt(_Map.Roots, index);

      if (index == root) {
        /* @NOTE: root was going to be migrated so we will select the next
         *  candidate here */

        for (auto next = RightAt(_Map.Rights, index);
             RightAt(_Map.Rights, next) < size;
             ++next) {
          if (RootAt(_Map.Roots, index) == root) {
            /* @NOTE: found the candidate move it now */

            if (!Move(next, index)) {
              return BadLogic("broken link").code();
            }
            break;
          }
        }
      } else if (RightAt(_Map.Rights, index) == size) {
        /* @NOTE: this node is on the end of a flow so we will update index
         *  of the previous node to the end of table */

        RightAt(_Map.Rights, GoToEnd(index, size, 1)) = _Map.Size;
      } else {
        /* @NOTE: this node is stacked inside 2 node of a flow so we will
         *  update index of the previous node to point to the next*/

        RightAt(_Map.Rights, GoToEnd(index, size, 1)) = 
          RightAt(_Map.Rights, index);
      }

      /* @NOTE: remove the old link since we have done everything */
      RightAt(_Map.Rights, index) = -1;
    }

    return ENoError;
  }

  /* @NOTE: Hashtable has reached its limitation and we must expand itself and
   * migrate data to a larger content */
  inline ErrorCodeE Migrate() {
    ErrorCodeE error = Expand();
    UInt size = _Map.Size;
    Bool* levels = _Map.Levels;
    IndexT* roots = _Map.Roots;
    KeyT* keys = (KeyT*)_Map.Keys;
    ValueT* values = (ValueT*)_Map.Values;

    if (error) {
      return error;
    }

    /* @NOTE: initialize value to expanded memory */
    memset(&LevelAt(levels, size), 0, size*sizeof(Bool));
    memset(&RootAt(roots, size), 0, size*sizeof(IndexT));

    /* @NOTE: update the mapping table back to _Map */
    _Map.Levels = levels;
    _Map.Roots = roots;
    _Map.Keys = keys;
    _Map.Values = values;

    if (error) {
      return error;
    } else if (_Style) {
      return ENoSupport;
    } else {
      /* @NOTE: update the size first */
      _Map.Size = 2*size;

      /* @NOTE: this for-loop will be used to migrate odd levels to new
       *  places */
      for (UInt i = 0; i < size; ++i) {
        if (LevelAt(levels, i)) {
          Int index = size + RootAt(roots, i);

          if (!Insert(KeyAt(keys, i), ValueAt(values, i), index)) {
            /* @NOTE: find the empty slot since the best place has been
             *  occupied */

            if ((index = FindEmpty(index)) < 0) {
              return BadLogic("migrate can\'t find any place to update").code();
            }

            if (!Insert(KeyAt(keys, i), ValueAt(values, i), index)) {
              return BadLogic("migrate can\'t perform function Insert()").code();
            }
          }

          if (!Deprecate(i, size)) {
            return BadLogic("fail to do Deprecate()").code();
          }

          /* @NOTE: recaculate root and levels */
          RootAt(roots, index) = RootAt(roots, i) + size;
          LevelAt(levels, index) = True;
        }
      }
    }

    return error;
  }

  /* @NOTE: this helper will support to find the end of a flow */
  inline Int GoToEnd(UInt index, Int size, UInt saveback = 0) {
    UInt saver[saveback + 1], pos{0};
    Bool rot{False};

    if (_Style) {
    } else {
      for (IndexT next = RightAt(_Map.Rights, index), count = 0; 
                         RightAt(_Map.Rights, next) < size;
                ++count, next = RightAt(_Map.Rights, next)) {
        if (count > size) {
          Bug(EBadLogic, "fall into an infinitive loop");
        }

        saver[pos] = next;

        /* @NOTE: pos will increase until it reach its limitation and rotate
         *  back */
        if (pos < saveback){
          pos++;
        } else {
          rot = True;
          pos = 0;
        }
      }

      if (rot) {
        return saver[pos == saveback? saveback: pos + 1];
      }
    }

    return -1;
  }

  inline IndexT FindEmpty(IndexT index){
    if (index < 0) {
      Bug(EBadLogic, "index mustn\'t be negative");
    }

    if (_Style) {
    } else {
      for (auto left = index, right = index, count = 0; 
           count < Cast(count, _Map.Size);
           ++count, --left, ++right) {
        if (left >= 0) {
          if (RightAt(_Map.Rights, left) < 0) {
            return left;
          }
        }

        if (right < IndexT(_Map.Size)) {
          if (RightAt(_Map.Rights, right) < 0) {
            return right;
          }
        } else if (left < 0) {
          break;
        }
      }
    }

    return -1;
  }

  inline IndexT Mod(IndexT hashing) {
    UInt size = _Map.Size;

    if (_Bitwise) {
      return hashing & (size - 1);
    } else {
      return hashing % size;
    }
  }

  Function<IndexT(KeyT*)> _Hash;
  Mapping _Map;
  Bool _Style, _Bitwise;
  UInt _Count;
};
}  // namespace Base
#endif  // BASE_HASHTABLE_H_

/* @TODO: class Base::Hashtable
 * - It seems that Hashtable slower than the previous version, check it
 * - How to apply Hashtable to Base::Table
 */
