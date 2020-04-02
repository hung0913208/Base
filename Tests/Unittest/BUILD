load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
  name = "Passed",
  srcs = [
    "Passed.cc",
  ],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest"
  ],
  copts = [
    "-std=c++11"
  ]
)

cc_test(
  name = "Fail",
  srcs = [
    "Fail.cc",
  ],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest"
  ],
  copts = [
    "-std=c++11"
  ]
)

