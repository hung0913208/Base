#include <Queue.h>
#include <Thread.h>
#include <Unittest.h>

#include <stdlib.h>     /* srand, rand */
#include <unistd.h>

#define MAX_THREADS 900
#define MAX_VALUE 18000

TEST(Queue, Simple) {
  Base::Queue<Int> queue{};
  Function<void()> perform = [&]() {
    for (Int value = 0; value < MAX_VALUE; ++value) {
      EXPECT_TRUE(queue.Put(RValue(value)));
    }

    for (Int expect = 0; expect < MAX_VALUE; ++expect) {
      Int value;

      queue.Done(queue.Get(value));
    }
  };

  TIMEOUT(100, { perform(); } );
  DEBUG(Base::Format{"QSize = {}"}.Apply(queue.QSize()));
  DEBUG(Base::Format{"WSize = {}"}.Apply(queue.WSize()));
}

TEST(Queue, Threads) {
  Base::Queue<Int> queue{};
  Function<void()> perform = [&]() {
    Base::Thread threads[3*MAX_THREADS];
    Int counter = 0;

    for (auto i = 0; i < MAX_THREADS; ++i) {
      threads[i].Start([&counter, &queue]() {
        INC(&counter); 
        queue.Put(RValue(counter));
      });

      threads[MAX_THREADS + i].Start([&queue]() {
        Int value;

        queue.Reject(queue.Get(value));
      });

      threads[2*MAX_THREADS + i].Start([&queue]() {
        Int value;

        queue.Done(queue.Get(value));
      });
    }
  };

  CRASHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucks.Unlock");
  });

  FINISHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucks.Unlock");
  });

  TIMEOUT(200, { perform(); });
  DEBUG(Base::Format{"QSize = {}"}.Apply(queue.QSize()));
  DEBUG(Base::Format{"WSize = {}"}.Apply(queue.WSize()));
}

TEST(Queue, Bandwidth) {
}

int main() {
  Base::Log::Level() = EDebug;
  return RUN_ALL_TESTS();
}
