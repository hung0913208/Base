#if !defined(BASE_PROPERTY_H_) && __cplusplus
#define BASE_PROPERTY_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Exception.h>
#include <Base/Type.h>
#include <Base/Utils.h>
#else
#include <Exception.h>
#include <Type.h>
#include <Utils.h>
#endif

namespace Base {
template <typename Type>
class Property : public Base::Refcount {
 public:
#define CONTENT 0
#define SETTER 1
#define GETTER 2
  using Getter = Function<Type&()>;
  using Setter = Function<void(Type)>;

  explicit Property(Getter getter = None, Setter setter = None):
      Refcount{Property<Type>::Release}, _Content{None}, _Getter{None},
      _Setter{None} {
    Secure(CONTENT, _Content = new Type{});

    if (getter){
      Secure(GETTER, _Getter = new Getter{getter});
    }

    if (setter) {
      Secure(SETTER, _Setter = new Setter{setter});
    }
  }

  Property(Property& src): Refcount{src} {
    _Content = src._Content;
    _Getter = src._Getter;
    _Setter = src._Setter;
  }

  Property(const Property& src): Refcount{src} {
    _Content = src._Content;
    _Getter = src._Getter;
    _Setter = src._Setter;
  }

  virtual ~Property() { }

  /* @NOTE: getter and setter methods */
  Property<Type>& set(Type&& value) {
    Secure([&]() {
      if (_Setter) {
        (*_Setter)(value);
      } else {
        *_Content = value;
      }
    });

    return *this;
  }

  Property<Type>& set(Type& value) {
    Secure([&]() {
      if (_Setter) {
        (*_Setter)(value);
      } else {
        *_Content = value;
      }
    });

    return *this;
  }

  Type&& get() {
    if (_Getter) {
      return std::move((*_Getter)());
    } else {
      return std::forward<Type>(*_Content);
    }
  }

  Type& ref(){
    if (_Getter) {
      return (*_Getter)();
    } else {
      return *_Content;
    }
  }

  /* @NOTE: getter and setter operators */
  virtual void operator=(Type& value) { set(value); }

  virtual void operator=(Type&& value) { set(RValue(value)); }

  virtual Type& operator()() { return ref(); }

 private:
  static void Release(Refcount* thiz) {
    if (thiz->Access(CONTENT)) {
      delete (Type*)thiz->Access(CONTENT);
    }

    if (thiz->Access(GETTER)) {
      delete (Getter*)thiz->Access(GETTER);
    }

    if (thiz->Access(SETTER)) {
      delete (Setter*)thiz->Access(SETTER);
    }
  }

  Type* _Content;
  Getter* _Getter;
  Setter* _Setter;

};

template <typename Type>
class SetProperty : public Property<Type> {
 public:
  using Getter = Function<Type&(UInt)>;
  using Setter = Function<void(Vector<Type>)>;

  explicit SetProperty(Getter getter = None, Setter setter = None)
      : Property<Type>{}, _Getter{getter}, _Setter{setter} {}

  virtual ~SetProperty() {}

  inline UInt size() { return _Size; }

  virtual void operator=(Type& UNUSED(value)) override {
    throw Except(ENoSupport, "SetProperty::operator=");
  }

  virtual void operator=(Type&& UNUSED(value)) override {
    throw Except(ENoSupport, "SetProperty::operator=");
  }

  virtual void operator=(Vector<Type>&& value) {
    _Size = value.size();

    if (_Setter) {
      _Setter(value);
    } else {
      _Content = value;
    }
  }

  virtual void operator=(Vector<Type>& value) {
    _Size = value.size();

    if (_Setter) {
      _Setter(value);
    } else {
      _Content = value;
    }
  }

  virtual Type& operator[](UInt index) {
    if (_Getter) {
      return _Getter(index);
    } else if (index < _Content.size()) {
      return _Content[index];
    } else {
      throw Exception(EOutOfRange);
    }
  }

  virtual Type& operator()(UInt index) {
    if (_Getter) {
      return _Getter(index);
    } else if (index < _Content.size()) {
      return _Content[index];
    } else {
      throw Exception(EOutOfRange);
    }
  }

  virtual Type& operator()() override { return (*this)[0]; }

 private:
  Vector<Type> _Content;
  UInt _Size;
  Getter _Getter;
  Setter _Setter;
};  // namespace Base

template <typename Key, typename Type>
class MapProperty : public Property<Type> {
 public:
  using Getter = Function<Type&(Key)>;
  using Setter = Function<void(Map<Key, Type>)>;

  explicit MapProperty(Getter getter = None, Setter setter = None)
      : Property<Type>{}, _Getter{getter}, _Setter{setter} {}

  virtual ~MapProperty() {}

  virtual MapProperty<Key, Type>& operator=(Map<Key, Type> mapping) {
    if (_Setter) {
      _Setter(mapping);
    } else {
      _Content = mapping;
    }

    return *this;
  }

  virtual Type& operator[](Key keyname) { return RValue((*this)(keyname)); }

  virtual Type& operator()(Key keyname) {
    if (_Getter) {
      return _Getter(keyname);
    } else if (_Content.find(keyname) != _Content.end()) {
      return _Content[keyname];
    } else {
      throw Except(EOutOfRange, Base::ToString(keyname) + " not found");
    }
  }

  virtual Type& operator()() override { throw Exception(); }

 private:
  Map<Key, Type> _Content;
  Getter _Getter;
  Setter _Setter;
};
}  // namespace Base
#endif  // BASE_PROPERTY_H_
