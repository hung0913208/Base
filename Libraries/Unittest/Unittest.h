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

#ifndef BASE_UNITTEST_H_
#define BASE_UNITTEST_H_
#include <Auto.h>
#include <Macro.h>
#include <Type.h>
#include <Vertex.h>
#include <signal.h>
#include <thread>

/* @NOTE: these are basic C-API of Unittest library, since they must be
 * simple and primitived. It would mean you need more line to adapt a single
 * step of checking.
 */
#if __cplusplus
extern "C" {
#endif
/* @NOTE: this struct is define a basic interface with Unittest's C++-APIs */
typedef struct Unittest {
  void *Context;

  void (*Assign)(struct Unittest* unit, Int index, Any* value);
  void (*Prepare)(struct Unittest* unit);
  void (*Clear)(struct Unittest* unit);
  void (*Testplan)(struct Unittest* unit);
  void (*Teardown)(struct Unittest* unit);
} Unittest;

/* @NOTE: these APIs will be used to define simple Unittests on C/C++ and
 * perform them after everything is done
 */
Unittest* BSDefineTest(CString suite, CString name,
                       void (*callback)(Unittest* unit));
Unittest* BSDefinePrepare(Unittest* unit, void (*callback)(Unittest* unit));
Unittest* BSDefineTeardown(Unittest* unit, void (*callback)(Unittest* unit));

int BSRunTests();
#if __cplusplus
}
#endif

/* @NOTE: these APIs are used to define how to check a single step of Unittest
 */
#if __cplusplus
void BSExpectTrue(Unittest* unit, Bool condition, String describe, String file,
                  UInt line);
void BSAssertTrue(Unittest* unit, Bool condition, String describe, String file,
                  UInt line);
void BSExpectFalse(Unittest* unit, Bool condition, String describe, String file,
                   UInt line);
void BSAssertFalse(Unittest* unit, Bool condition, String describe, String file,
                   UInt line);
#else
void BSExpectTrue(Unittest* unit, Bool condition, CString describe,
                  CString file, UInt line);
void BSAssertTrue(Unittest* unit, Bool condition, CString describe,
                  CString file, UInt line);
void BSExpectFalse(Unittest* unit, Bool condition, CString describe,
                   CString file, UInt line);
void BSAssertFalse(Unittest* unit, Bool condition, CString describe,
                   CString file, UInt line);
#endif

#if !__cplusplus && GNU_COMPILER && C_VERSION >= 201112L
#define TypeIdForTesting(x)                                                  \
  _Generic((x), _Bool : 0, unsigned char : 1, char : 2, signed char : 3,     \
           short int : 4, unsigned short int : 5, int : 6, unsigned int : 7, \
           long int : 8, unsigned long int : 9, long long int : 10,          \
           unsigned long long int : 11, float : 12, double : 13,             \
           long double : 14, char* : 15, void* : 16, int* : 17, default      \
           : -1)

void* BSTypeIdToType(UInt type_id);

/* @NOTE: because C is very simple so I can't adapt undefined type and I
 * can't use 'overload functions'. So the easiest way is to assign Value
 * with my custom unknown type 'Any' and use it with these functions below
 * to implement:
 * - BSExpectEqual
 * - BSAssertEqual
 * - BSExpectLower
 * - BSAssertLower
 * - BSExpectLowerOrEqual
 * - BSAssertLowerOrEqual
 * - BSExpectGreater
 * - BSAssertGreater
 * - BSExpectGreaterOrEqual
 * - BSAssertGreaterOrEqual
 */
void BSExpectWithTwoDescription(Unittest* unit, CString left,
                                CString comparition, CString right,
                                CString file, UInt line);
void BSAssertWithTwoDescription(Unittest* unit, CString left,
                                CString comparition, CString right,
                                CString file, UInt line);
#endif

/* @NOTE: since there would be hard to adapt Unittest's C-API, i must propose
 * some simple C++ helpers. There are very simple and can support to adapt a
 * single step of checking with only in single line
 */
#if __cplusplus
namespace Base {
namespace Unit {
class Case;
}  // namespace Unit
}  // namespace Base

using Case = Base::Unit::Case;
/* @NOTE: These functions bellow define to 'BSExpectEqual'*/
template <typename Left, typename Right>
void BSExpectEqual(Case* testcase, Left&& left, String left_describe,
                   Right&& right, String right_describe);

template <typename Left, typename Right>
void BSExpectEqual(Case* testcase, const Left& left, String left_describe,
                   const Right& right, String right_describe);

