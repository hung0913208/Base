#if !defined(BASE_TYPE_STRING_H_) && !USE_STD_STRING
#define BASE_TYPE_STRING_H_

#if __cplusplus
#include <Common.h>
#include <Refcount.h>

#ifndef KERNEL
#if defined(BASE_TYPE_H_)
#include <string>
#endif

#include <ostream>
#include <istream>
#endif

namespace Base{
class String{
 public:
  String(String& str);
  String(String&& str);
  String(const String& str);
  String(const CString str);
  String(const CString str, UInt size);
  String(UInt size, Char c);

  String();
  virtual ~String();

  /* @NOTE: append String to another String*/
  Base::String& append(const Char c);
  Base::String& append(Int n, const Char c);
  Base::String& append(const Base::String& str);
  Base::String& append(const Base::String& str, Int subpos, Int sublen);
  Base::String& append(const CString s);
  Base::String& append(const CString s, Int n);

  /* @NOTE: interact with String's parameters */
  CString c_str() const;
  CString data() const;
  Char& front() const;
  Char& back() const;
  UInt size() const;
  Bool empty() const;

  /* @NOTE: clear */
  Void clear();

  /* @NOTE: resize String */
  Void resize(UInt size);

  /* @NOTE: make a reverse string */
  Void reverse();

  /* @NOTE: make a copy instead of reference */
  String& copy();

  /* @NOTE: access character according index */
  Char& operator[](int index) const;

  /* @NOTE: assign values with operator+ */
  Base::String& operator=(Char c);
  Base::String& operator=(Base::String& str);
  Base::String& operator=(Base::String&& str);
  Base::String& operator=(const Base::String& str);
  Base::String& operator=(const CString str);

  /* @NOTE: join with new string using operator+ */
  Base::String operator+(String&& c);
  Base::String operator+(const Char c);
  Base::String operator+(const Base::String& c);
  Base::String operator+(const CString c);

  /* @NOTE: join with new string using operator+= */
  Base::String& operator+=(Base::String&& c);
  Base::String& operator+=(const Char c);
  Base::String& operator+=(const Base::String& c);
  Base::String& operator+=(const CString c);

  /* @NOTE: compare 2 string with operator< */
  Bool operator<(const Base::String& dst) const;
  Bool operator<(Base::String&& dst) const;

  /* @NOTE: compare 2 string with operator== */
  Bool operator==(const CString dst) const;
  Bool operator==(const Base::String& dst) const;
  Bool operator==(Base::String&& dst) const;

  /* @NOTE: compare 2 string with operator== */
  Bool operator!=(const CString dst) const;
  Bool operator!=(const Base::String& dst) const;
  Bool operator!=(Base::String&& dst) const;

  /* @NOTE: create substring */
  String substr(UInt pos, Int len = -1);

  /* @NOTE: swap 2 strings */
  friend void swap(Base::String& str1, Base::String& str2);

 private:
  CString _String;
  Bool _Autoclean;
  UInt _Size;
  Int* _Ref;
};
} // namespace Base

/* @NOTE: join with new string with operator+ */
Base::String operator+(const Char c, Base::String& string);
Base::String operator+(const Char c, Base::String&& string);
Base::String operator+(const CString cstring, Base::String& string);
Base::String operator+(const CString cstring, Base::String&& string);

#ifndef KERNEL
/* @NOTE: push string to std::ostream  with operator<< */
std::ostream& operator<<(std::ostream& cout, Base::String& string);
std::ostream& operator<<(std::ostream& cout, Base::String&& string);

/* @NOTE: pull string from std::istream  with operator<< */
std::istream& operator>>(std::istream& cout, Base::String& string);
#else
#endif  // KERNEL
#endif  // __cplusplus
#endif  // BASE_TYPE_STRING_H

