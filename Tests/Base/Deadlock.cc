#include <Atomic.h>
#include <Lock.h>
#include <Thread.h>
#include <Unittest.h>
#include <deque>

namespace Base {
namespace Debug {
void DumpWatch(String parameter);
} // namespace Debug
} // namespace Base

#define NUM_OF_THREAD 1000
TEST(DeadLock, TestLocker){
  auto perform = [&]() {
    Mutex mutex;
    EXPECT_FALSE(MUTEX(&mutex));
    EXPECT_FALSE(LOCK(&mutex));
    EXPECT_FALSE(UNLOCK(&mutex));
    EXPECT_FALSE(DESTROY(&mutex));
  };

  TIMEOUT(10, { perform(); });
}

TEST(DeadLock, ReuseStopper0) {
  Base::Lock locks[NUM_OF_THREAD];
  Int counter{0};

  auto perform = [&]() {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    /* @NOTE: random choice to make a stopper or not */
    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      locks[i]();
    }

    /* @NOTE: check if the locks are locked */
    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      EXPECT_TRUE(locks[i]);
    }

    /* @NOTE: the problem is very simple, we use a locked lock without checking
     * its status and the UI-Thread is stuck when these threads join */
    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      threads[i].Start([i, locks, &counter]() {
        (locks[i])();
        INC(&counter);
      });

      EXPECT_NEQ(threads[i].Status(), Base::Thread::Unknown);

      if (threads[i].Status() != Base::Thread::Initing) {
        EXPECT_NEQ(threads[i].Identity(), ULong(0));
      }
    }
  };

#if DEBUGING
  CRASHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
  });

  FINISHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucs.Unlock");
  });
#endif

  TIMEOUT(10, { perform(); });

  /* @NOTE: check if the locks are locked */
  for (auto i = 0; i < NUM_OF_THREAD; ++i) {
    EXPECT_FALSE(locks[i]);
  }

  EXPECT_EQ(counter, NUM_OF_THREAD);
}

TEST(DeadLock, ReuseStopper1) {
  Base::Lock locks[NUM_OF_THREAD];
  Int counter{0};

#if DEBUGING
  CRASHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
  });

  FINISHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucs.Unlock");
  });
#endif

  TIMEOUT(10, {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    /* @NOTE: random choice to make a stopper or not */
    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      locks[i]();
    }

    /* @NOTE: the problem is very simple, we use a locked lock without checking
     * its status and the UI-Thread is stuck when these threads join */
    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      threads[i].Start([i, locks, &counter]() {
        locks[i](True);
        INC(&counter);
      });
    }
  });

  EXPECT_EQ(counter, NUM_OF_THREAD);
}

TEST(DeadLock, ReuseStopper2) {
  Base::Lock lock{};
  Int counter{0};

#if DEBUGING
  CRASHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch( "Stucs.Unlock");
  });

  FINISHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucs.Unlock");
  });
#endif

  TIMEOUT(10, {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    /* @NOTE: the problem is very simple, we use a locked lock without checking
     * its status and the UI-Thread is stuck when these threads join */
    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      threads[i].Start([&counter, lock]() {
        lock();
        INC(&counter);
      });
    }
  });

  EXPECT_EQ(counter, NUM_OF_THREAD);
}

TEST(DeadLock, ReuseStopper3) {
  Base::Lock lock{True};
  Int counter{0};

#if DEBUGING
  CRASHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
  });

  FINISHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucs.Unlock");
  });
#endif

  TIMEOUT(60, {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    /* @NOTE: the problem is very simple, we use a locked lock without checking
     * its status and the UI-Thread is stuck when these threads join */
    EXPECT_TRUE(lock);

    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      threads[i].Start([lock, &counter]() {
        lock(True);
        INC(&counter);
      });
    }

    EXPECT_FALSE(lock);
  });

  EXPECT_EQ(counter, NUM_OF_THREAD);
}

TEST(DeadLock, ReuseStopper4) {
  Base::Lock locks[NUM_OF_THREAD];
  Int counter{0};

#if DEBUGING
  CRASHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
  });

  FINISHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucs.Unlock");
  });
#endif

  TIMEOUT(100, {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    /* @NOTE: random choice to make a stopper or not */
    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      locks[i]();
    }

    /* @NOTE: check if the locks are locked */
    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      EXPECT_TRUE(locks[i]);
    }

    /* @NOTE: the problem is very simple, we use a locked lock without checking
     * its status and the UI-Thread is stuck when these threads join */
    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      threads[i].Start<Base::Lock*>([&counter](Base::Lock* lock) {
        (*lock)(True);
        INC(&counter);
      }, &locks[i]);
    }
  });

  EXPECT_EQ(counter, NUM_OF_THREAD);
}

TEST(DeadLock, WaitUntilEnd) {
  Base::Lock wait_colaboratory{True}, wait_new_job{True}, fetching;

#if DEBUGING
  CRASHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
  });

  FINISHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucs.Unlock");
  });
#endif

  TIMEOUT(10, {
    /* @NOTE: these parameters are used as provided resource from outsite */
    Base::Thread threads[NUM_OF_THREAD];
    Vector<String> requests{};

    /* @NOTE: these parameters are used as features of a class */
    Queue<String> queue{};
    Bool abandon{False};

    /* @NOTE: the problem happens when we have many threads run on parallel and
     * a queue will work as a load balancing. On UI-Thread we set */
    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      threads[i].Start([&queue, &wait_colaboratory, &wait_new_job, &fetching, &abandon]() {

        /* @NOTE: wait until their colaboraters show up */
        wait_colaboratory.Wait([]() -> Bool { return True; });
                              // [&]() { VERBOSE << "Pass step 1 of Thread "
                              //                 << Base::ToString(i)
                              //                 << Base::EOL; });

        /* @TODO: receive jobs and perform them from top to down respectively */
        while (!abandon && queue.size() > 0) {
          String job;

          /* @NOTE: wait if we are jobless to save resource */
          wait_new_job.Wait([&]() -> Bool { return queue.size() == 0; });

          // VERBOSE << "Pass step 2 of Thread " << Base::ToString(i)
          //         << Base::EOL;

          /* @NOTE: receive a new job */
          fetching.Safe([&]() {
            if (queue.size()) {
              job = queue.back();
              queue.pop();
            }
          });

          /* @TODO: do it right now */
        }

        // VERBOSE << "Pass step 3 of Thread " << Base::ToString(i)
        //         << Base::EOL;
      });
    }

    /* @TODO: push several task from UI-Thread to Co-Workers */
    for (auto job : requests){}

    /* @TODO: send a request abandone to the whole system */
    abandon = True;
  });
}

int main() {
  Base::Log::Level() = EDebug;
  return RUN_ALL_TESTS();
}
