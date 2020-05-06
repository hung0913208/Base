load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
  name = "Argparse",
  srcs = ["Argparse.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest"
  ],
  copts = [
    "-std=c++11",
    "-pthread"
  ]
)

cc_test(
  name = "Atomic",
  srcs = ["Atomic.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest"
  ],
  copts = [
    "-std=c++11",
    "-pthread"
  ]
)

cc_test(
  name = "Deadlock",
  srcs = ["Deadlock.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest",
    ":Exception",
    ":Refcount",
    ":String"
  ],
  copts=[
    "-std=c++11",
    "-pthread"
  ]
)

cc_test(
  name = "Exception",
  srcs = ["Exception.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest"
  ],
  copts = [
    "-std=c++11",
    "-pthread"
  ]
)

cc_test(
  name = "Glob",
  srcs = ["Glob.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest"
  ],
  copts = [
    "-std=c++11",
    "-pthread"
  ]
)

cc_test(
  name = "Hashtable",
  srcs = ["Hashtable.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest"
  ],
  copts = [
    "-std=c++11",
    "-pthread"
  ]
)

cc_test(
  name = "Lock",
  srcs = ["Lock.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest",
    ":Deadlock",
    ":Thread"
  ],
  copts = [
    "-std=c++11",
    "-pthread"
  ]
)

cc_test(
  name = "Logcat",
  srcs = ["Logcat.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest"
  ],
  copts = [
    "-std=c++11",
    "-pthread"
  ]
)

cc_test(
  name = "Popen",
  srcs = ["Popen.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest",
    ":Thread",
    ":Lock",
  ],
  copts = [
    "-std=c++11",
    "-pthread"
  ]
)

cc_test(
  name = "Property",
  srcs = ["Property.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest"
  ],
  copts = [
    "-std=c++11",
    "-pthread"
  ]
)

cc_test(
  name = "Protect",
  srcs = ["Protect.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest"
  ],
  copts = [
    "-std=c++11",
    "-pthread"
  ]
)

cc_test(
  name = "Refcount",
  srcs = ["Refcount.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest"
  ],
  copts = [
    "-std=c++11",
    "-pthread"
  ]
)

cc_test(
  name = "Stream",
  srcs = ["Stream.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest"
  ],
  copts = [
    "-std=c++11",
    "-pthread"
  ]
)

cc_test(
  name = "String",
  srcs = ["String.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest"
  ],
  copts=[
    "-std=c++11",
    "-pthread"
  ]
)

cc_test(
  name = "Thread",
  srcs = ["Thread.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest",
    ":Deadlock"
  ],
  copts = [
    "-std=c++11",
    "-pthread"
  ]
)

cc_test(
  name = "Vertex",
  srcs = ["Vertex.cc"],
  deps = [
    "//Libraries/Base:Base",
    "//Libraries/Unittest:Unittest"
  ],
  copts = [
    "-std=c++11",
    "-pthread"
  ]
)
