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
  struct MappingV1 {
    Int KType, VType;
    UInt Size;
    Void *Keys, *Values;
    Bool *Levels;
    IndexT *Roots, *Indexes;
  };

  struct MappingV2 {
    Int KType, VType;
    UInt Size;
    Void *Keys, *Values;
    Bool *Levels;
    IndexT *Roots, *Left, *Right;
  };

  explicit Hashtable(Function<IndexT(KeyT*)> hashing, 
                     Bool use_bintree = False):
      _Hash{hashing}, _Style{use_bintree}, _Bitwise{True}, _Count{0} {
    memset(&_Map, 0, sizeof(_Map));

    if (_Style) {
      _Map.v2.Left = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
      _Map.v2.Right = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
      _Map.v2.Roots = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
      _Map.v2.Levels = (Bool*) ABI::Malloc(sizeof(Bool)*2);
      _Map.v2.Size = 2;
    } else {
      _Map.v1.Levels = (Bool*) ABI::Malloc(sizeof(Bool)*2);
      _Map.v1.Roots = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
      _Map.v1.Indexes = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
      _Map.v1.Size = 2;
    }
    Clear();
  }

  explicit Hashtable(IndexT(*hashing)(KeyT*), Bool use_bintree = False):
      _Hash{hashing}, _Style{use_bintree}, _Bitwise{True}, _Count{0} {
    memset(&_Map, 0, sizeof(_Map));

    if (_Style) {
      _Map.v2.Left = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
      _Map.v2.Right = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
      _Map.v2.Roots = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
      _Map.v2.Levels = (Bool*) ABI::Malloc(sizeof(Bool)*2);
      _Map.v2.Size = 2;
    } else {
      _Map.v1.Levels = (Bool*) ABI::Malloc(sizeof(Bool)*2);
      _Map.v1.Roots = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
      _Map.v1.Indexes = (IndexT*) ABI::Malloc(sizeof(IndexT)*2);
      _Map.v1.Size = 2;
    }
    Clear();
  }

  explicit Hashtable(UInt size, IndexT(*hashing)(KeyT*),
                     Bool use_bintree = False):
      _Hash{hashing}, _Style{use_bintree}, _Count{0} {
    auto n = log(size)/log(2);

    _Bitwise = (n == UInt(n));
    memset(&_Map, 0, sizeof(_Map));

    if (_Style) {
      _Map.v2.Left = (IndexT*) ABI::Malloc(sizeof(IndexT)*size);
      _Map.v2.Right = (IndexT*) ABI::Malloc(sizeof(IndexT)*size);
      _Map.v2.Roots = (IndexT*) ABI::Malloc(sizeof(IndexT)*size);
      _Map.v2.Levels = (Bool*) ABI::Malloc(sizeof(Bool)*size);
      _Map.v2.Size = size;
    } else {
      _Map.v1.Levels = (Bool*) ABI::Malloc(sizeof(Bool)*size);
      _Map.v1.Roots = (IndexT*) ABI::Malloc(sizeof(IndexT)*size);
      _Map.v1.Indexes = (IndexT*) ABI::Malloc(sizeof(IndexT)*size);
      _Map.v1.Size = size;
    }
    Clear();
  }

  virtual ~Hashtable() { Clear(True); }

  /* @NOTE: put key-value to our Hashtable */
  inline ErrorCodeE Put(KeyT&& key, ValueT&& value) {
    KeyT* keys = (KeyT*)(_Style? _Map.v2.Keys: _Map.v1.Keys);
    Bool* levels = _Style? _Map.v2.Levels: _Map.v1.Levels;
    IndexT* roots = _Style? _Map.v2.Roots: _Map.v1.Roots, *indexes = None;
    IndexT hashing = _Hash(&key), mask = 0, next = 0;
    UInt size = _Style? _Map.v2.Size: _Map.v1.Size;
    UInt index = Mod(hashing);

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

      indexes = _Map.v1.Indexes;
      mask = IndexAt(indexes, index);
      next = index;

      if (IndexAt(indexes, index) >= 0) {
        if (KeyAt(keys, index) == key) {
          goto insert_keyval;
        }

        /* @NOTE: maybe we will found the key if we keep checking indexes 
         * so we only change the key's value */

        for (auto i = 0; latest < Cast(latest, size); ++i) {
          if (i > Cast(i, size)) {
            Bug(EBadLogic, "It seem we are in an infinitive loop");
          } else if (KeyAt(keys, indexes[latest]) == key) {
            index = latest;
            goto insert_keyval;
          } else if (Cast(size, indexes[latest]) != size) {
            latest = indexes[latest];
            continue;
          }

          index = latest;
          break;
        }

       /* @NOTE: not found the key, going to find a new slot to insert this
        * key-value */

find_new_slot:
        if ((next = FindEmpty(index)) < 0) {
          return OutOfRange.code();
        } else {
          mask = size;
        }

        /* @NOTE: make the current index to point to the end of Hashtable, with
         * that we can find the key-value better */

        if (UInt(next) != index) {
          IndexAt(indexes, index) = next;
          index = next;
        }
      } else if (KeyAt(keys, index) == key) {
        goto insert_keyval;
      } else {
        goto find_new_slot;
      }
    }

    /* @NOTE: save level and root values of each item to use on migrating
     * pharse */

    LevelAt(levels, index) = Bool(1 & (hashing / size));
    RootAt(roots, index) = hashing%size;

    /* @NOTE: increase couter */
    _Count++;

