#include <Argparse.h>
#include <Unittest.h>

using namespace Base;

TEST(Argument, Show) {
  ArgParse parser;

  parser.AddArgument<String>("--config", "The schema model of this program");
  parser.AddArgument<Bool>("--rebuild", "This flag is used to force doing "
                                        "from scratch");
  parser.AddArgument<Bool>("--debug", "This flag is used to turn on debug "
                                      "log with a specific level");

  parser.Usage();
}

int main() {
  return RUN_ALL_TESTS();
}
