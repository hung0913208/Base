#include <Unittest.h>
#include <Refcount.h>
#include <Thread.h>

using namespace Base;
#define NUM_OF_THREAD 1000

class Target: public Refcount {
 public:
  ~Target() {}

  Target(const Target& src): Refcount{src} {}
  Target(Target&& src): Refcount{src} {}
  Target(): Refcount() {}
};

TEST(Refcount, Case0) {
  Target root{};
  Target copy1{root};
  Target copy2 = root;
  Target copy3{copy1};
  Target copy4{copy2};
  Target copy5 = copy1;
  Target copy6 = copy2;

  EXPECT_EQ(root.Count(), 6);
}

TEST(Refcount, Case1) {
  Target* root = new Target{};
  Target copy1{*root};
  Target copy2{copy1};
  Target copy3{*root};

  delete root;
  EXPECT_EQ(copy1.Count(), 2);
}

class Exception0: Refcount {
 public:
  ~Exception0() {}

  Exception0(const Exception0& src): Refcount{src} {}
  Exception0(Exception0&& src): Refcount{src} {}
  Exception0(): Refcount() {}
};

class Exception1 {
 public:
  Exception1(Target target): _Target{target} {}

 private:
  Target _Target;
};

TEST(Refcount, Exception) {
  TIMEOUT(500, {
    Base::Thread threads[NUM_OF_THREAD];

    for (auto i = 0; i < NUM_OF_THREAD; ++i) {
      threads[i].Start([&]() {
        try { throw Exception0(); }
        catch(Exception0& except) { }
      });
    }
  });

  TIMEOUT(500, {
    Base::Thread threads[NUM_OF_THREAD/2];

    for (auto i = 0; i < NUM_OF_THREAD/2; ++i) {
      threads[i].Start([&]() {
        try { throw Exception1(Target{}); }
        catch(Exception1& except) { }
      });
    }
  });
}

int main() {
  return RUN_ALL_TESTS();
}
