#include <Unittest.h>
#include <Logcat.h>
#include <Vertex.h>

TEST(Vertex, Template) {
  auto perform = [&]() {
    Bool begin{False}, end{False};
    Int a = 10;

    /* @NOTE: we expect everything would be done inside here below */
    {
      Base::Vertex<Int, True> sample{
          [&](Int* value) {
            INFO << "start with value " << Base::ToString(*value) << Base::EOL;
            begin = True;
          },
          [&](Int* value) {
            INFO << "end with value " << Base::ToString(*value) << Base::EOL;
            end = True;
          },
          a};

      EXPECT_FALSE(begin);
      EXPECT_FALSE(end);

      Base::Vertex<Int, False> inherited = sample.generate();

      EXPECT_TRUE(begin);
      EXPECT_FALSE(end);
    }

    EXPECT_TRUE(end);
  };

  TIMEOUT(3, { perform(); });
}

TEST(Vertex, Implement) {
  auto perform = [&]() {
    Bool begin{False}, end{False};
    Int a = 10;

    /* @NOTE: we expect everything would be done inside here below */
    {
      Base::Vertex<Int, False> sample{
          [&](Int* value) {
            INFO << "start with value " << Base::ToString(*value) << Base::EOL;
            begin = True;
          },
          [&](Int* value) {
            INFO << "end with value " << Base::ToString(*value) << Base::EOL;
            end = True;
          },
          a};

      EXPECT_TRUE(begin);
      EXPECT_FALSE(end);
    }
  };

  TIMEOUT(3, { perform(); });
}


int main() {
  return RUN_ALL_TESTS();
}

