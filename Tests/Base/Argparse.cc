#include <Argparse.h>
#include <Unittest.h>

TEST(Argparse, Simple) {
  Base::ArgParse parser{};
  String argv[] = {"Simple", "--text", "abc",
                              "--integer", "10",
                              "--bool", "true"};
  UInt argc = 7;

  parser.AddArgument<String>("--text");
  parser.AddArgument<Int>("--integer");
  parser.AddArgument<Bool>("--bool");

  EXPECT_NO_THROW({ parser(argc, argv, False); });
  EXPECT_EQ(parser["--text"].Get<String>(), "abc");
  EXPECT_EQ(parser["--integer"].Get<Int>(), 10);
  EXPECT_EQ(parser["--bool"].Get<Bool>(), True);
}

int main() {
  return RUN_ALL_TESTS();
}