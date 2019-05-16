// Copyright (c) 2018,  All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.

// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.

// 3. Neither the name of ORGANIZATION nor the names of
//    its contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <Stream.h>
#include <Unittest.h>
#include <Utils.h>
#include <Vertex.h>
#include <signal.h>
#include <time.h>
#include <numeric>

#define RUN_LABEL "[ RUN      ]"
#define FAIL_LABEL "[     FAIL ]"
#define PASS_LABEL "[     PASS ]"
#define CRASH_LABEL "[    CRASH ]"
#define ASSERT_LABEL "[   ASSERT ]"
#define IGNORED_LABEL "[  IGNORED ]"
#define EXCEPTION_LABEL "[   EXCEPT ]"
#define SPLIT_SUITED "[----------]"
#define PRESENT_BOUND "[==========]"

namespace Base {
namespace Unit {
class Assertion : Exception {};

// @TODO:
class Suite {
 public:
  explicit Suite(UInt index, String name) : _Name{name}, _Index{index} {}
  virtual ~Suite() {}

  /* @NOTE: update status of the suite */
  Void OnTesting(ULong core, Shared<Case>& testcase);
  Void OnPassing(ULong core);

  /* @NOTE: get basic information of the suite */
  String Information();
  String Name();

 private:
  String _Name;
  UInt _Index;
};
}  // namespace Unit

namespace Internal {
ULong GetUniqueId();
Mutex* CreateMutex();

static Vertex<Mutex, True> Secure([](Mutex* mutex) { Locker::Lock(*mutex, True); },
                                  [](Mutex* mutex) { Locker::Unlock(*mutex); },
                                  CreateMutex());

Vector<Pair<Base::Unit::Suite, Vector<Shared<Case>>>>* Units{None};
Vector<Pair<Pair<Long, ULong>, Vector<ULong>>> Testing;

extern void AtExit(Function<void()> callback);
extern void CatchSignal(UInt signal, Function<void(siginfo_t*)> callback);

namespace Summary{
void WatchStopper();
void Milestone();
}  // namesapce Summary

namespace Unit {
void PrepareSuites() {
  UInt count_tests =
      (Units ? std::accumulate(
                   Units->begin(), Units->end(), 0,
                   [](UInt result,
                      Pair<Base::Unit::Suite, Vector<Shared<Case>>> it) {
                     return result + it.Right.size();
                   })
             : 0);

  /* @NOTE: present the short information about the test */
  VLOG(EInfo) << GREEN(PRESENT_BOUND) << " Run " << ToString(count_tests);
  if (count_tests > 1) {
    VLOG(EInfo) << " tests from ";
  } else {
    VLOG(EInfo) << " test from ";
  }

  VLOG(EInfo) << ToString(Units ? Units->size() : 0);
  VLOG(EInfo) << (Units && (Units->size() > 1) ? " test cases\n" : " test case\n");

  /* @TODO: perform global configuration */
  VLOG(EInfo) << GREEN(SPLIT_SUITED) << " Global test environment set-up.\n";
}

void AssignValueToTestcase(Unittest* unit, Int index, Any* value) {
  auto testcase = reinterpret_cast<Case*>(unit->Context);

  if (testcase) {
    testcase->Assign(index, Auto{*value});
  } else {
    throw Except(EBadLogic, "unit->Context mustn\'t be None");
  }
}

void ClearValueOfTestcase(Unittest* unit) {
  auto testcase = reinterpret_cast<Case*>(unit->Context);

  if (testcase) {
    testcase->Clear();
  } else {
    throw Except(EBadLogic, "unit->Context mustn\'t be None");
  }
}

Bool IsTestcaseExisted(Case* unit) {
  for (auto& suite : *Units) {
    auto& list_cases = suite.Right;

    for (UInt i = 0; i < list_cases.size(); ++i) {
      if (list_cases[i].get() == unit) {
        return True;
      }
    }
  }

  return False;
}

Bool IsTestcaseExisted(Unittest* unit) {
  return IsTestcaseExisted((Case*)unit->Context);
}

Bool CheckUnitStep(Case* UNUSED(unittest)) { return False; }
}  // namespace Unit
}  // namespace Internal

Case::Case(String suite, String name)
    : Status{[&]() -> StatusE& { return _Status; },
             [](StatusE) { throw Except(ENoSupport, ""); }},
      Teardown{[&]() -> TeardownD& { return _Unit->Teardown; },
               [](TeardownD) { throw Except(ENoSupport, ""); }},
      Unit{[&]() -> Shared<Unittest>& { return _Unit; },
           [](Shared<Unittest>) { throw Except(ENoSupport, ""); }},
      _Status{EUnknown},
      _Suite{suite},
      _Name{name} {
  _Unit = std::make_shared<Unittest>();
  _Unit->Context = this;

  _Unit->Assign = None;
  _Unit->Clear = None;
  _Unit->Testplan = None;
  _Unit->Teardown = None;
}

Shared<Case> Case::Make(String suite, String name, Shared<Case> implementer) {
  Bool registed{False};
  UInt index{0};

  /* @NOTE: if suite is still under implemented, register it now */
  if (Internal::Units) {
    for (; index < Internal::Units->size(); ++index) {
      if ((*Internal::Units)[index].Left.Name() == suite) {
        registed = True;
        break;
      }
    }
  } else {
    Internal::Units = new Vector<Pair<Unit::Suite, Vector<Shared<Case>>>>();
  }

  if (!registed) {
    index = Internal::Units->size();
    Internal::Units->push_back(Base::Pair<Suite, Vector<Shared<Case>>>(
        Unit::Suite{index, suite}, Vector<Shared<Case>>{}));
  }

  /* @NOTE: register testcase to Internal::Units */
  (*Internal::Units)[index].Right.push_back(implementer);
  implementer->_Name = name;

  return implementer;
}  // namespace Base

Case::~Case() {}

/* @NOTE: this method will be used to run a testcase and return its status */
Bool Case::Perform() {
  /* @NOTE: just to prevent recursive callback */
  if (_Status == EWaiting) return False;

  _Status = EWaiting;
  _Unit->Context = (void*)this;
  _Unit->Assign = Internal::Unit::AssignValueToTestcase;
  _Unit->Clear = Internal::Unit::ClearValueOfTestcase;

  /* @NOTE: perform Testplan inside try - catch */
  try {
    if (_Unit->Testplan) {
      _Unit->Testplan(_Unit.get());
    } else {
      Define();
    }
  } catch (Base::Exception& except) {
    if (dynamic_cast<Unit::Assertion*>(&except)) {
      /* @NOTE: got an assertion, show message here */

      VLOG(EError) << (RED << ASSERT_LABEL)
                   << Format{" testcase {}::{} complains {}"}
                        .Apply(_Suite, _Name, except.message())
                   << Base::EOL;
    } else {
      except.Ignore();
      VLOG(EError) << (RED << EXCEPTION_LABEL)
                   << (Format{" got Base::Exception \'{}\'"} << except.message())
                   << Base::EOL;
    }

    _Status = EFail;
  } catch (std::exception& except) {
    VLOG(EError) << (RED << EXCEPTION_LABEL)
                 << (Format{" got std::exception \'{}\'"} << except.what())
                 << Base::EOL;

    _Status = EFail;
  } catch (...) {
    VLOG(EError) << (RED << EXCEPTION_LABEL) << " got unknown exception"
                 << Base::EOL;
    _Status = EFail;
  }

  /* @NOTE: perform Teardown inside try - catch */
  try {
    if (_Unit->Teardown) _Unit->Teardown(_Unit.get());
  } catch (Base::Exception& except) {
    if (dynamic_cast<Unit::Assertion*>(&except)) {
      /* @NOTE: got an assertion, show message here */

      VLOG(EWarning) << (RED << ASSERT_LABEL)
                     << Format{" teardown testcase {}::{} complains {}"}
                          .Apply(_Suite, _Name, except.message())
                     << Base::EOL;
    } else {
      except.Ignore();
      VLOG(EWarning) << (RED << EXCEPTION_LABEL)
                     << (Format{" teardown got Base::Exception \'{}\'"}
                     << except.message())
                     << Base::EOL;
    }

  } catch (std::exception& except) {
    VLOG(EWarning) << (RED << EXCEPTION_LABEL)
                   << (Format{" teardown got std::exception \'{}\'"}
                   << except.what())
                   << Base::EOL;
  } catch (...) {
    VLOG(EWarning) << (RED << EXCEPTION_LABEL)
                   << " teardown got unknown exception"
                   << Base::EOL;
  }

  return _Status != EFail;
}

// @TODO
String Case::Information() {
  if (_Status == EUnknown) {
    return _Suite + "." + _Name;
  } else {
    return _Suite + "." + _Name;
  }
}

// @TODO:
String Case::Report() {
  if (_Status == EUnknown) {
    if (!Perform() && _Status <= EPass) {
      _Status = EFail;
    }
  }

  switch (_Status) {
    default:
    case EUnknown:
      /* @NOTE: never reach this line */
      throw Except(EBadLogic, "there was an unknown logic bug here");

    case EPass:
      return "";

    case EIgnored:
      return "";

    case EFail:
    case EFatal:
      return "";
  }
}

// @NOTE: add value to input
ErrorCodeE Case::Assign(Int index, Auto&& value) {
  if (index < Cast(index, _Inputs.size())) {
    _Inputs[index] = RValue(value);
  } else if (index < 0 || index == Cast(index, _Inputs.size())) {
    _Inputs.push_back(value);
  } else {
    return OutOfRange.code();
  }

  return ENoError;
}

// @NOTE: clear inputs of testcase's step
ErrorCodeE Case::Clear() {
  if (_Inputs.size() == 0) return DoNothing.code();

  _Inputs.clear();
  return ENoError;
}

void Case::Define() { throw Except(ENoSupport, ""); }

void Case::Ignore(Bool apply){
  if (_Status != EFail && _Status != EFatal){
    _Status = apply? EIgnored: EPass;
  }
}

// @TODO:
void Case::Fatal(Bool crash) {
  if (_Status != EIgnored) {
    _Status = EFatal;
  }

  if (crash) {
    INFO << RED(CRASH_LABEL) << " " << Information()
         << " --> exit(255)" << Base::EOL;

    abort();
  } else {
    INFO << YELLOW(IGNORED_LABEL) << " Ignore this fatal" << Base::EOL;
  }
}

// @TODO:
void Case::Fail() {
  if (_Status != EIgnored) {
    _Status = EFail;
  } else {
    INFO << YELLOW(IGNORED_LABEL) << " Ignore this fail" << Base::EOL;
  }
}

// @TODO:
void Case::Pass() {
  if (_Status == EIgnored) return;
  _Status = (_Status == EPass or _Status == EUnknown) ? EPass : _Status;
}

Case* Case::Self() {
  using namespace Base::Internal;

  auto UNUSED(guranteer) = Secure.generate();

  if (Units) {
    ULong tid{GetUniqueId()};

    for (auto& testcase: Testing) {
      auto& index = testcase.Left;
      auto& owner = testcase.Right;

      if (index.Left < 0) {
        continue;
      } else if (Find(owner.begin(), owner.end(), tid) >= 0) {
        return (*Units)[index.Left].Right[index.Right].get();
      }
    }

    throw Except(EBadLogic, Format{"not found testcase in "
                                   "thread {}"}.Apply(tid));
  } else {
    throw Except(EBadLogic, "Can\'t access a testcase when there "
                            "were nothing implimented");
  }
}

Void Unit::Suite::OnTesting(ULong core, Shared<Case>& testcase) {
  using namespace Base::Internal;

  if (core < Testing.size()) {
    auto UNUSED(guranteer) = Secure.generate();
    auto tid = GetUniqueId();
    auto& cases = (*Units)[_Index].Right;
    auto& owners = Testing[core].Right;

    /* @NOTE: update that we are perform this case on specific core */
    Testing[core].Left.Left = _Index;
    Testing[core].Left.Right = Find(cases.begin(), cases.end(), testcase);

    if (Find(owners.begin(), owners.end(), tid) < 0) {
      owners.push_back(tid);
    }
  }
}

Void Unit::Suite::OnPassing(ULong core) {
  auto UNUSED(guranteer) = Internal::Secure.generate();

  /* @NOTE: assign to -1 will elimiate the suite on the testing board */
  if (core < Internal::Testing.size()) {
    Internal::Testing[core].Left.Left = -1;
  }
}

String Unit::Suite::Name() { return _Name; }

String Unit::Suite::Information() {
  UInt how_many_tests = (*Internal::Units)[_Index].Right.size();
  String result = ToString(how_many_tests);

  result.append(how_many_tests > 1 ? " tests from " : " test from ");
  result.append((*Internal::Units)[_Index].Left._Name);

  return result;
}
}  // namespace Base

