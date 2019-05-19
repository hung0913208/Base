#include <Logcat.h>
#include <Thread.h>
#include <Unittest.h>
#include <Utils.h>
#include <Vertex.h>

#define NUM_OF_THREAD 10

namespace Base{
namespace Internal{
ULong GetUniqueId();
} // namespace Internal
} // namespace Base

TEST(Thread, Simple) {
  Int counter = 0;
  {
    Base::Thread single_thread;

    single_thread.Start([&]() {
      for (counter = 0; counter < 100; counter++) {
      }
    });
  }

  EXPECT_EQ(counter, 100);
}

TEST(Thread, Error) {
  using namespace Base::Internal;

  Base::Vertex<void> escaping{[](){ Base::Log::Disable(EError, -1); },
                              [](){ Base::Log::Enable(EError, -1); }};
  TIMEOUT(50, {
    Base::Thread threads[NUM_OF_THREAD];

    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      threads[i].Start([&]() {
        BadAccess << Base::Format{"from {}"}.Apply(GetUniqueId())
                  << Base::EOL;
      });
    }
  });
}

TEST(Thread, Exception) {
  using namespace Base::Internal;

  Base::Vertex<void> escaping{[](){ Base::Log::Disable(EError, -1); },
                              [](){ Base::Log::Enable(EError, -1); }};

  TIMEOUT(50, {
    Base::Thread threads[NUM_OF_THREAD];

    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      threads[i].Start([&]() {
        try {
          Except(EBadAccess,
                 Base::Format{"from {}"}.Apply(GetUniqueId()));
        } catch(Base::Exception& except) {
          except.Ignore();
        }
      });
    }
  });
}

int main() {
  return RUN_ALL_TESTS();
}
