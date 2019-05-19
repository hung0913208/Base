#include <Lock.h>
#include <Thread.h>
#include <Unittest.h>
#include <deque>

#define NUM_OF_THREAD 1000
TEST(DeadLock, TestLocker){
  Mutex mutex;

  pthread_mutex_init(&mutex, None);
  pthread_mutex_lock(&mutex);
  pthread_mutex_unlock(&mutex);
}

TEST(DeadLock, ReuseStopper0) {
  Base::Lock locks[NUM_OF_THREAD];

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
      threads[i].Start([=]() {
        (locks[i])();
      });
    }
  });
}

TEST(DeadLock, ReuseStopper1) {
  Base::Lock locks[NUM_OF_THREAD];

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
      threads[i].Start([=]() {
        locks[i](True);
      });
    }
  });
}

TEST(DeadLock, ReuseStopper2) {
  Base::Lock lock;

  TIMEOUT(30, {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    /* @NOTE: the problem is very simple, we use a locked lock without checking
     * its status and the UI-Thread is stuck when these threads join */
    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      threads[i].Start([=]() {
        lock();
      });
    }
  });
}

TEST(DeadLock, ReuseStopper3) {
  Base::Lock lock;

  TIMEOUT(10, {
    /* @NOTE:  be carefull with lock since we are working on parallel so
     * it may take race condition if we use lambda recklessly */
    Base::Thread threads[NUM_OF_THREAD];

    /* @NOTE: the problem is very simple, we use a locked lock without checking
     * its status and the UI-Thread is stuck when these threads join */
    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      threads[i].Start([=]() {
        lock(True);
      });
    }
  });
}

TEST(DeadLock, ReuseStopper4) {
  Base::Lock locks[NUM_OF_THREAD];

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
      threads[i].Start<Base::Lock*>([](Base::Lock* lock) {
        (*lock)(True);
      }, &locks[i]);
    }
  });
}

TEST(DeadLock, WaitUntilEnd) {
  Base::Lock wait_colaboratory{True}, wait_new_job{True}, fetching;

  auto perform = [&]() {
    /* @NOTE: these parameters are used as provided resource from outsite */
    Base::Thread threads[NUM_OF_THREAD];
    Vector<String> requests{};

    /* @NOTE: these parameters are used as features of a class */
    Queue<String> queue{};
    Bool abandon{False};

    /* @NOTE: the problem happens when we have many threads run on parallel and
     * a queue will work as a load balancing. On UI-Thread we set */
    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      threads[i].Start([i, &queue, &wait_colaboratory, &wait_new_job,
                        &fetching, &abandon]() {

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
  };
  TIMEOUT(20, { perform(); });
}

int main() {
  return RUN_ALL_TESTS();
}
