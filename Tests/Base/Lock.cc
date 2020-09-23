#include <Atomic.h>
#include <Lock.h>
#include <Thread.h>
#include <Unittest.h>
#include <Vertex.h>

#include <stdlib.h> /* srand, rand */
#include <unistd.h>

#define NUM_OF_THREAD 200

namespace Base {
namespace Debug {
void DumpWatch(String parameter);
} // namespace Debug
} // namespace Base

TEST(Lock, Rotate) {
  Base::Lock lock{};
  Int counter{0};

  auto perform = [&]() {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      auto done = threads[i].Start([i, &lock, &counter]() {
        Base::Vertex<Void> escaping{[&]() { lock(); }, [&]() { lock(); }};
        INC(&counter);
      });

      if (done) {
        IGNORE({ EXPECT_NEQ(threads[i].Status(), Base::Thread::Unknown); });
      }

      if (threads[i].Status() != Base::Thread::Initing) {
        EXPECT_NEQ(threads[i].Identity(), ULong(0));
      }
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

  TIMEOUT(50, { perform(); });

  /* @NOTE: check if the locks are locked */
  IGNORE({
    EXPECT_FALSE(lock);
  });

  EXPECT_EQ(counter, NUM_OF_THREAD);
}

TEST(Lock, Jolting) {
  Base::Lock lock{};
  Int counter{0};

  auto perform = [&]() {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      auto done = threads[i].Start([i, &lock, &counter]() {
        Base::Vertex<Void> escaping{[&]() { lock(True); },
                                    [&]() { lock(False); }};
        INC(&counter);
      });

      if (done) {
        IGNORE({ EXPECT_NEQ(threads[i].Status(), Base::Thread::Unknown); });
      }

      if (threads[i].Status() != Base::Thread::Initing) {
        EXPECT_NEQ(threads[i].Identity(), ULong(0));
      }
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

  TIMEOUT(50, { perform(); });

  /* @NOTE: check if the locks are locked */
  EXPECT_FALSE(lock);
  EXPECT_EQ(counter, NUM_OF_THREAD);
}

TEST(Lock, Race0) {
  Base::Lock lock{};
  Vector<Int> storage{};
  Int counter{0};

  auto perform = [&]() {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      auto done = threads[i].Start([&lock, &counter, &storage]() {
        Base::Vertex<Void> escaping{[&]() { lock(); }, [&]() { lock(); }};
        storage.push_back(counter);
        storage.pop_back();
        INC(&counter);
      });

      if (done) {
        IGNORE({ EXPECT_NEQ(threads[i].Status(), Base::Thread::Unknown); });
      }

      if (threads[i].Status() != Base::Thread::Initing) {
        EXPECT_NEQ(threads[i].Identity(), ULong(0));
      }
    }
  };

  CRASHDUMP({
    VERBOSE << Base::Format{"crash at {}"}.Apply(storage.back()) << Base::EOL;

    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucks.Unlock");
  });

  FINISHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucks.Unlock");
  });

  TIMEOUT(50, { perform(); });

  /* @NOTE: check if the locks are locked */
  IGNORE({ EXPECT_FALSE(lock); });
  EXPECT_EQ(counter, NUM_OF_THREAD);
}

TEST(Lock, Race1) {
  Base::Lock lock{};
  Vector<Int> storage{};
  Int counter{0};

  auto perform = [&]() {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      auto done = threads[i].Start([i, &lock, &counter, &storage]() {
        Base::Vertex<Void> escaping{[&]() { lock(True); },
                                    [&]() { lock(False); }};
        storage.push_back(counter);
        storage.pop_back();
        INC(&counter);
      });

      if (done) {
        IGNORE({ EXPECT_NEQ(threads[i].Status(), Base::Thread::Unknown); });
      }

      if (threads[i].Status() != Base::Thread::Initing) {
        EXPECT_NEQ(threads[i].Identity(), ULong(0));
      }
    }
  };

  CRASHDUMP({
    VERBOSE << Base::Format{"crash at {}"}.Apply(storage.back()) << Base::EOL;

    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucks.Unlock");
  });

  FINISHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucks.Unlock");
  });

  TIMEOUT(50, { perform(); });

  /* @NOTE: check if the locks are locked */
  EXPECT_FALSE(lock);
  EXPECT_EQ(counter, NUM_OF_THREAD);
}

TEST(Lock, Race2) {
  Base::Lock lock{};
  Vector<Int> storage{};
  Int counter{0};

  auto perform = [&]() {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      auto done = threads[i].Start([&lock, &counter, &storage]() {
        lock.Safe([&]() {
          storage.push_back(counter);
          storage.pop_back();
          INC(&counter);
        });
      });

      if (done) {
        IGNORE({ EXPECT_NEQ(threads[i].Status(), Base::Thread::Unknown); });
      }

      if (threads[i].Status() != Base::Thread::Initing) {
        EXPECT_NEQ(threads[i].Identity(), ULong(0));
      }
    }
  };

  CRASHDUMP({
    VERBOSE << Base::Format{"crash at {}"}.Apply(storage.back()) << Base::EOL;

    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucks.Unlock");
  });

  FINISHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucks.Unlock");
  });

  TIMEOUT(50, { perform(); });

  /* @NOTE: check if the locks are locked */
  EXPECT_FALSE(lock);
  EXPECT_EQ(counter, NUM_OF_THREAD);
}