template <typename Left, typename Right>
void BSExpectEqual(Case* testcase, Left&& left, String left_describe,
                   const Right& right, String right_describe);

template <typename Left, typename Right>
void BSExpectEqual(Case* testcase, const Left& left, String left_describe,
                   Right&& right, String right_describe);

/* @NOTE: These functions bellow define to 'BSAssertEqual'*/
template <typename Left, typename Right>
void BSAssertEqual(Case* testcase, Left&& left, String left_describe,
                   Right&& right, String right_describe);

template <typename Left, typename Right>
void BSAssertEqual(Case* testcase, const Left& left, String left_describe,
                   const Right& right, String right_describe);

template <typename Left, typename Right>
void BSAssertEqual(Case* testcase, Left&& left, String left_describe,
                   const Right& right, String right_describe);

template <typename Left, typename Right>
void BSAssertEqual(Case* testcase, const Left& left, String left_describe,
                   Right&& right, String right_describe);

/* @NOTE: These functions bellow define to 'BSExpectNotEqual'*/
template <typename Left, typename Right>
void BSExpectNotEqual(Case* testcase, Left&& left, String left_describe,
                      Right&& right, String right_describe);

template <typename Left, typename Right>
void BSExpectNotEqual(Case* testcase, const Left& left, String left_describe,
                      const Right& right, String right_describe);

template <typename Left, typename Right>
void BSExpectNotEqual(Case* testcase, Left&& left, String left_describe,
                      const Right& right, String right_describe);

template <typename Left, typename Right>
void BSExpectNotEqual(Case* testcase, const Left& left, String left_describe,
                      Right&& right, String right_describe);

/* @NOTE: These functions bellow define to 'BSAssertNotEqual'*/
template <typename Left, typename Right>
void BSAssertNotEqual(Case* testcase, Left&& left, String left_describe,
                      Right&& right, String right_describe);

template <typename Left, typename Right>
void BSAssertNotEqual(Case* testcase, const Left& left, String left_describe,
                      const Right& right, String right_describe);

template <typename Left, typename Right>
void BSAssertNotEqual(Case* testcase, Left&& left, String left_describe,
                      const Right& right, String right_describe);

template <typename Left, typename Right>
void BSAssertNotEqual(Case* testcase, const Left& left, String left_describe,
                      Right&& right, String right_describe);

/* @NOTE: These functions bellow define to 'BSExpectLower'*/
template <typename Left, typename Right>
void BSExpectLower(Case* testcase, Left&& left, String left_describe,
                   Right&& right, String right_describe);

template <typename Left, typename Right>
void BSExpectLower(Case* testcase, const Left& left, String left_describe,
                   const Right& right, String right_describe);

template <typename Left, typename Right>
void BSExpectLower(Case* testcase, Left&& left, String left_describe,
                   const Right& right, String right_describe);

template <typename Left, typename Right>
void BSExpectLower(Case* testcase, const Left& left, String left_describe,
                   Right&& right, String right_describe);

/* @NOTE: These functions bellow define to 'BSAssertLower'*/
template <typename Left, typename Right>
void BSAssertLower(Case* testcase, Left&& left, String left_describe,
                   Right&& right, String right_describe);

template <typename Left, typename Right>
void BSAssertLower(Case* testcase, const Left& left, String left_describe,
                   const Right& right, String right_describe);

template <typename Left, typename Right>
void BSAssertLower(Case* testcase, Left&& left, String left_describe,
                   const Right& right, String right_describe);

template <typename Left, typename Right>
void BSAssertLower(Case* testcase, const Left& left, String left_describe,
                   Right&& right, String right_describe);

/* @NOTE: These functions bellow define to 'BSExpectGreater'*/
template <typename Left, typename Right>
void BSExpectGreater(Case* testcase, Left&& left, String left_describe,
                     Right&& right, String right_describe);

template <typename Left, typename Right>
void BSExpectGreater(Case* testcase, const Left& left, String left_describe,
                     const Right& right, String right_describe);

template <typename Left, typename Right>
void BSExpectGreater(Case* testcase, Left&& left, String left_describe,
                     const Right& right, String right_describe);

template <typename Left, typename Right>
void BSExpectGreater(Case* testcase, const Left& left, String left_describe,
                     Right&& right, String right_describe);

/* @NOTE: These functions bellow define to 'BSAssertGreater'*/
template <typename Left, typename Right>
void BSAssertGreater(Case* testcase, Left&& left, String left_describe,
                     Right&& right, String right_describe);

