#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Macro.h>
#else
#include <Macro.h>
#endif

#if !defined(BASE_TYPE_ARRAY_H_) && (USE_BASE_VECTOR || APPLE)
#define BASE_TYPE_ARRAY_H_

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/Common.h>
#include <Base/Type/Refcount.h>
#else
#include <Common.h>
#include <Refcount.h>
#endif

namespace Base {
template <typename Type>
class Array : Refcount{
 public:
  typedef Type* Iterator;

  explicit Array(Type* data, ULong size);
  virtual ~Array();

  Array();
  Array(const Array& src);

  Type& operator[](ULong index);

  const Type* data();
  Type& at(ULong index);
  Type& front();
  Type& back();

  Iterator begin();
  Iterator end();
  Iterator rbegin();
  Iterator rend();
  const Iterator cbegin();
  const Iterator cend();
  const Iterator crbegin();
  const Iterator crend();

  Bool empty();
  ULong size();
  ULong capacity();

  void reserve(ULong capacity);
  void resize(ULong size, Type&& sample);
  void resize(ULong size, Type& sample);
  void resize(ULong size);

 protected:
  static void release(Refcount* thiz) {
    if (thiz->Access(0)) {
      delete (Type*) thiz->Access(0);
    }
  }

  ULong _Begin, _End, _Capacity;
  Type* _Buffer;
};

template <typename Type>
Array<Type>::Array(Type* data, ULong size) : Refcount{Array<Type>::release} {
  Secure(0, _Buffer = new Type[size + 2]);
  _Capacity = size;
  _Begin = 1;
  _End = size + 1;
}

template <typename Type>
Array<Type>::~Array() { }

template <typename Type>
Array<Type>::Array() : Refcount{Array<Type>::release} {
  Secure(0, _Buffer = new Type[2]);
  _Buffer = new Type[2];
  _Capacity = 0;
  _Begin = 1;
  _End = 1;
}

template <typename Type>
Array<Type>::Array(const Array& src) : Refcount{src} {
}

template <typename Type>
Type& Array<Type>::operator[](ULong index) {
}

template <typename Type>
const Type* Array<Type>::data() { return &_Buffer[_Begin]; }

template <typename Type>
Type& Array<Type>::at(ULong index) {
  return _Buffer[index];
}

template <typename Type>
Type& Array<Type>::front() { return _Buffer[_Begin]; }

template <typename Type>
Type& Array<Type>::back() { return _Buffer[_End - 1]; }

template <typename Type>
typename Array<Type>::Iterator Array<Type>::begin() {
  return &_Buffer[_Begin + 1];
}

template <typename Type>
typename Array<Type>::Iterator Array<Type>::end() {
  return &_Buffer[_End];
}

template <typename Type>
typename Array<Type>::Iterator Array<Type>::rbegin() {
  return &_Buffer[_End - 1];
}

template <typename Type>
typename Array<Type>::Iterator Array<Type>::rend() {
  return _Buffer[_Begin];
}

template <typename Type>
const typename Array<Type>::Iterator Array<Type>::cbegin() {
  return _Buffer[_Begin + 1];
}

template <typename Type>
const typename Array<Type>::Iterator Array<Type>::cend() {
  return _Buffer[_End];
}

template <typename Type>
const typename Array<Type>::Iterator Array<Type>::crbegin() {
  return _Buffer[_End - 1];
}

template <typename Type>
const typename Array<Type>::Iterator Array<Type>::crend() {
  return _Buffer[_Begin];
}

template <typename Type>
Bool Array<Type>::empty() { return _Begin >= _End; }

template <typename Type>
ULong Array<Type>::size() { return _End - _Begin - 2; }
} // namespace Base
#endif  // BASE_TYPE_ARRAY_H_