class CaseImpl : public Base::Unit::Case {
 public:
  explicit CaseImpl(String suite, String name) : Case{suite, name} {
    _Unit->Assign = [](Unittest* unit, Int index, Any* value) {
      using namespace Base::Internal::Unit;
      using namespace Base::Internal;
      using namespace Base::Unit;
      using namespace Base;

      auto testcase = reinterpret_cast<Case*>(unit->Context);

      if (!IsTestcaseExisted(testcase)) {
        throw Except(EBadAccess,
                     Format("Unittest with address {}").Apply(ULong(unit)));
      } else {
        auto error = testcase->Assign(index, Base::Auto{*value});

        if (error) {
          throw Exception(error);
        }
      }
    };

    _Unit->Clear = [](Unittest* unit) {
      using namespace Base::Internal::Unit;
      using namespace Base::Internal;
      using namespace Base::Unit;
      using namespace Base;

      auto testcase = reinterpret_cast<Case*>(unit->Context);

      if (!IsTestcaseExisted(testcase)) {
        throw Except(EBadAccess,
                     Format("Unittest with address {}").Apply(ULong(unit)));
      } else {
        auto error = testcase->Clear();

        if (error) {
          throw Exception(error);
        }
      }
    };

    _Unit->Testplan = None;
    _Unit->Teardown = None;
  }

