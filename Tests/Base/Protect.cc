#include <Logcat.h>
#include <Type.h>
#include <Unittest.h>
#include <Utils.h>

#include <sys/mman.h>

TEST(Protect, Execuable) {
  auto address = ABI::Memallign(sysconf(_SC_PAGE_SIZE), 1.0);
  auto codes = PROT_EXEC | PROT_READ | PROT_WRITE;

  ASSERT_NEQ(address, None);
  EXPECT_EQ(Base::Protect(address, sysconf(_SC_PAGE_SIZE), codes), ENoError);

  if (address) {
    ABI::Free(address);
  }
}

int main() { return RUN_ALL_TESTS(); }
