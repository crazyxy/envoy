licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

envoy_cc_library(
    name = "codec_lib",
    srcs = ["codec_impl.cc"],
    hdrs = ["codec_impl.h"],
    deps = [
        "//source/common/http/http1:codec_lib",
        "//source/common/http/http2:codec_lib",
        "//source/common/common:base64_lib",
        "//source/common/common:hex_lib",
    ],
)