  void Plan(void (*callback)(Unittest* unit)) { _Unit->Testplan = callback; }

  void Define() final {
    if (_Unit->Testplan) {
      _Unit->Testplan(_Unit.get());
    } else {
      _Status = Base::Unit::EFatal;
    }
  }
};

extern "C" Unittest* BSDefineTest(CString suite, CString name,
                                  void (*callback)(Unittest* unit)) {
  auto testcase = std::make_shared<CaseImpl>(suite, name);

  testcase->Plan(callback);
  return testcase->Unit().get();
}

extern "C" Int BSRunTests() {
  using namespace Base;
  using namespace Base::Internal;
  using Session = Pair<Pair<Long, ULong>, Vector<ULong>>;

  Bool did_everything_pass = True;
  ULong index = 0;

  Base::Internal::Unit::PrepareSuites();

  /* @TODO: use CatchSignal to catch any unexpect signal like SIGSEGV, SIGKILL,
   * etc ...
   */
  CatchSignal(SIGSEGV, [](siginfo_t* UNUSED(si)) {

  });

  if (Units) {
    Session sess(Pair<Long, ULong>(-1, 0), Vector<ULong>());

    sess.Right.push_back(GetUniqueId());
    Testing.push_back(sess);
  } else {
    goto finish;
  }

  /* @NOTE: everything is finished, perform reporting now */
  for (auto& suite : *Units) {
    double elapsed_suite_milisecs;
    clock_t begin;

    VLOG(EInfo) << GREEN(SPLIT_SUITED) << " " << suite.Left.Information() << Base::EOL;

    begin = clock();
    for (auto& test : suite.Right) {
      double elapsed_test_milisecs = 0.0;
      /* @NOTE: using a single loop is the simple and safe way to implement my
       * testing flow, consider using Parallel to improve performance */

      VLOG(EInfo) << YELLOW(RUN_LABEL) << " " << test->Information() << Base::EOL;

      /* @NOTE: perform this test now */
      if (test->Status() == Base::Unit::EUnknown) {
        clock_t begin;

        /* @NOTE: notice that we are on testing the case */
        suite.Left.OnTesting(index, test);

        /* @NOTE: start timer */
        begin = clock();

        test->Perform();
        elapsed_test_milisecs = 1000.0 * (clock() - begin) / CLOCKS_PER_SEC;
      }

      /* @NOTE: check status of this test and present it to console */
      if (test->Status() == Base::Unit::EFail) {
        did_everything_pass = False;

        VLOG(EInfo) << RED(FAIL_LABEL) << " " << test->Information() << " ("
                    << Base::ToString(elapsed_test_milisecs) << " ms)"
                    << Base::EOL;
      } else if (test->Status() == Base::Unit::EFatal) {
        did_everything_pass = False;
        VLOG(EInfo) << RED(FAIL_LABEL) << " " << test->Information() << " ("
                    << Base::ToString(elapsed_test_milisecs) << " ms)"
                    << Base::EOL;

        /* @NOTE: with fatal errors we must abandon remaining tests */
        break;
      } else if (test->Status() == Base::Unit::EIgnored) {
        VLOG(EInfo) << YELLOW(IGNORED_LABEL) << " " << test->Information()
                    << Base::EOL;
      } else {
        VLOG(EInfo) << GREEN(PASS_LABEL) << " " << test->Information() << " ("
                    << Base::ToString(elapsed_test_milisecs) << " ms)"
                    << Base::EOL;
      }
    }

    /* @NOTE: calculate how many time has been passed with this test suite */
    elapsed_suite_milisecs = 1000.0 * (clock() - begin) / CLOCKS_PER_SEC;

    /* @NOTE: notice that we finish this test suite */
    VLOG(EInfo) << GREEN(SPLIT_SUITED) << " " << suite.Left.Information() 
                << " ("
                << Base::ToString(elapsed_suite_milisecs) << " ms)"
                << Base::EOL;

    /* @NOTE: notify that we are passing the suite */
    suite.Left.OnPassing(index);
  }

finish:
  /* @TODO: export Test result, I will use JUnit format since it's widely used
   * and very easy to implement. Just to keep everything simple and clear
   */
  /* @TODO: consider using Python to implement from here since we can do many
   * thing with Python, including testing on many platforms and system
   */

  delete Units;
  return did_everything_pass ? 0 : 255;
}

