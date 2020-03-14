load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
  name = "Multiplexing",
  srcs = glob([
    "Source/Internal/*.c"
  ]),
  hdrs = glob([
    "*.h",
    "Type/*.h"
  ]),
  includes = [
    "Type",
    ""
  ],
)

cc_library(
  name = "Base",
  srcs = glob([
    "Source/*.cc",
    "Source/Internal/*.cc",
  ]),
  hdrs = glob([
    "*.h",
    "Type/*.h"
  ]),
  visibility = [
    "//visibility:public"
  ],
  includes = [
    "Type",
    ""
  ],
  copts = [
    "-std=c++11",
    "-pthread"
  ],
  linkopts = [
    "-pthread"
  ],
  deps = [":Multiplexing"]
)

cc_binary(
  name = "libbase.so",
  deps = [":Base"],
  linkshared = 1,
  copts = [
    "-std=c++11",
    "-pthread"
  ],
  linkopts = [
    "-pthread",
    "-ldl"
  ]
)
