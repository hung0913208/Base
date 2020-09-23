#include <Atomic.h>
#include <List.h>
#include <Thread.h>
#include <Unittest.h>

#define MAX_THREADS 1000

TEST(List, Simple) {
  Function<void()> perform = [&]() {
    Base::List list{};
    UInt sample0, sample1, sample2;

    ULong token0 = list.Add(&sample0);
    ULong token1 = list.Add(&sample1);

    EXPECT_NEQ(token0, ULong(0));
    EXPECT_NEQ(token1, ULong(0));

    list.Dump();
    EXPECT_EQ(Int(list.Size()), 2);

    EXPECT_EQ(list.Exist(token0), 1);
    EXPECT_EQ(list.Exist(token1), 1);
    EXPECT_EQ(list.Exist(&sample0), 1);
    EXPECT_EQ(list.Exist(&sample1), 1);
    EXPECT_EQ(list.Exist(token1 + 1), 0);
    EXPECT_EQ(list.Exist(&sample2), 0);
    EXPECT_NEQ(list.Exist(token1 + 1), 1);
    EXPECT_NEQ(list.Exist(&sample2), 1);

    EXPECT_TRUE(list.Del(token0, &sample0));
    EXPECT_TRUE(list.Del(token1, &sample1));

    EXPECT_EQ(Int(list.Size()), 0);
    EXPECT_EQ(list.Exist(token0), 0);
    EXPECT_EQ(list.Exist(token1), 0);
    EXPECT_EQ(list.Exist(&sample0), 0);
    EXPECT_EQ(list.Exist(&sample1), 0);

    EXPECT_EQ(list.Add(token0, &sample0), ENoError);
    EXPECT_EQ(Int(list.Size()), 1);
    EXPECT_EQ(list.Exist(token0), 1);
  };

  TIMEOUT(300, { perform(); });
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

  TIMEOUT(300, { perform(); });
}

TEST(ListWithDump, ThreadWWithoutSleep) {
  Base::List list{-1, True};

  Function<void()> perform = [&]() {
    Base::Thread threads[MAX_THREADS];

    for (auto i = 0; i < MAX_THREADS; ++i) {
      threads[i].Start([&list]() {
        UInt sample;
        ULong token = list.Add(&sample);

        if (!list.Del(token, &sample)) {
          Notice(EBadAccess, "can't delete as expected");
        }
      });
    }
  };

  TIMEOUT(300, { perform(); });

  EXPECT_EQ(list.Size(), UInt(0));
  EXPECT_NEQ(list.Size(True), UInt(0));
}

TEST(ListWithDump, ThreadWithSleep) {
  Base::List list{-1, True};

  Function<void()> perform = [&]() {
    Base::Thread threads[MAX_THREADS];

    for (auto i = 0; i < MAX_THREADS; ++i) {
      threads[i].Start([&list]() {
        UInt sample;
        ULong token = list.Add(&sample);

        usleep(rand() % 100 + 1);

        if (!list.Del(token, &sample)) {
          Notice(EBadAccess, "can't delete as expected");
        }
      });
    }
  };

  TIMEOUT(300, { perform(); });

  EXPECT_EQ(list.Size(), UInt(0));
  EXPECT_NEQ(list.Size(True), UInt(0));
}

TEST(ListNoDump, ThreadWWithoutSleep) {
  Base::List list{};

  Function<void()> perform = [&]() {
    Base::Thread threads[MAX_THREADS];

    for (auto i = 0; i < MAX_THREADS; ++i) {
      threads[i].Start([&list]() {
        UInt sample;
        ULong token = list.Add(&sample);

        if (!list.Del(token, &sample)) {
          Bug(EBadAccess, "can't delete as expected");
        }
      });
    }
  };

  TIMEOUT(300, { perform(); });

  EXPECT_EQ(list.Size(), UInt(0));
  EXPECT_NEQ(list.Size(True), UInt(0));
}

TEST(ListNoDump, ThreadWWithSleep) {
  Base::List list{};

  Function<void()> perform = [&]() {
    Base::Thread threads[MAX_THREADS];

    for (auto i = 0; i < MAX_THREADS; ++i) {
      threads[i].Start([&list]() {
        UInt sample;
        ULong token = list.Add(&sample);

        usleep(rand() % 100 + 1);

        if (!list.Del(token, &sample)) {
          Bug(EBadAccess, "can't delete as expected");
        }
      });
    }
  };

  TIMEOUT(300, { perform(); });

  EXPECT_EQ(list.Size(), UInt(0));
  EXPECT_NEQ(list.Size(True), UInt(0));
}

int main() { return RUN_ALL_TESTS(); }
