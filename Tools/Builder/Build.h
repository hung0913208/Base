#ifndef BUILD_LIBBUILD_H_
#define BUILD_LIBBUILD_H_
#include <Argparse.h>
#include <Graph.h>
#include <Utils.h>
#include <Type.h>

namespace Build{
class Plugin: std::enable_shared_from_this<Plugin> {
 public:
  struct Function {
   public:
    explicit Function(String language, String module, String function,
                      Vector<std::type_info*> parameters);

   private:
    Vector<Base::Auto> _Parameter;
    String _Language;
  };

  virtual ~Plugin();

  Bool Exist(String function);
  Bool Install(String name, Plugin::Function& function);

  template<typename Result>
  Result Run(String function, Map<String, Base::Auto> parameters) {
    using namespace Base;

    if (!Exist(function)) {
      throw Except(ENotFound, function);
    } else {
      Auto result{};
      Error error{Run(function, parameters, result)};

      if (error) {
        throw Base::Exception(error);
      } else if (result.Type() == typeid(Result)) {
        auto message = Format{"function {} has return {} but you require {}"}
                  .Apply(function, result.Nametype(), Nametype<Result>());
        throw Except(EBadLogic, message);
      } else {
        return result.Get<Result>();
      }
    }
  }

 protected:
  virtual Base::Error Run(String& function,
                          Map<String, Base::Auto>& parameters,
                          Base::Auto& result);

 private:
  Map<String, Function*> _Functions;
};

class Redirect{
 public:
  enum StatusE {
    EClosed = 0,
    EGood = 1,
    EBusy = 2,
    EOverload = 3,
    EUnstable = 4
  };

  virtual ~Redirect();

  ErrorCodeE Register(Shared<Plugin> plugin);
  ErrorCodeE RunAtHere(Shared<Base::Rule> rule);
  ErrorCodeE Put(Shared<Base::Rule> rule);
  ErrorCodeE Code();

 protected:
  /* @NOTE: these methods are used to control jobs to nodes. There are
   *  2 types of node: local nodes and remote node. When our system is
   *  very busy, it will transit the job to another nodes*/
  virtual ErrorCodeE Consume(Shared<Base::Rule> rule);
  virtual ErrorCodeE Transit(Shared<Base::Rule> rule);

  /* @NOTE: this method is used to broadcast the new job to nodes and
   *  to make sure that everyone knows the task they would have done.
   *  Of couse, these jobs are taged to be reported after finishing */
  virtual ErrorCodeE Broadcast(Shared<Base::Rule> rule);

  /* @NOTE: this method is used to broadcast the finishing job to nodes */
  virtual ErrorCodeE Done(Shared<Base::Rule> rule);

  /* @NOTE: these methods are used to manage decentralized systems */
  virtual ErrorCodeE Heartbeat(String uri);
  virtual ErrorCodeE Join(String uri);
  virtual ErrorCodeE Depart(String uri);

  /* @NOTE: these methods are used to lock and unlock decentralized systems */
  virtual ErrorCodeE Lock(String uri);
  virtual ErrorCodeE Unlock(String uri);

  /* @NOTE: this callback is used to get status of a node */
  virtual StatusE Status();

 private:
  Vector<Base::Pair<String, Int>> _Status;
  Queue<Shared<Base::Rule>> _Jobqueue;
  ErrorCodeE _Code;
};

class Main{
 public:
  Main(Base::ArgParse& parser, Vector<CString> parameters);
  ~Main();

  int operator()();

 protected:
  Bool Reload(String module, String name, String parameters,
              UInt language, String entry);

 private:
  Map<String, Shared<Plugin>> _Plugins;
  Shared<Redirect> _Redirect;
};
}  // namespace Build
#endif  // BUILD_MAIN_H_
