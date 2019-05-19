#include <Exception.h>
#include <Logcat.h>
#include <Type.h>
#include <Unittest.h>
#include <Vertex.h>

TEST(Exception, Simple){
  auto perform = [&](){
    Base::Vertex<void> escaping{[](){ Base::Log::Disable(EError, -1); },
                                [](){ Base::Log::Enable(EError, -1); }};
    Bool catched{False};

    try{
      throw Except(EDoNothing, "test simple case");
    } catch(Base::Exception &error){
      EXPECT_EQ(error.code(), EDoNothing);
      catched = True;
    }
    EXPECT_TRUE(catched);
  };

  TIMEOUT(15, { perform(); });
}

int main() {
  return RUN_ALL_TESTS();
}
