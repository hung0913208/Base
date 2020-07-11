#include <Auto.h>
#include <Unittest.h>
#include <Utils.h>

TEST(Tie, Simple) {
  Vector<Base::Auto> args;
  Int a, b;

  args.push_back(Base::Auto::As(1));
  args.push_back(Base::Auto::As(2));

  Base::Bond(a, b) = args;

  EXPECT_EQ(a, 1);
  EXPECT_EQ(b, 2);
}

int main() { return RUN_ALL_TESTS(); }
