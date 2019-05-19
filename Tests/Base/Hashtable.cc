#include <Hashtable.h>
#include <Unittest.h>

#define MAX_SIZE 1000

TEST(Hashtable, Simple) {
  for (UInt size = 1; size < MAX_SIZE; ++size) {
    Base::Hashtable<UInt, UInt> int1{size,
      [](UInt* value) -> Int{ return *value; }
    };

    for (UInt index = 0; index < size; ++index) {
      int1.Put(index, index);
    }

    for (UInt index = 0; index < size; ++index) {
      EXPECT_EQ(int1.Get(RValue(index)), index);
    }
  }
}

TEST(Hashtable, LevelUp){
  Base::Hashtable<UInt, UInt> int1{5,
    [](UInt* value) -> Int{ return *value; }
  };

  for (UInt index = 0; index < 10; ++index) {
    int1.Put(index, index);
  }

  for (UInt index = 0; index < 10; ++index) {
    EXPECT_EQ(int1.Get(RValue(index)), index);
  }
}

int main() { RUN_ALL_TESTS(); }
