#ifndef BASE_ARGPARSE_H_
#define BASE_ARGPARSE_H_
#include <Auto.h>
#include <Exception.h>
#include <Utils.h>
#include <Type.h>

#if __cplusplus
#include <typeinfo>

namespace Base {
namespace ArgParseN{
template<typename Type>
inline Bool Convert(Auto& UNUSED(content), CString UNUSED(argv)) {
  return !(NoSupport << Format{"type {}"}.Apply(Nametype<Type>()));
}

template<>
Bool Convert<Bool>(Auto& content, CString argv);

template<>
Bool Convert<UInt>(Auto& content, CString argv);

template<>
Bool Convert<ULong>(Auto& content, CString argv);

template<>
Bool Convert<Int>(Auto& content, CString argv);

template<>
Bool Convert<Float>(Auto& content, CString argv);

template<>
Bool Convert<String>(Auto& content, CString argv);
}  // namespace ArgParseN

class ArgParse: std::enable_shared_from_this<ArgParse> {
 private:
  struct Argument {
   public:
    /* @NOTE: converting function */
    Function<Bool(Auto&, CString)> Convert;

    /* @NOTE: information of this Argument */
    String Helper;
    Auto Buffer;
    Bool Untainted;
    Bool Optional;

    Argument(): Untainted{True}{}
  };

 public:
  ArgParse(String program = "", String usage = "",
           String description = "", String epilogue = "",
           Vector<Shared<ArgParse>> parents = Vector<Shared<ArgParse>>{});
  virtual ~ArgParse();

  Auto& operator[](String name);
  Vector<CString> operator()(Int argc, const CString argv[],
                             Bool force_close = True);
  Vector<CString> operator()(Int argc, CString argv[], Bool force_close = True);
  Vector<String> operator()(Int argc, String argv[], Bool force_close = True);

  void Usage();

  template<typename Type>
  ErrorCodeE AddArgument(String name, String help = ""){
    return AddArgument<Type>(name, Auto{Type{}}, False, help);
  }

  template<typename Type>
  ErrorCodeE AddArgument(String name, Auto value, String help = "") {
    return AddArgument<Type>(name, value, True, help);
  }

  template<typename Type>
  ErrorCodeE AddArgument(String name, Auto value,
                         Bool optional = True, String help = "") {
    if (value.Type() == typeid(Type)) {

      if (_Arguments.find(name) == _Arguments.end()) {
        Argument arg{};

        arg.Buffer = value;
        arg.Helper = help;
        arg.Optional = optional;
        arg.Convert = ArgParseN::Convert<Type>;

        _Arguments.insert(std::make_pair(name, arg));
        _Arguments[name].Optional = optional;
        return ENoError;
      } else {
        return (BadLogic << Format{"duplicate `{}`"}.Apply(name)).code();
      }
    }

    return (BadLogic << Format{"value must be {} (actually it's "
                               "{} now)"}.Apply(Nametype<Type>(),
                                                value.Nametype())).code();
  }

 protected:
  Bool Apply(String name, CString value);
  Bool Apply(String name, String value);
  Bool IsParsingOkey();

 private:
  /* @NOTE: usage information */
  String _Program, _Usage, _Description, _Epilogue;

  /* @NOTE: keep track parameters */
  Map<String, Argument> _Arguments;
  Vector<Shared<ArgParse>> _Childs;
};
}  // namespace Base
#endif
#endif  // BASE_ARGPARSE_H_
