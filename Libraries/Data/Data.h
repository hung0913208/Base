#ifndef DATA_FORMAT_H_
#define DATA_FORMAT_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Auto.h>
#include <Base/Type.h>
#else
#include <Auto.h>
#include <Type.h>
#endif

#include <typeinfo>

namespace Base {
namespace Data {
class List{
 public:
  UInt Size();
  UInt Insert(Any value);
  UInt Insert(Auto value);
  ErrorCodeE Remove(UInt index);

  template<typename Type>
  Type& At(UInt index){
    return (*this)[index].Get<Type>();
  }

  template<typename Type>
  Type& At(ULong index){
    return (*this)[index].Get<Type>();
  }

  template<typename Type>
  Type& At(Int index){
    return (*this)[index].Get<Type>();
  }

  template<typename Type>
  Type& At(Long index){
    return (*this)[index].Get<Type>();
  }

  Auto& operator[](UInt index);
  Auto& operator[](ULong index);
  Auto& operator[](Int index);
  Auto& operator[](Long index);

 private:
  Vector<Auto> _Values;
};

class Dict{
 public:
  UInt Size();

  ErrorCodeE InsertKey(Any key);
  ErrorCodeE InsertKey(Auto key);
  ErrorCodeE InsertValue(Any value);
  ErrorCodeE InsertValue(Auto value);

  ErrorCodeE Insert(String name, Auto value);
  ErrorCodeE Remove(String name);
  Bool Find(String name);
  Bool Find(String name, const std::type_info& type);

  ErrorCodeE Insert(UInt name, Auto value);
  ErrorCodeE Remove(UInt name);
  Bool Find(UInt name);
  Bool Find(UInt name, const std::type_info& type);

  ErrorCodeE Insert(Int name, Auto value);
  ErrorCodeE Remove(Int name);
  Bool Find(Int name);
  Bool Find(Int name, const std::type_info& type);

  ErrorCodeE Insert(ULong name, Auto value);
  ErrorCodeE Remove(ULong name);
  Bool Find(ULong name);
  Bool Find(ULong name, const std::type_info& type);

  ErrorCodeE Insert(Long name, Auto value);
  ErrorCodeE Remove(Long name);
  Bool Find(Long name);
  Bool Find(Long name, const std::type_info& type);

  ErrorCodeE Insert(Byte name, Auto value);
  ErrorCodeE Remove(Byte name);
  Bool Find(Byte name);
  Bool Find(Byte name, const std::type_info& type);

  ErrorCodeE Insert(Bool name, Auto value);
  ErrorCodeE Remove(Bool name);
  Bool Find(Bool name);
  Bool Find(Bool name, const std::type_info& type);

  ErrorCodeE Insert(Float name, Auto value);
  ErrorCodeE Remove(Float name);
  Bool Find(Float name);
  Bool Find(Float name, const std::type_info& type);

  ErrorCodeE Insert(Double name, Auto value);
  ErrorCodeE Remove(Double name);
  Bool Find(Double name);
  Bool Find(Double name, const std::type_info& type);

  template<typename Key>
  Auto& Get(Key UNUSED(key)) {
    throw Except(ENoSupport, Base::Nametype<Key>());
  }

  Auto& Get(String key);
  Auto& Get(UInt key);
  Auto& Get(Int key);
  Auto& Get(ULong key);
  Auto& Get(Long key);
  Auto& Get(Byte key);
  Auto& Get(Bool key);
  Auto& Get(Float key);
  Auto& Get(Double key);

  Auto& operator[](String name);
  Auto& operator[](UInt name);
  Auto& operator[](Int name);
  Auto& operator[](ULong name);
  Auto& operator[](Long name);
  Auto& operator[](Byte name);
  Auto& operator[](Bool name);
  Auto& operator[](Float name);
  Auto& operator[](Double name);

 private:
  Auto _Key;
  Map<Long, Auto> _Long;
  Map<Bool, Auto> _Boolean;
  Map<Double, Auto> _Double;
  Map<String, Auto> _String;
};

class Struct {
 public:
  explicit Struct(Map<String, Base::Auto> definition);
  virtual ~Struct();

  /* @NOTE: get the whole defenition of this Struct */
  Map<String, Base::Auto>& Mapping();

  /* @NOTE: get how many element of this Struct */
  UInt Size();

  /* @NOTE: get each element of this Struct */
  Base::Auto& operator[](const CString name);

 private:
  Map<String, Base::Auto> _Definition;
};
} // namespace Data

class Parser{
 public:
 protected:
  virtual ErrorCodeE Parse(UInt index, Byte c) = 0;
  virtual ErrorCodeE Record(Byte value) = 0;
};

class Generator{
 public:
 protected:
  virtual ErrorCodeE Generate() = 0;
};
} // namespace Base
#endif  // DATA_FORMAT_H_