void BSExpectTrue(Unittest* unit, Bool condition, String describe, String file,
                  UInt line) {
  using namespace Base::Internal::Unit;
  using namespace Base::Internal;
  using namespace Base::Unit;
  using namespace Base;

  auto testcase = reinterpret_cast<Case*>(unit->Context);

  if (!IsTestcaseExisted(testcase)) {
    throw Except(EBadAccess,
                 Format("Unittest with address {}").Apply(ULong(unit)));
  } else if (condition == False) {
    testcase->Expect(" is True", describe, file, line);
    testcase->Actual(" is False", describe);
    testcase->Fail();
  } else {
    testcase->Pass();
  }
}

void BSAssertTrue(Unittest* unit, Bool condition, String describe, String file,
                  UInt line) {
  using namespace Base::Internal::Unit;
  using namespace Base::Internal;
  using namespace Base::Unit;
  using namespace Base;

  auto testcase = reinterpret_cast<Case*>(unit->Context);

  if (!IsTestcaseExisted(testcase)) {
    throw Except(EBadAccess,
                 Format("Unittest with address {}").Apply(ULong(unit)));
  } else if (condition == False) {
    testcase->Expect(" is True", describe, file, line);
    testcase->Actual(" is False", describe);
    testcase->Fatal();
  } else {
    testcase->Pass();
  }
}

