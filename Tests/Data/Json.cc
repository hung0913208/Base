#include <Json.h>
#include <Logcat.h>

#include <fstream>

#if USE_GTEST
#include <gtest/gtest.h>
#else
#include <Unittest.h>
#endif

using namespace std::placeholders;
using namespace Base::Data;

class FileStream: public Base::Stream{
 public:
  explicit FileStream(String path):
    Stream{std::bind(&FileStream::Write, this, _1, _2),
           std::bind(&FileStream::Read, this, _1, _2)},
  #ifdef BASE_TYPE_STRING_H_
    _FileD{path.c_str()}{}
  #else
    _FileD{path}{}
  #endif

 private:
  ErrorCodeE Read(Bytes& buffer, UInt* size) {
    if (_FileD.eof()){
      return OutOfRange.code();
    }

    _FileD.read(reinterpret_cast<char*>(buffer), *size);
    if (!_FileD) {
      *size = _FileD.gcount();
    }

    return ENoError;
  }

  ErrorCodeE Write(Bytes&& data, UInt* size) {
    _FileD.write(reinterpret_cast<char*>(data), size);
    return ENoError;
  }

  std::fstream _FileD;
};

TEST(Json, Example0){
  auto perform = [&](){
    String json = "{\"abc\": \"xyz\"}";
    Base::Json content{json};

    EXPECT_EQ(content(), ENoError);

#if WATCHING
    EXPECT_EQ(content.Get<String, String>("abc"), "xyz");
#else
    /* @NOTE: i will try will GTest to make sure this is an issue from GCC */

    String result = content.Get<String, String>("abc");
    EXPECT_EQ(result, "xyz");
#endif
  };

  TIMEOUT(10, { perform(); });
}

TEST(Json, Example1){
  auto perform = [&](){
    String json = "{\"abc\": [\"xyz\", 10, True, 10.01]}";
    Base::Json content{json};

    EXPECT_EQ(content(), ENoError);

#if WATCHING
    EXPECT_EQ(content.Get<String, Base::Data::List>("abc").At<String>(0), "xyz");
    EXPECT_EQ(content.Get<String, Base::Data::List>("abc").At<Bool>(2), True);
    EXPECT_EQ(content.Get<String, Base::Data::List>("abc").At<Long>(1), 10);
#else
    String sval = content.Get<String, Base::Data::List>("abc").At<String>(0);
    Long ival = content.Get<String, Base::Data::List>("abc").At<Long>(1);
    Bool bval = content.Get<String, Base::Data::List>("abc").At<Bool>(2);
    Double dval = content.Get<String, Base::Data::List>("abc").At<Double>(3);

    EXPECT_EQ(sval, "xyz");
    EXPECT_EQ(ival, 10);
    EXPECT_EQ(bval, True);
    EXPECT_EQ(dval, 10.01);
#endif
  };

  TIMEOUT(10, { perform(); });
}

#if DEV
/* @TODO: this test case  will wait until `Builder 2` finishes because
 * it will provide new feature `fetching git` */
TEST(Json, Streaming){
  Base::Json content{std::make_shared<FileStream>("")};
 
  EXPECT_EQ(content(), ENoError);
}
#endif

int main() {
  return RUN_ALL_TESTS();
}