template <typename Left, typename Right>
void BSAssertGreater(Case* testcase, const Left& left, String left_describe,
                     const Right& right, String right_describe);

template <typename Left, typename Right>
void BSAssertGreater(Case* testcase, Left&& left, String left_describe,
                     const Right& right, String right_describe);

template <typename Left, typename Right>
void BSAssertGreater(Case* testcase, const Left& left, String left_describe,
                     Right&& right, String right_describe);

/* @NOTE: These functions bellow define to 'BSExpectLowerOrEqual'*/
template <typename Left, typename Right>
void BSExpectLowerOrEqual(Case* testcase, Left&& left, String left_describe,
                          Right&& right, String right_describe);

template <typename Left, typename Right>
void BSExpectLowerOrEqual(Case* testcase, const Left& left,
                          String left_describe, const Right& right,
                          String right_describe);

template <typename Left, typename Right>
void BSExpectLowerOrEqual(Case* testcase, Left&& left, String left_describe,
                          const Right& right, String right_describe);

template <typename Left, typename Right>
void BSExpectLowerOrEqual(Case* testcase, const Left& left,
                          String left_describe, Right&& right,
                          String right_describe);

/* @NOTE: These functions bellow define to 'BSAssertLowerOrEqual'*/
template <typename Left, typename Right>
void BSAssertLowerOrEqual(Case* testcase, Left&& left, String left_describe,
                          Right&& right, String right_describe);

template <typename Left, typename Right>
void BSAssertLowerOrEqual(Case* testcase, const Left& left,
                          String left_describe, const Right& right,
                          String right_describe);

template <typename Left, typename Right>
void BSAssertLowerOrEqual(Case* testcase, Left&& left, String left_describe,
                          const Right& right, String right_describe);

template <typename Left, typename Right>
void BSAssertLowerOrEqual(Case* testcase, const Left& left,
                          String left_describe, Right&& right,
                          String right_describe);

/* @NOTE: These functions bellow define to 'BSExpectGreaterOrEqual'*/
template <typename Left, typename Right>
void BSExpectGreaterOrEqual(Case* testcase, Left&& left, String left_describe,
                            Right&& right, String right_describe);

template <typename Left, typename Right>
void BSExpectGreaterOrEqual(Case* testcase, const Left& left,
                            String left_describe, const Right& right,
                            String right_describe);

template <typename Left, typename Right>
void BSExpectGreaterOrEqual(Case* testcase, Left&& left, String left_describe,
                            const Right& right, String right_describe);

template <typename Left, typename Right>
void BSExpectGreaterOrEqual(Case* testcase, const Left& left,
                            String left_describe, Right&& right,
                            String right_describe);

/* @NOTE: These functions bellow define to 'BSAssertGreaterOrEqual'*/
template <typename Left, typename Right>
void BSAssertGreaterOrEqual(Case* testcase, Left&& left, String left_describe,
                            Right&& right, String right_describe);

template <typename Left, typename Right>
void BSAssertGreaterOrEqual(Case* testcase, const Left& left,
                            String left_describe, const Right& right,
                            String right_describe);

template <typename Left, typename Right>
void BSAssertGreaterOrEqual(Case* testcase, Left&& left, String left_describe,
                            const Right& right, String right_describe);

template <typename Left, typename Right>
void BSAssertGreaterOrEqual(Case* testcase, const Left& left,
                            String left_describe, Right&& right,
                            String right_describe);
#endif


/* @NOTE: define my Unittest's core is here, it's a C++ core */
#if __cplusplus
namespace Base {
namespace Internal {
namespace Unit {
Bool CheckUnitStep(Case* unittest);
}  // namespace Unit
}  // namespace Internal

namespace Unit {
void Init(Int* argc, CString* argv);
namespace Pharse {
class Prepare;
class Teardown;
} // namespace Pharse

enum StatusE {
  EWaiting = -2,
  EUnknown = -1,
  EPass = 0,
  EFail = 1,
  EIgnored = 2,
  EFatal = 3
};

class Case {
 public:
  virtual ~Case();

  /* @NOTE: these functions support how to iteract with Unittest */
  ErrorCodeE Assign(Int index, Auto&& value);
  ErrorCodeE Clear();
  Void Bypass();
  Bool Perform();
  String Report();
  String Information();

  /* @NOTE: these functions are used to report status of each test step */
  void Ignore(Bool apply = False);
  void Fatal(Bool crash = False);
  void Fail();
  void Pass();

  /* @NOTE: these template functions are used to show reason why a step fail */
  inline void Expect(String message, String file, UInt line) {
    INFO << "    Expect: " <<  message << " at "
         << file << Base::ToString(line) << Base::EOL;

    _Complain = message;
  }