void BSExpectFalse(Unittest* unit, Bool condition, String describe, String file,
                   UInt line) {
  using namespace Base::Internal::Unit;
  using namespace Base::Internal;
  using namespace Base::Unit;
  using namespace Base;

  auto testcase = reinterpret_cast<Case*>(unit->Context);

  if (!IsTestcaseExisted(testcase)) {
    throw Except(EBadAccess,
                 Format("Unittest with address {}").Apply(ULong(unit)));
  } else if (condition == True) {
    testcase->Expect(" is False", describe, file, line);
    testcase->Actual(" is True", describe);
    testcase->Fail();
  } else {
    testcase->Pass();
  }
}

void BSAssertFalse(Unittest* unit, Bool condition, String describe, String file,
                   UInt line) {
  using namespace Base::Internal::Unit;
  using namespace Base::Internal;
  using namespace Base::Unit;
  using namespace Base;

  auto testcase = reinterpret_cast<Case*>(unit->Context);

  if (!IsTestcaseExisted(testcase)) {
    throw Except(EBadAccess,
                 Format("Unittest with address {}").Apply(ULong(unit)));
  } else if (condition == True) {
    testcase->Expect(" is False", describe, file, line);
    testcase->Actual(" is True", describe);
    testcase->Fatal();
  } else {
    testcase->Pass();
  }
}

extern "C" void BSExpectTrue(Unittest* unit, Bool condition, CString describe,
                             CString file, UInt line) {
  using namespace Base::Internal::Unit;
  using namespace Base::Internal;
  using namespace Base::Unit;
  using namespace Base;

  auto testcase = reinterpret_cast<Case*>(unit->Context);

  if (!IsTestcaseExisted(testcase)) {
    throw Except(EBadAccess,
                 Format("Unittest with address {}").Apply(ULong(unit)));
  } else if (condition == False) {
    testcase->Expect(" is True", describe, file, line);
    testcase->Actual(" is False", describe);
    testcase->Fail();
  } else {
    testcase->Pass();
  }
}

