#include <Unittest.h>
#include <Type.h>
#include <Logcat.h>
#include <Utils.h>

#include <sys/mman.h>

TEST(Protect, Execuable) {
  auto address = ABI::Calloc(10, 1);
  auto codes = PROT_EXEC|PROT_READ|PROT_WRITE;

  ASSERT_NEQ(address, None);

  IGNORE({
    EXPECT_EQ(Base::Protect(address, 10, codes), ENoError);
  });

  if (address) {
    ABI::Free(address);
  }
}

int main() {
  return RUN_ALL_TESTS();
}
