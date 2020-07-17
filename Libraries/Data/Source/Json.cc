#include <Json.h>
#include <Vertex.h>
#include <cmath>

using namespace std::placeholders;

namespace Base {
class StringStream: public Stream {
 public:
  StringStream(String buffer):
    Stream{std::bind(&StringStream::WriteToString, this, _1, _2),
           std::bind(&StringStream::ReadFromString, this, _1, _2)},
    _Buffer{buffer},_Index{0}
  {}

  String Cache() final { return _Buffer; }

 private:
  ErrorCodeE ReadFromString(Bytes& buffer, UInt* size) {
    if (_Buffer.size() - _Index < *size) {
      *size = _Buffer.size() - _Index;
    }

    if (_Index >= _Buffer.size()) {
      auto message = Format{"the stream consumed {}/{}"}
                            .Apply(_Index, _Buffer.size());

      return (OutOfRange << message).code();
    }

    memcpy(buffer, _Buffer.c_str() + _Index, *size);
    _Index += *size;
    return ENoError;
  }

  ErrorCodeE WriteToString(Bytes&& buffer, UInt* size) {
    _Buffer += String{(char*)buffer, *size};
    return ENoError;
  }

  String _Buffer;
  UInt _Index;
};

Json::Json(Shared<Stream> stream): _Stream{stream} {
  try {
    _Stream->Cache();
    _Cached = True;
  } catch(Base::Exception& error) {
    error.Ignore();
    _Cached = False;
  }

  _Stages.push(Initial);
}

Json::Json(String buffer): _Cached{True} {
  _Stream = Shared<Stream>{new StringStream(buffer)};
  _Stages.push(Initial);
}

Void Json::ForEach(Data::Struct& pattern,
                   Function<Void(Map<String, Auto>&)> callback) {
  auto item = pattern.Mapping();
  auto parser = [](Auto& contain, Map<String, Auto>& output) -> Bool {
    if (contain.Type().hash_code() == typeid(Data::Dict).hash_code()) {
      auto& dict = contain.Get<Data::Dict>();

      for (auto& elm: output) {
        auto& key = std::get<0>(elm);
        auto& val = std::get<1>(elm);

        if (!dict.Find(key)) {
          return False;
        } else if (!dict.Find(key, val.Type())) {
          return False;
        } else {
          val.Reff(dict.Get(key));
        }
      }

      return True;
    }

    return False;
  };

  /* @NOTE: fetch data and line up them on to the callback */
  if (_Result.Type().hash_code() == typeid(Data::List).hash_code()) {
    auto& list = _Result.Get<Data::List>();

    for (UInt i = 0; i < list.Size(); ++i) {
      auto& dict = list[i];

      if (parser(dict, item)) {
        callback(item);
      }
    }
  } else if (parser(_Result, item)) {
    callback(item);
  }
}

ErrorCodeE Json::Generate() {
  return NoSupport("FIXME: this code still underdevelop").code();
}

ErrorCodeE Json::Record(Byte c) {
  if (_Stages.top() == BeginString) {
    if (!_Cached) {
      auto& str = _Pending.top().Sentence.Value.v2;

      if (str.Data) {
        auto tmp = (CString)ABI::Realloc(str.Data, str.Size + 1);

        if (!tmp) {
          return (DrainMem << "caused by ABI::Realloc()").code();
        }

        str.Data = tmp;
        tmp[str.Size] = '\0';
        tmp[str.Size - 1] = c;
        str.Size++;
      } else {
        str.Data = (CString) ABI::Calloc(2, sizeof(Byte));

        if (!str.Data) {
          return (DrainMem << "caused by ABI::Calloc()").code();
        }

        str.Data[0] = c;
        str.Data[1] = '\0';
      }
    } else {
      _Pending.top().Sentence.Value.v1.End++;
    }
  } else if (_Stages.top() == ParseNumber) {
    if (!IsNumber(c)) {
      return BadLogic.code();
    }

    if (c == '-' || c == '+') {
      return (NoSupport << "redefine sign").code();
    } else if (c == '.') {
      Double tmp = Double(_Pending.top().Number.Value.Integer);

      _Pending.top().Number.Value.Real = tmp;
      _Pending.top().Number.Floating = True;
      _Pending.top().Number.Shift = 0.1;
    } else if (_Pending.top().Number.Floating) {
      Double shift = _Pending.top().Number.Shift;

      _Pending.top().Number.Value.Real += shift*(c - '0');
      _Pending.top().Number.Shift *= 0.1;
    } else {
      _Pending.top().Number.Value.Integer *= 10;
      _Pending.top().Number.Value.Integer += c - '0';
    }
  } else if (_Stages.top() == ParseBool) {
    auto& boolean = _Pending.top().Boolean;
    auto sample = "False";

    if (boolean.Value) {
      sample = "True";
    }

    if (sample[boolean.Index] != c) {
      return (BadLogic << Format{"requires {} but have {}"}
                            .Apply(sample[boolean.Index], c)).code();
    } else {
      boolean.Index++;
    }
  } else {
    return (NoSupport << Stage()).code();
  }

  return ENoError;
}

Int Json::Select(Byte c, String&& sample, Vector<StageE>&& next_stages) {
  for (UInt i = 0; i < sample.size(); ++i) {
    if (c == sample[i]) {
      if (Insert(next_stages[i])) {
        return i;
      } else if (Change(next_stages[i])) {
        return i;
      } else {
        throw Except(EBadLogic,
                     Format{"can\'t switch to stage {}"} << next_stages[i]);
      }
    }
  }

  return -1;
}

Bool Json::Insert(Json::StageE stage) {
  auto current = _Stages.top();

  switch(stage) {
  case ParseBool:
  case ParseNumber:
  case BeginString:
    switch(current) {
    case ParseItem:
    case ParseKey:
    case ParseValue:
      break;

    default:
      return False;
    }
    break;

  case ParseDict:
  case ParseList:
    switch(current) {
    case Release:
    case ParseItem:
    case ParseValue:
      break;

    default:
      return False;
    }
    break;

  default:
    return False;
  }

  _Stages.push(stage);
  return True;
}

Bool Json::Change(Json::StageE stage) {
  auto current = _Stages.top();

  /* @NOTE: remove lastest */
  _Stages.pop();

  if (stage == Revert) {
    switch(_Stages.top()) {
    case Release:
      switch(current){
      case GenerateDict:
      case GenerateList:
        return True;

      default:
        return False;
      }

    case ParseKey:
      switch(current) {
      case EndString:
      case ParseBool:
      case ParseNumber:
        return True;

      default:
        return False;
      }

    case ParseValue:
    case ParseItem:
      switch(current) {
      case GenerateDict:
      case GenerateList:
      case EndString:
      case ParseBool:
      case ParseNumber:
        return True;

      default:
        return False;
      }

    default:
      return False;
    }
  } else {
    _Stages.push(stage);
  }

  return True;
}

ErrorCodeE Json::Parse(UInt index, Byte c) {
  ErrorCodeE error{};
  StageE stage{};

  try {
    while ((stage = _Stages.top()) != Release) {
      Vector<StageE> next{};
      String samples{};

      switch(stage) {
      case Initial:
        next = {stage, stage, stage, ParseDict, ParseList};

        /* @NOTE: switch from Initial to Release just to make sure that
         * everything will come to this ending point */
        Change(Release);

        /* @NOTE: select the next stage */
        if (Select(c, " \t\n{[", RValue(next)) < 0) {
          return (NotFound << "stage Initial is looking open dict `{` or "
                           << (Format("list `[` but not `{}` ") << c)).code();
        }
        continue;

      case Revert:
        return (NoSupport << "stage Revert only happen during calling "
                             "method `Change`").code();

      case ParseDict:
        RegistryDict();
        break;

      case ParseKey:
        /* @NOTE: after an element finish its circle life, a new one will be
         * created here */

        next = {stage, stage, stage, ParseValue, BeginString};
        samples = " \t\n:\"";
        goto parse_dict;

      case ParseValue:
        /* @NOTE: after we finish parsing key, we are going to parse
         * dict's value */

        next = {stage, stage, stage, ParseKey, GenerateDict,
                BeginString, ParseList, ParseDict};
        samples = " \t\n,}\"[{";

       parse_dict:
        if (Select(c, RValue(samples), RValue(next)) < 0) {
          /* @NOTE: maybe we can receive number or boolean value so we will
           * handle it here */

          if (IsNumber(c)) {
            RegistryNumber(c);
          } else if (IsBoolean(c)) {
            RegistryBoolean(c);
          } else {
            return (BadLogic << "Found unknown value").code();
          }
        }

        if (_Stages.top() != GenerateDict) {
          if (_Stages.top() == BeginString) {
            RegistryString(index);
          }

          if (_Stages.top() == ParseDict || _Stages.top() == ParseList) {
            continue;
          }
          break;
        }


      case GenerateDict:
        /* @NOTE: check if the current stage is GenerateDict, we must deliver
         * the new dict now */

        if (stage != GenerateDict) {
          if ((error = ExportDict())) {
            return error;
          }
        } else {
          return (NoSupport << "GenerateDict only support on switching from "
                               "another stage").code();
        }
        break;

      case ParseList:
        RegistryList();
        break;

      case ParseItem:
        next = {stage, stage, stage, ParseItem, GenerateList, BeginString,
                ParseDict, ParseList};

        if (Select(c, " \t\n,]\"{[", RValue(next)) < 0) {
          /* @NOTE: maybe we can receive number or boolean value so we will
           * handle it here */

          if (IsNumber(c)) {
            RegistryNumber(c);
          } else if (IsBoolean(c)) {
            RegistryBoolean(c);
          } else {
            return (BadLogic << "Found unknown value").code();
          }
        }

        if (_Stages.top() != GenerateList) {
          if (_Stages.top() == BeginString) {
            RegistryString(index);
          }

          if (_Stages.top() == ParseDict || _Stages.top() == ParseList) {
            continue;
          }
          break;
        }

      case GenerateList:
        /* @NOTE: check if the current stage is GenerateList, we must deliver
         * the new dict now */

        if (stage != GenerateDict) {
          if ((error = ExportList())) {
            return error;
          }
        } else {
          return (NoSupport << "GenerateList only support on switching from "
                               "another stage").code();
        }
        break;

      case BeginString:
        /* @NOTE: start new string here */

        if (Select(c, "\"", {EndString})) {
          if ((error = Record(c))) {
            return error;
          } else {
            break;
          }
        }

      case EndString:
        /* @NOTE: deliver string to source code */

        if ((error = ExportString())) {
          return error;
        }
        break;

      case ParseNumber:
        if (IsNumber(c)) {
          if ((error = Record(c))) {
            return error;
          }
        } else if ((error = ExportNumber())) {
          return error;
        } else {
          continue;
        }
        break;

      case ParseBool:
      {
        Bool done = False;

       recheck:
        if (_Pending.top().Boolean.Value) {
          if (_Pending.top().Boolean.Index >= sizeof("true") - 1) {
            if ((error = ExportBoolean())) {
              return error;
            }
          }
        } else if (_Pending.top().Boolean.Index >= sizeof("false") - 1) {
          if ((error = ExportBoolean())) {
            return error;
          }
        }

        if (!done) {
          done = True;

          if (IsBoolean(c)) {
            if ((error = Record(c))) {
              return error;
            }
          } else {
            return BadLogic.code();
          }

          goto recheck;
        }
        break;
      }

      default:
        break;
      }

      return ENoError;
    }
  } catch(Base::Exception& error) {
    return error.code();
  }
  return ENoError;
}

ErrorCodeE Json::operator()(){
  ErrorCodeE error;
  Byte character;

  for (UInt index = 0; _Stages.top() != Release; ++index) {
    error = _Stream->ReadBlock(&character, sizeof(character));

    if (error) {
      return error;
    } else if ((error = Parse(index, character))) {
      return error;
    }
  }

  return ENoError;
}

Bool Json::IsBoolean(Byte c) {
  if (_Stages.top() == ParseBool) {
    auto sample = "False";

    if (_Pending.top().Boolean.Value) {
      sample = "True";
    }

    if (sample[_Pending.top().Boolean.Index] == c) {
      return True;
    } else {
      return False;
    }
  } else {
    return strchr("tTfF", c);
  }
}

Bool Json::IsNumber(Byte c) {
  return strchr("+-0123456789.", c);
}

void Json::RegistryString(UInt index) {
  if (_Stages.top() == BeginString) {
    AddDetail();
    _Pending.top().Sentence.Caching = _Cached;

    if (_Cached) {
      _Pending.top().Sentence.Value.v1.Begin = index + 1;
      _Pending.top().Sentence.Value.v1.End = index + 1;
    }
  }
}

void Json::RegistryNumber(Byte c) {
  if (_Stages.top() != ParseNumber) {
    AddDetail();

    _Pending.top().Number.Sign = True;
    if (c == '.') {
      _Pending.top().Number.Floating = True;
      _Pending.top().Number.Value.Real = 0.0;
    } else if (c == '-') {
      _Pending.top().Number.Sign = False;
    } else if (c == '+') {
      _Pending.top().Number.Sign = True;
    } else {
      _Pending.top().Number.Floating = False;
      _Pending.top().Number.Value.Integer = c - '0';
    }

    _Stages.push(ParseNumber);
  }
}

void Json::RegistryBoolean(Byte c) {
  if (_Stages.top() != ParseBool) {
    AddDetail();

    _Pending.top().Boolean.Value = strchr("tT", c) != None;
    _Pending.top().Boolean.Index = 1;
    _Stages.push(ParseBool);
  }
}

void Json::RegistryDict() {
  if (_Stages.top() == ParseDict) {
    AddDetail();

    _Pending.top().Dict = new Data::Dict{};
    if (!Change(ParseKey)) {
      throw Except(EBadLogic, "problem with ParseDict -> ParseKey");
    }
  }
}

void Json::RegistryList() {
  if (_Stages.top() == ParseList) {
    AddDetail();

    _Pending.top().List = new Data::List{};
    if (!Change(ParseItem)) {
      throw Except(EBadLogic, "problem with ParseList -> ParseItem");
    }
  }
}

ErrorCodeE Json::ExportString() {
  auto& str = _Pending.top().Sentence;
  auto result = Any{};

  Vertex<void> escaping([&]() { }, [&]() { BSNone2Any(&result); });

  if (str.Caching) {
    auto begin = str.Value.v1.Begin;
    auto end = str.Value.v1.End;
    auto value = new String{_Stream->Cache().c_str() + begin,
                            end - begin};

    /* @NOTE: assign String to Any */
    BSAssign2Any(&result, value, CTYPE(String),
                 [](void* content) { delete (String*)content; });
  } else if (str.Value.v2.Data) {
    auto value = new String{str.Value.v2.Data, str.Value.v2.Size};

    /* @NOTE: assign String to Any */
    BSAssign2Any(&result, value, CTYPE(String),
                 [](void* content) { delete (String*)content; });
  } else {
    return (BadLogic << "can\'t export a None to String").code();
  }

  if (!Change(Revert)) {
    return (BadAccess << "can\'t revert " << Stage()).code();
  } else {
    _Pending.pop();
  }

  return AddToContent(result);
}

ErrorCodeE Json::ExportNumber() {
  auto& num = _Pending.top().Number;
  auto result = Any{};

  Vertex<void> escaping([&]() { }, [&]() { BSNone2Any(&result); });

  if (num.Floating) {
    Double sign = num.Sign? 1.0: -1.0;

    BSAssign2Any(&result,
                 new Double{sign*num.Value.Real},
                 CTYPE(Double),
                 [](void* content) { delete (Double*)content; });
  } else {
    Long sign = num.Sign? 1: -1;

    BSAssign2Any(&result,
                 new Long{sign*num.Value.Integer},
                 CTYPE(Long),
                 [](void* content) { delete (Long*)content; });
  }

  if (!Change(Revert)) {
    return (BadAccess << "can\'t revert from {}").code();
  } else {
    _Pending.pop();
  }

  return AddToContent(result);
}

ErrorCodeE Json::ExportBoolean() {
  auto result = Any{};

  Vertex<void> escaping([&]() { }, [&]() { BSNone2Any(&result); });

  /* @NOTE: assign Bool to Any */
  BSAssign2Any(&result,
               new Bool{_Pending.top().Boolean.Value},
               CTYPE(Bool),
               [](void* content) { delete (Bool*)content; });

  if (!Change(Revert)) {
    return (BadAccess << "can\'t revert from {}").code();
  } else {
    _Pending.pop();
  }

  return AddToContent(result);
}

ErrorCodeE Json::ExportList() {
  Data::List* list = _Pending.top().List;
  Any result = Any{};

  Vertex<void> escaping([&]() { }, [&]() { BSNone2Any(&result); });

  /* @NOTE: revert to previous stage */
  if (!Change(Revert)) {
    return (BadAccess << "can\'t revert from {}").code();
  } else {
    _Pending.pop();
  }

  /* @NOTE: assign List to Any */
  BSAssign2Any(&result, list, CTYPE(Data::List),
               [](void* content){ delete (Data::List*)content; });
  return AddToContent(result);
}

ErrorCodeE Json::ExportDict() {
  Data::Dict* dict = _Pending.top().Dict;
  Any result = Any{};

  Vertex<void> escaping([&]() { }, [&]() { BSNone2Any(&result); });

  /* @NOTE: revert to previous stage */
  if (!Change(Revert)) {
    return (BadAccess << "can\'t revert from {}").code();
  } else {
    _Pending.pop();
  }

  /* @NOTE: assign Dict to Any */
  BSAssign2Any(&result, dict, CTYPE(Data::Dict),
               [](void* content){ delete (Data::Dict*)content; });
  return AddToContent(result);
}

ErrorCodeE Json::AddToContent(Any value){
  switch(_Stages.top()){
  case ParseItem:
    _Pending.top().List->Insert(value);
    break;

  case ParseKey:
    return _Pending.top().Dict->InsertKey(value);

  case ParseValue:
    return _Pending.top().Dict->InsertValue(value);

  case Release:
    _Result = Auto::FromAny(value);

    if (!_Result) {
      return BadLogic("release requires value").code();
    }
    break;

  default:
    return (NoSupport << Stage()).code();
  }

  return ENoError;
}

void Json::AddDetail(){
  Detail value{};

  memset(&value, 0, sizeof(Detail));
  _Pending.push(value);
}

String Json::Stage(){
  switch(_Stages.top()) {
  case Initial:
    return "Initial";

  case Revert:
    return "Revert";

  case Release:
    return "Release";

  case ParseDict:
    return "ParseDict";

  case GenerateDict:
    return "GenerateDict";

  case ParseList:
    return "ParseList";

  case GenerateList:
    return "GenerateList";

  case BeginString:
    return "BeginString";

  case EndString:
    return "EndString";

  case ParseItem:
    return "ParseItem";

  case ParseKey:
    return "ParseKey";

  case ParseValue:
    return "ParseValue";

  case ParseNumber:
    return "ParseNumber";

  case ParseBool:
    return "ParseBool";
  }
  return "Unknown";
}
}  // namespace Base