  inline void Actual(String message) {
    INFO << "    Actual: " << message  << Base::EOL;
  }

  inline void Expect(String format, String value, String file, UInt line) {
    INFO << " Check if " << value << format << " at " << file << ":"
         << Base::ToString(line) << Base::EOL;
    INFO << "    Expect: " << value << format << Base::EOL;

    _Complain = Format{"{} {}"}.Apply(value, format);
  }

  inline void Actual(String format, String value) {
    INFO << "    Actual: " << value << format << Base::EOL;
  }

  inline void Expect(String comparition, String left, String right, String file,
                     UInt line) {
    INFO << " Check if " << left << " " << comparition << " " << right << " at "
         << file << ":" << Base::ToString(line) << Base::EOL;
    INFO << "    Expect: " << left << " " << comparition << " " << right
         << Base::EOL;

    _Complain = Format{"{} {} {}"}.Apply(left, comparition, right);
  }

  inline void Actual(String comparition, String left, String right) {
    INFO << "    Actual: " << left << " " << comparition << " " << right
         << Base::EOL;
  }

  /* @NOTE: these property will be used to manage each Unittest */
  Property<StatusE> Status;
  Property<Pharse::Prepare*> Prepare;
  Property<Pharse::Teardown*> Teardown;
  Property<Shared<Unittest>> Unit;

  /* @NOTE: always use this function to create a new case */
  static Shared<Case> Make(String suite, String name, Shared<Case> implementer);

  /* @NOTE: this function will help to point to the current Case */
  static Case* Self();

  /* @NOTE: this function will help to check unittest's steps */
  friend Bool Internal::Unit::CheckUnitStep(Case* unittest);
  friend Void Unit::Init(Int* argc, CString* argv);

 protected:
  /* @NOTE: define our testcase with this virtual method */
  explicit Case(String suite, String name);
  virtual void Define();

  Shared<Unittest> _Unit;
  Pharse::Prepare* _Prepare;
  Pharse::Teardown* _Teardown;
  Vector<Auto> _Inputs;
  StatusE _Status;
  String _Suite, _Name;
  String _Complain;
};

namespace Pharse {
class Prepare {
 public:
  explicit Prepare(Shared<Case> unit);
  virtual ~Prepare();

  /* @NOTE: this function is used to perform the pharse Prepare */
  void Perform();

 protected:
  virtual void Define() = 0;

 private:
  Vector<Prepare*> _Steps;
  Shared<Case> _Case;
};

class Teardown {
 public:
  explicit Teardown(Shared<Case> unit);
  virtual ~Teardown();

  /* @NOTE: this function is used to perform the pharse Teardown */
  void Perform();

 protected:
  virtual void Define() = 0;

 private:
  Vector<Teardown*> _Steps;
  Shared<Case> _Case;
};
} // namespace Pharse

class Trap {
 public:
  explicit Trap(Case* testcase) : _Case{testcase} {}
  virtual ~Trap() {}

  virtual void operator<<(Function<void()> callback) = 0;

 protected:
  Case* _Case;
};

class Timeout : Trap {
 public:
  explicit Timeout(Case* testcase, ULong seconds, UInt line, String file);
  explicit Timeout(Case* testcase, Function<void()> watchdog, UInt line,
                   String file);

  void operator<<(Function<void()> callback);

 private:
  void HandleSignal(siginfo_t* siginfo, String file, UInt line);

  Function<void(siginfo_t*)> _Handler;
  ULong _Seconds;
  Bool _Bypass;
};

class Dump : Trap {
 public:
  enum TypeE {
    EThrowing = 0,
    EExiting = 1,
    ECrashing = 2,
    EFinishing = 3
  };

  explicit Dump(Case* testcase, UInt type);
  virtual ~Dump();

  static void Register(Shared<Dump> dumper);
  static void Clear();

  void operator<<(Function<void()> callback) final;

  void Assign(String name, Base::Auto object);
  void Preview();

 protected:

  Map<String, Auto> _Variables;
  Function<void()> _Handler;
  UInt _Type;
};
}  // namespace Unit
}  // namespace Base
#endif

#if !USE_GTEST

/* @NOTE: this macro will run start our testcase run on parallel */
#define RUN_ALL_TESTS() BSRunTests()

/* @NOTE: these macros are defined specifically to according languages */
#if __cplusplus
// @BEGIN
/* @NOTE: this macro will help testcase to point to exact unittest */
#define THIS Base::Unit::Case::Self()

