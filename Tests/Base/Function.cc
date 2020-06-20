#include <Type.h>
#include <Function.h>
#include <Unittest.h>

#if !USE_STD_FUNCTION
Bool global = False;

TEST(Function, Simple) {
  Bool local = False;
  Base::Function<void()> fn0 = []() { global = True; };
  Base::Function<void()> fn1 = [=]() { global = True; local = True; };
  Base::Function<void()> fn2 = [&]() { global = True; local = True; };

  global = False;
  fn0();
  EXPECT_TRUE(global);

  global = False;
  local = False;
  fn1();
  EXPECT_TRUE(global);
  EXPECT_FALSE(local);

  global = False;
  local = False;
  fn2();
  EXPECT_TRUE(global);
  EXPECT_TRUE(local);
}
#endif

int main(int argc, CString argv[]) {
  return RUN_ALL_TESTS();
}
