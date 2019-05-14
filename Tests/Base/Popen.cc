#include <Auto.h>
#include <Monitor.h>
#include <Unittest.h>
#include <Utils.h>

using namespace Base;

TEST(Popen, Simple){
  auto perform = []() {
    Bool is_read{False};
    Shared<Monitor> monitor{Monitor::Make("simple", Monitor::EPipe)};
    Fork fork{[]() -> Int{
      const CString args[] = {"uname", "-a"};

      DEBUG("This test is running on a child process");
      execvp(args[0], (CString const*)args);
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

          fprintf(stderr, "read %d bytes\n", len);
          write(STDOUT_FILENO, buf, len);
          is_read = True;
        }

        return ENoError;
      });

    EXPECT_EQ(monitor->Loop(
        [&](Monitor&) -> Bool {
          return !(fork.Status() & Fork::EExited);
        }),
      ENoError);

    EXPECT_TRUE(is_read);
  };

  TIMEOUT(5, { perform(); });
}

int main(){
  Base::Log::Level() = EDebug;
  return RUN_ALL_TESTS();
}