/* @NOTE: this macro is used to define our Testcase */
#define TEST(Suite, TestCase)                                        \
  namespace Test {                                                   \
  namespace Suite {                                                  \
  class TestCase : public Base::Unit::Case {                         \
   public:                                                           \
    TestCase() : Case{#Suite, #TestCase} {}                          \
                                                                     \
    void Define() final;                                             \
  };                                                                 \
                                                                     \
  static auto Unit##TestCase = Base::Unit::Case::Make(               \
      #Suite, #TestCase, std::make_shared<Test::Suite::TestCase>()); \
  } /* namespace Suite */                                            \
  } /* namespace Test */                                             \
                                                                     \
  void Test::Suite::TestCase::Define()

/* @NOTE: if we would prepare something before the test case started, we should
 * use this macro to define a prepare step and the test case will start with
 * this code first before doing anything else  */

#define PREPARE(Suite, TestCase)                                     \
  namespace Test {                                                   \
  namespace Suite {                                                  \
  namespace Prepares {                                               \
  class TestCase : public Base::Unit::Pharse::Prepare {              \
   public:                                                           \
    TestCase(Shared<Base::Unit::Case> unit) : Prepare{unit} {}       \
                                                                     \
    void Define() final;                                             \
  };                                                                 \
                                                                     \
  static auto Unit##TestCase =                                       \
      std::make_shared<Test::Suite::Prepares::TestCase>(             \
                                      Test::Suite::Unit##TestCase);  \
  } /* namespace Prepares */                                         \
  } /* namespace Suite */                                            \
  } /* namespace Test */                                             \
  void Test::Suite::Prepares::TestCase::Define()

/* @NOTE: when a testcase finish its task, no matter it's passed or failed, if
 * we define a teardown callback, this will perform after testcase is departed
 */
#define TEARDOWN(Suite, TestCase)                                    \
  namespace Test {                                                   \
  namespace Suite {                                                  \
  namespace Teardowns {                                              \
  class TestCase : public Base::Unit::Pharse::Teardown {             \
   public:                                                           \
    explicit TestCase(Shared<Base::Unit::Case> unit) :               \
        Teardown{unit} {}                                            \
                                                                     \
    void Define() final;                                             \
  };                                                                 \
  static auto Unit##TestCase =                                       \
      std::make_shared<Test::Suite::Teardowns::TestCase>(            \
                                      Test::Suite::Unit##TestCase);  \
  } /* namespace Teardowns */                                        \
  } /* namespace Suite */                                            \
  } /* namespace Test */                                             \
                                                                     \
  void Test::Suite::Teardowns::TestCase::Define()

/* @NOTE: this macro will be used to set a timeout, if the timeout expired,
 * Unittest will be turn to crash state */
#define TIMEOUT(Timer, Process)                                                 \
  {                                                                             \
    using namespace std;                                                        \
    using namespace std::chrono;                                                \
    using namespace std::this_thread;                                           \
                                                                                \
    Base::Unit::Timeout{this, Timer, __LINE__, __FILE__} << [&]() { Process; }; \
  }

/* @NOTE: this macro will be used to turn off fail state to warning state */
#define IGNORE(Block)                                           \
  {                                                             \
    Base::Vertex<void> escaping{[&](){ THIS->Ignore(True); },   \
                                [&](){ THIS->Ignore(False); }}; \
                                                                \
    Block;                                                      \
  }

/* @NOTE: this macro will be used to dump infomation at crashing time */
#define CRASHDUMP(Process)                                        \
  {                                                               \
    using namespace Base::Unit;                                   \
                                                                  \
    auto dumper = std::make_shared<Dump>(this, Dump::ECrashing);  \
                                                                  \
    Dump::Register(dumper);                                       \
    (*dumper) << [&]() { Process; };                              \
  }

/* @NOTE: this macro will be used to dump infomation at finishing time of testcases */
#define FINISHDUMP(Process)                                       \
  {                                                               \
    using namespace Base::Unit;                                   \
                                                                  \
    auto dumper = std::make_shared<Dump>(this, Dump::EFinishing); \
                                                                  \
    Dump::Register(dumper);                                       \
    (*dumper) << [&]() { Process; };                              \
  }

/* @NOTE: this macro will be used to dump infomation at exiting time */
#define EXITDUMP(Process)                                                       \
  {                                                                             \
    Base::Unit::Dump{this, Base::Unit::Dump::EExiting} << [&]() { Process; };   \
  }

#define EXPECT_TRUE(condition) \
  BSExpectTrue(THIS->Unit().get(), condition, #condition, __FILE__, __LINE__)
#define ASSERT_TRUE(condition) \
  BSAssertTrue(THIS->Unit().get(), condition, #condition, __FILE__, __LINE__)

#define EXPECT_FALSE(condition) \
  BSExpectFalse(THIS->Unit().get(), condition, #condition, __FILE__, __LINE__)
#define ASSERT_FASLE(condition) \
  BSAssertFalse(THIS->Unit().get(), condition, #condition, __FILE__, __LINE__)

#define EXPECT_EQ(left, right) \
  BSExpectEqual(THIS, left, #left, right, #right, __FILE__, __LINE__)
#define ASSERT_EQ(left, right) \
  BSAssertEqual(THIS, left, #left, right, #right, __FILE__, __LINE__)

#define EXPECT_NEQ(left, right) \
  BSExpectNotEqual(THIS, left, #left, right, #right, __FILE__, __LINE__)
#define ASSERT_NEQ(left, right) \
  BSAssertNotEqual(THIS, left, #left, right, #right, __FILE__, __LINE__)

#define EXPECT_LT(left, right) \
  BSExpectLower(THIS, left, #left, right, #right, __FILE__, __LINE__)
#define ASSERT_LT(left, right) \
  BSAssertLower(THIS, left, #left, right, #right, __FILE__, __LINE__)

#define EXPECT_LE(left, right) \
  BSExpectLowerOrEqual(THIS, left, #left, right, #right, __FILE__, __LINE__)
#define ASSERT_LE(left, right) \
  BSAssertLowerOrEqual(THIS, left, #left, right, #right, __FILE__, __LINE__)

#define EXPECT_GT(left, right) \
  BSExpectGreater(THIS, left, #left, right, #right, __FILE__, __LINE__)
#define ASSERT_GT(left, right) \
  BSAssertGreater(THIS, left, #left, right, #right, __FILE__, __LINE__)

#define EXPECT_GE(left, right) \
  BSExpectGreaterOrEqual(THIS, left, #left, right, #right, __FILE__, __LINE__)
#define ASSERT_GE(left, right) \
  BSAssertGreaterOrEqual(THIS, left, #left, right, #right, __FILE__, __LINE__)

#define EXPECT_NO_THROW(statement)                                 \
  try {                                                            \
    statement;                                                     \
    THIS->Pass();                                                  \
  } catch(...) {                                                   \
    THIS->Expect("Don\'t have any exception", __FILE__, __LINE__); \
    THIS->Actual(" An exception occur");                           \
    THIS->Fail();                                                  \
  }

#define ASSERT_NO_THROW(statement)                                 \
  try {                                                            \
    statement;                                                     \
    THIS->Pass();                                                  \
  } catch(...) {                                                   \
    THIS->Expect("Don\'t have any exception", __FILE__, __LINE__); \
    THIS->Actual(" An exception occur");                           \
    THIS->Fatal();                                                 \
  }
// @END
#elif !__cplusplus && GNU_COMPILER && C_VERSION >= 201112L
// @BEGIN
/* @NOTE: since C don't work with class name and there are many library working
 * only on C so it would be best to define a simple solution to build a unittest
 * on C only.
 */

/* @NOTE: this macro is used to define our Testcase */
#define TEST(Suite, TestCase)                                \
  /* @NOTE: define our testcase here */                      \
  extern void Run##Suite##TestCase(Unittest unit);           \
  static Unittest Case##Suite##TestCase =                    \
      BSDefineTest(#Suite, #TestCase, Run##Suite##TestCase); \
                                                             \
  void Run##Suite##TestCase(Unittest unit)

#define EXPECT_TRUE(condition) \
  BSExpectTrue(this, condition, #condition, __FILE__, __LINE__)
#define ASSERT_TRUE(condition) \
  BSAssertTrue(this, condition, #condition, __FILE__, __LINE__)

#define EXPECT_FALSE(condition) \
  BSExpectFalse(unit, condition, #condition, __FILE__, __LINE__)
#define ASSERT_FASLE(condition) \
  BSAssertFalse(unit, condition, #condition, __FILE__, __LINE__)

#define EXPECT_EQ(left, right)                                                 \
  {                                                                            \
    typeof(left) lvalue = left;                                                \
    typeof(right) rvalue = right;                                              \
                                                                               \
    UInt ltype = TypeIdForTesting(lvalue);                                     \
    UInt rtype = TypeIdForTesting(rvalue);                                     \
    unit->Clear();                                                             \
    unit->Assign(unit, 0, BSAssign2Any(None, &lvalue, BSTypeIdToType(ltype))); \
    unit->Assign(unit, 1, BSAssign2Any(None, &rvalue, BSTypeIdToType(rtype))); \
    BSExpectWithTwoDescription(unit, #left, "==", #right, __FILE__, __LINE__); \
  }
#define ASSERT_EQ(left, right)                                                 \
  {                                                                            \
    typeof(left) lvalue = left;                                                \
    typeof(right) rvalue = right;                                              \
                                                                               \
    UInt ltype = TypeIdForTesting(lvalue);                                     \
    UInt rtype = TypeIdForTesting(rvalue);                                     \
    unit->Clear();                                                             \
    unit->Assign(unit, 0, BSAssign2Any(None, &lvalue, BSTypeIdToType(ltype))); \
    unit->Assign(unit, 1, BSAssign2Any(None, &rvalue, BSTypeIdToType(rtype))); \
    BSAssertWithTwoDescription(unit, #left, "==", #right, __FILE__, __LINE__); \
  }

#define EXPECT_NEQ(left, right)                                                \
  {                                                                            \
    typeof(left) lvalue = left;                                                \
    typeof(right) rvalue = right;                                              \
                                                                               \
    UInt ltype = TypeIdForTesting(lvalue);                                     \
    UInt rtype = TypeIdForTesting(rvalue);                                     \
    unit->Clear();                                                             \
    unit->Assign(unit, 0, BSAssign2Any(None, &lvalue, BSTypeIdToType(ltype))); \
    unit->Assign(unit, 1, BSAssign2Any(None, &rvalue, BSTypeIdToType(rtype))); \
    BSExpectWithTwoDescription(unit, #left, "!=", #right, __FILE__, __LINE__); \
  }

#define ASSERT_NEQ(left, right)                                                \
  {                                                                            \
    typeof(left) lvalue = left;                                                \
    typeof(right) rvalue = right;                                              \
                                                                               \
    UInt ltype = TypeIdForTesting(lvalue);                                     \
    UInt rtype = TypeIdForTesting(rvalue);                                     \
    unit->Clear();                                                             \
    unit->Assign(unit, 0, BSAssign2Any(None, &lvalue, BSTypeIdToType(ltype))); \
    unit->Assign(unit, 1, BSAssign2Any(None, &rvalue, BSTypeIdToType(rtype))); \
    BSAssertWithTwoDescription(unit, #left, "!=", #right, __FILE__, __LINE__); \
  }

#define EXPECT_LT(left, right)                                                 \
  {                                                                            \
    typeof(left) lvalue = left;                                                \
    typeof(right) rvalue = right;                                              \
                                                                               \
    UInt ltype = TypeIdForTesting(lvalue);                                     \
    UInt rtype = TypeIdForTesting(rvalue);                                     \
    unit->Clear();                                                             \
    unit->Assign(unit, 0, BSAssign2Any(None, &lvalue, BSTypeIdToType(ltype))); \
    unit->Assign(unit, 1, BSAssign2Any(None, &rvalue, BSTypeIdToType(rtype))); \
    BSExpectWithTwoDescription(unit, #left, "<", #right, __FILE__, __LINE__);  \
  }

#define ASSERT_LT(left, right)                                                 \
  {                                                                            \
    typeof(left) lvalue = left;                                                \
    typeof(right) rvalue = right;                                              \
                                                                               \
    UInt ltype = TypeIdForTesting(lvalue);                                     \
    UInt rtype = TypeIdForTesting(rvalue);                                     \
    unit->Clear();                                                             \
    unit->Assign(unit, 0, BSAssign2Any(None, &lvalue, BSTypeIdToType(ltype))); \
    unit->Assign(unit, 1, BSAssign2Any(None, &rvalue, BSTypeIdToType(rtype))); \
    BSAssertWithTwoDescription(unit, #left, "<", #right, __FILE__, __LINE__);  \
  }

#define EXPECT_LE(left, right)                                                 \
  {                                                                            \
    typeof(left) lvalue = left;                                                \
    typeof(right) rvalue = right;                                              \
                                                                               \
    UInt ltype = TypeIdForTesting(lvalue);                                     \
    UInt rtype = TypeIdForTesting(rvalue);                                     \
    unit->Clear();                                                             \
    unit->Assign(unit, 0, BSAssign2Any(None, &lvalue, BSTypeIdToType(ltype))); \
    unit->Assign(unit, 1, BSAssign2Any(None, &rvalue, BSTypeIdToType(rtype))); \
    BSExpectWithTwoDescription(unit, #left, "<=", #right, __FILE__, __LINE__); \
  }

#define ASSERT_LE(left, right)                                                 \
  {                                                                            \
    typeof(left) lvalue = left;                                                \
    typeof(right) rvalue = right;                                              \
                                                                               \
    UInt ltype = TypeIdForTesting(lvalue);                                     \
    UInt rtype = TypeIdForTesting(rvalue);                                     \
    unit->Clear();                                                             \
    unit->Assign(unit, 0, BSAssign2Any(None, &lvalue, BSTypeIdToType(ltype))); \
    unit->Assign(unit, 1, BSAssign2Any(None, &rvalue, BSTypeIdToType(rtype))); \
    BSAssertWithTwoDescription(unit, #left, "<=", #right, __FILE__, __LINE__); \
  }

#define EXPECT_GT(left, right)                                                 \
  {                                                                            \
    typeof(left) lvalue = left;                                                \
    typeof(right) rvalue = right;                                              \
                                                                               \
    UInt ltype = TypeIdForTesting(lvalue);                                     \
    UInt rtype = TypeIdForTesting(rvalue);                                     \
    unit->Clear();                                                             \
    unit->Assign(unit, 0, BSAssign2Any(None, &lvalue, BSTypeIdToType(ltype))); \
    unit->Assign(unit, 1, BSAssign2Any(None, &rvalue, BSTypeIdToType(rtype))); \
    BSExpectWithTwoDescription(unit, #left, ">", #right, __FILE__, __LINE__);  \
  }
#define ASSERT_GT(left, right)                                                 \
  {                                                                            \
    typeof(left) lvalue = left;                                                \
    typeof(right) rvalue = right;                                              \
                                                                               \
    UInt ltype = TypeIdForTesting(lvalue);                                     \
    UInt rtype = TypeIdForTesting(rvalue);                                     \
    unit->Clear();                                                             \
    unit->Assign(unit, 0, BSAssign2Any(None, &lvalue, BSTypeIdToType(ltype))); \
    unit->Assign(unit, 1, BSAssign2Any(None, &rvalue, BSTypeIdToType(rtype))); \
    BSAssertWithTwoDescription(unit, #left, ">=", #right, __FILE__, __LINE__); \
  }

#define EXPECT_GE(left, right)                                                 \
  {                                                                            \
    typeof(left) lvalue = left;                                                \
    typeof(right) rvalue = right;                                              \
                                                                               \
    UInt ltype = TypeIdForTesting(lvalue);                                     \
    UInt rtype = TypeIdForTesting(rvalue);                                     \
    unit->Clear();                                                             \
    unit->Assign(unit, 0, BSAssign2Any(None, &lvalue, BSTypeIdToType(ltype))); \
    unit->Assign(unit, 1, BSAssign2Any(None, &rvalue, BSTypeIdToType(rtype))); \
    BSExpectWithTwoDescription(unit, #left, ">=", #right, __FILE__, __LINE__); \
  }
#define ASSERT_GE(left, right)                                                 \
  {                                                                            \
    typeof(left) lvalue = left;                                                \
    typeof(right) rvalue = right;                                              \
                                                                               \
    UInt ltype = TypeIdForTesting(lvalue);                                     \
    UInt rtype = TypeIdForTesting(rvalue);                                     \
    unit->Clear();                                                             \
    unit->Assign(unit, 0, BSAssign2Any(None, &lvalue, BSTypeIdToType(ltype))); \
    unit->Assign(unit, 1, BSAssign2Any(None, &rvalue, BSTypeIdToType(rtype))); \
    BSAssertWithTwoDescription(unit, #left, ">=", #right, __FILE__, __LINE__); \
  }

// @END
#else
#error "No support your compiler"
#endif  // __cplusplus

/* @NOTE: this part is the place to adapt my C++ helper APIs */
#if __cplusplus
#define UNITTEST_IMPLEMENT True

#include "AssertEqual.h"
#include "AssertGreater.h"
#include "AssertGreaterOrEqual.h"
#include "AssertLower.h"
#include "AssertLowerOrEqual.h"
#include "AssertNotEqual.h"
#include "ExpectEqual.h"
#include "ExpectGreater.h"
#include "ExpectGreaterOrEqual.h"
#include "ExpectLower.h"
#include "ExpectLowerOrEqual.h"
#include "ExpectNotEqual.h"
#endif
#endif  // USE_GTEST
#endif  // BASE_UNITTEST_H_