insert_keyval:
    if (Insert(key, value, index)) {
      IndexAt(indexes, index) = mask;
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
    return Put(std::get<0>(instance), std::get<1>(instance));
  }

  /* @NOTE: get value from existence key */
  template<typename ResultT=ValueT>
  inline ResultT Get(KeyT& key) {
    return Get<ResultT>(std::move(key));
  }

  template<typename ResultT=ValueT>
  inline ResultT Get(KeyT&& key) {
    ResultT output;
    Error error{Get(RValue(key), output)};

    if (error) {
      throw Exception(error);
    }

    return output;
  }

  template<typename ResultT=ValueT>
  inline Error Get(KeyT&& key, ResultT& output) {
    Error error{};
    ValueT* values{None};
    Void* pointer{None};
    KeyT* keys{None};
    UInt size = _Style? _Map.v2.Size: _Map.v1.Size, select{0};
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
      return NoSupport << Nametype<ResultT>();
    }

    /* @NOTE: select value base on key value */
    if (_Style) {
      return (NoSupport << "Hashtable with binary tree is still on developing");
    } else {
      IndexT* indexes = _Map.v1.Indexes;
      size = _Map.v1.Size;
      keys = (KeyT*)_Map.v1.Keys;
      values = (ValueT*)_Map.v1.Values;

      if (size == 0) {
        error = DoNothing << "Hashtable is empty recently";
      } else if (!keys) {
        error = BadLogic << "Hastable::Keys is empty recently";
      } else if (!values) {
        error = BadLogic << "Hastable::Values is empty recently";
      } else {
        for (UInt index = Mod(hashing), i = 0; index < size;
                  index = IndexAt(indexes, index), ++i) {
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
      if (error){
        return error;
      } else {
        return NotFound << Format{"key with hash {}"}.Apply(hashing);
      }
    }

    /* @NOTE: return according result */
    switch(select) {
    case 2:
    {
      ResultT result{};

      if (ToPointer(result, pointer)) {
        output = result;
      } else {
        return BadLogic << Format{"is {} pointer?"}.Apply(Nametype<ResultT>());
      }
    }
    break;

    case 1:
    case 3:
    case 4:
      output = *((ResultT*)(pointer));
      break;

    default: /* @NOTE: we shouldn't reach to here */
      return BadLogic << "bug from compiler, please report it";
    }

    return error;
  }

  /* @NOTE: clear everything with this method */
  virtual void Clear(Bool all = False) {
    if (_Style) {
      if (all) {
        if (_Map.v2.Left) free(_Map.v2.Left);
        if (_Map.v2.Right) free(_Map.v2.Right);
        if (_Map.v2.Levels) free(_Map.v2.Levels);
        if (_Map.v2.Roots) free(_Map.v2.Roots);
      } else {
        if (_Map.v2.Levels) {
          memset(_Map.v2.Levels, 0, _Map.v2.Size*sizeof(Bool));
        }
        if (_Map.v2.Roots) {
          memset(_Map.v2.Roots, -1, _Map.v2.Size*sizeof(IndexT));
        }
        if (_Map.v2.Left) {
          memset(_Map.v2.Left, -1, _Map.v2.Size*sizeof(IndexT));
        }
        if (_Map.v2.Right) {
          memset(_Map.v2.Right, -1, _Map.v2.Size*sizeof(IndexT));
        }
      }

      if (_Map.v2.Keys) free(_Map.v2.Keys);
      if (_Map.v2.Values) free(_Map.v2.Values);

      if (!all) {
        _Map.v2.Keys = ABI::Malloc(sizeof(KeyT)*_Map.v2.Size);
        _Map.v2.Values = ABI::Malloc(sizeof(ValueT)*_Map.v2.Size);
      }
    } else {
      if (all) {
        if (_Map.v1.Indexes) free(_Map.v1.Indexes);
        if (_Map.v1.Levels) free(_Map.v1.Levels);
        if (_Map.v1.Roots) free(_Map.v1.Roots);
      } else{
        if (_Map.v1.Indexes) {
          memset(_Map.v1.Indexes, -1, _Map.v1.Size*sizeof(IndexT));
        }
        if (_Map.v1.Levels) {
          memset(_Map.v1.Levels, 0, _Map.v1.Size*sizeof(Bool));
        }
        if (_Map.v1.Roots) {
          memset(_Map.v1.Roots, 0, _Map.v1.Size*sizeof(IndexT));
        }
      }

      if (_Map.v1.Keys) free(_Map.v1.Keys);
      if (_Map.v1.Values) free(_Map.v1.Values);

      if (!all) {
        _Map.v1.Keys = ABI::Malloc(sizeof(KeyT)*_Map.v1.Size);
        _Map.v1.Values = ABI::Malloc(sizeof(ValueT)*_Map.v1.Size);
      }
    }

    if (all) memset(&_Map, 0, sizeof(_Map));
  }

 protected:
  inline virtual KeyT& KeyAt(KeyT* array, UInt index) { return array[index]; }

  inline virtual ValueT& ValueAt(ValueT* array, UInt index) { return array[index]; }

  inline virtual IndexT& IndexAt(IndexT* array, UInt index) { return array[index]; }

  inline virtual IndexT& RootAt(IndexT* array, UInt index) { return array[index]; }

  inline virtual Bool& LevelAt(Bool* array, UInt index) { return array[index]; }

  inline virtual IndexT& LeftAt(IndexT* array, UInt index) { return array[index]; }

  inline virtual IndexT& RightAt(IndexT* array, UInt index) { return array[index]; }

  inline virtual ErrorCodeE Expand() {
    UInt size = _Style? _Map.v2.Size: _Map.v1.Size;
    Bool* levels = _Style? _Map.v2.Levels: _Map.v1.Levels;
    IndexT* roots = _Style? _Map.v2.Roots: _Map.v1.Roots;
    KeyT* keys = (KeyT*)(_Style? _Map.v2.Keys: _Map.v1.Keys);
    ValueT* values = (ValueT*)(_Style? _Map.v2.Values: _Map.v1.Values);
    ErrorCodeE error = ENoError;

    /* @NOTE: realocate variable keys */
    if (!(keys = (KeyT*)ABI::Realloc(keys, 2*size*sizeof(KeyT)))) {
      error = DrainMem(Format{"ABI::Realloc() keys with {}"
                              "bytes"}.Apply(2*size*sizeof(KeyT))).code();
    } else if (_Style) {
      _Map.v2.Keys = keys;
    } else {
      _Map.v1.Keys = keys;
    }

    /* @NOTE: realocate variable values */
    if (error || !(values = (ValueT*)ABI::Realloc(values, 2*size*sizeof(ValueT)))) {
      if (!error) {
        error = DrainMem(Format{"ABI::Realloc() values with {} "
                                "bytes"}.Apply(2*size*sizeof(ValueT))).code();
      }
    } else if (_Style) {
      _Map.v2.Values = values;
    } else {
      _Map.v1.Values = values;
    }

    /* @NOTE: realocate variable levels */
    if (error || !(levels = (Bool*)ABI::Realloc(levels, 2*size*sizeof(Bool)))) {
      if (!error) {
        error = DrainMem(Format{"ABI::Realloc() levels with {} "
                                "bytes"}.Apply(2*size*sizeof(Bool))).code();
      }
    } else if (_Style) {
      _Map.v2.Levels = levels;
    } else {
      _Map.v1.Levels = levels;
    }

    /* @NOTE: realocate variable roots */
    if (error || !(roots = (IndexT*)ABI::Realloc(roots, 2*size*sizeof(IndexT)))) {
      if (!error) {
        error = DrainMem(Format{"ABI::Realloc() roots with {} "
                                "bytes"}.Apply(2*size*sizeof(IndexT))).code();
      }
    } else if (_Style) {
      _Map.v2.Roots = roots;
    } else {
      _Map.v1.Roots = roots;
    }

    if (_Style) {
      return ENoSupport;
    } else {
      IndexT* indexes = _Map.v1.Indexes;

      /* @NOTE: realocate variable indexes */
      if (error || !(indexes = (IndexT*)ABI::Realloc(indexes, 2*size*sizeof(IndexT)))) {
        if (!error) {
          error = DrainMem(Format{"ABI::Realloc() indexes with {} "
                                  "bytes"}.Apply(2*size*sizeof(IndexT))).code();
        }
      } else {
        memset(&indexes[size], -1, size*sizeof(IndexT));
        _Map.v1.Indexes = indexes;
      }
    }

    return ENoError;
  }

  /* @NOTE: access value according index, type must be KeyT or ValueT */
  template<typename ResultT>
  inline ResultT* At(UInt index) {
    if (typeid(ResultT) == typeid(ValueT)) {
      if (_Style) {
        return (ResultT*)(&ValueAt((ValueT*)_Map.v2.Values, index));
      } else {
        return (ResultT*)(&ValueAt((ValueT*)_Map.v1.Values, index));
      }
    } else if (typeid(ResultT) != typeid(KeyT)) {
      if (_Style) {
        return (ResultT*)(&KeyAt((KeyT*)_Map.v2.Keys, index));
      } else {
        return (ResultT*)(&KeyAt((KeyT*)_Map.v1.Keys, index));
      }
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
      if (LeftAt(_Map.v2.Left, index) >= 0 ||
          RightAt(_Map.v2.Right, index) >= 0) {
        return False;
      }

      KeyAt((KeyT*)_Map.v2.Keys, index) = key;
      ValueAt((ValueT*)_Map.v2.Values, index) = value;
    } else {
      if (IndexAt(_Map.v1.Indexes, index) >= 0) {
        return False;
      }

      KeyAt((KeyT*)_Map.v1.Keys, index) = key;
      ValueAt((ValueT*)_Map.v1.Values, index) = value;
    }

    return True;
  }

  /* @NOTE: this helper will support how to move a key-value from place
   *  to place */
  inline Bool Move(UInt from, UInt to) {
    if (_Style) {
      if (!Insert(KeyAt((KeyT*)_Map.v2.Keys, from),
                  ValueAt((ValueT*)_Map.v2.Values, from),
                  to)) {
        return False;
      }

      LeftAt(_Map.v2.Left, from) = -1;
      RightAt(_Map.v2.Right, from) = -1;
    } else {
      if (!Insert(KeyAt((KeyT*)_Map.v1.Keys, from),
                  ValueAt((ValueT*)_Map.v1.Values, from),
                  to)) {
        return False;
      }

      IndexAt(_Map.v1.Indexes, from) = -1;
    }

    return True;
  }

  /* @NOTE: this helper will support how to deprecate a slot */
  inline ErrorCodeE Deprecate(Int index, Int size){
    if (_Style) {
      LeftAt(_Map.v2.Left, index) = -1;
      RightAt(_Map.v2.Right, index) = -1;
    } else {
      auto root = RootAt(_Map.v1.Roots, index);
      auto indexes = _Map.v1.Indexes;

      if (index == root) {
        /* @NOTE: root was going to be migrated so we will select the next
         *  candidate here */

        for (auto next = IndexAt(indexes, index);
             IndexAt(indexes, next) < size;
             ++next) {
          if (RootAt(_Map.v1.Roots, index) == root) {
            /* @NOTE: found the candidate move it now */

            if (!Move(next, index)) {
              return BadLogic("broken link").code();
            }
            break;
          }
        }
      } else if (IndexAt(indexes, index) == size) {
        /* @NOTE: this node is on the end of a flow so we will update index
         *  of the previous node to the end of table */

        IndexAt(indexes, GoToEnd(index, size, 1)) = _Map.v1.Size;
      } else {
        /* @NOTE: this node is stacked inside 2 node of a flow so we will
         *  update index of the previous node to point to the next*/

        IndexAt(indexes, GoToEnd(index, size, 1)) = IndexAt(indexes, index);
      }

      /* @NOTE: remove the old link since we have done everything */
      IndexAt(_Map.v1.Indexes, index) = -1;
    }

    return ENoError;
  }

  /* @NOTE: Hashtable has reached its limitation and we must expand itself and
   * migrate data to a larger content */
  inline ErrorCodeE Migrate() {
    ErrorCodeE error = Expand();
    UInt size = _Style? _Map.v2.Size: _Map.v1.Size;
    Bool* levels = _Style? _Map.v2.Levels: _Map.v1.Levels;
    IndexT* roots = _Style? _Map.v2.Roots: _Map.v1.Roots;
    KeyT* keys = (KeyT*)(_Style? _Map.v2.Keys: _Map.v1.Keys);
    ValueT* values = (ValueT*)(_Style? _Map.v2.Values: _Map.v1.Values);

    if (error) {
      return error;
    }

    /* @NOTE: initialize value to expanded memory */
    memset(&LevelAt(levels, size), 0, size*sizeof(Bool));
    memset(&RootAt(roots, size), 0, size*sizeof(IndexT));

    /* @NOTE: update the mapping table back to _Map */
    if (_Style) {
      _Map.v2.Levels = levels;
      _Map.v2.Roots = roots;
      _Map.v2.Keys = keys;
      _Map.v2.Values = values;
    } else {
      _Map.v1.Levels = levels;
      _Map.v1.Roots = roots;
      _Map.v1.Keys = keys;
      _Map.v1.Values = values;
    }

    if (error) {
      return error;
    } else if (_Style) {
      return ENoSupport;
    } else {
      /* @NOTE: update the size first */
      _Map.v1.Size = 2*size;

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
      IndexT* indexes = _Map.v1.Indexes;

      for (IndexT next = IndexAt(indexes, index), count = 0; 
                         IndexAt(indexes, next) < size;
                ++count, next = IndexAt(indexes, next)) {
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
    if (_Style) {
    } else {
      auto indexes = _Map.v1.Indexes;

      for (auto left = index, right = index; ; left--, right++) {
        if (left >= 0) {
          if (IndexAt(indexes, left) < 0) {
            return left;
          }
        }

        if (right < IndexT(_Map.v1.Size)) {
          if (IndexAt(indexes, right) < 0) {
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
    UInt size = _Style? _Map.v2.Size: _Map.v1.Size;

    if (_Bitwise) {
      return hashing & (size - 1);
    } else {
      return hashing % size;
    }
  }

  union { MappingV1 v1; MappingV2 v2; } _Map;
  Function<IndexT(KeyT*)> _Hash;
  Bool _Style, _Bitwise;
  UInt _Count;
};
}  // namespace Base
#endif  // BASE_HASHTABLE_H_

/* @TODO: class Base::Hashtable
 * - It seems that Hashtable slower than the previous version, check it
 * - How to apply Hashtable to Base::Table
 */
