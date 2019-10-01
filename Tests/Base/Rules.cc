#include <Rules.h>
#include <Thread.h>
#include <Unittest.h>

#define NUM_OF_THREAD 1000

TEST(Rules, Sailing) {
#undef ENDING
#define ENDING 10

  TIMEOUT(10, {
    Base::Thread threads[NUM_OF_THREAD];
    Base::Rules rules{NUM_OF_THREAD};

    EXPECT_TRUE(rules.Init(0));
    EXPECT_NO_THROW(rules.Switch([](Int UNUSED(index), Int status) -> Int {
      return status + 1; 
    }));

    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      threads[i].Start([i, &rules]() {
        while (rules[i].Status() != ENDING) {
          EXPECT_TRUE(rules[i].Next());
        }
      });
    }
  });
}

TEST(Rules, Consuming) {
#undef ENDING
#undef RUNING
#undef WAITING
#define ENDING 0
#define LOADED 3
#define RUNING 2
#define WAITING 1
#define INITING 0

  TIMEOUT(100, {
    Base::Thread threads[NUM_OF_THREAD];
    Base::Rules rules{NUM_OF_THREAD};
    Int count{2*NUM_OF_THREAD};

    EXPECT_TRUE(rules.Init(0));
    EXPECT_TRUE(rules[0].Init(1));

    EXPECT_NO_THROW(rules[0].Switch([&](Int UNUSED(idx), Int st) -> Int {
      if (rules.Count(WAITING) + rules.Count(RUNING) == NUM_OF_THREAD) {
        auto result = (--count) > 0;

        if (result) {
          EXPECT_TRUE(rules[0].Select(WAITING).Up(RUNING));
        }

        return result;
      } else {
        return rules[0].Wait(LOADED);
      }
    }));

    EXPECT_NO_THROW(rules.Switch([&rules](Int idx, Int st) -> Int {
      if (st == RUNING) {
        sleep(0.1); 
      } else {
        return rules[idx].Wait(RUNING);
      }

      return 0;
    }));

    EXPECT_NO_THROW(rules.OnSwitching(INITING, WAITING) = [&](Int idx) {
      if (idx != 0) {
        if (rules.Count(WAITING) + rules.Count(RUNING) == NUM_OF_THREAD) {
          rules[0].Next(LOADED);
        }
      }
    });

    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      threads[i].Start([i, &rules]() {
        rules[i].Next(WAITING);

        while (rules[0].Status() != ENDING) {
          EXPECT_TRUE(rules[i].Next());
        }
      });
    }
  });
}

int main() {
  return RUN_ALL_TESTS();
}
