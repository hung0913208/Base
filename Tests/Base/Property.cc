#include <Logcat.h>
#include <Property.h>
#include <Type.h>
#include <Unittest.h>

TEST(Property, Default) {
  auto perform0 = [&]() {
    Base::Property<Int> sample;
    Int value = 20;

    sample = 10;
    EXPECT_EQ(sample(), 10);

    sample = value;
    EXPECT_EQ(sample(), value);

    sample.set(10);
    EXPECT_EQ(sample(), 10);

    sample.set(value);
    EXPECT_EQ(sample(), value);
  };

  auto perform1 = [&]() {
    Base::Property<String> sample;
    String value = "20";

    sample = "10";
    EXPECT_EQ(sample(), "10");

    sample = value;
    EXPECT_EQ(sample(), value);

    sample.set("10");
    EXPECT_EQ(sample(), "10");

    sample.set(value);
    EXPECT_EQ(sample(), value);
  };

  auto perform2 = [&]() {
    Base::Property<CString> sample;
    CString value = (char *)"20";

    sample = (char *)"10";
    EXPECT_EQ(sample(), "10");

    sample = value;
    EXPECT_EQ(sample(), value);

    sample.set((char *)"10");
    EXPECT_EQ(sample(), "10");

    sample.set(value);
    EXPECT_EQ(sample(), value);
  };

  TIMEOUT(1, { perform0(); });
  TIMEOUT(1, { perform1(); });
  TIMEOUT(1, { perform2(); });
}

TEST(Property, Set) {
  auto perform0 = [&]() {
    Base::SetProperty<Int> set;
    Vector<Int> reversed{10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

    set = Vector<Int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    for (UInt i = 0; i < set.size(); ++i) {
      EXPECT_EQ(set(i), Int(i + 1));
      EXPECT_EQ(set[i], Int(i + 1));
    }

    set = reversed;
    for (UInt i = 0, j = 10; i < set.size(); ++i, --j) {
      EXPECT_EQ(set(i), Int(j));
      EXPECT_EQ(set[i], Int(j));
    }
  };

  auto perform1 = [&]() {
    Base::SetProperty<String> set;
    Vector<String> reversed{"10", "9", "8", "7", "6", "5", "4", "3", "2", "1"};

    set = Vector<String>{"1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};

    for (UInt i = 0; i < set.size(); ++i) {
      EXPECT_EQ(set(i), Base::ToString(i + 1));
      EXPECT_EQ(set[i], Base::ToString(i + 1));
    }

    set = reversed;
    for (UInt i = 0, j = 10; i < set.size(); ++i, --j) {
      EXPECT_EQ(set(i), Base::ToString(j));
      EXPECT_EQ(set[i], Base::ToString(j));
    }
  };

  auto perform2 = [&]() {
    Base::SetProperty<CString> set;
    Vector<CString> reversed{
        (char *)"10", (char *)"9", (char *)"8", (char *)"7", (char *)"6",
        (char *)"5",  (char *)"4", (char *)"3", (char *)"2", (char *)"1"};

    set = Vector<CString>{(char *)"1", (char *)"2", (char *)"3", (char *)"4",
                          (char *)"5", (char *)"6", (char *)"7", (char *)"8",
                          (char *)"9", (char *)"10"};
    for (UInt i = 0; i < set.size(); ++i) {
      EXPECT_TRUE(!strcmp(set(i), Base::ToString(i + 1).c_str()));
      EXPECT_TRUE(!strcmp(set[i], Base::ToString(i + 1).c_str()));
    }

    set = reversed;
    for (UInt i = 0, j = 10; i < set.size(); ++i, --j) {
      EXPECT_TRUE(!strcmp(set(i), Base::ToString(j).c_str()));
      EXPECT_TRUE(!strcmp(set[i], Base::ToString(j).c_str()));
    }
  };

  TIMEOUT(1, { perform0(); });
  TIMEOUT(1, { perform1(); });
  TIMEOUT(1, { perform2(); });
}

TEST(Property, Map) {
  auto perform0 = [&]() {
    Base::MapProperty<Int, String> mapp;

    mapp = {{10, "ten"}, {3, "three"}, {5, "five"}};

    EXPECT_EQ(mapp[10], "ten");
    EXPECT_EQ(mapp[3], "three");
    EXPECT_EQ(mapp[5], "five");
  };

  auto perform1 = [&]() {
    Base::MapProperty<String, Int> mapp;

    mapp = {{"ten", 10}, {"three", 3}, {"five", 5}};

    EXPECT_EQ(mapp["ten"], 10);
    EXPECT_EQ(mapp["three"], 3);
    EXPECT_EQ(mapp["five"], 5);
  };

  TIMEOUT(1, { perform0(); });
  TIMEOUT(1, { perform1(); });
}

int main() {
  return RUN_ALL_TESTS();
}
