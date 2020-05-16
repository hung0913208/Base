#ifndef BASE_AUTO_H_
#define BASE_AUTO_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Exception.h>
#include <Base/Logcat.h>
#include <Base/Property.h>
#include <Base/Stream.h>
#include <Base/Utils.h>
#else
#include <Exception.h>
#include <Logcat.h>
#include <Property.h>
#include <Stream.h>
#include <Utils.h>
#endif

#if __cplusplus
extern "C" {
#endif

typedef struct Any {
  /* @NOTE: everything about a variable */
  void *Context, *Type;
  Int* Reference;

  /* @NOTE: callbacks of deleting and cloning this variable */
  void (*Del)(void* value);
  void* (*Clone)(void* value);

#if __cplusplus
  Any(): Context{None}, Type{None}, Reference{None}, Del{None}, Clone{None} {}
#endif
} Any;

/* @NOTE: Any is a flexible type and it can be release at the end of program
 * or assign with None value to any
 */
Any* BSAssign2Any(Any* any, void* context, void* type, void (*del)(void*));

/* @NOTE: a flexible way to assign None to any and remove everything inside
 */
void BSNone2Any(Any* any);
#if __cplusplus
}
#endif

#if __cplusplus
#include <cstring>
#include <typeinfo>

#define CTYPE(value) ((void*)(&typeid(value)))

/* @TODO: consider about threadsafe with this class since we will mainly use
 * this to handle with unknown type from every platform so we must sure
 * everything is okey from the begining
 */
namespace Base {
class Auto {
 public:
  typedef void (*DelD)(void* value);
  typedef void* (*CloneD)(void* value);

  using TypeD = const std::type_info&;
  using RawD = void*;

  /* @NOTE: constructors and destructor */
  Auto(const Any& src);
  Auto(const Auto& src);
  Auto(Auto&& src);
  Auto();
  ~Auto();

  explicit Auto(std::nullptr_t);

  template <typename ValueT>
  explicit Auto(const ValueT value) {
    memset(&_Context, 0, sizeof(_Context));

    if (typeid(ValueT).hash_code() == typeid(Any&).hash_code()) {
      (*this) = value;
    } else if (typeid(ValueT).hash_code() == typeid(Auto&).hash_code()) {
      (*this) = value;
    } else {
      (*this) = RValue(value);
    }
  }

  /* @NOTE: these operators are used to work with Auto, None or Any */
  Auto& operator=(std::nullptr_t);
  Auto& operator=(Auto& src);
  Auto& operator=(Any& src);
  Auto& operator=(Auto&& src);
  Auto& operator=(Any&& src);

  Bool operator==(const Any& value);
  Bool operator==(const Auto& value);
  Bool operator==(Any&& value);
  Bool operator==(Auto&& value);
  Bool operator==(std::nullptr_t);

  Bool operator!=(const Any& value);
  Bool operator!=(const Auto& value);
  Bool operator!=(Any&& value);
  Bool operator!=(Auto&& value);
  Bool operator!=(std::nullptr_t);

  /* @NOTE: these template operators are used to work with any type on C++ */
  template <typename ValueT>
  Auto& operator=(ValueT value) {
    /* @NOTE: since we can use with any type means we can assign with any
     * value including Base::Auto and Any
     */

    if (_Context.Type && Type().hash_code() == typeid(ValueT).hash_code()) {
      *(reinterpret_cast<ValueT*>(_Context.Context)) = value;
      return *this;
    } else {
      Clear();
    }

    /* @NOTE: since we must assign a new Reference counter because Auto is
     * assign to a new value now */
    if (!(_Context.Reference = (Int*)ABI::Malloc(sizeof(Int)))) {
      throw Except(EDrainMem, "ABI::Malloc a Auto::_Context.Reference");
    } else {
      /* @NOTE: register callback _Clone which is used to clone a new value
       */

      memset(_Context.Reference, 0, sizeof(Int));
      _Context.Clone = [](RawD raw) -> RawD {
        return new ValueT{*((ValueT*)(raw))};
      };

      /* @NOTE: register callback _Del which is used to delete Auto's value
       */
      _Context.Del = [](RawD raw) {
        if (raw) {
          delete reinterpret_cast<ValueT*>(raw);
        } else {
          /* @TODO: this line is never reach, just to make sure, add more
           * tool to report automatically to our channel if bugs happen
           */

          throw Except(EBadLogic, "double free or corruption");
        }
      };
    }

    try {
      _Context.Context = reinterpret_cast<RawD>(new ValueT{value});
      _Context.Type = (void*)(&(typeid(ValueT)));
    } catch (std::bad_alloc& exception) {
      /* @NOTE: free Reference counter here */
      ABI::Free(_Context.Reference);

      /* @NOTE: assign everything to None type here*/
      ABI::Memset(&_Context, 0, sizeof(Any));

      /* @NOTE: throw an exception here since allocation got trouble */
      throw Base::Exception(EDrainMem);
    }
    return *this;
  }

