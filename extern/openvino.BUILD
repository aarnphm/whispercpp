

package(
	default_visibility = ["//visibility:public"],
)

cc_library(
	name = "openvino_old_headers",
	hdrs = glob([
		"include/ie/**/*.*"
	]),
	strip_include_prefix = "include/ie",
	visibility = ["//visibility:public"],
)

cc_library(
	name = "openvino_new_headers",
	hdrs = glob([
		#"include/openvino/**/*.*"
		"include/**/*.*"
	]),
	strip_include_prefix = "include",
	visibility = ["//visibility:public"],
	deps = [
		"@linux_openvino//:openvino_old_headers",
	]
)

cc_library(
	name = "ngraph",
	hdrs = glob([
		"include/ngraph/**/*.*"
	]),
	strip_include_prefix = "include",
	visibility = ["//visibility:public"],
)

cc_library(
	name = "openvino",
	srcs = glob([
		"lib/aarch64/libopenvino.so"
	]),
	strip_include_prefix = "include/ie",
	visibility = ["//visibility:public"],
	deps = [
		"@linux_openvino//:ngraph",
		"@linux_openvino//:openvino_new_headers",
        ],
)
