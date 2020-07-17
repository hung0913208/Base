#include <Wrapping.h>
#include <Unittest.h>

using namespace Base;
static Bool EnteredPrint0 = False;
static Bool EnteredPrint1 = False;

extern "C" Void Print(void) {
  String console, buffer;
  Stream stream{[&](Bytes&& buffer, UInt* buffer_size) -> ErrorCodeE {
    console += String{(char*)buffer, *buffer_size};
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

  EnteredPrint0 = True;
}

extern "C" Void PrintWithCString(CString param) {
  String console, buffer;
  Stream stream{[&](Bytes&& buffer, UInt* buffer_size) -> ErrorCodeE {
    console += String{(char*)buffer, *buffer_size};
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

  stream << param;
  EXPECT_EQ(console, String(param));

  stream >> buffer;
  EXPECT_EQ(buffer, String(param));

  EnteredPrint1 = True;
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

static Bool EnteredTest2 = False;

extern "C" void TestParameters0(Int i, Long l, Double d) {
  EnteredTest2 = True;

  EXPECT_EQ(i, Int(10));
  EXPECT_EQ(l, Long(10));
  IGNORE({
    EXPECT_EQ(d, Double(10.0));
  });
}

extern "C" void TestParameters1(Int i1, Int i2, Int i3, Int i4, Int i5, Int i6) {
  EnteredTest2 = True;

  EXPECT_EQ(i1, Int(10));
  EXPECT_EQ(i2, Int(10));
  EXPECT_EQ(i3, Int(10));
  EXPECT_EQ(i4, Int(10));
  EXPECT_EQ(i5, Int(10));
  EXPECT_EQ(i6, Int(10));
}

extern "C" void TestParameters2(Double i1, Double i2, Double i3, Double i4, Double i5, Double i6, Double i7, Double i8) {
  EnteredTest2 = True;

  EXPECT_EQ(i1, Double(10.0));
  EXPECT_EQ(i2, Double(10.0));
  EXPECT_EQ(i3, Double(10.0));
  EXPECT_EQ(i4, Double(10.0));
  EXPECT_EQ(i5, Double(10.0));
  EXPECT_EQ(i6, Double(10.0));
  EXPECT_EQ(i7, Double(10.0));
  EXPECT_EQ(i8, Double(10.0));
}

extern "C" void TestParameters3(Int i1, Int i2, Int i3, Int i4, Int i5, Int i6, Int i7) {
  EnteredTest2 = True;

  EXPECT_EQ(i1, Int(10));
  EXPECT_EQ(i2, Int(10));
  EXPECT_EQ(i3, Int(10));
  EXPECT_EQ(i4, Int(10));
  EXPECT_EQ(i5, Int(10));
  EXPECT_EQ(i6, Int(10));
  EXPECT_EQ(i7, Int(10));
}

extern "C" void TestParameters4(Double i1, Double i2, Double i3, Double i4, Double i5, Double i6, Double i7, Double i8, Double i9) {
  EnteredTest2 = True;

  EXPECT_EQ(i1, Double(10.0));
  EXPECT_EQ(i2, Double(10.0));
  EXPECT_EQ(i3, Double(10.0));
  EXPECT_EQ(i4, Double(10.0));
  EXPECT_EQ(i5, Double(10.0));
  EXPECT_EQ(i6, Double(10.0));
  EXPECT_EQ(i7, Double(10.0));
  EXPECT_EQ(i8, Double(10.0));
  EXPECT_EQ(i9, Double(10.0));
}

TEST(Instruct, Parameters0) {
  Vector<Auto> parameters{};

  parameters.clear();
  EnteredTest2 = False;

  parameters.push_back(Base::Auto::As<Int>(10));
  parameters.push_back(Base::Auto::As<Long>(10));
  parameters.push_back(Base::Auto::As<Double>(10.0));

  Base::Wrapping::Instruct((Void*)(TestParameters0), parameters);
  EXPECT_TRUE(EnteredTest2);
}


TEST(Instruct, Parameters1) {
  Vector<Auto> parameters{};

  parameters.clear();
  EnteredTest2 = False;

  parameters.push_back(Base::Auto::As<Int>(10));
  parameters.push_back(Base::Auto::As<Int>(10));
  parameters.push_back(Base::Auto::As<Int>(10));
  parameters.push_back(Base::Auto::As<Int>(10));
  parameters.push_back(Base::Auto::As<Int>(10));
  parameters.push_back(Base::Auto::As<Int>(10));
  
  Base::Wrapping::Instruct((Void*)(TestParameters1), parameters);
  EXPECT_TRUE(EnteredTest2);
}

TEST(Instruct, Parameters2) {
  Vector<Auto> parameters{};

  parameters.clear();
  EnteredTest2 = False;

  parameters.push_back(Base::Auto::As<Double>(10));
  parameters.push_back(Base::Auto::As<Double>(10));
  parameters.push_back(Base::Auto::As<Double>(10));
  parameters.push_back(Base::Auto::As<Double>(10));
  parameters.push_back(Base::Auto::As<Double>(10));
  parameters.push_back(Base::Auto::As<Double>(10));
  parameters.push_back(Base::Auto::As<Double>(10));
  parameters.push_back(Base::Auto::As<Double>(10));

  Base::Wrapping::Instruct((Void*)(TestParameters2), parameters);
  EXPECT_TRUE(EnteredTest2);
}

TEST(Instruct, Parameters3) {
  Vector<Auto> parameters{};

  parameters.clear();
  EnteredTest2 = False;

  parameters.push_back(Base::Auto::As<Int>(10));
  parameters.push_back(Base::Auto::As<Int>(10));
  parameters.push_back(Base::Auto::As<Int>(10));
  parameters.push_back(Base::Auto::As<Int>(10));
  parameters.push_back(Base::Auto::As<Int>(10));
  parameters.push_back(Base::Auto::As<Int>(10));
  parameters.push_back(Base::Auto::As<Int>(10));
  
  Base::Wrapping::Instruct((Void*)(TestParameters3), parameters);
  EXPECT_TRUE(EnteredTest2);
}


TEST(Instruct, Parameters4) {
  Vector<Auto> parameters{};

  parameters.clear();
  EnteredTest2 = False;

  parameters.push_back(Base::Auto::As<Double>(10));
  parameters.push_back(Base::Auto::As<Double>(10));
  parameters.push_back(Base::Auto::As<Double>(10));
  parameters.push_back(Base::Auto::As<Double>(10));
  parameters.push_back(Base::Auto::As<Double>(10));
  parameters.push_back(Base::Auto::As<Double>(10));
  parameters.push_back(Base::Auto::As<Double>(10));
  parameters.push_back(Base::Auto::As<Double>(10));
  parameters.push_back(Base::Auto::As<Double>(10));

  Base::Wrapping::Instruct((Void*)(TestParameters4), parameters);
  EXPECT_TRUE(EnteredTest2);
}

#if defined(__GNUC__) && !defined(__clang__)
static Bool EnteredTest3 = False;

#ifdef USE_PYTHON
PY_MODULE(CTest) {
  Procedure("Print", Print);
  Procedure<CString>("PrintWithCString", PrintWithCString);
  EnteredTest3 = True;
}

TEST(PyWrapping, Simple) {
  EnteredTest3 = False;
  EnteredPrint0 = False;
  EnteredPrint1 = False;

  EXPECT_NO_THROW({
    EXPECT_TRUE(Wrapping::Create(new PY_MODULE_CLI(CTest)));
  });

  EXPECT_TRUE(Wrapping::Test("Python", "CTest", "Print"));
  EXPECT_TRUE(EnteredTest3);
  EXPECT_TRUE(EnteredPrint0);

  EXPECT_TRUE(Wrapping::Test("Python", "CTest", "PrintWithCString", "hello"));
  EXPECT_TRUE(EnteredPrint1);
}
#endif

#ifdef USE_RUBY
RUBY_MODULE(CTest) {
  Procedure("Print", Print);
  Procedure<CString>("PrintWithCString", PrintWithCString);
  EnteredTest3 = True;
}

TEST(RubyWrapping, Simple) {
  EnteredTest3 = False;
  EnteredPrint0 = False;
  EnteredPrint1 = False;


  EXPECT_TRUE(Wrapping::Test("Ruby", "CTest", "Print"));
  EXPECT_TRUE(EnteredTest3);
  EXPECT_TRUE(EnteredPrint0);

  EXPECT_TRUE(Wrapping::Test("Ruby", "CTest", "Print", "hello"));
  EXPECT_TRUE(EnteredPrint1);
}
#endif
#endif

int main(){
  return RUN_ALL_TESTS();
}
