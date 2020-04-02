#include <Argparse.h>

namespace Base {
namespace ArgParseN{
template<>
Bool Convert<Bool>(Auto& content, CString argv) {
  if (!argv) {
    return False;
  } else {
    String value{argv};

    if (value == "True" || value == "true" || value != "0") {
      content.Set(True);
    } else if (value == "False" || value == "false" || value == "0") {
      content.Set(False);
    } else {
      return !(BadLogic << Format{"can\'t recognize `{}`"}.Apply(argv));
    }

    return True;
  }
}

template<>
Bool Convert<UInt>(Auto& content, CString argv) {
  if (!argv) {
    return False;
  } else {
    content.Set(UInt(Base::ToInt(argv)));
    return True;
  }
}

template<>
Bool Convert<ULong>(Auto& content, CString argv){
  if (!argv) {
    return False;
  } else {
    content.Set(ULong(Base::ToInt(argv)));
    return True;
  }
}

template<>
Bool Convert<Int>(Auto& content, CString argv){
  if (!argv) {
    return False;
  } else {
    content.Set(Base::ToInt(argv));
    return True;
  }
}

template<>
Bool Convert<Float>(Auto& content, CString argv){
  if (!argv) {
    return False;
  } else {
    content.Set(ToFloat(argv));
    return True;
  }
}

template<>
Bool Convert<Double>(Auto& content, CString argv){
  if (!argv) {
    return False;
  } else {
    content.Set(Double(ToFloat(argv)));
    return True;
  }
}

template<>
Bool Convert<String>(Auto& content, CString argv){
  if (!argv) {
    return False;
  } else {
    content.Set(ToString(argv));
    return True;
  }
}

void ExitWithError(String error){
  ERROR << error << Base::EOL;
  exit(-1);
}
}  // namespace ArgParseN

ArgParse::ArgParse(String program, String usage, String description,
                   String epilogue, Vector<Shared<ArgParse>> parents):
    _Program{program}, _Usage{usage}, _Description{description},
    _Epilogue{epilogue} {
  for (auto& item: parents) {
    item->_Childs.push_back(shared_from_this());
  }
}

ArgParse::~ArgParse(){ }

void ArgParse::Usage() {
  UInt max_length_of_defintion{0};

  INFO << "Program: " << _Program << Base::EOL;

  /* @NOTE: show description if it needs */
  if (_Description.size() > 0) {
    INFO << "\t" << _Description << Base::EOL;
  }

  /* @NOTE: add a new space line */
  INFO << Base::EOL;

  /* @NOTE: calculate max right margin of definitions */
  for (auto& item: _Arguments) {
    auto& definition = std::get<0>(item);

    if (max_length_of_defintion < definition.size()) {
      max_length_of_defintion = definition.size();
    }
  }

  max_length_of_defintion += 2;

  /* @NOTE: show full parameters */
  for (auto& item: _Arguments) {
    auto& definition = std::get<0>(item);
    auto& argument = std::get<1>(item);

    /* @NOTE: show defintion */
    INFO << "\t" << definition;

    for (UInt i = 0; i < max_length_of_defintion - definition.size(); ++i) {
        INFO << " ";
    }
    
    /* @NOTE: show helper */
    INFO << argument.Helper;

    /* @NOTE: show this argument is optional or not*/
    if (argument.Optional) {
      INFO << " (optional)";
    }

    INFO << Base::EOL;
  }

  if (_Epilogue.size() > 0) {
    INFO << _Epilogue << Base::EOL;
  }
}

Auto& ArgParse::operator[](String name){
  if (_Arguments.find(name) != _Arguments.end()) {
    return _Arguments.at(name).Buffer;
  } else {
    throw Except(ENotFound, Format{"argument {}"}.Apply(name));
  }
}

Vector<CString> ArgParse::operator()(Int argc, const CString argv[],
                                     Bool force_close) {
  return this->operator()(argc, const_cast<CString*>(argv),
                          force_close);
}

Vector<String> ArgParse::operator()(Int argc, String argv[],
                                    Bool force_close){
  Vector<String> result{};

  if (_Program.size() == 0) {
    _Program = argv[0];
  }

  for (auto i = 1; i < argc; i += 2) {
    Bool done{False};
    String name{argv[i]};
    String value{argv[i + 1]};

    if (name[0] == '-' && !(name.size() > 2 && name[1] != '-')) {
      if (!Apply(name, value)) {
      /* @NOTE: adapt value to its childs */

        for (auto it = _Childs.begin(); it != _Childs.end() && !done; it++) {
          if ((*it)->_Arguments.find(name) == (*it)->_Arguments.end()){
            continue;
          } else if ((*it)->Apply(name, value)) {
            done = True;
          } else {
            ArgParseN::ExitWithError("Converter got unexpected error");
          }
        }
      }
    }

    /* @NOTE: saved unparsed value since it will be parsed manually */
    if (!done) {
      result.push_back(name);
      result.push_back(value);
    }
  }

  if (!IsParsingOkey()){

    if (force_close) {
      Usage();
      ArgParseN::ExitWithError("Please check your command again "
                               "with the instruction above");
    } else {
      throw Except(EBadLogic, "Please check your command again");
    }
  }

  return result;
}

Vector<CString> ArgParse::operator()(Int argc, CString argv[],
                                     Bool force_close){
  Vector<CString> result{};

  if (_Program.size() == 0) {
    _Program = ToString(argv[0]);
  }

  for (auto i = 1; i < argc; i += 2) {
    Bool done{False};
    CString name{const_cast<CString>(argv[i])};
    CString value{const_cast<CString>(argv[i + 1])};

    if (name[0] == '-' && !(strlen(name) > 2 && name[1] != '-')) {
      if (!Apply(name, value)) {
      /* @NOTE: adapt value to its childs */

        for (auto it = _Childs.begin(); it != _Childs.end() && !done; it++) {
          if ((*it)->_Arguments.find(name) == (*it)->_Arguments.end()){
            continue;
          } else if ((*it)->Apply(name, value)) {
            done = True;
          } else {
            ArgParseN::ExitWithError("Converter got unexpected error");
          }
        }
      }
    }

    /* @NOTE: saved unparsed value since it will be parsed manually */
    if (!done) {
      result.push_back(name);
      result.push_back(value);
    }
  }

  if (!IsParsingOkey()){
    if (force_close) {
      Usage();
      ArgParseN::ExitWithError("Please check your command again "
                               "with the instruction above");
    } else {
      throw Except(EBadLogic, "Please check your command again");
    }
  }

  return result;
}

Bool ArgParse::Apply(String name, String value) {
  using namespace std;

  for (auto& item: _Arguments) {
    if (get<0>(item) == name) {
      if (!get<1>(item).Convert(get<1>(item).Buffer,
                                const_cast<CString>(value.c_str()))) {
        return False;
      } else {
        get<1>(item).Untainted = False;
      }
    }
  }

  return True;
}

Bool ArgParse::Apply(String name, CString value) {
  using namespace std;

  for (auto& item: _Arguments) {
    if (get<0>(item) == name) {
      if (!get<1>(item).Convert(get<1>(item).Buffer, value)) {
        return False;
      } else {
        get<1>(item).Untainted = False;
      }
    }
  }

  return True;
}

Bool ArgParse::IsParsingOkey(){
  for (auto& item: _Arguments) {
    if (std::get<1>(item).Untainted && !std::get<1>(item).Optional) {
      return False;
    }
  }

  for (auto& child: _Childs) {
    if (!child->IsParsingOkey()) {
      return False;
    }
  }

  return True;
}
}  // namespace Base
