#include <Auto.h>
#include <Monitor.h>
#include <Json.h>
#include <Logcat.h>
#include <Utils.h>
#include <Unittest.h>

#include <fstream>


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
    _FileD.write(reinterpret_cast<char*>(data), *size);
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

TEST(Json, Streaming) {
  Base::Json content{std::make_shared<FileStream>("/tmp/sample.json")};

  EXPECT_EQ(content(), ENoError);
}

PREPARE(Json, Streaming) {
  using namespace Base;

  Shared<Monitor> monitor{Monitor::Make("simple", Monitor::EPipe)};
  Fork fork{[]() -> Int {
    const CString args[] = {
      "/usr/bin/curl", 
      "-ksSo",
      "/tmp/sample.json",                            
      "https://raw.githubusercontent.com/zemirco/sf-city-lots-json/master/citylots.json",
      None
    };

    execv(args[0], (CString const*)args);
    return -1;
  }};

  monitor->Trigger(Auto::As(fork),
    [&](Auto fd, Auto& UNUSED(content)) -> ErrorCodeE {
      while (True) {
        Char buf[512];
        UInt len = read(fd.Get<Int>(), buf, 512);

        if (len <= 0) {
          break;
        }
      }

      return ENoError;
    }
  );

  monitor->Loop([&](Monitor&) -> Bool { return fork.Status() >= 0; });
}

int main() {
  return RUN_ALL_TESTS();
}

