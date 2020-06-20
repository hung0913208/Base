#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Macro.h>
#else
#include <Macro.h>
#endif

#if !defined(BASE_TYPE_VECTOR_H_) && (USE_BASE_VECTOR || APPLE)
#define BASE_TYPE_VECTOR_H_
#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Type/Array.h>
#else
#include <Array.h>
#endif

namespace Base {
template <typename Type>
class Vector : public Array<Type> {
 public:
  explicit Vector(Type* data, ULong size);
  virtual ~Vector();

  Vector(const Vector& src);
  Vector();

  Vector<Type>& operator=(const Vector<Type>& src);
  Vector<Type>& operator=(Vector<Type>&& src);
  Type& operator[](ULong index);

  void push_back(Type value);
  Type pop_back();

  void erase(typename Array<Type>::Iterator iterator);
  void clear();
};

template <typename Type>
Vector<Type>::Vector(Type* data, ULong size) {}

template <typename Type>
Vector<Type>::~Vector() {}

template <typename Type>
Vector<Type>::Vector(const Vector& src) {}

template<typename Type>
Vector<Type>::Vector() {}

template<typename Type>
Vector<Type>& Vector<Type>::operator=(const Vector& src) {
}

template<typename Type>
Vector<Type>& Vector<Type>::operator=(Vector&& src) {
}

template<typename Type>
Type& Vector<Type>::operator[](ULong index) {
}

template<typename Type>
void Vector<Type>::push_back(Type value) {
}

template<typename Type>
Type Vector<Type>::pop_back() {
}

template<typename Type>
void Vector<Type>::erase(typename Array<Type>::Iterator iterator) {
}

template<typename Type>
void Vector<Type>::clear() {
}
} // namespace Base
#endif  // BASE_TYPE_VECTOR_H_
