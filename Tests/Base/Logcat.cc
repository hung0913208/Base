#include <Logcat.h>
#include <Macro.h>
#include <Unittest.h>
#include <Vertex.h>

#if LINUX
#include <syslog.h>
#endif

#define MESSAGE "Test with error `ENoSupport`"
TEST(Logcat, Error) {
  auto perform = [&]() {
    Base::Vertex<void> escaping{[](){ Base::Log::Disable(EError, -1); },
                                [](){ Base::Log::Enable(EError, -1); }};

    auto no_error = NoError;
    auto no_support = (NoSupport << MESSAGE);

    EXPECT_EQ(no_error.code(), ENoError);
    EXPECT_EQ(NoError.code(), ENoError);

    EXPECT_EQ((NoSupport << MESSAGE).message(), MESSAGE);

    EXPECT_EQ(NoSupport(MESSAGE).message(), MESSAGE);

    {
      auto copy_of_no_support = no_support;
      EXPECT_EQ(copy_of_no_support.message(), MESSAGE);
    }

    {
      auto copy_of_no_support = RValue(no_support);
      EXPECT_EQ(copy_of_no_support.message(), MESSAGE);
    }
  };

  TIMEOUT(15, { perform(); });
}

#ifdef _SYS_SYSLOG_H
/* @NOTE: this test case only appeares if we can load syslog.h successfully */

TEST(Logcat, Syslog) {
  INFO << "LOG_ERR = " << Base::ToString(LOG_PRI(LOG_ERR)) << Base::EOL;
  INFO << "LOG_INFO = " << Base::ToString(LOG_PRI(LOG_INFO)) << Base::EOL;
  INFO << "LOG_DEBUG = " << Base::ToString(LOG_PRI(LOG_DEBUG)) << Base::EOL;
  INFO << "LOG_NOTICE = " << Base::ToString(LOG_PRI(LOG_NOTICE)) << Base::EOL;
  INFO << "LOG_WARNING = " << Base::ToString(LOG_PRI(LOG_WARNING)) << Base::EOL;
}
#endif // _SYS_SYSLOG_H

int main() {
  return RUN_ALL_TESTS();
}
