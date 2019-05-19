#include <Glob.h>
#include <Macro.h>
#include <Unittest.h>

TEST(Glob, Simple) {
  auto paths = Base::Glob{"/bin/*"}.AsList();

#ifndef WINDOW
  EXPECT_NEQ(Base::Find(paths.begin(), paths.end(), String{"/bin/sh"}), -1);
#endif
  EXPECT_NEQ(paths.size(), UInt(0));
}

int main() { return RUN_ALL_TESTS(); }