extern "C" void BSAssertTrue(Unittest* unit, Bool condition, CString describe,
                             CString file, UInt line) {
  using namespace Base::Internal::Unit;
  using namespace Base::Internal;
  using namespace Base::Unit;
  using namespace Base;

  auto testcase = reinterpret_cast<Case*>(unit->Context);

  if (!IsTestcaseExisted(testcase)) {
    throw Except(EBadAccess,
                 Format("Unittest with address {}").Apply(ULong(unit)));
  } else if (condition == False) {
    testcase->Expect(" is True", describe, file, line);
    testcase->Actual(" is False", describe);
    testcase->Fatal();
  } else {
    testcase->Pass();
  }
}

extern "C" void BSExpectFalse(Unittest* unit, Bool condition, CString describe,
                              CString file, UInt line) {
  using namespace Base::Internal::Unit;
  using namespace Base::Internal;
  using namespace Base::Unit;
  using namespace Base;

  auto testcase = reinterpret_cast<Case*>(unit->Context);

  if (!IsTestcaseExisted(testcase)) {
    throw Except(EBadAccess, BSFormat("Unittest with address %x", unit));
  } else if (condition == True) {
    testcase->Expect(" is False", describe, file, line);
    testcase->Actual(" is True", describe);
    testcase->Fail();
  } else {
    testcase->Pass();
  }
}

extern "C" void BSAssertFalse(Unittest* unit, Bool condition, CString describe,
                              CString file, UInt line) {
  using namespace Base::Internal::Unit;
  using namespace Base::Internal;
  using namespace Base::Unit;
  using namespace Base;

  auto testcase = reinterpret_cast<Case*>(unit->Context);

  if (!IsTestcaseExisted(testcase)) {
    throw Except(EBadAccess, BSFormat("Unittest with address %x", unit));
  } else if (condition == True) {
    testcase->Expect(" is False", describe, file, line);
    testcase->Actual(" is True", describe);
    testcase->Fatal();
  } else {
    testcase->Pass();
  }
}

extern "C" void BSExpectWithTwoDescription(Unittest* unit, CString left,
                                           CString comparision, CString right,
                                           CString file, UInt line) {
  using namespace Base::Internal::Unit;
  using namespace Base::Internal;
  using namespace Base::Unit;
  using namespace Base;

  auto testcase = reinterpret_cast<Case*>(unit->Context);

  if (!IsTestcaseExisted(testcase)) {
    throw Except(EBadAccess, BSFormat("Unittest with address %x", unit));
  } else if (CheckUnitStep(testcase) == True) {
    String actual = "";

    if (String(comparision) == "<") {
      actual = ">=";
    } else if (String(comparision) == ">") {
      actual = "<=";
    } else if (String(comparision) == "<=") {
      actual = ">";
    } else if (String(comparision) == ">=") {
      actual = "<";
    } else if (String(comparision) == "==") {
      actual = "!=";
    } else if (String(comparision) == "!=") {
      actual = "==";
    } else {
      throw Except(EBadLogic, Format("Unsupport %s").Apply(comparision));
    }

    testcase->Expect(comparision, left, right, file, line);
    testcase->Actual(actual, left, right);
    testcase->Fail();
  } else {
    testcase->Pass();
  }
}

extern "C" void BSAssertWithTwoDescription(Unittest* unit, CString left,
                                           CString comparision, CString right,
                                           CString file, UInt line) {
  using namespace Base::Internal::Unit;
  using namespace Base::Internal;
  using namespace Base::Unit;
  using namespace Base;

  auto testcase = reinterpret_cast<Case*>(unit->Context);

  if (!IsTestcaseExisted(testcase)) {
    throw Except(EBadAccess, BSFormat("Unittest with address %x", unit));
  } else if (CheckUnitStep(testcase) == True) {
    String actual = "";

    if (String(comparision) == "<") {
      actual = ">=";
    } else if (String(comparision) == ">") {
      actual = "<=";
    } else if (String(comparision) == "<=") {
      actual = ">";
    } else if (String(comparision) == ">=") {
      actual = "<";
    } else if (String(comparision) == "==") {
      actual = "!=";
    } else if (String(comparision) == "!=") {
      actual = "==";
    } else {
      throw Except(EBadLogic, Format("Unsupport %s").Apply(comparision));
    }

    testcase->Expect(comparision, left, right, file, line);
    testcase->Actual(actual, left, right);
    testcase->Fatal();
  } else {
    testcase->Pass();
  }
}
