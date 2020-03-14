load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
  name = "Unittest",
  srcs = glob(["Source/*.cc"]),
  hdrs = glob(["Implement/*.h", "*.h"]),
  deps = [
    "//Libraries/Base:Base"
  ],
  visibility = [
    "//visibility:public"
  ],
  includes = [
    "Implement",
    ""
  ],
  copts = [
    "-std=c++11"
  ],
  linkopts = [
    "-ldl"
  ]
)

cc_binary(
  name = "libunittest.so",
  deps = [":Unittest"],
  linkshared = 1,
  copts = [
    "-pthread"
  ],
  linkopts = [
    "-ldl"
  ]
)
