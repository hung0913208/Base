#include <Auto.h>
#include <Monitor.h>
#include <Unittest.h>
#include <Utils.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>

using namespace Base;

TEST(Popen, Simple){
  auto perform = []() {
    Bool is_read{False};
    Shared<Monitor> monitor{Monitor::Make("simple", Monitor::EPipe)};
    Fork fork{[]() -> Int {
      Int status;
      Fork fork{[]() -> Int {
        const CString args[] = {"/bin/cat", "/tmp"};

        execv(args[0], (CString const*)args);
        return -1;
      }, False};

      DEBUG("This test is running on a child process");
      do {
        if (waitpid(fork.PID(), &status, WUNTRACED | WCONTINUED) < 0) {
          break;
        }

        if (WIFEXITED(status)) {
          return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
          INFO << "killed by signal " << ToString(WTERMSIG(status)) << Base::EOL;
        } else if (WIFSTOPPED(status)) {
          INFO << "stopped by signal " << ToString(WSTOPSIG(status)) << Base::EOL;
        } else if (WIFCONTINUED(status)) {
          INFO << "continued" << Base::EOL;
        }
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));
      return -1;
    }, False};

    monitor->Trigger(Auto::As(fork),
      [&](Auto fd, Auto& UNUSED(content)) -> ErrorCodeE {
        while (True) {
          Char buf[512];
          UInt len = read(fd.Get<Int>(), buf, 512);

          if(len <= 0) {
            break;
          }

          fprintf(stderr, "read %d bytes\n", len);
          write(STDOUT_FILENO, buf, len);
          is_read = True;
        }

        return ENoError;
      });

    EXPECT_EQ(monitor->Loop(
      [&](Monitor&) -> Bool { return fork.Status() >= 0; }),
      ENoError);
    INFO << "exit code is " << Base::ToString(fork.ECode()) << Base::EOL;
    EXPECT_TRUE(is_read);
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
      INFO << "exited, status=" << ToString(WEXITSTATUS(status)) << Base::EOL;
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

