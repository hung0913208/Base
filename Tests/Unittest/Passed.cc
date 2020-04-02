#include <Unittest.h>

static Bool Prepared = False;
static Bool Teardowned = False;

TEST(Unittest, Simple) {
  EXPECT_EQ(1, 1);
  EXPECT_EQ("a", "a");
  EXPECT_EQ(1.0, 1.0);

  ASSERT_EQ(1, 1);
  ASSERT_EQ("a", "a");
  ASSERT_EQ(1.0, 1.0);

  EXPECT_NEQ(1, 2);
  EXPECT_NEQ("a", "b");
  EXPECT_NEQ(1.0, 2.0);

  ASSERT_NEQ(1, 2);
  ASSERT_NEQ("a", "b");
  ASSERT_NEQ(1.0, 2.0);

  EXPECT_LT(1, 2);
  EXPECT_LT(1.0, 2.0);

  ASSERT_LT(1, 2);
  ASSERT_LT(1.0, 2.0);

  EXPECT_GT(2, 1);
  EXPECT_GT(2.0, 1.0);

  ASSERT_GT(2, 1);
  ASSERT_GT(2.0, 1.0);

  EXPECT_LE(1, 2);
  EXPECT_LE(1.0, 2.0);

  ASSERT_LE(1, 2);
  ASSERT_LE(1.0, 2.0);

  EXPECT_GE(2, 1);
  EXPECT_GE(2.0, 1.0);

  ASSERT_GE(2, 1);
  ASSERT_GE(2.0, 1.0);

  EXPECT_LE(1, 1);
  EXPECT_LE(1.0, 1.0);

  ASSERT_LE(1, 1);
  ASSERT_LE(1.0, 1.0);

  EXPECT_GE(1, 1);
  EXPECT_GE(1.0, 1.0);

  ASSERT_GE(1, 1);
  ASSERT_GE(1.0, 1.0);
}

PREPARE(Unittest, Simple) {
  Prepared = True;
}

TEARDOWN(Unittest, Simple) {
  Teardowned = True;
}

int main() {
  Base::Log::Level() = EDebug;
  
  if (RUN_ALL_TESTS() != 0) {
    Bug(EBadLogic, "Test shouldn\'t be fail");
  }
  
  if (Prepared != True) {
    Bug(EBadLogic, "It seems pharse prepare isn\'t called");
  }
  
  if (Teardowned != True) {
    Bug(EBadLogic, "It seems pharse teardown isn\'t called");
  }
}
