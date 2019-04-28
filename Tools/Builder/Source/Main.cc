#include <Argparse.h>
#include <Build.h>
#include <Glob.h>
#include <Graph.h>
#include <Json.h>
#include <Macro.h>
#include <Type.h>
#include <Vertex.h>

/* @DESITINATION: Builder 2 will be redevelop again to solve many problem from
 * the old one
 * - First, i think Builder 2 must resolve the big problem of hanging with busy
 * system the architecture from layer parsing to layer languages  are good but
 * the performing layer is very bad and can be hanged forever in some rare
 * cases. Plus, the code must split to 2 cases, single-core and multi-cores.
 * This would increase testing on diferent system and reduce quality code.
 * - Second, i don't realy like the ideal using Python to do everything since i
 * would like to build a decentralize system on the top of Builder. Builder has
 * imitated Bazel to develop an easy and scalable tool, which means this must be
 * run on scaling systems and could help to provide a scaling environment. I really
 * like the ideal of Microsoft/xlang but the design still very clumsy so i would
 * like to announce a new way implementing it.
 * - Third, i don't like the ideal of making a new bridge whenever you want to
 * connect c/c++ to other languages (python, java, ... etc). Why do i have to do
 * that? I would prefer using my farvorit language and do what every i want without
 * coding c/c++ or remember C-API's python so this tool must solve this problem.
 */

using namespace Base;
using namespace Build;

namespace Build{
namespace Internal{
Shared<Build::Redirect> MakeRedirect(String type, Vector<CString> parameters);
} // namespace Internal
} // namespace Build

/* @NOTE: exit codes */
#define FAIL_WITH_REGISTER_PLUGIN_TO_WATCHER 127
#define FAIL_WHEN_ANALYZE_SCRIPT 125
#define FAIL_PERFORMING_RULE 126
#define FAIL_BY_SYSTEM 128
#define EVERYTHING_DONE 0

/* @NOTE: windows and unix use different slashing definition */
#if WINDOW
#define SLASH '\\'
#else
#define SLASH '/'
#endif

/* @NOTE: list of languages we will support */
#define C 0
#define D 1
#define GO 2
#define CPP 3
#define RUBY 4
#define BASH 5
#define SWIFT 6
#define PYTHON 7
#define JAVASCRIPT 8

int main(int argc, const char* argv[]) {
  Vector<String> unparsed;
  ArgParse parser;

  parser.AddArgument<String>("--config", "The schema model of this program");
  parser.AddArgument<Bool>("--rebuild", "This flag is used to force doing "
                                        "from scratch");
  parser.AddArgument<Bool>("--debug", "This flag is used to turn on debug "
                                      "log with a specific level");

  /* @DESIGN: There are core elements of Builder 2:
   * - Plugin: will be used as an abstract layer to cover our extensions
   * - Graph: when the build script is loaded, I will add rule to Graph to
   * manage build steps, since I would like to use a statefull management to
   * gurantee these steps always run on this line:
   * compiling -> (linking, archiving) -> testing
   * - Redirect: provide the way to perform and watching child processes on
   * real-time. It also provide a way to hook scripts before and after each
   * steps to solve many problem during running.
   * - Rule: it just contains helper functions
   *
   * Basic running flow:
   * 1/ load configuration, these must be in JSON type
   * 2/ load Plugins into memory
   * 3/ load build script, plugins will decide which one would be build script
   * or not
   * 4/ check graph if it has any ready rules, we will request watcher to perform
   * these rules on child processes.
   */
  try {
    return Build::Main(parser, parser(argc, argv))();
  } catch (Exception& error) {
    return FAIL_BY_SYSTEM;
  } catch (std::exception& error) {
    ERROR << error.what();
    return FAIL_BY_SYSTEM;
  }
}

Main::Main(ArgParse& parser, Vector<CString> arguments) {
  Data::Struct plugin({
    { "module", Auto::As<String>("") },
    { "name", Auto::As<String>("") },
    { "parameters", Auto::As<String>("") },
    { "language", Auto::As<UInt>(0) },
    { "entry", Auto::As<String>("") }
  });

  Json {parser["config"].Get<String>()}.ForEach(plugin,
    [&](Map<String, Auto>& item){
      UInt language = ToInt(item["language"]);
      String module = ToString(item["module"]);
      String source = ToString(item["entry"]);
      String function = ToString(item["name"]);
      String parameters = ToString(item["parameters"]);

      if (!Reload(module, function, parameters, language, source)) {
        throw Except(EBadLogic,
                     Format{"fail to load {}::{}"}.Apply(module, function));
      }
    });

  _Redirect = Internal::MakeRedirect(parser["director"].Get<String>(),
                                     arguments);
}

Main::~Main(){}

