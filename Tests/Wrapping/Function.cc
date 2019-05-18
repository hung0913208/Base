#include <Wrapping.h>
#include <Unittest.h>

using namespace Base;

Void Print(void) {
  String console, buffer;
  Stream stream{[&](Bytes&& buffer, UInt buffer_size) -> ErrorCodeE {
    console += String{(char*)buffer, buffer_size};
    return ENoError;
  },
  [&](Bytes& buffer, UInt* size_of_received) -> ErrorCodeE {
    if (console.size() == 0) {
      return DoNothing.code();
    } else if (*size_of_received == 0 && !buffer) {
      *size_of_received = console.size();
      buffer = (Bytes)malloc(console.size());
    } else if (*size_of_received > console.size()) {
      *size_of_received = console.size();
    }

    memcpy(buffer, console.c_str(), *size_of_received);
    return ENoError;
  }};

  stream << "test";
  EXPECT_EQ(console, "test");

  stream >> buffer;
  EXPECT_EQ(buffer, "test");
}

static Bool EnteredTest1{False};
void TestEnter() {
  EnteredTest1 = True;
}

TEST(Instruct, Simple) {
  Vector<Auto> parameters{};

  Base::Wrapping::Instruct((Void*)(TestEnter), parameters);
  EXPECT_TRUE(EnteredTest1);
}

static Bool EnteredTest2{False};
void TestParameters(Int i, Long l, Double d) {
  EnteredTest2 = True;

  EXPECT_EQ(i, Int(10));
  EXPECT_EQ(l, Long(10));
  IGNORE({
    EXPECT_EQ(d, Double(10.0));
  });
}

TEST(Instruct, Parameters) {
  Vector<Auto> parameters{};

  parameters.push_back(Base::Auto::As<Int>(10));
  parameters.push_back(Base::Auto::As<Long>(10));
  parameters.push_back(Base::Auto::As<Double>(10.0));

  Base::Wrapping::Instruct((Void*)(TestParameters), parameters);
  EXPECT_TRUE(EnteredTest2);
}

#if defined(__GNUC__) && !defined(__clang__)
#ifdef USE_PYTHON
PY_MODULE(CTest) {
  Procedure("Print", Print);
}

TEST(PyWrapping, Simple) {
  EXPECT_NO_THROW({
    EXPECT_TRUE(Wrapping::Create(new PY_MODULE_CLI(CTest)));
  });
  EXPECT_TRUE(Wrapping::Test("Python", "CTest", "Print"));
}
#endif

#ifdef USE_RUBY
RUBY_MODULE(CTest) {
  Procedure("Print", Print);
}

TEST(RubyWrapping, Simple) {
  EXPECT_TRUE(Wrapping::Test("Ruby", "CTest", "Print"));
}
#endif
#endif

int main(){
  Base::Log::Level() = EDebug;
  return RUN_ALL_TESTS();
}
