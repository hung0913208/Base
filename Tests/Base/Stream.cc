#include <Stream.h>
#include <Unittest.h>

TEST(Stream, Console) {
  String console, buffer;
  Base::Stream stream{[&](Bytes&& buffer, UInt buffer_size) -> ErrorCodeE {
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

int main() {
  return RUN_ALL_TESTS();
}