  template <typename ValueT>
  Bool operator==(const ValueT& value) {
    return !((*this) != value);
  }

  template <typename ValueT>
  Bool operator==(const ValueT&& value) {
    return !((*this) != RValue(value));
  }

  template <typename ValueT>
  Bool operator!=(ValueT& value) {
    return ((*this) != std::move(value));
  }

  template <typename ValueT>
  Bool operator!=(ValueT&& value) {
    if (!_Context.Type || Type().hash_code() != typeid(ValueT).hash_code()) {
      return False;
    } else if (!_Context.Context) {
      /* @TODO: this condition mustn't not be entered, if not, we must auto
       * report to bugzila */

      return False;
    } else {
      return memcmp(&value, _Context.Context, sizeof(ValueT));
    }
  }

  template <typename ValueT>
  Bool operator>(ValueT& value) {
    return (*this) > std::move(value);
  }

  template <typename ValueT>
  Bool operator>(ValueT&& value) {
    if (!_Context.Type || Type().hash_code() != typeid(ValueT).hash_code()) {
      return False;
    } else if (!_Context.Context) {
      /* @TODO: this condition mustn't not be entered, if not, we must auto
       * report to bugzila */

      return False;
    } else {
      return Get<ValueT>() > value;
    }
  }

  template <typename ValueT>
  Bool operator<(const ValueT& value) {
    return (*this) < std::move(value);
  }

  template <typename ValueT>
  Bool operator<(ValueT& value) {
    return (*this) < std::move(value);
  }

  /* @NOTE: these operators bellow are used with stdc++ */
  friend Bool operator<(const Auto& left, const Auto& right) {
    return left._Context.Context < right._Context.Context;
  }

  friend Bool operator>(const Auto& left, const Auto& right) {
    return left._Context.Context > right._Context.Context;
  }

  friend Bool operator==(const Auto& left, const Auto& right) {
    return left._Context.Context == right._Context.Context;
  }

  friend Bool operator!=(const Auto& left, const Auto& right) {
    return left._Context.Context != right._Context.Context;
  }

  /* @NOTE: this operator is used to check whether Auto is None or not */
  operator bool();

  /* @NOTE: these method provide information about the variable inside Auto
   */
  String Nametype();
  TypeD Type();

  /* @NOTE: this method is used to get raw data of Auto directly, please
   * consider before using it to prevent memory corruption or bad access
   * */
  const Void* Raw();

  /* @NOTE: this method is used to strip off variable inside Auto */
  Tuple<RawD, DelD> Strip();

  /* @NOTE: these methods are used to iteract directly with callbacks */
  DelD& Delete();
  CloneD& Clone();

  /* @NOTE: these method will be used to make reference with anothe object
   *  Auto or Any */
  Bool Reff(Auto src);
  Bool Reff(Any src);

  /* @NOTE: this method is used to make a copy instead of a referal object */
  Auto Copy();

  /* @NOTE: these template method iteract directly with variable inside Auto
   */
  template <typename ValueT>
  ValueT& Get() {
    if (!_Context.Type || typeid(ValueT).hash_code() != Type().hash_code()) {
      throw Except(EBadAccess, "ValueT must be " + Nametype());
    }

    return *((ValueT*)_Context.Context);
  }

