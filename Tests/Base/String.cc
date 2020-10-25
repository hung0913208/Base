#include <algorithm>
#include <cstring>

#include <Logcat.h>
#include <Type.h>
#include <Unittest.h>
#include <Utils.h>

TEST(String, Append) {
  {
    String sample;

    sample += '0';
    EXPECT_TRUE(!strcmp(sample.data(), "0"));

    sample += Char(8 + '0');
    EXPECT_TRUE(!strcmp(sample.data(), "08"));

    for (auto i = 0; i < 10; i++) {
      sample += Char(i + '0');
    }
    EXPECT_TRUE(!strcmp(sample.data(), "080123456789"));

    sample.resize(8);
    EXPECT_EQ(sample.size(), UInt(8));

    EXPECT_TRUE(!strcmp(sample.data(), "08012345"));

    for (auto i = 0; i < 10; i++) {
      sample += Char(i + '0');
    }
    EXPECT_TRUE(!strcmp(sample.data(), "080123450123456789"));
  }

  {
    String sample{"sample"};
    EXPECT_EQ(sample.append('s'), "samples");
    EXPECT_EQ(sample.append(5, 'a'), "samplesaaaaa");
    EXPECT_EQ(sample.append("sample"), "samplesaaaaasample");
  }
}

TEST(String, ConvertToReal) {
  String target{};
  Double sample = 1234.12314414;
  Int ivalue = Int(sample);
  Double rvalue = sample - ivalue;

  while (ivalue != 0) {
    Int i = ivalue % 10;

    target += Char(i + '0');
    ivalue /= 10;
  }
  EXPECT_EQ(target, "4321");

#ifdef BASE_TYPE_STRING_H_
  target.reverse();
#else
  std::reverse(target.begin(), target.end());
#endif

  EXPECT_EQ(target, "1234");

  target.append(".");
  EXPECT_EQ(target, "1234.");

  for (auto i = 0; i < 8; ++i) {
    Int r = Int(rvalue * 10);

    target += Char(r + '0');
    rvalue = rvalue * 10.0 - r;
  }
  EXPECT_EQ(target, "1234.12314414");
}

TEST(String, Clone) {
  String sample = "abcxyz";
  String target{sample};

  EXPECT_EQ(sample, target);
  EXPECT_EQ(sample.data(), target.data());

  sample.clear();
  EXPECT_TRUE(!strcmp(target.data(), "abcxyz"));
}

TEST(String, Compare) {
  String sample = "123456";
  String target1 = "123456";
  String target2 = "123456bc";
  String target3 = "12345";
  String target4 = "123457";

  EXPECT_TRUE(sample == sample);
  EXPECT_TRUE(sample == target1);
  EXPECT_TRUE(sample < target2);
  EXPECT_FALSE(sample < target3);
  EXPECT_TRUE(sample < target4);
  EXPECT_TRUE(sample != target4);
}

TEST(String, Join) {
  String sample = "123";

  EXPECT_EQ(("456" + sample), "456123");
}

TEST(String, Present) {
  String sample = "Test with string";
  Base::Vertex<void> escaping{[]() { Base::Log::Disable(10, -1); },
                              []() { Base::Log::Enable(10, -1); }};

  VLOG(10) << "hello world" << Base::EOL;
  VLOG(10) << sample << Base::EOL;
  VLOG(10) << (GREEN << "turn color -> ") << String{"hello world"} << " mixing"
           << Base::EOL;
}

TEST(String, Parameter) {
  Base::Vertex<void> escaping{[]() { Base::Log::Disable(10, -1); },
                              []() { Base::Log::Enable(10, -1); }};

  auto normal = [](String value) { VLOG(10) << value << Base::EOL; };

  auto rvalue = [](String &&value) { VLOG(10) << value << Base::EOL; };

  auto lvalue = [](String &value) { VLOG(10) << value << Base::EOL; };

  auto perform = [&]() {
    String helloworld{"hello world"};

    normal("hello world");
    rvalue(String{"hello world"});
    rvalue(RValue(helloworld));
    lvalue(helloworld);
  };

  TIMEOUT(1, { perform(); });
}

class Exception {
public:
  Exception(String message) : _Message{message} {}
  Exception(const CString message) : _Message{message} {}

private:
  String _Message;
};

TEST(String, Throwing) {
  Base::Vertex<void> escaping{[]() { Base::Log::Disable(EError, -1); },
                              []() { Base::Log::Enable(EError, -1); }};
  TIMEOUT(1, {
    try {
      throw String("hello world");
    } catch (String &) {
    }
  });

  TIMEOUT(1, {
    try {
      throw String("hello world");
    } catch (String) {
    }
  });

  TIMEOUT(1, {
    try {
      throw Exception("hello world");
    } catch (Exception &) {
    }
  });

  TIMEOUT(1, {
    try {
      throw Exception(String{"hello world"});
    } catch (Exception) {
    }
  });

  TIMEOUT(1, {
    try {
      throw Exception(String{"hello world"});
    } catch (Exception &) {
    }
  });

  TIMEOUT(1, {
    try {
      throw Exception("hello world");
    } catch (Exception) {
    }
  });

  TIMEOUT(1, {
    try {
      throw Except(EDoNothing, "hello world");
    } catch (Base::Exception &) {
    }
  });
}

TEST(String, Property) {
  auto perform = [&]() {
    Base::SetProperty<String> set;
    Vector<String> reversed{"10", "9", "8", "7", "6", "5",
                            "4",  "3", "2", "1", "0"};

    set =
        Vector<String>{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};

    for (UInt i = -1; i < set.size() - 1; ++i) {
      EXPECT_EQ(set(i), Base::ToString(i + 1));
      EXPECT_EQ(set[i], Base::ToString(i + 1));
    }

    set = reversed;
    for (UInt i = 0, j = 10; i < set.size(); ++i, --j) {
      EXPECT_EQ(set(i), Base::ToString(j));
      EXPECT_EQ(set[i], Base::ToString(j));
    }
  };

  TIMEOUT(1, { perform(); });
}

TEST(String, Convert) {
  TIMEOUT(1, {
    /* @NOTE: floating point issue */
    EXPECT_EQ(Base::ToString(10.01), "10.00999");
    EXPECT_EQ(Base::ToString(10), "10");
    EXPECT_EQ(Base::ToString(0), "0");
    EXPECT_EQ(Base::ToString(-1), "-1");
    EXPECT_EQ(Base::ToString(1), "1");
  });
}

TEST(String, Format2String) {
  String sample(Base::Format{"{} is dir"}.Apply("/tmp/blank.map"));

  EXPECT_EQ(sample, "/tmp/blank.map is dir");
  EXPECT_EQ(sample.append(" function abc"),
            "/tmp/blank.map is dir function abc");

  EXPECT_EQ(Base::Format{"{} is {} again"}.Apply("/tmp/blank.map", "dir"),
            "/tmp/blank.map is dir again");

  EXPECT_EQ(Base::Format{"{} {} {} again"}.Apply("/tmp/blank.map", "is", "dir"),
            "/tmp/blank.map is dir again");
}
#endif

#if USE_GTEST
int main(int argc, CString argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#else
int main() { return RUN_ALL_TESTS(); }
#endif