Bool Main::Reload(String module, String name, String parameters,
                  UInt language, String source) {
  Plugin* plugin = None;

  switch(language) {
#if USE_PYTHON
  case PYTHON:
  {
    Base::Python* wrapper = None;

    if (_Plugins.find(module) == _Plugins.end()) {
      _Plugins[module] = (plugin = new PluginN::Python(module));
    }

    if (!(wrapper = Wrapping::Module("Python", module))) {
      auto ok = Wrapping::Create(new Base::Python(name, [=]() -> ErrorCodeE {
        /* @TODO: */
        return ENoError;
      }));

      if (ok) wrapper->Init();
    }

    /* @NOTE: after finish definition, we must register this function to be
     * used as the upper callback */
    if (!plugin->Install(name, Plugin::Function(parameters))) {
      return False;
    } else if (wrapper->Upper(name, parameters)) {
      return False;
    }
    break;
  }
#endif

#if USE_RUBY
  case RUBY:
  {
    Base::Ruby* wrapper = None;

    if (_Plugins.find(module) == _Plugins.end()) {
      _Plugins[module] = (plugin = new PluginN::Ruby(module));
    }

    if (!(wrapper = Wrapping::Module("Python", module))) {
      auto ok = Wrapping::Create(new Base::Ruby(name, [=]() -> ErrorCodeE {
        /* @TODO: */
        return ENoError;
      }));

      if (ok) wrapper->Init();
    }

    /* @NOTE: after finish definition, we must register this function to be
     * used as the upper callback */
    if (!plugin->Install(name, Plugin::Function(parameters))) {
      return False;
    } else if (wrapper->Upper(name, parameters)) {
      return False;
    }
    break;
  }
#endif

#if USE_BASH
  case BASH:
  {
    Base::Bash* wrapper = None;

    if (_Plugins.find(module) == _Plugins.end()) {
      _Plugins[module] = (plugin = new PluginN::Bash(module));
    }

    if (!(wrapper = Wrapping::Module("Bash", module))) {
      auto ok = Wrapping::Create(new Base::Bash(name, [=]() -> ErrorCodeE {
        /* @TODO: */
        return ENoError;
      }));

      if (ok) wrapper->Init();
    }

    plugin->Install(name, Plugin::Function(parameters));
    break;
  }
#endif

#if USE_JAVASCIPT
  case JAVASCIPT:
  {
    Base::Javascript* wrapper = None;

    if (_Plugins.find(module) == _Plugins.end()) {
      _Plugins[module] = (plugin = new PluginN::Javascript(module));
    }

    if (!(wrapper = Wrapping::Module("Javascript", module))) {
      auto ok = Wrapping::Create(new Base::Javascript(name, [=]() -> ErrorCodeE {
        /* @TODO: */
        return ENoError;
      }));

      if (ok) wrapper->Init();
    }

    plugin->Install(name, Plugin::Function(parameters));
    break;
  }
#endif

  case C:
  case D:
  case GO:
  case CPP:
  case SWIFT:
    return False;

  default:
    return False;
  }

  return True;
}

Int Main::operator()() {
  Graph graph;

  /* @NOTE: configure watcher */
  for (auto& plugin: _Plugins) {
    if (!std::get<1>(plugin)->Exist("Hook")) {
      continue;
    } else if (_Redirect->Register(std::get<1>(plugin))) {
      return FAIL_WITH_REGISTER_PLUGIN_TO_WATCHER;
    }
  }

  /* @NOTE: load files from repo, detect build scripts, convert to rules and
   *  perform them */
  for (auto& path: Glob{"./**"}.AsList()) {
    String filename = Cut(path, SLASH, -1);
    ErrorCodeE error = ENoError;

    /* @NOTE: perform analyzing files to build a computing graph on runtime */
    for (auto& plugin: _Plugins) {
      Bool is_build_script{False};

      if (!std::get<1>(plugin)->Exist("IsBuildScript")) {
        continue;
      }

      /* @NOTE: check if this is a build script, we will add them to graph to
       * prepare a new delivery stage */
      is_build_script = std::get<1>(plugin)->Run<Bool>("IsBuildScript", {
        { "path", Auto::As<String>(path) }
      });

      /* @NOTE: perform analyzing step to make sure the script is okey */
      if (is_build_script) {
        error = std::get<1>(plugin)->Run<ErrorCodeE>("Analyze", {
          { "file", Auto::As<String>(path) },
          { "graph", Auto{}.Set<Graph>(graph) }
        });
      } else {
        continue;
      }

      if (error) {
        return FAIL_WHEN_ANALYZE_SCRIPT;
      }

      /* @NOTE: finish parsing the script to rules and load them on to graph */
      break;
    }

    for (auto& ptr: graph.Ready()) {
      /* @NOTE: okey now we must transfer ready rules from graph on to
       * performing threads which are used to perform and watching commands */

      Base::Vertex<void> escaping{[&](){ graph.Lock(); },
                                  [&](){ graph.Unlock(); }};

      if (ptr.expired()){
        throw Except(EBadAccess, "requires a rule that has been removed");
      } else {
        Shared<Rule> rule = ptr.lock();

        /* @NOTE: apply the task may be fail if the system is overload or
         *  unstable. At this time, we must do everything again */
        while (rule && _Redirect->Put(rule) == EDoAgain) {}
      }
    }
  }

  return _Redirect->Code();
}

/* @TODO:
 * - Xay dung Redirect: cho implement tren Redirect.cc
 * - Xay dung Plugin : cho implement tren Plugin.cc va Plugins
 */
