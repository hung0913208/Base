#include <Auto.h>
#include <Unittest.h>

TEST(Auto, Assign) {
  Base::Auto int1{10};
  Base::Auto str1{"test"};
  Base::Auto float1{10.2};

  IGNORE({
    EXPECT_EQ(int1.Get<Int>(), 10);
    EXPECT_EQ(str1.Get<char const*>(), "test");
    EXPECT_EQ(float1.Get<Double>(), 10.2);
  });
}

TEST(Auto, Reff) {
  Base::Auto sample{"test"};
  Base::Auto target{};
  Base::Auto none{};

  EXPECT_TRUE(target.Reff(sample));
  EXPECT_FALSE(target.Reff(none));
}

TEST(Auto, Move) {
  Base::Auto sample0{10};
  Base::Auto sample1{(CString)("test")};

  Int output0 = 0;
  CString output1 = None;

  EXPECT_EQ(sample0.Move(output0, True), ENoError);
  EXPECT_EQ(output0, 10);

  EXPECT_EQ(sample1.Move(output1, True), ENoError);
  ASSERT_NEQ(output1, None);
  EXPECT_TRUE(!strcmp(output1, "test"));
}

TEST(Auto, Reuse) {
  Base::Auto target{"10"};

  EXPECT_TRUE(!strcmp(target.Get<const CString>(), "10"));

  target = 10;
  EXPECT_EQ(target.Get<Int>(), 10);

  target.Set(10.2);
  EXPECT_EQ(target.Get<Double>(), 10.2);
}

int main() { return RUN_ALL_TESTS(); }
