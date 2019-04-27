#ifndef DATA_JSON_H_
#define DATA_JSON_H_
#include <Auto.h>
#include <Data.h>
#include <Logcat.h>
#include <Type.h>
#include <Stream.h>
#include <Utils.h>

namespace Base {
class Json: Parser, Generator{
 private:
  enum StageE{
    Initial = 0,
    Revert = 1,
    Release = 2,
    ParseDict = 3,
    GenerateDict = 4,
    ParseList = 5,
    GenerateList = 6,
    BeginString = 7,
    EndString = 8,
    ParseItem = 9,
    ParseKey = 10,
    ParseValue = 11,
    ParseNumber = 12,
    ParseBool = 13
  };

  union Detail {
    struct{
      Bool Value;
      UInt Index;
    } Boolean;

    struct {
      Bool Floating, Sign;
      Double Shift;

      union {
        Double Real;
        Long Integer;
      } Value;
    } Number;

    struct {
      Bool Caching;

      union {
        struct { UInt Begin, End; } v1;
        struct {
          CString Data;
          UInt Size;
        } v2;
      } Value;
    } Sentence;

    Data::Dict* Dict;
    Data::List* List;
  };

 public:
  explicit Json(Shared<Stream> stream);
  explicit Json(String buffer);

  ErrorCodeE operator()();

  template<typename Key, typename Store>
  Store& Get(Key name) {
    if (!_Result) {
      throw Except(EDoNothing, "please do parsing before doing "
                               "anything");
    }

    /* @FIXME: comparing the same const std::type_info& should return True on
     * both Debug and Release modes but not. This is a workaround to overcome
     * the issue from gcc */
    if (_Result.Nametype() == Nametype<Data::List>()) {
      throw Except(ENoSupport,
                   Format{"it seems root was {} but you are admiring"
                          " it as {}"}.Apply(_Result.Nametype(),
                                             Nametype<Data::List>()));
    } else {
      return _Result.Get<Data::Dict>().Get(name).template Get<Store>();
    }
  }

  template<typename Type>
  Type& At(UInt index) {
    if (!_Result) {
      throw Except(EDoNothing, "please do parsing before doing anything");
    }

    if (_Result.Nametype() == Nametype<Data::Dict>()) {
      throw Except(ENoSupport, "it seems root was List but you are "
                               "admiring it as Dict");
    } else {
      return _Result.Get<Data::List>()[index].Get<Type>();
    }
  }

  Void ForEach(Data::Struct& pattern,
               Function<Void(Map<String, Auto>&)> callback);

 protected:
  ErrorCodeE Generate() final;
  ErrorCodeE Record(Byte value) final;
  ErrorCodeE Parse(UInt index, Byte c) final;

 private:
  /* @NOTE: these helpers will support how to switch to next tags */
  Int Select(Byte c, String&& sample, Vector<StageE>&& next_stages);
  Bool Change(StageE stage);
  Bool Insert(StageE stage);

  /* @NOTE: check value type */
  Bool IsBoolean(Byte c);
  Bool IsNumber(Byte c);

  /* @NOTE: register element to pending queueu */
  void RegistryString(UInt index);
  void RegistryNumber(Byte c);
  void RegistryBoolean(Byte c);
  void RegistryDict();
  void RegistryList();

  /* @NOTE: export element to upper */
  ErrorCodeE ExportString();
  ErrorCodeE ExportNumber();
  ErrorCodeE ExportBoolean();
  ErrorCodeE ExportList();
  ErrorCodeE ExportDict();

  ErrorCodeE AddToContent(Any value);
  void AddDetail();
  String Stage();

  /* @NOTE: parameters */
  Bool _Cached;
  Auto _Result;
  Shared<Stream> _Stream;
  Stack<StageE> _Stages;
  Stack<Detail> _Pending;
};
}
#endif  // DATA_JSON_H_
