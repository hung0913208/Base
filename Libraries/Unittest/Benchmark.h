#ifndef BASE_BENCHMARK_H_
#define BASE_BENCHMARK_H_
#include <Config.h>

#if USE_BASE_WITH_FULL_PATH_HEADER
#include <Unittest/Unittest.h>
#else
#include <Unittest.h>
#endif

#if __cplusplus
namespace Base {
namespace Unit {
class Problem {
 public:
  using Input = Vector<Pair<String, Auto>>;
  using Output = Auto;

  explicit Problem(String problem);
  virtual ~Problem();

  inline UInt Start() {
    auto session = INC(&_Count);

    _Sessions[session] = Pair<UInt, UInt>(0, False);
    return session; 
  }

  inline ErrorCodeE Pick(UInt session, Pair<Int, Input>& result) {
    if (_Sessions.find(session) != _Sessions.end()) {
      if (_Session[session].Right) {
        return EDoNothing;
      }

      if (_Cases.find(_Sessions[session].Left) != _Cases.end()) {
        auto input = _Cases[_Sessions[session].Left];

        result = Pair<UInt, Input>(_Sessions[session].Left - 1, input);
        _Sessions[session].Left++;
      } else {
        /* @TODO: pick input using range-matrix so we could generate
         * many test case but we can't generate output if we don't have
         * specific instruction here */

        return EDoNothing;
      }

      return ENoError;
    }

    return EBadAccess;
  }

  inline ErrorCodeE Verify(UInt step, Output& output) {
    if (_Cases.find(step) != _Cases.end()) {
      if (output.Type() == _Cases[step].Right.Type()) {
        return _Cases[step].Right == output? ENoError: EBadLogic;
      } else {
        return EBadLogic;
      }
    } else {
      return EBadAccess;
    }
  }

  inline Bool Stop(UInt session) { 
    if (_Sessions.find(session) != _Sessions.end()) {
      _Sessions[session].Right = True;
      return True;
    } else {
      return False;
    }
  }

  inline Problem& Load(String file) {
    Input input{};
    Output output{};
    Error error{Problem::ParseFromFile(file, input, output)};

    if (error) {
      throw Exception(error);
    } else {
      _Cases.push_back(Pair(input, output));
    }

    return *this;
  }

  template<typename Result, typename ...Inputs>
  inline Problem& Case(Result result, Inputs... inputs) {
    auto input = Problem::PackToVector(this, args...);
    auto output = Base::Auto::As(result);

    _Cases.push_back(Pair(input, output));
    return *this;
  }

  static Shared<Problem> Make(String problem);

 protected:
  virtual void Define();

 private:
  static Error ParseFromFile(String file, Input& input, Output& output);

  template<typename ...Args>
  static Vector<Auto> PackToVector(const Args&... args) {
    Vector<Auto> result{};

    _PackToVector(result, args...);
    return result;
  }

  template<typename T, typename ...Args>
  static void _PackToVector(Vector<Auto>& result, const T &value,
                            const Args&... args) {
    _PackToVector(result, value);

    if (sizeof...(args) >= 1) {
      _PackToVector(result, args...);
    }
  }

  template<typename T>
  static void _PackToVector(Vector<Auto>& result, const T &value) {
    result.push_back(Auto::As(value));
  }

  Map<UInt, Pair<UInt, Bool>> _Sessions;
  Vector<Pair<Input, Output>> _Cases;
  String _Problem;
  UInt _Count;
};

class Benchmark: Case {
 public:
  explicit Benchmark(String problem, String resolve): Case(problem, resolve);
  virtual ~Benchmark();

 protected:
  inline void Define() final {
    auto problem = Problem::Make(_Suite);

    if (problem) {
      UInt fail{0};
      UInt session{problem->Start()};
      ErrorCodeE error{ENoError};
      Pair<Int, Problem::Input> step{};

      while (!(error = problem->Pick(session, step))) {
        /* @NOTE: load our input base on the input template of our problem
         * we will fill out our arguments and let the playground doing its
         * task to solve the problem. We must run this on main-thread only
         * to prevent unexpect behavious */

        for (auto& param : step.Right) {
          arguments[param.Left] = param.Right;
        }

        /* @NOTE: perform our solution to find the outcome. It should be safe
         * to define a problem with a limited time execution so with it we
         * could choose fast enough solutions to verify */

        if (_Capacity > 1) {
          for (auto i = 0; i < _Capacity; ++i) {
            RunInThread(i);
          }

          if (problem->Timeout() > 0) {
            TIMEOUT(problem->Timeout(), { WaitUntilEnd(); });
          } else {
            WaitUntilEnd();
          }
        } else if (problem->Timeout() > 0) {
          TIMEOUT(problem->Timeout(), { RunInThread(0); });
        } else {
          RunInThread(0);
        }
 
        /* @NOTE: if we reach here, it would mean that our solution is passing
         * within a limited time execution and we can verify it with our test
         * case here */

        if (step.Left >= 0) {
          if (Verify(step.Left, _Result)) {
            /* @TODO: well we fail this test step, report it right here */

            fail++;
          }
        } else {
          /* @TODO: the test case we are working here is auto-generated so we
           * can't do anything to verify the outcome. We should relay mainly
           * to reviewers to verify it */
        }
      }

      if (error != EDoNothing || fail > 0) {
        Fail();
      } else {
        Pass();
      }
    } else {
      /* @NOTE: we don't define any problem to use this solution so we can't
       * verify it or doing anything else except ignore this case */

      Ignore(True);
    }
  }

