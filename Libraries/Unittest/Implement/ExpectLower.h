#ifndef UNITTEST_EXPECT_LOWER_H_
#define UNITTEST_EXPECT_LOWER_H_

#ifndef UNITTEST_IMPLEMENT
#include <Unittest.h>
#endif  // UNITTEST_IMPLEMENT

template <typename Left, typename Right>
void BSExpectLower(Case* testcase, Left&& left, String left_describe,
                   Right&& right, String right_describe, String file,
                   UInt line) {
  if (left < right) {
    testcase->Pass();
  } else {
    testcase->Expect("<", left_describe, right_describe, file, line);
    testcase->Actual(">=", Base::ToString(left), Base::ToString(right));
    testcase->Fail();
  }
}

template <typename Left, typename Right>
void BSExpectLower(Case* testcase, const Left& left, String left_describe,
                   const Right& right, String right_describe, String file,
                   UInt line) {
  if (left < right) {
    testcase->Pass();
  } else {
    testcase->Expect("<", left_describe, right_describe, file, line);
    testcase->Actual(">=", Base::ToString(left), Base::ToString(right));
    testcase->Fail();
  }
}

template <typename Left, typename Right>
void BSExpectLower(Case* testcase, Left&& left, String left_describe,
                   const Right& right, String right_describe, String file,
                   UInt line) {
  if (left < right) {
    testcase->Pass();
  } else {
    testcase->Expect("<", left_describe, right_describe, file, line);
    testcase->Actual(">=", Base::ToString(left), Base::ToString(right));
    testcase->Fail();
  }
}

template <typename Left, typename Right>
void BSExpectLower(Case* testcase, const Left& left, String left_describe,
                   Right&& right, String right_describe, String file,
                   UInt line) {
  if (left < right) {
    testcase->Pass();
  } else {
    testcase->Expect("<", left_describe, right_describe, file, line);
    testcase->Actual(">=", Base::ToString(left), Base::ToString(right));
    testcase->Fail();
  }
}
#endif  // UNITTEST_EXPECT_LOWER_H_
