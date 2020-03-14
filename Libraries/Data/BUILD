load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
  name = "Data",
  srcs = glob(["Source/*.cc"]),
  hdrs = glob(["*.h"]),
  deps = [
    "//Libraries/Base:Base"
  ],
  visibility = [
    "//visibility:public"
  ],
  includes = [""],
  copts = [
    "-std=c++11"
  ]
)

cc_binary(
  name = "libdata.so",
  deps = [":Data"],
  linkshared = 1
)
