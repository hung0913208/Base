#include <Atomic.h>
#include <List.h>
#include <Thread.h>
#include <Unittest.h>

#define MAX_THREADS 900

TEST(List, Simple) {
  Function<void()> perform = [&]() {
    Base::List list{};
    UInt sample0, sample1;

    ULong token0 = list.Add(&sample0);
    ULong token1 = list.Add(&sample1);

    EXPECT_NEQ(token0, ULong(0));
    EXPECT_NEQ(token1, ULong(0));

    EXPECT_EQ(Int(list.Size()), 2);

    EXPECT_TRUE(list.Del(token0, &sample0));
    EXPECT_TRUE(list.Del(token1, &sample1));

    EXPECT_EQ(Int(list.Size()), 0);
  };

  TIMEOUT(10, { perform(); });
}

TEST(List, Autoremove) {
  Function<void()> perform = [&]() {
    Base::List list{};
    UInt sample0, sample1;

    ULong token0 = list.Add(&sample0);
    ULong token1 = list.Add(&sample1);

    EXPECT_NEQ(token0, ULong(0));
    EXPECT_NEQ(token1, ULong(0));

    EXPECT_EQ(Int(list.Size()), 2);
  };

  TIMEOUT(10, { perform(); });
}

TEST(List, Thread) {
  Base::List list{20, True}; 

  Function<void()> perform = [&]() {
    Base::Thread threads[MAX_THREADS];

    for (auto i = 0; i < MAX_THREADS; ++i) {
      threads[i].Start([i, &list]() {
        UInt sample;
        ULong token = list.Add(&sample);

        if (!list.Del(token, &sample)) {
          Bug(EBadAccess, "can't delete as expected");
        }
      });
    }
  };

  TIMEOUT(10, { perform(); });
  EXPECT_EQ(list.Size(), UInt(0));
}

int main() { return RUN_ALL_TESTS(); }
