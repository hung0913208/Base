#include <Auto.h>
#include <Monitor.h>
#include <Unittest.h>
#include <Utils.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>

using namespace Base;

TEST(Popen, Simple0){
  auto perform = []() {
    Bool is_read{False};
    Shared<Monitor> monitor{Monitor::Make("simple", Monitor::EPipe)};
    Fork fork{[]() -> Int {
      const CString args[] = {"/bin/cat", "/tmp", None};

      execv(args[0], (CString const*)args);
      return -1;
    }};

    monitor->Trigger(Auto::As(fork),
      [&](Auto fd, Auto& UNUSED(content)) -> ErrorCodeE {
        while (True) {
          Char buf[512];
          UInt len = read(fd.Get<Int>(), buf, 512);

          DEBUG(Format{"receive {} bytes"}.Apply(len));
          if(len <= 0) {
            break;
          }

          INFO << buf << Base::EOL;
          is_read = True;
        }

        return ENoError;
      });

    EXPECT_EQ(monitor->Loop(
      [&](Monitor&) -> Bool { return fork.Status() >= 0; }),
      ENoError);
    EXPECT_NEQ(fork.ECode(), -1);
    EXPECT_NEQ(fork.ECode(), 0);
    EXPECT_TRUE(is_read);
  };

  TIMEOUT(5, { perform(); });
}

TEST(Popen, Simple1){
  auto perform = []() {
    Bool is_read{False};
    Shared<Monitor> monitor{Monitor::Make("simple", Monitor::EPipe)};
    Fork fork{[]() -> Int {
      const CString args[] = {"/bin/uname", "-a", None};

      execv(args[0], (CString const*)args);
      return -1;
    }};

    monitor->Trigger(Auto::As(fork),
      [&](Auto fd, Auto& UNUSED(content)) -> ErrorCodeE {
        while (True) {
          Char buf[512];
          UInt len = read(fd.Get<Int>(), buf, 512);

          DEBUG(Format{"receive {} bytes"}.Apply(len));
          if(len <= 0) {
            break;
          }

          INFO << buf << Base::EOL;
          is_read = True;
        }

        return ENoError;
      });

    EXPECT_EQ(monitor->Loop(
      [&](Monitor&) -> Bool { return fork.Status() >= 0; }),
      ENoError);
    EXPECT_NEQ(fork.ECode(), -1);
    EXPECT_NEQ(fork.ECode(), 0);
    EXPECT_TRUE(is_read);
  };

  TIMEOUT(5, { perform(); });
}

TEST(Popen, Simple2){
  auto perform = []() {
    Bool is_read{False};
    Shared<Monitor> monitor{Monitor::Make("simple", Monitor::EPipe)};
    Fork fork{[]() -> Int {
      const CString args[] = {"/bin/uname", "-a", None};
      const Int code = execv(args[0], (CString const*)args);

      FATAL << "Can\'t use execv to perform /bin/uname -a" << Base::EOL;
      return code;
    }};

    monitor->Trigger(Auto::As(fork),
      [&](Auto fd, Auto& UNUSED(content)) -> ErrorCodeE {

        while (True) {
          Char buf[512];
          UInt len = read(fd.Get<Int>(), buf, 512);

          DEBUG(Format{"receive {} bytes"}.Apply(len));
          if ((is_read = len > 0)) {
            INFO << buf << Base::EOL;
          } else {
            break;
          }
        }

        return ENoError;
      });

    EXPECT_EQ(monitor->Loop(
      [&](Monitor&) -> Bool { return fork.Status() >= 0; }),
      ENoError);
    EXPECT_NEQ(fork.ECode(), -1);
    EXPECT_EQ(fork.ECode(), 0);
    EXPECT_FALSE(is_read);
  };

  TIMEOUT(5, { perform(); });
}

TEST(Popen, Stacked0){
  /* @NOTE: this test only check if we can't catch error code as the chain from target
   * to watching process and redirect it to master */

  auto perform = []() {
    Shared<Monitor> monitor{Monitor::Make("simple", Monitor::EPipe)};
    Fork fork{[]() -> Int {
      Int status;
      Fork fork{[]() -> Int {
        const CString args[] = {"/bin/cat", "/tmp", None};
        return execv(args[0], (CString const*)args);
      }};

      do {
        if (waitpid(fork.PID(), &status, WUNTRACED | WCONTINUED) < 0) {
          break;
        }

        if (WIFEXITED(status)) {
          return WEXITSTATUS(status);
        }
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));
      return -1;
    }};

    monitor->Trigger(Auto::As(fork),
      [&](Auto fd, Auto& UNUSED(content)) -> ErrorCodeE {
        while (True) {
          Char buf[512];
          UInt len = read(fd.Get<Int>(), buf, 512);

          if(len <= 0) {
            break;
          }
        }

        return ENoError;
      });

    EXPECT_EQ(monitor->Loop(
      [&](Monitor&) -> Bool { return fork.Status() >= 0; }),
      ENoError);
    EXPECT_NEQ(fork.ECode(), -1);
    EXPECT_NEQ(fork.ECode(), 0);
  };

  TIMEOUT(5, { perform(); });
}

TEST(Popen, Stacked1){
  /* @NOTE: this test only check if we can't catch error code as the chain from target
   * to watching process and redirect it to master */

  auto perform = []() {
    Shared<Monitor> monitor{Monitor::Make("simple", Monitor::EPipe)};
    Fork fork{[]() -> Int {
      Int status;
      Fork fork{[]() -> Int {
        const CString args[] = {"/bin/uname", "-a", None};
        return execv(args[0], (CString const*)args);
      }, False};

      do {
        if (waitpid(fork.PID(), &status, WUNTRACED | WCONTINUED) < 0) {
          break;
        }

        if (WIFEXITED(status)) {
          return WEXITSTATUS(status);
        }
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));
      return -1;
    }};

    monitor->Trigger(Auto::As(fork),
      [&](Auto fd, Auto& UNUSED(content)) -> ErrorCodeE {
        while (True) {
          Char buf[512];
          UInt len = read(fd.Get<Int>(), buf, 512);

          if (len <= 0) {
            break;
          }
        }

        return ENoError;
      });

    EXPECT_EQ(monitor->Loop(
      [&](Monitor&) -> Bool { return fork.Status() >= 0; }),
      ENoError);
    EXPECT_NEQ(fork.ECode(), -1);
    EXPECT_EQ(fork.ECode(), 0);
  };

  TIMEOUT(5, { perform(); });
}

TEST(Popen, Exec) {
  Int status;
  Fork fork{[]() -> Int{
    const CString args[] = {"/bin/cat", "/tmp"};

    execv(args[0], (CString const*)args);
    return -1;
  }};

  do {
    if (waitpid(fork.PID(), &status, WUNTRACED | WCONTINUED) < 0) {
      break;
    }

    if (WIFEXITED(status)) {
      EXPECT_NEQ(WEXITSTATUS(status), 0);
    } else if (WIFSIGNALED(status)) {
      INFO << "killed by signal " << ToString(WTERMSIG(status)) << Base::EOL;
    } else if (WIFSTOPPED(status)) {
      INFO << "stopped by signal " << ToString(WSTOPSIG(status)) << Base::EOL;
    } else if (WIFCONTINUED(status)) {
      INFO << "continued" << Base::EOL;
    }
  } while (!WIFEXITED(status) && !WIFSIGNALED(status));
}

int main(){
  Base::Log::Level() = EDebug;
  return RUN_ALL_TESTS();
}

