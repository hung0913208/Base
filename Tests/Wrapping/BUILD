load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
  name = "Function",
  srcs = [
    "Function.cc"
  ],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Wrapping:Wrapping",
    "//Libraries/Unittest:Unittest"
  ],
  copts=[
    "-std=c++11",
    "-pthread"
  ]
)
