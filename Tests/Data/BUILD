load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
  name = "Json",
  srcs = [
    "Json.cc",
  ],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Data:Data",
    "//Libraries/Unittest:Unittest"
  ],
  copts = [
    "-std=c++11"
  ]
)

cc_test(
  name = "Memmap",
  srcs = [
    "Memmap.cc"
  ],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Data:Data",
    "//Libraries/Unittest:Unittest"
  ],
  copts = [
    "-std=c++11"
  ]
)