  inline Bool Footer() final {
    /* @NOTE: check status of this test and present it to console */

    if (Status() == Base::Unit::EFail) {
      VLOG(EInfo) << RED(FAIL_LABEL) << " " << Information()
                  << Base::EOL;
    } else if (Status() == Base::Unit::EFatal) {
      VLOG(EInfo) << RED(FAIL_LABEL) << " " << Information()
                  << Base::EOL;
    } else if (Status() == Base::Unit::EIgnored) {
      VLOG(EInfo) << YELLOW(IGNORED_LABEL) << " " << Information()
                  << Base::EOL;
    } else {
      VLOG(EInfo) << GREEN(PASS_LABEL) << " " << test->Information()
                  << Base::EOL;
    }

    return True;
  }

  virtual void Playground(Map<String, Auto>& arguments, Auto& result);

 private:
  static void RunInThread(MInt index);
  static int Complexity();

  Map<String, Auto>> _Arguments;
  Vector<Auto> _Results;
  UInt _Capacity;
};
} // namespace Unit
} // namespace Base

/* @NOTE: this macro is used to define a problem with specific test cases
 * and configuration like time execution and input range so the benchmark
 * can base on these to define benchmarking test cases */
#define PROBLEM(Name)                                                  \
  namespace Algorithm {                                                \
  class Name : public Base::Unit::Problem {                            \
   public:                                                             \
    Name() : Case{#Name} {}                                            \
                                                                       \
    void Define() final;                                               \
  };                                                                   \
  } /* namespace Algorithm */                                          \
  void Algorithm::Problem::Define()

/* @NOTE: this macro is used to define a solution to solve a problem and all
 * solution of a problem is grouped within a namespace. We should define our
 * bechmark code similar like this here:
 * -------------------------------------------------------------------------
 *
 * >> BENCHMARK(ProblemA, Solution1) {
 * >>   ARGUMENT(Int, param0);  // <-- This is used to access argument `param0`
 * >>   ARGUMENT(Bool, param1); // <-- This is used to access argument `param1`
 * >> 
 * >>   // write your test here or call to another function to solve the problem
 * >>
 * >>   RETURN(123); // <-- This is used to return the value so your test cases
 * >>                //     will pick it and detect whether or not this problem
 * >>                //     has test case to verify or not. If it has, your 
 * >>                //     result will be passed to it to verify
 * >> }
 * 
 * ------------------------------------------------------------------------- */
#define BENCHMARK(Problem, Solution)                                   \
  namespace Benchmark {                                                \
  namespace Problem {                                                  \
  class Solution : public Base::Unit::Benchmark {                      \
   public:                                                             \
    Solution() : Case{#Problem, #Solution} {}                          \
                                                                       \
    void Playground() final;                                           \
  };                                                                   \
                                                                       \
  static auto Unit##Solution = Base::Unit::Benchmark::Make(            \
      #Problem, #Solution, std::make_shared<Test::Suite::TestCase>()); \
  } /* namespace Problem */                                            \
  } /* namespace Benchmark */                                          \
                                                                       \
  void Test::Problem::Solution::Playground()

/* @NOTE: this macro is used to access specific argument, we should use it to
 * access the argument from the our benchmark test since it gurantees both type
 * and value perspectively */
#define ARGUMENT(Type, Index) arguments[#Index].Get<Type>()

/* @NOTE: this macro is used to assign return value which is collected during 
 * perform our solution. This value would be pass down to checking steps and it
 * is verified both by test-step and test-integrity which are defined inside the
 * class Problem */
#define RETURN(Value)                                                  \
  {                                                                    \
    result = Base::Auto::As(Value);                                    \
    return;                                                            \
  }
#endif
#endif  // BASE_BENCHMARK_H_
