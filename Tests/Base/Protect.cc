#include <Unittest.h>
#include <Type.h>
#include <Logcat.h>
#include <Utils.h>

#include <sys/mman.h>

TEST(Protect, Execuable) {
  auto address = ABI::Calloc(10, 1);
  auto codes = PROT_EXEC|PROT_READ|PROT_WRITE;

  EXPECT_EQ(Base::Protect(address, 10, codes), ENoError);
}

int main() {
  return RUN_ALL_TESTS();
}
