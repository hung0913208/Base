#include <ABI.h>
#include <String.h>
#include <Logcat.h>
#include <Utils.h>

#if !USE_STD_STRING
using namespace ABI;

namespace Base{
String::String(Base::String& str) {
  _Size = str._Size;

  if (str._Ref) {
    _Ref = str._Ref;
    _String = str._String;
    _Autoclean = str._Autoclean;

    if (str._String) {
      _Ref = str._Ref;
      (*_Ref)++;
    } else {
      _Ref = None;
    }
  } else {
    _Ref = (Int*)ABI::Malloc(sizeof(Int));
    _String = (CString)ABI::Calloc(_Size + 1, sizeof(Char));

    if (!_Ref || !_String) {
      Abort(EDrainMem);
    } else {
      memcpy(_String, str._String, _Size);
      *_Ref = 1;
    }
  }
}

String::String(Base::String&& str): _Autoclean{True} {
  _Size = str._Size;

  if (str._Ref) {
    _String = str._String;
    _Autoclean = str._Autoclean;

    if (str._String) {
      _Ref = str._Ref;
      (*_Ref)++;
    } else {
      _Ref = None;
    }
  } else {
    _Ref = (Int*)ABI::Malloc(sizeof(Int));
    _String = (CString)ABI::Calloc(_Size + 1, sizeof(Char));

    if (!_Ref || !_String) {
      Abort(EDrainMem);
    } else {
      memcpy(_String, str._String, _Size);
      *_Ref = 1;
    }
  }
}

String::String(const Base::String& str): _Autoclean{True} {
  _Size = str._Size;

  if (str._Ref) {
    _Ref = str._Ref;
    _String = str._String;
    _Autoclean = str._Autoclean;

    if (str._String) {
      _Ref = str._Ref;
      (*_Ref)++;
    } else {
      _Ref = None;
    }
  } else {
    _Ref = (Int*)ABI::Malloc(sizeof(Int));
    _String = (CString)ABI::Calloc(_Size + 1, sizeof(Char));

    if (!_Ref || !_String) {
      Abort(EDrainMem);
    } else {
      memcpy(_String, str._String, _Size);
      *_Ref = 1;
    }
  }
}

String::String(const CString str): _Autoclean{False} {
  /* @NOTE: need to improve the way to load const CString since Base::String
   * consumes more memory than std::string */

  _Ref = (Int*)ABI::Malloc(sizeof(Int));

  if (!_Ref) {
    Abort(EDrainMem);
  } else {
    *_Ref = 1;
  }

  _Size = Strlen(const_cast<CString>(str));
  _String = const_cast<CString>(str);
}

String::String(const CString str, UInt size): _Autoclean{True} {
  _Ref = (Int*)ABI::Malloc(sizeof(Int));

  if (!_Ref) {
    Abort(EDrainMem);
  } else {
    *_Ref = 1;
    _Size = size;
    _String = (CString)ABI::Calloc(_Size + 1, sizeof(Char));

    if (_String) {
      Memcpy(_String, const_cast<CString>(str), _Size);
    } else {
      free(_Ref);
      Abort(EDrainMem);
    }
  }
}

String::String(UInt size, Char c): _Autoclean{True} {
  _Ref = (Int*)ABI::Malloc(sizeof(Int));

  if (!_Ref) {
    Abort(EDrainMem);
  } else {
    *_Ref = 1;
    _Size = size;
    _String = (CString)ABI::Calloc(_Size + 1, sizeof(Char));

    if (_String) {
      for (UInt i = 0; i < size; ++i) {
        _String[i] = c;
      }
    } else {
      free(_Ref);
      Abort(EDrainMem);
    }
  }
}

String::String(): _String{None}, _Autoclean{False}, _Size{0}, _Ref{None} {}

String::~String(){ clear(); }

String& String::append(const Char c) {
  return append(1, c);
}

String& String::append(Int n, const Char c) {
  UInt current_size = size();

  resize(current_size + n);
  for (auto i = 0; i < n; ++i) {
    _String[current_size + i] = c;
  }

  return *this;
}

Base::String& String::append(const Base::String& str) {
  return append(str.data(), str.size());
}

Base::String& String::append(const Base::String& str, Int subpos, Int sublen) {
  return append(str.data() + subpos, sublen);
}

Base::String& String::append(const CString s) {
  return append(s, Strlen(const_cast<CString>(s)));
}

Base::String& String::append(const CString s, Int n) {
  UInt current_size = size();

  resize(size() + n);
  for (auto i = 0; i < n; ++i) {
    _String[current_size + i] = s[i];
  }

  return *this;
}

CString String::c_str() const { return _String; }

CString String::data() const { return _String; }

Char& String::front() const {
  if (!_String) {
    throw Except(EBadAccess, "get None string");
  }

  return _String[0];
}

Char& String::back() const {
  if (!_String) {
    throw Except(EBadAccess, "get None string");
  }

  return _String[_Size - 1];
}

UInt String::size() const { return _Size; }

Bool String::empty() const { return _Size == 0 || _String == None; }

Void String::clear() {
  Bool passed = False;

  if (_Ref != None) {
    if (*_Ref > 0) { // <-- hanging here, possible cause by gcc protecting memory
      (*_Ref)--;
      passed = True;
    }
  }

  if ((passed && *_Ref == 0)) {
    if (_String && _Autoclean) {
      free(_String);
    }

    if (_Ref != None) {
      free(_Ref);
    }
  }

  _String = None;
  _Size = 0;
  _Ref = None;
}

Void String::resize(UInt size) {
  if (!_String || !_Autoclean) {
    auto tmp = (CString)ABI::Calloc(size + 1, sizeof(Char));
    auto saved = _String? (size > _Size? _Size: size): 0;

    if (!tmp) {
      Abort(EDrainMem);
    } else if (_String) {
      memcpy(tmp, _String, saved);
    }

    if (_Ref) { clear(); }
    _String = tmp;
    _Autoclean = True;
    _Size = saved;

    if (!_Ref) {
      _Ref = (Int*)malloc(sizeof(Int));
      *_Ref = 1;
    }
  } else if (size > _Size) {
    /* @NOTE: don't use Realloc reachlessly since it will broken the link between
     *  reference string */

    auto tmp = (CString)ABI::Calloc(sizeof(Char), size + 1);

    if (!tmp) {
      Abort(EDrainMem);
    } else {
      memcpy(tmp, _String, _Size);
      clear();

      if (!(_Ref = (Int*)ABI::Malloc(sizeof(Int)))) {
        ABI::Free(tmp);
        Abort(EDrainMem);
      }

      _Autoclean = True;
      _String = tmp;
      _Size = size;
      *_Ref = 1;
    }
  } else {
    _String[size] = '\0';
  }

  _Size = size;
}

Void String::reverse() {
  for (UInt i = 0; i < _Size/2; ++i) {
    Char tmp = _String[i];

    _String[i] = _String[_Size - 1 - i];
    _String[_Size - 1 - i] = tmp;
  }
}

String String::substr(UInt pos, Int len) {
  if (len < 0) {
    return String{&(_String[pos])}.copy();
  } else {
    return String{&(_String[pos]), UInt(len)}.copy();
  }
}

Char& String::operator[](Int index) const {
  if (0 <= index && index < Int(_Size)) {
    return _String[index];
  } else {
    throw Except(EOutOfRange, Base::ToString(index));
  }
}

Base::String& String::operator=(Char c) {
  auto tmp = ABI::Calloc(2, sizeof(Char));

  if (!tmp) {
    Abort(EDrainMem);
  } else {
    clear();

    _Ref = (Int*)ABI::Calloc(1, sizeof(Int));
    _Size = 1;
    _String = (CString)tmp;

    if (_Ref) {
      _String[0] = c;
      *_Ref = 1;
    } else {
      Abort(EDrainMem);
    }
  }

  return *this;
}

Base::String& String::operator=(Base::String& str) {
  clear();
  _Size = str._Size;

  if (str._Ref) {
    _Ref = str._Ref;
    _String = str._String;
    _Autoclean = str._Autoclean;

    if (str._String) {
      _Ref = str._Ref;
      (*_Ref)++;
    } else {
      _Ref = None;
    }
  } else {
    _Ref = (Int*)ABI::Malloc(sizeof(Int));
    _String = (CString)ABI::Calloc(_Size + 1, sizeof(Char));
    _Autoclean = True;

    if (!_Ref || !_String) {
      Abort(EDrainMem);
    } else {
      memcpy(_String, str._String, _Size);
      *_Ref = 1;
    }
  }

  return *this;
}

Base::String& String::operator=(Base::String&& str) {
  clear();
  _Size = str._Size;

  if (str._Ref) {
    _Ref = str._Ref;
    _String = str._String;
    _Autoclean = str._Autoclean;

    if (str._String) {
      _Ref = str._Ref;
      (*_Ref)++;
    } else {
      _Ref = None;
    }
  } else {
    _Ref = (Int*)ABI::Malloc(sizeof(Int));
    _String = (CString)ABI::Calloc(_Size + 1, sizeof(Char));
    _Autoclean = True;

    if (!_Ref || !_String) {
      Abort(EDrainMem);
    } else {
      memcpy(_String, str._String, _Size);
      *_Ref = 1;
    }
  }

  return *this;
}

Base::String& String::operator=(const Base::String& str) {
  return this->operator=(const_cast<String&>(str));
}

Base::String& String::operator=(const CString str) {
  auto len = Strlen(const_cast<CString>(str));
  auto tmp = ABI::Calloc(len, sizeof(Char));

  if (!tmp) {
    Abort(EDrainMem);
  } else {
    clear();

    if (!(_Ref = (Int*)ABI::Calloc(1, sizeof(Int)))) {
      Abort(EDrainMem);
    }

    *_Ref = 1;
    _Size = len;
    _String = (CString)tmp;
    _Autoclean = True;

    Memcpy(_String,
           const_cast<CString>(str),
           len);
  }

  return *this;
}

Base::String String::operator+(String&& str){
  Base::String result;

  result.append(_String, _Size);
  result.append(str);
  return result;
}

Base::String String::operator+(const Char c){
  return this->operator+(&c);
}

Base::String String::operator+(const Base::String& str){
  Base::String result;

  result.append(_String, _Size);
  result.append(str);
  return result;
}

Base::String String::operator+(const CString cstr){
  Base::String result;

  result.append(_String, _Size);
  result.append(cstr, Strlen(const_cast<CString>(cstr)));
  return result;
}

Base::String& String::operator+=(Base::String&& str){
  return append(RValue(str));
}

Base::String& String::operator+=(const Char c){
  return append(c);
}

Base::String& String::operator+=(const Base::String& str){
  return append(str);
}

Base::String& String::operator+=(const CString cstr){
  return append(cstr);
}

Bool String::operator<(const Base::String& dst) const {
  if (size() != dst.size()) {
    return size() < dst.size();
  } else {
    for (UInt i = 0; i < _Size; ++i) {
      if (_String[i] != dst._String[i]) {
        return _String[i] < dst._String[i];
      }
    }

    return False;
  }
}

Bool String::operator<(Base::String&& dst) const{
  if (size() != dst.size()) {
    return size() < dst.size();
  } else {
    for (UInt i = 0; i < _Size; ++i) {
      if (_String[i] != dst._String[i]) {
        return _String[i] < dst._String[i];
      }
    }

    return False;
  }
}

Bool String::operator==(const CString dst) const{
  return !this->operator!=(dst);
}

Bool String::operator==(const Base::String& dst) const{
  return !this->operator!=(dst);
}

Bool String::operator==(Base::String&& dst) const{
  return !this->operator!=(dst);
}

Bool String::operator!=(const CString dst) const{
  if (!dst ^ !_String) {
    return False;
  } else if (!dst && !_String) {
    return True;
  }

  for (UInt i = 0; i < _Size; ++i) {
    if (dst[i] != _String[i]) {
      return True;
    }
  }

  return False;
}

Bool String::operator!=(const Base::String& dst) const{
  if (_Size == dst.size()) {
    return this->operator!=(dst.data());
  } else {
    return True;
  }
}

Bool String::operator!=(Base::String&& dst) const{
  if (_Size == dst.size()) {
    return this->operator!=(dst.data());
  } else {
    return True;
  }
}

String& String::copy() {
  if (!_Autoclean) {
    auto tmp = _String;
    auto size = _Size;

    clear();

    _Ref = (Int*)ABI::Malloc(sizeof(Int));
    _Size = size;
    _String = (CString)ABI::Calloc(_Size + 1, sizeof(Char));
    _Autoclean = True;

    if (!_Ref || !_String) {
      Abort(EDrainMem);
    } else {
      ABI::Memcpy(_String, tmp, size);
      *_Ref = 1;
    }
  }
  return *this;
}
} // namespace Base

Base::String operator+(const Char c, Base::String& string){
  return Base::String{1, c} + string;
}

Base::String operator+(const Char c, Base::String&& string){
  return Base::String{1, c} + string;
}

Base::String operator+(const CString cstring, Base::String& string){
  return Base::String{cstring} + string;
}

Base::String operator+(const CString cstring, Base::String&& string){
  return Base::String{cstring} + string;
}

#ifndef KERNEL
std::ostream& operator<<(std::ostream& cout, Base::String& string){
  cout << string.data();
  return cout;
}

std::ostream& operator<<(std::ostream& cout, Base::String&& string){
  cout << string.data();
  return cout;
}

std::istream& operator>>(std::istream& cin, Base::String& string){
  std::string tmp;

  cin >> tmp;
  string.resize(tmp.size());
  Memcpy(const_cast<CString>(string.data()),
         const_cast<CString>(tmp.data()),
         tmp.size());
  return cin;
}
#else
#endif
#endif  // USE_STD_STRING
