#include <Hashtable.h>
#include <Unittest.h>
#include <Utils.h>
#include <cstdlib>

#define MAX_SIZE 1000

TEST(HashtableV1, Simple) {
  auto perform = [&]() {
    for (UInt size = 1; size < MAX_SIZE; ++size) {
      Base::Hashtable<UInt, UInt> int1{size,
        [](UInt* value) -> Int{ return (*value); }
      };

      for (UInt index = 0; index < size; ++index) {
        EXPECT_EQ(int1.Put(index, index), ENoError);
      }

      for (UInt index = 0; index < size; ++index) {
        EXPECT_EQ(int1.Get(RValue(index)), index);
      }
    }
  };

  TIMEOUT(50, { perform(); });
}

TEST(HashtableV1, Random) {
  auto perform = [&]() {
    Base::Pair<UInt, UInt> mapping[MAX_SIZE];

    for (UInt size = 1; size < MAX_SIZE; ++size) {
      Base::Hashtable<UInt, UInt> int1{size,
        [](UInt* key) -> Int{ return (*key); }
      };

      for (UInt i = 0; i < size; ++i) {
        mapping[i].Left = rand();
        mapping[i].Right = rand();

        EXPECT_EQ(int1.Put(mapping[i].Left, mapping[i].Right), ENoError);
      }
    
      for (UInt i = 0; i < size; ++i) { 
        EXPECT_EQ(int1.Get(mapping[i].Left), mapping[i].Right);
      }
    }
  };

  TIMEOUT(50, { perform(); });
}

TEST(HashtableV1, LevelUp){
  auto perform = [&]() {
    Base::Pair<UInt, UInt> mapping[MAX_SIZE];

    for (UInt level = 1; level < MAX_SIZE/5; ++level) {
      Base::Hashtable<UInt, UInt> int1{5,
        [](UInt* value) -> Int{ return *value; }
      };
    
      for (UInt i = 0; i < 5*level; ++i) {
        mapping[i].Left = rand();
        mapping[i].Right = rand();

        EXPECT_EQ(int1.Put(mapping[i].Left, mapping[i].Right), ENoError);
      }

      for (UInt i = 0; i < 5*level; ++i) { 
        EXPECT_EQ(int1.Get(mapping[i].Left), mapping[i].Right);
      }
    }
  };

  TIMEOUT(80, { perform(); });
}

TEST(HashtableV1, PutGet) {
  auto perform = [&]() {
    for (UInt size = 1; size < MAX_SIZE; ++size) {
      Base::Hashtable<UInt, UInt> int1{size,
        [](UInt* key) -> Int{ return *key; }
      };

      for (UInt i = 0; i < size; ++i) {
        EXPECT_EQ(int1.Put(Base::Pair<UInt, UInt>{i*size, i}), ENoError);
      }
   
      for (UInt i = 0; i < size; ++i) { 
        EXPECT_EQ(int1.Get(i*size), i);
      }
    }
  };

  TIMEOUT(200, { perform(); });
}

int main() { 
  Base::Log::Level() = EDebug;
  return RUN_ALL_TESTS(); 
}