  template <typename ValueT, typename ClassT>
  ValueT& GetSubclass() {
    if (!_Context.Type || typeid(ClassT).hash_code() != Type().hash_code()) {
      throw Except(EBadAccess, "ClassT must be " + Nametype());
    }

    if (IsSubclassOf<ValueT, ClassT>()) {
      return *(dynamic_cast<ValueT*>(((ClassT*)_Context.Context)));
    } else {
      throw Except(EBadAccess, "can\'t cast to " + Base::Nametype<ValueT>());
    }
  }

  template <typename ValueT>
  ValueT& GetBaseclass() {
    return *((ValueT*)_Context.Context);
  }

  /* @NOTE: check if the current object is hierachied by ClassT */
  template <class ValueT, typename ClassT>
  bool IsSubclassOf() {
    if (!_Context.Type || typeid(ClassT).hash_code() != Type().hash_code()) {
      throw Except(EBadAccess, "ClassT must be " + Nametype());
    }

    return dynamic_cast<const ValueT*>(((ClassT*)_Context.Context)) != None;
  }

  /* @NOTE: pushing indicates that you would like to push this value
   * directly to Auto and it can do everything with this memory */
  template <typename ValueT>
  Auto& Set(const ValueT& value, Bool pushing = False) {
    if (pushing) {
      Clear();

      /* @NOTE: since we push a memory address, not a value to Auto, it
       * can't gurantee that everything is safe here so we don't support
       * Reference counter and callbacks with this variable */
      _Context.Clone = None;
      _Context.Del = None;
      _Context.Reference = None;

      /* @NOTE: save information of this variable now */
      _Context.Type = (void*)(&typeid(ValueT));
      _Context.Context = (void*)(const_cast<ValueT*>(&value));

      return *this;
    } else {
      return ((*this) = value);
    }
  }

  template <typename ValueT>
  Auto& Set(ValueT&& value) {
    return ((*this) = value);
  }

  /* @NOTE: pulling indicates that you would like to move value from Auto to
   * outside or on reverse */
  template <typename ValueT>
  ErrorCodeE Move(ValueT& value, Bool pulling = False) {
    if (!_Context.Type || typeid(ValueT).hash_code() != Type().hash_code()) {
      return (BadAccess << "ValueT must be" << Nametype() << Base::EOL).code();
    }

    if (pulling) {
      value = *(reinterpret_cast<ValueT*>(_Context.Context));

      /* @NOTE: pulling will indicate that we don't manage removing memory when
       *  reference counter reach its limitation */

      Clear();
      ABI::Memset(&_Context, 0, sizeof(Any));
    } else {
      if (!_Context.Context) {
        return (BadAccess << "can\'t move value to None" << Base::EOL).code();
      }

      *(reinterpret_cast<ValueT*>(_Context.Context)) = value;
    }
    return ENoError;
  }

  template <typename ValueT>
  ErrorCodeE Move(ValueT&& value) {
    /* @NOTE: check everything before assign value */
    if (!_Context.Context) {
      return (BadAccess << "can\'t move value to None" << Base::EOL).code();
    }

    /* @NOTE: this workaround is used to prevent hanging issue of
     *  std::type_info::operator!= */
    if (!_Context.Type || Base::Nametype<ValueT>() != Nametype()) {
      return (BadLogic << "ValueT isn't " << Nametype() << Base::EOL).code();
    }

    /* @NOTE: everything okey now */
    *(reinterpret_cast<ValueT*>(_Context.Context)) = value;
    return ENoError;
  }

  Property<Any> Variable;

  /* @NOTE: conventional functions to create object Auto */
  template <typename Type>
  static Auto As(Type sample){
    return Auto{}.Set<Type>(sample);
  }

  static Auto FromAny(Any sample){
    return Auto{sample};
  }

 protected:
  void Clear();

 private:
  Any _Context;
};
}  // namespace Base
#endif
#endif  // BASE_AUTO_H_
