#include <Auto.h>
#include <Unittest.h>
#include <cstring>

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

class Parent {
 public:
  virtual ~Parent() {}

  virtual String Introduce() {
    return "Parent";
  }

  Parent() {}
};

class Children : public Parent {
 public:
  ~Children() {}

  String Introduce() final {
    return "Children";
  }

  Children(): Parent{} {}

};

TEST(Auto, Hierarchy) {
  Base::Auto target{Children{}};

  INFO << target.Nametype() << Base::EOL;
  INFO << target.Get<Children>().Introduce() << Base::EOL;
  INFO << target.GetSubclass<Parent, Children>().Introduce() << Base::EOL;
  INFO << target.GetBaseclass<Parent>().Introduce() << Base::EOL;
}

TEST(Auto, Copy) {
  auto perform = [&]() {
    Base::Auto target{"10"};
    Base::Auto cloned = target.Copy();
    
    EXPECT_NEQ(cloned, None);
    EXPECT_NEQ((ULong)&cloned.Get<const CString>(),
               (ULong)&target.Get<const CString>());
    EXPECT_EQ(strcmp(cloned.Get<const CString>(), target.Get<const CString>()), 0);
  };

  TIMEOUT(1, { perform(); });
}

TEST(Auto, SetWithPushing) {
  Base::Auto target{"10"};
  Base::Auto refereal{target};
  Base::Auto cloned = target.Copy();
   
  EXPECT_NO_THROW({ target.Set("12", True); });

  EXPECT_NEQ(strcmp(cloned.Get<const CString>(), target.Get<char[3]>()), 0);
  EXPECT_NEQ(refereal.Get<const CString>(), None);
  EXPECT_NEQ(strcmp(refereal.Get<const CString>(), target.Get<char[3]>()), 0);
  EXPECT_NEQ((ULong)&cloned.Get<const CString>(),
             (ULong)&refereal.Get<const CString>());
  EXPECT_NEQ((ULong)&target.Get<char[3]>(),
             (ULong)&refereal.Get<const CString>());
}

TEST(Auto, Comparision) {
  Base::Auto left{10};
  Base::Auto right0{12};
  Any right1;

  try {
    left != right0;
  } catch (Base::Exception &error) {
    EXPECT_EQ(error.code(), EBadLogic);
  }

  try {
    left == right0;
  } catch (Base::Exception &error) {
    EXPECT_EQ(error.code(), EBadLogic);
  }

  try {
    left == right1;
  } catch (Base::Exception &error) {
    EXPECT_EQ(error.code(), EBadLogic);
  }

  try {
    left != right1;
  } catch (Base::Exception &error) {
    EXPECT_EQ(error.code(), EBadLogic);
  }
}

int main() {
  return RUN_ALL_TESTS(); 
}