TEST(Lock, Slowdown0) {
  Base::Lock lock{};
  Vector<Int> storage{};
  Int counter{0};

  auto perform = [&]() {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      auto done = threads[i].Start([&lock, &counter, &storage]() {
        Base::Vertex<Void> escaping{[&]() { lock(); }, [&]() { lock(); }};
        usleep(rand() % 100 + 1);

        storage.push_back(counter);
        storage.pop_back();
        INC(&counter);
      });

      if (done) {
        IGNORE({ EXPECT_NEQ(threads[i].Status(), Base::Thread::Unknown); });
      }

      if (threads[i].Status() != Base::Thread::Initing) {
        EXPECT_NEQ(threads[i].Identity(), ULong(0));
      }
    }
  };

  CRASHDUMP({
    VERBOSE << Base::Format{"crash at {}"}.Apply(storage.back()) << Base::EOL;

    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucks.Unlock");
  });

  FINISHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucks.Unlock");
  });

  TIMEOUT(50, { perform(); });

  /* @NOTE: check if the locks are locked */
  IGNORE({ EXPECT_FALSE(lock); });
  EXPECT_EQ(counter, NUM_OF_THREAD);
}

TEST(Lock, Slowdown1) {
  Base::Lock lock{};
  Vector<Int> storage{};
  Int counter{0};

  auto perform = [&]() {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      auto done = threads[i].Start([&lock, &counter, &storage]() {
        Base::Vertex<Void> escaping{[&]() { lock(True); },
                                    [&]() { lock(False); }};
        usleep(rand() % 100 + 1);

        storage.push_back(counter);
        storage.pop_back();
        INC(&counter);
      });

      if (done) {
        IGNORE({ EXPECT_NEQ(threads[i].Status(), Base::Thread::Unknown); });
      }

      if (threads[i].Status() != Base::Thread::Initing) {
        EXPECT_NEQ(threads[i].Identity(), ULong(0));
      }
    }
  };

  CRASHDUMP({
    VERBOSE << Base::Format{"crash at {}"}.Apply(storage.back()) << Base::EOL;

    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucks.Unlock");
  });

  FINISHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucks.Unlock");
  });

  TIMEOUT(50, { perform(); });

  /* @NOTE: check if the locks are locked */
  EXPECT_FALSE(lock);
  EXPECT_EQ(counter, NUM_OF_THREAD);
}

TEST(Lock, Slowdown2) {
  Base::Lock lock{};
  Vector<Int> storage{};
  Int counter{0};

  auto perform = [&]() {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      auto done = threads[i].Start([i, &lock, &counter, &storage]() {
        lock.Safe([&]() {
          usleep(rand() % 100 + 1);

          storage.push_back(counter);
          storage.pop_back();
          INC(&counter);
        });
      });

      if (done) {
        IGNORE({ EXPECT_NEQ(threads[i].Status(), Base::Thread::Unknown); });
      }

      if (threads[i].Status() != Base::Thread::Initing) {
        EXPECT_NEQ(threads[i].Identity(), ULong(0));
      }
    }
  };

  CRASHDUMP({
    VERBOSE << Base::Format{"crash at {}"}.Apply(storage.back()) << Base::EOL;

    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucks.Unlock");
  });

  FINISHDUMP({
    Base::Debug::DumpWatch("Stucks");
    Base::Debug::DumpWatch("Counters");
    Base::Debug::DumpWatch("Stucks.Unlock");
  });

  TIMEOUT(50, { perform(); });

  /* @NOTE: check if the locks are locked */
  EXPECT_FALSE(lock);
  EXPECT_EQ(counter, NUM_OF_THREAD);
}
#if 0
TEST(Lock, Random) {
  Base::Lock lock{};
  Int counter{0};

  auto perform = [&]() {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      auto done = threads[i].Start([i, &lock, &counter]() {
        Bool flag = rand()%2 == 1;

        Base::Vertex<Void> escaping{[&](){ lock(flag); }, [&](){ lock(!flag); }};
        usleep(rand()%100 + 1);

        INC(&counter);
      });

      if (done) {
        IGNORE({
          EXPECT_NEQ(threads[i].Status(), Base::Thread::Unknown);
        });
      }

      if (threads[i].Status() != Base::Thread::Initing) {
        EXPECT_NEQ(threads[i].Identity(), ULong(0));
      }
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

  TIMEOUT(100, { perform(); });

  /* @NOTE: check if the locks are locked */
  EXPECT_FALSE(lock);
  EXPECT_EQ(counter, NUM_OF_THREAD);
}
#endif

int main() {
  Base::Log::Level() = EDebug;
  return RUN_ALL_TESTS();
}
