#include <Unittest.h>

#define FAIL_LABEL "[     FAIL ]"

UInt Step1 = 0;
UInt Step2 = 0;

TEST(Unittest, Assertion) {
  EXPECT_EQ(1, 1);
  Step1++; ASSERT_EQ(1, 2);
  Step1++; ASSERT_NEQ(1, 1);
  Step1++; ASSERT_GT(1, 2);
  Step1++; ASSERT_LT(2, 1);
  Step1++; ASSERT_LE(2, 1);
  Step1++; ASSERT_GE(1, 2);
  Step1++; EXPECT_NEQ(1, 1);
}

TEST(Unittest, Expect) {
  Step2++; EXPECT_NEQ(1, 1);
  Step2++; EXPECT_EQ(1, 2);
  Step2++; EXPECT_GT(1, 2);
  Step2++; EXPECT_LT(2, 1);
  Step2++; EXPECT_LE(2, 1);
  Step2++; EXPECT_GE(1, 2);
}

int main() {
  UInt code = 0;

  Base::Log::Level() = EDebug;

  if (RUN_ALL_TESTS() == 0) {
    Bug(EBadLogic, "Test shouldn\'t be passed");
  } 

  if (Step1 > 1) {
    VLOG(EInfo) << RED(FAIL_LABEL) << " It seems that Assert can\'t stop test "
                                      "case as expected"
                << Base::EOL;
    code = 1;
  }

  if (Step2 < 6) {
    VLOG(EInfo) << RED(FAIL_LABEL) << " It seems that Expect has stop test case "
                                      "sooner than expected"
                << Base::EOL;
    code = 2;
  }

  return code;
}
