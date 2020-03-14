load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
  name = "InstructC++",
  srcs = glob([
    "Source/Instruct/*.cc"
  ]),
  hdrs = glob([
    "Implement/*.h", 
    "*.h"
  ]),
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

cc_library(
  name = "InstructC",
  srcs = glob([
    "Source/Instruct/*.c"
  ]),
  hdrs = glob([
    "Implement/*.h", 
    "*.h"
  ]),
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
  linkopts = [
    "-ldl"
  ]
)

cc_library(
  name = "Wrapping",
  srcs = glob([
    "Source/*.cc",
    "Source/Instruct/*.cc",
    "Source/IPython/*.cc",
    "Source/IBase/*.cc"
  ]),
  hdrs = glob([
    "Implement/*.h", 
    "*.h"
  ]),
  deps = [
    "//Libraries/Base:Base",
    ":InstructC++",
    ":InstructC",
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
  name = "libwrapping.so",
  deps = [":Wrapping"],
  linkshared = 1
)
