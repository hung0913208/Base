#include <Data.h>

namespace Base {
namespace Data {
UInt List::Size() { return _Values.size(); }

UInt List::Insert(Any value) {
  _Values.push_back(Auto::FromAny(value));
  return _Values.size() - 1;
}

UInt List::Insert(Auto value) {
  _Values.push_back(value);
  return _Values.size() - 1;
}

ErrorCodeE List::Remove(UInt index) {
  if (index < Size()) {
    _Values.erase(_Values.begin() + index);
    return ENoError;
  }

  return (OutOfRange << Format{"index({}) < {}"}.Apply(index, Size())).code();
}

Auto& List::operator[](UInt index){
  if (index >= Size()) {
    throw Except(EOutOfRange,
                 Format{"index({}) < {}"}.Apply(index, Size()));
  }
  return _Values[index];
}

Auto& List::operator[](ULong index){
  if (index >= Size()) {
    throw Except(EOutOfRange,
                 Format{"index({}) < {}"}.Apply(index, Size()));
  }
  return _Values[index];
}

Auto& List::operator[](Int index){
  if (UInt(index) >= Size()) {
    throw Except(EOutOfRange,
                 Format{"index({}) < {}"}.Apply(index, Size()));
  }
  return _Values[index];
}

Auto& List::operator[](Long index){
  if (ULong(index) >= Size()) {
    throw Except(EOutOfRange,
                 Format{"index({}) < {}"}.Apply(index, Size()));
  }
  return _Values[index];
}

UInt Dict::Size() {
  return _Long.size() + _Boolean.size() + _Double.size() + _String.size();
}

ErrorCodeE Dict::InsertKey(Any key) {
  return InsertKey(Auto::FromAny(key));
}

ErrorCodeE Dict::InsertKey(Auto key) {
  if (key.Type() == typeid(Bool)) {
    if (_Boolean.find(key.Get<Bool>()) != _Boolean.end()) {
      return BadLogic.code();
    }
  } else if (key.Type() == typeid(Long)) {
    if (_Long.find(key.Get<Long>()) != _Long.end()) {
      return BadLogic.code();
    }
  } else if (key.Type() == typeid(Double)) {
    if (_Double.find(key.Get<Double>()) != _Double.end()) {
      return BadLogic.code();
    }
  } else if (key.Type() == typeid(String)) {
    if (_String.find(key.Get<String>()) != _String.end()) {
      return BadLogic.code();
    }
  } else {
    return NoSupport.code();
  }

  _Key = key;
  return ENoError;
}

ErrorCodeE Dict::InsertValue(Any value) {
  return InsertValue(Auto::FromAny(value));
}

ErrorCodeE Dict::InsertValue(Auto value) {
  if (_Key) {
    if (_Key.Type() == typeid(Bool)) {
      _Boolean[_Key.Get<Bool>()] = value;
    } else if (_Key.Type() == typeid(Long)) {
      _Long[_Key.Get<Long>()] = value;
    } else if (_Key.Type() == typeid(Double)) {
      _Double[_Key.Get<Double>()] = value;
    } else if (_Key.Type() == typeid(String)) {
      _String[String(_Key.Get<String>())] = value;
    } else {
      return NoSupport.code();
    }

    return ENoError;
  }

  return BadLogic.code();
}

ErrorCodeE Dict::Insert(String name, Auto value) {
  _String[name] = value;
  return ENoError;
}

ErrorCodeE Dict::Remove(String name) {
  if (_String.find(name) != _String.end()){
    _String.erase(name);
    return ENoError;
  } else {
    return (NotFound << name).code();
  }
}

Bool Dict::Find(String name) {
  return _String.find(name) != _String.end();
}

Bool Dict::Find(String name, const std::type_info& type) {
  return Find(name) && _String[name].Type() == type;
}

ErrorCodeE Dict::Insert(UInt name, Auto value) {
  _Long[name] = value;
  return ENoError;
}

ErrorCodeE Dict::Remove(UInt name) {
  if (_Long.find(name) != _Long.end()) {
    _Long.erase(name);
    return ENoError;
  } else {
    return (NotFound << Base::ToString(name)).code();
  }
}

Bool Dict::Find(UInt name) {
  return _Long.find(name) != _Long.end();
}

Bool Dict::Find(UInt name, const std::type_info& type) {
  return Find(name) && _Long[name].Type() == type;
}

ErrorCodeE Dict::Insert(Int name, Auto value) {
  _Long[name] = value;
  return ENoError;
}

ErrorCodeE Dict::Remove(Int name) {
  if (_Long.find(name) != _Long.end()) {
    _Long.erase(name);
    return ENoError;
  } else {
    return (NotFound << Base::ToString(name)).code();
  }
}

Bool Dict::Find(Int name) {
  return _Long.find(name) != _Long.end();
}

Bool Dict::Find(Int name, const std::type_info& type) {
  return Find(name) && _Long[name].Type() == type;
}

ErrorCodeE Dict::Insert(ULong name, Auto value) {
  _Long[name] = value;
  return ENoError;
}

ErrorCodeE Dict::Remove(ULong name) {
  if (_Long.find(name) != _Long.end()) {
    _Long.erase(name);
    return ENoError;
  } else {
    return (NotFound << Base::ToString(name)).code();
  }
}

Bool Dict::Find(ULong name) {
  return _Long.find(name) != _Long.end();
}

Bool Dict::Find(ULong name, const std::type_info& type) {
  return Find(name) && _Long[name].Type() == type;
}

ErrorCodeE Dict::Insert(Long name, Auto value) {
  _Long[name] = value;
  return ENoError;
}

ErrorCodeE Dict::Remove(Long name) {
  if (_Long.find(name) != _Long.end()) {
    _Long.erase(name);
    return ENoError;
  } else {
    return (NotFound << Base::ToString(name)).code();
  }
}

Bool Dict::Find(Long name) {
  return _Long.find(name) != _Long.end();
}

Bool Dict::Find(Long name, const std::type_info& type) {
  return Find(name) && _Long[name].Type() == type;
}

ErrorCodeE Dict::Insert(Byte name, Auto value) {
  _Long[name] = value;
  return ENoError;
}

ErrorCodeE Dict::Remove(Byte name) {
  if (_Long.find(name) != _Long.end()) {
    _Long.erase(name);
    return ENoError;
  } else {
    return (NotFound << Base::ToString(name)).code();
  }
}

Bool Dict::Find(Byte name) {
  return _Long.find(name) != _Long.end();
}

Bool Dict::Find(Byte name, const std::type_info& type) {
  return Find(name) && _Long[name].Type() == type;
}

ErrorCodeE Dict::Insert(Bool name, Auto value){
  _Boolean[name] = value;
  return ENoError;
}

ErrorCodeE Dict::Remove(Bool name){
  if (_Boolean.find(name) != _Boolean.end()) {
    _Boolean.erase(name);
    return ENoError;
  } else {
    return (NotFound << Base::ToString(name)).code();
  }
}

Bool Dict::Find(Bool name) {
  return _Boolean.find(name) != _Boolean.end();
}

Bool Dict::Find(Bool name, const std::type_info& type) {
  return Find(name) && _Boolean[name].Type() == type;
}

ErrorCodeE Dict::Insert(Float name, Auto value){
  _Double[name] = value;
  return ENoError;
}

ErrorCodeE Dict::Remove(Float name){
  if (_Double.find(name) != _Double.end()) {
    _Double.erase(name);
    return ENoError;
  } else {
    return (NotFound << Base::ToString(name)).code();
  }
}

Bool Dict::Find(Float name) {
  return _Double.find(name) != _Double.end();
}

Bool Dict::Find(Float name, const std::type_info& type) {
  return Find(name) && _Double[name].Type() == type;
}

ErrorCodeE Dict::Insert(Double name, Auto value){
  _Double[name] = value;
  return ENoError;
}

ErrorCodeE Dict::Remove(Double name){
  if (_Double.find(name) != _Double.end()) {
    _Double.erase(name);
    return ENoError;
  } else {
    return (NotFound << Base::ToString(name)).code();
  }
}

Bool Dict::Find(Double name) {
  return _Double.find(name) != _Double.end();
}

Bool Dict::Find(Double name, const std::type_info& type) {
  return Find(name) && _Double[name].Type() == type;
}

Auto& Dict::operator[](String name){
  if (_String.find(name) != _String.end()){
    return _String[name];
  } else {
    throw Except(ENotFound, name);
  }
}

Auto& Dict::operator[](UInt name){
  if (_Long.find(name) != _Long.end()) {
    return _Long[name];
  } else {
    throw Except(ENotFound, Base::ToString(name));
  }
}

Auto& Dict::operator[](Int name){
  if (_Long.find(name) != _Long.end()) {
    return _Long[name];
  } else {
    throw Except(ENotFound, Base::ToString(name));
  }
}

Auto& Dict::operator[](ULong name){
  if (_Long.find(name) != _Long.end()) {
    return _Long[name];
  } else {
    throw Except(ENotFound, Base::ToString(name));
  }
}

Auto& Dict::operator[](Long name){
  if (_Long.find(name) != _Long.end()) {
    return _Long[name];
  } else {
    throw Except(ENotFound, Base::ToString(name));
  }
}

Auto& Dict::operator[](Byte name){
  if (_Long.find(name) != _Long.end()) {
    return _Long[name];
  } else {
    throw Except(ENotFound, Base::ToString(name));
  }
}

Auto& Dict::operator[](Bool name){
  if (_Boolean.find(name) != _Boolean.end()) {
    return _Boolean[name];
  } else {
    throw Except(ENotFound, Base::ToString(name));
  }
}

Auto& Dict::operator[](Float name){
  if (_Double.find(name) != _Double.end()) {
    return _Double[name];
  } else {
    throw Except(ENotFound, Base::ToString(name));
  }
}

Auto& Dict::operator[](Double name){
  if (_Double.find(name) != _Double.end()) {
    return _Double[name];
  } else {
    throw Except(ENotFound, Base::ToString(name));
  }
}

Auto& Dict::Get(String key){
  return (*this)[key];
}

Auto& Dict::Get(UInt key){
  return (*this)[key];
}

Auto& Dict::Get(Int key){
  return (*this)[key];
}

Auto& Dict::Get(ULong key){
  return (*this)[key];
}

Auto& Dict::Get(Long key){
  return (*this)[key];
}

Auto& Dict::Get(Byte key){
  return (*this)[key];
}

Auto& Dict::Get(Bool key){
  return (*this)[key];
}

Auto& Dict::Get(Float key){
  return (*this)[key];
}

Auto& Dict::Get(Double key){
  return (*this)[key];
}

Struct::Struct(Map<String, Base::Auto> definition): _Definition{definition}{
}

Struct::~Struct(){}

UInt Struct::Size(){ return _Definition.size(); }

Map<String, Base::Auto>& Struct::Mapping(){
  return _Definition;
}

Base::Auto& Struct::operator[](const CString name){
  String sname{name};

  if (_Definition.find(sname) != _Definition.end()) {
    return _Definition[sname];
  } else {
    throw Except(ENotFound, sname);
  }
}
} // namespace Data
} // namespace Base
