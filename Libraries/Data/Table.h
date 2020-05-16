#ifndef BASE_TABLE_H_
#define BASE_TABLE_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Base/Auto.h>
#include <Base/Type.h>
#include <Data/Memmap.h>
#else
#include <Auto.h>
#include <Memmap.h>
#include <Type.h>
#endif

namespace Base {
class Table {
 public:
  struct Mapping {
    ULong ByteSize;
    ULong Used;
    UInt  Level;
    ULong* Routings;

    union {
      Any* Keys;
      ULong KeyColumn;
      ULong* Hashing;
    } Indexes;
  };

  struct RecordV1 {
    UInt ByteSize;

    // gcc 6.1+ have a bug where flexible members aren't properly handled
    // https://github.com/google/re2/commit/b94b7cd42e9f02673cd748c1ac1d16db4052514c
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ == 6 && \
      __GNUC_MINOR__ >= 1
    Any Context[0];
#else
    Any Context[];
#endif
  };

  struct RecordV2 {
    UInt ByteSize, ColumnCount;

    // gcc 6.1+ have a bug where flexible members aren't properly handled
    // https://github.com/google/re2/commit/b94b7cd42e9f02673cd748c1ac1d16db4052514c
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ == 6 && \
      __GNUC_MINOR__ >= 1
    Any Context[0];
#else
    Any Context[];
#endif
  };

  explicit Table(String file = "");
  virtual ~Table();

  template<typename Key, typename Value>
  Value& Get(Key UNUSED(key)) {
    throw NoSupport;
  }

  template<typename Key, typename Value>
  ErrorCodeE Put(Key UNUSED(key), Value UNUSED(value)) {
    return ENoError;
  }

 protected:
  ULong Index(ULong hashing);
  ULong Redirect(ULong index);
  Bool Reload();

 private:
  static Bytes Flatten(Mapping& mapping);
  static Mapping* Unflatten(Bytes buffer, ULong size);

  UInt _CountEntries;
  ULong _Maximum;
  Memmap _Memory;
  Mapping** _Entries;
};
}  // namespace Base
#endif  // BASE_TABLE_H_

