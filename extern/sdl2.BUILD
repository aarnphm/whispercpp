load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "objc_library")
load("@build_bazel_rules_apple//apple:apple_binary.bzl", "apple_binary")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "srcs_files",
    srcs = glob(["**"]),
)

filegroup(
    name = "hdrs_files",
    srcs = glob(["include/**/*.h"]),
)

alias(
    name = "windows",
    actual = "@bazel_tools//src/conditions:windows",
)

alias(
    name = "macos",
    actual = "@bazel_tools//src/conditions:darwin",
)

alias(
    name = "linux",
    actual = "@bazel_tools//src/conditions:linux",
)

SDL_LINKOPTS = select({
    "//:windows": [
        "-DEFAULTLIB:user32",
        "-DEFAULTLIB:gdi32",
        "-DEFAULTLIB:winmm",
        "-DEFAULTLIB:imm32",
        "-DEFAULTLIB:ole32",
        "-DEFAULTLIB:oleaut32",
        "-DEFAULTLIB:version",
        "-DEFAULTLIB:uuid",
        "-DEFAULTLIB:shell32",
        "-DEFAULTLIB:advapi32",
        "-DEFAULTLIB:hid",
        "-DEFAULTLIB:setupapi",
        "-DEFAULTLIB:opengl32",
        "-DEFAULTLIB:kernel32",
    ],
    "//conditions:default": [],
})

## src/dynapi
cc_library(
    name = "dynapi_lib",
    srcs = glob(["src/dynapi/*.c"]),
    hdrs = glob(["src/dynapi/*.h"]),
    deps = ["//:SDL_internal"],
    alwayslink = True,
)

## src/thread
cc_library(
    name = "thread_windows_lib",
    srcs = glob(["src/thread/windows/*.c"]),
    hdrs = glob(["src/thread/windows/*.h"]),
    deps = ["//:SDL_internal"],
)

cc_library(
    name = "thread_stdcpp_lib",
    srcs = glob(["src/thread/stdcpp/*.c"]),
    hdrs = glob(["src/thread/stdcpp/*.h"]),
    deps = ["//:SDL_internal"],
)

cc_library(
    name = "thread_pthread_lib",
    srcs = glob(["src/thread/pthread/*.c"]),
    hdrs = glob([
        "src/thread/pthread/*.h",
        "src/thread/*.h",
    ]),
    deps = [
        ":dynapi_lib",
        ":thread_pthread_headers",
        ":thread_generic_headers",
        ":SDL_internal",
    ] + select({
        "//:linux": [":core_lib"],
        "//conditions:default": [],
    }),
    alwayslink = True,
)

cc_library(
    name = "thread_pthread_headers",
    hdrs = glob(["src/thread/pthread/*.h"]),
    visibility = ["//:__subpackages__"],
)

cc_library(
    name = "thread_generic_lib",
    srcs = glob(["src/thread/generic/*.c"]),
    hdrs = glob(["src/thread/generic/*.h"]),
    deps = [
        ":SDL_internal",
        ":dynapi_lib",
        ":thread_headers",
        ":thread_pthread_headers",
    ],
)

cc_library(
    name = "thread_generic_headers",
    hdrs = glob([
        "src/thread/generic/*.h",
        "src/thread/generic/*.c",
    ]),
    visibility = ["//:__subpackages__"],
)

cc_library(
    name = "thread_generic_syscond_lib",
    srcs = ["src/thread/generic/SDL_syscond.c"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "thread_lib",
    srcs = glob(["src/thread/*.c"]),
    hdrs = glob(["src/thread/*.h"]),
    deps = [
        "//:SDL_internal",
        ":dynapi_lib",
    ] + select({
        "//:windows": [
            ":thread_windows_lib",
            ":thread_generic_syscond_lib",
        ],
        "//conditions:default": [
            ":thread_stdcpp_lib",
            ":thread_pthread_lib",
            ":thread_generic_lib",
        ],
    }),
    alwayslink = True,
)

cc_library(
    name = "thread_headers",
    hdrs = glob(["src/thread/*.h"]),
)

## src/atomic
cc_library(
    name = "atomic_lib",
    srcs = glob(["src/atomic/*.c"]),
    hdrs = glob(["src/atomic/*.h"]),
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

## src/audio
objc_library(
    name = "audio_coreaudio_lib",
    srcs = glob(["src/audio/coreaudio/*.m"]),
    hdrs = glob(["src/audio/coreaudio/*.h"]),
    copts = [
        "-fno-objc-arc",
    ],
    sdk_frameworks = [
        "Audiotoolbox",
        "CoreAudio",
    ],
    visibility = ["//:__subpackages__"],
    deps = [
        ":audio_headers",
        ":dynapi_lib",
        ":thread_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "audio_wasapi_lib",
    srcs = glob(["src/audio/wasapi/*.c"]) + glob(["src/audio/wasapi/*.cpp"]),
    hdrs = glob(["src/audio/wasapi/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
)

cc_library(
    name = "audio_directsound_lib",
    srcs = glob(["src/audio/directsound/*.c"]),
    hdrs = glob(["src/audio/directsound/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
)

cc_library(
    name = "audio_winmm_lib",
    srcs = glob(["src/audio/winmm/*.c"]),
    hdrs = glob(["src/audio/winmm/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
)

cc_library(
    name = "audio_disk_lib",
    srcs = glob(["src/audio/disk/*.c"]),
    hdrs = glob(["src/audio/disk/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":audio_headers",
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "audio_dummy_lib",
    srcs = glob(["src/audio/dummy/*.c"]),
    hdrs = glob(["src/audio/dummy/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":audio_headers",
        ":dynapi_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "audio_alsa_lib",
    srcs = glob(["src/audio/alsa/*.c"]),
    hdrs = glob(["src/audio/alsa/*.h"]),
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "audio_jack_lib",
    srcs = glob(["src/audio/jack/*.c"]),
    hdrs = glob(["src/audio/jack/*.h"]),
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "audio_esd_lib",
    srcs = glob(["src/audio/esd/*.c"]),
    hdrs = glob(["src/audio/esd/*.h"]),
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "audio_arts_lib",
    srcs = glob(["src/audio/arts/*.c"]),
    hdrs = glob(["src/audio/arts/*.h"]),
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "audio_nas_lib",
    srcs = glob(["src/audio/nas/*.c"]),
    hdrs = glob(["src/audio/nas/*.h"]),
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "audio_fusionsound_lib",
    srcs = glob(["src/audio/fusionsound/*.c"]),
    hdrs = glob(["src/audio/fusionsound/*.h"]),
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "audio_sndio_lib",
    srcs = glob(["src/audio/sndio/*.c"]),
    hdrs = glob(["src/audio/sndio/*.h"]),
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "audio_pulseaudio_lib",
    srcs = glob(["src/audio/pulseaudio/*.c"]),
    hdrs = glob(["src/audio/pulseaudio/*.h"]),
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "audio_lib",
    srcs = glob(["src/audio/*.c"]),
    hdrs = glob(["src/audio/*.h"]),
    deps = [
        "//:SDL_internal",
        ":dynapi_lib",
        ":audio_disk_lib",
        ":audio_dummy_lib",
    ] + select({
        "//:windows": [
            ":audio_wasapi_lib",
            ":audio_directsound_lib",
            ":audio_winmm_lib",
        ],
        "//:macos": [
            ":thread_lib",
            ":audio_coreaudio_lib",
        ],
        "//conditions:default": [
            ":thread_lib",
            ":thread_generic_lib",
            ":audio_alsa_lib",
            ":audio_jack_lib",
            ":audio_esd_lib",
            ":audio_arts_lib",
            ":audio_nas_lib",
            ":audio_sndio_lib",
            ":audio_fusionsound_lib",
            ":audio_pulseaudio_lib",
        ],
    }),
    alwayslink = True,
)

cc_library(
    name = "audio_headers",
    hdrs = glob(["src/audio/*.h"]),
)

## src/core
cc_library(
    name = "core_windows_lib",
    srcs = glob(["src/core/windows/*.c"]),
    hdrs = glob(["src/core/windows/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
)

cc_library(
    name = "core_unix_lib",
    srcs = glob(["src/core/unix/*.c"]),
    hdrs = glob(["src/core/unix/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "core_linux_lib",
    srcs = glob(
        ["src/core/linux/*.c"],
        exclude = [
            "src/core/linux/SDL_dbus.c",
            "src/core/linux/SDL_fcitx.c",
        ],
    ),
    hdrs = glob(["src/core/linux/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":events_headers",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "core_lib",
    srcs = glob(["src/core/*.c"]),
    hdrs = glob(["src/core/*.h"]),
    visibility = ["//visibility:public"],
    deps = ["//:SDL_internal"] + select({
        "//:windows": [
            ":core_windows_lib",
        ],
        "//:macos": [
            ":core_unix_lib",
        ],
        "//conditions:default": [
            ":core_unix_lib",
            ":core_linux_lib",
        ],
    }),
    alwayslink = True,
)

## src/video
cc_library(
    name = "video_khronos_lib",
    srcs = glob(["src/video/khronos/**/*.c"]),
    hdrs = glob(["src/video/khronos/**/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
    alwayslink = True,
)

cc_library(
    name = "video_yuv2rgb_lib",
    srcs = glob(["src/video/yuv2rgb/**/*.c"]),
    hdrs = glob(["src/video/yuv2rgb/**/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "video_dummy_lib",
    srcs = glob(["src/video/dummy/**/*.c"]),
    hdrs = glob(["src/video/dummy/**/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":events_headers",
        ":video_headers",
        ":video_khronos_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "video_windows_lib",
    srcs = glob(
        ["src/video/windows/**/*.c"],
        exclude = ["src/video/windows/SDL_windowsmouse.c"],
    ),
    hdrs = glob(["src/video/windows/**/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
)

objc_library(
    name = "video_cocoa_lib",
    srcs = glob(
        ["src/video/cocoa/*.m"],
        exclude = [
            "src/video/cocoa/SDL_cocoamouse.m",
            "src/video/cocoa/SDL_cocoaevents.m",
            "src/video/cocoa/SDL_cocoawindow.m",
            "src/video/coca/SDL_cocoavideo.m",
        ],
    ),
    hdrs = glob(["src/video/cocoa/*.h"]),
    includes = ["src/video/khronos"],
    sdk_frameworks = [
        "Cocoa",
        "Carbon",
        "IOKit",
        "CoreVideo",
        "OpenGL",
        "Metal",
    ],
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":events_headers",
        ":video_headers",
        ":video_khronos_lib",
        "//:SDL_internal",
    ],
)

objc_library(
    name = "video_cocoa_headers",
    hdrs = glob(["src/video/cocoa/*.h"]),
    visibility = ["//:__subpackages__"],
)

cc_library(
    name = "video_x11_lib",
    srcs = glob(["src/video/x11/**/*.c"]),
    hdrs = glob(["src/video/x11/**/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "video_nacl_lib",
    srcs = glob(["src/video/nacl/**/*.c"]),
    hdrs = glob(["src/video/nacl/**/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":events_headers",
        ":video_headers",
        ":video_khronos_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "video_kmsdrm_lib",
    srcs = glob(["src/video/kmsdrm/**/*.c"]),
    hdrs = glob(["src/video/kmsdrm/**/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":events_headers",
        ":video_headers",
        ":video_khronos_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "video_directfb_lib",
    srcs = glob(["src/video/directfb/**/*.c"]),
    hdrs = glob(["src/video/directfb/**/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":events_headers",
        ":video_headers",
        ":video_khronos_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "video_wayland_lib",
    srcs = glob(["src/video/wayland/**/*.c"]),
    hdrs = glob(["src/video/wayland/**/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "video_lib",
    srcs = glob(
        ["src/video/*.c"],
        exclude = ["src/video/SDL_video.c"],
    ),
    hdrs = glob([
        "src/video/*.h",
        "src/render/**/*.h",
    ]) + select({
        "//:macos": glob([
            "src/video/cocoa/*.h",
        ]),
    }),
    includes = ["src/video/khronos"],
    visibility = ["//visibility:public"],
    deps = [
        "//:SDL_internal",
        ":video_khronos_lib",
        ":dynapi_lib",
        ":timer_lib",
        ":video_yuv2rgb_lib",
        ":video_dummy_lib",
    ] + select({
        "//:windows": [
            ":video_windows_lib",
        ],
        "//:macos": [
            ":video_cocoa_lib",
        ],
        "//conditions:default": [
            ":video_x11_lib",
            ":video_nacl_lib",
            ":video_kmsdrm_lib",
            ":video_directfb_lib",
            ":video_wayland_lib",
        ],
    }),
    alwayslink = True,
)

cc_library(
    name = "video_headers",
    hdrs = glob(["src/video/*.h"]),
)

## src/events
cc_library(
    name = "events_lib",
    srcs = glob(
        ["src/events/*.c"],
        exclude = [
            "src/events/SDL_mouse.c",
            "src/events/SDL_keyboard.c",
        ],
    ),
    hdrs = glob(
        ["src/events/*.h"],
        exclude = [
            "src/events/SDL_mouse_c.h",
            "src/events/SDL_mouse.h",
            "src/events/SDL_keyboard_c.h",
            "src/events/SDL_keyboard.h",
        ],
    ),
    visibility = ["//visibility:public"],
    deps = [
        ":dynapi_lib",
        ":joystick_lib",
        ":timer_lib",
        ":video_khronos_lib",
        ":video_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "events_headers",
    hdrs = glob(["src/events/*.h"]),
    visibility = ["//visibility:public"],
)

## src/timer
cc_library(
    name = "timer_windows_lib",
    srcs = glob(["src/timer/windows/*.c"]),
    hdrs = glob(["src/timer/windows/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
)

cc_library(
    name = "timer_unix_lib",
    srcs = glob(["src/timer/unix/*.c"]),
    hdrs = glob(["src/timer/unix/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":timer_headers",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "timer_dummy_lib",
    srcs = glob(["src/timer/dummy/*.c"]),
    hdrs = glob(["src/timer/dummy/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "timer_lib",
    srcs = glob(["src/timer/*.c"]),
    hdrs = glob(["src/timer/*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        "//:SDL_internal",
        ":dynapi_lib",
        ":thread_headers",
        ":thread_pthread_headers",
    ] + select({
        "//:windows": [
            ":timer_windows_lib",
        ],
        "//:macos": [
            ":timer_unix_lib",
        ],
        "//conditions:default": [
            ":timer_dummy_lib",
            ":timer_unix_lib",
            ":thread_generic_lib",
        ],
    }),
    alwayslink = True,
)

cc_library(
    name = "timer_headers",
    hdrs = glob(["src/timer/*.h"]),
)

## src/libm
cc_library(
    name = "libm_lib",
    srcs = glob(["src/libm/*.c"]),
    hdrs = glob(["src/libm/*.h"]),
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

## src/stdlib
cc_library(
    name = "stdlib_lib",
    srcs = glob(["src/stdlib/*.c"]),
    hdrs = glob(["src/stdlib/*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        ":atomic_lib",
        ":libm_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

## src/render
cc_library(
    name = "render_software_lib",
    srcs = glob(["src/render/software/*.c"]),
    hdrs = glob(["src/render/software/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":render_headers",
        ":video_lib",
        "//:SDL_internal",
        "//:dynapi_lib",
    ],
    alwayslink = True,
)

cc_library(
    name = "render_software_headers",
    hdrs = glob(["src/render/software/*.h"]),
    visibility = ["//:__subpackages__"],
)

cc_library(
    name = "render_direct3d_lib",
    srcs = glob(["src/render/direct3d/*.c"]),
    hdrs = glob(["src/render/direct3d/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
)

cc_library(
    name = "render_direct3d11_lib",
    srcs = glob(["src/render/direct3d11/*.c"]),
    hdrs = glob(["src/render/direct3d11/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
)

cc_library(
    name = "render_opengl_lib",
    srcs = glob(["src/render/opengl/*.c"]),
    hdrs = glob([
        "src/render/opengl/*.h",
        "src/video/**/*.h",
    ]),
    visibility = ["//:__subpackages__"],
    deps = [
        "dynapi_lib",
        "render_headers",
        "video_khronos_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "render_opengles_lib",
    srcs = glob(["src/render/opengles/*.c"]),
    hdrs = glob(["src/render/opengles/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":render_headers",
        ":video_khronos_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "render_opengles2_lib",
    srcs = glob(["src/render/opengles2/*.c"]),
    hdrs = glob(["src/render/opengles2/*.h"]),
    includes = ["src/video/khronos"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":render_headers",
        ":video_khronos_lib",
        ":video_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

objc_library(
    name = "render_metal_lib",
    srcs = glob(["src/render/metal*.m"]),
    hdrs = glob(["src/render/metal/*.h"]),
    copts = [
        "-fno-objc-arc",
    ],
    sdk_frameworks = [
        "Cocoa",
        "Metal",
    ],
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":video_cocoa_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "render_lib",
    srcs = glob(["src/render/*.c"]),
    hdrs = glob(["src/render/*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        "//:SDL_internal",
        ":dynapi_lib",
        ":render_software_lib",
    ] + select({
        "//:windows": [
            ":render_direct3d_lib",
            ":render_direct3d11_lib",
            ":render_opengl_lib",
            ":render_opengles_lib",
            ":render_opengles2_lib",
        ],
        "//:macos": [
            ":render_metal_lib",
            ":render_opengl_lib",
            ":render_opengles_lib",
            ":render_opengles2_lib",
        ],
        "//conditions:default": [
            ":render_opengl_lib",
            ":render_opengles_lib",
            ":render_opengles2_lib",
        ],
    }),
    alwayslink = True,
)

cc_library(
    name = "render_headers",
    hdrs = glob(["src/render/*.h"]),
)

## src/hidapi
cc_library(
    name = "hidapi_windows_lib",
    srcs = glob(["src/hidapi/windows/*.c"]),
    hdrs = glob(["src/hidapi/windows/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
)

cc_library(
    name = "hidapi_linux_lib",
    srcs = glob(["src/hidapi/linux/*.c"]),
    hdrs = glob(["src/hidapi/linux/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "hidapi_mac_lib",
    srcs = glob(["src/hidapi/mac/*.c"]),
    hdrs = glob(["src/hidapi/mac/*.h"]),
    includes = ["src/hidapi/hidapi"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":hidapi_headers",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "hidapi_mac_headers",
    hdrs = glob(["src/hidapi/mac/*.h"]),
    visibility = ["//:__subpackages__"],
)

cc_library(
    name = "hidapi_lib",
    srcs = select({
        "//:windows": glob(
            ["src/hidapi/*.c"],
            exclude = ["src/hidapi/SDL_hidapi.c"],
        ),
        "//:macos": glob(
            ["src/hidapi/*.c"],
        ),
        "//conditions:default": glob(["src/hidapi/*.c"]),
    }),
    hdrs = glob([
        "src/hidapi/*.h",
        "src/core/**/*.h",
    ]) + ["src/hidapi/hidapi/hidapi.h"] + select({
        "//:macos": [
            "src/hidapi/mac/hid.c",
        ],
        "//conditions:default": [],
    }),
    includes = [
        "src/core",
        "src/hidapi",
    ],
    visibility = ["//visibility:public"],
    deps = ["//:SDL_internal"] + select({
        "//:windows": [
            "hidapi_windows_lib",
        ],
        "//:macos": [
            ":hidapi_mac_lib",
            ":hidapi_mac_headers",
        ],
        "//conditions:default": [
            ":hidapi_linux_lib",
        ],
    }),
    alwayslink = True,
)

cc_library(
    name = "hidapi_headers",
    hdrs = glob(["*.h"]) + ["src/hidapi/hidapi/hidapi.h"],
)

## src/joystick
cc_library(
    name = "joystick_hidapi_lib",
    srcs = glob([
        "src/joystick/hidapi/**/*.c",
        "src/joystick/hidapi/**/*.h",
    ]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":hidapi_lib",
        ":joystick_headers",
        ":thread_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "joystick_linux_lib",
    srcs = glob(["src/joystick/linux/*.c"]),
    hdrs = glob(["src/joystick/linux/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
)

objc_library(
    name = "joystick_iphoneos_lib",
    srcs = glob([
        "src/joystick/iphoneos/*.m",
    ]),
    hdrs = glob([
        "src/joystick/iphoneos/*.h",
    ]),
    sdk_frameworks = [
        "ForceFeedback",
    ],
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":joystick_headers",
        ":joystick_hidapi_lib",
        ":video_lib",
        "//:SDL_internal",
    ],
)

objc_library(
    name = "joystick_darwin_lib",
    srcs = glob(
        ["src/joystick/darwin/*.c"],
        exclude = ["src/joystick/darwin/SDL_iokitjoystick.c"],
    ),
    hdrs = glob(["src/joystick/darwin/*.h"]),
    sdk_frameworks = ["ForceFeedback"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":haptic_darwin_headers",
        ":joystick_headers",
        ":joystick_hidapi_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "joystick_dummy_lib",
    srcs = ["src/joystick/dummy/SDL_sysjoystick.c"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":joystick_headers",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "joystick_virtual_lib",
    srcs = glob(["src/joystick/virtual/*.c"]),
    hdrs = glob(["src/joystick/virtual/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":joystick_headers",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "joystick_windows_lib",
    srcs = glob(["src/joystick/windows/*.c"]),
    hdrs = glob(["src/joystick/windows/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
)

cc_library(
    name = "joystick_lib",
    srcs = glob(
        ["src/joystick/*.c"],
        exclude = ["src/joystick/SDL_gamecontroller.c"],
    ),
    hdrs = glob(["src/joystick/*.h"]),
    deps = [
        "//:SDL_internal",
        ":dynapi_lib",
        ":thread_lib",
        ":video_lib",
        ":video_khronos_lib",
        ":joystick_dummy_lib",
        ":joystick_virtual_lib",
        ":joystick_hidapi_lib",
    ] + select({
        "//:windows": [":joystick_windows_lib"],
        "//:macos": [":joystick_darwin_lib"],
        "//conditions:default": [":joystick_linux_lib"],
    }),
    alwayslink = True,
)

cc_library(
    name = "joystick_headers",
    hdrs = glob(["src/joystick/*.h"]),
)

## src/haptic
cc_library(
    name = "haptic_windows_lib",
    srcs = glob(["src/haptic/windows/*.c"]),
    hdrs = glob(["src/haptic/windows/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
)

cc_library(
    name = "haptic_darwin_lib",
    srcs = glob(["src/haptic/darwin/*.c"]),
    hdrs = glob(["src/haptic/darwin/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":haptic_headers",
        ":joystick_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "haptic_darwin_headers",
    hdrs = glob([
        "src/haptic/darwin/*.h",
        "src/haptic/darwin/*.c",
    ]),
    visibility = ["//:__subpackages__"],
)

cc_library(
    name = "haptic_dummy_lib",
    srcs = glob(["src/haptic/dummy/*.c"]),
    hdrs = glob(["src/haptic/dummy/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":haptic_headers",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "haptic_linux_lib",
    srcs = glob(["src/haptic/linux/*.c"]),
    hdrs = glob(["src/haptic/linux/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "haptic_lib",
    srcs = glob(["src/haptic/*.c"]),
    hdrs = glob(["src/haptic/*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        "//:SDL_internal",
        ":dynapi_lib",
        ":joystick_lib",
    ] + select({
        "//:windows": [
            ":haptic_windows_lib",
        ],
        "//:macos": [
            ":haptic_darwin_lib",
        ],
        "//conditions:default": [
            ":haptic_dummy_lib",
            ":haptic_linux_lib",
        ],
    }),
    alwayslink = True,
)

cc_library(
    name = "haptic_headers",
    hdrs = glob(["src/haptic/*.h"]),
    visibility = ["//visibility:public"],
)

## src/sensor
cc_library(
    name = "sensor_windows_lib",
    srcs = glob(["src/sensor/windows/*.c"]),
    hdrs = glob(["src/sensor/windows/*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
)

objc_library(
    name = "sensor_coremotion_lib",
    srcs = glob(["src/sensor/coremotion/*.m"]),
    hdrs = glob(["src/sensor/coremotion/*.h"]),
    copts = ["-fno-objc-arc"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":audio_headers",
        ":dynapi_lib",
        ":thread_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "sensor_lib",
    srcs = [
        "src/sensor/SDL_sensor.c",
        "src/sensor/SDL_sensor_c.h",
        "src/sensor/SDL_syssensor.h",
        "src/sensor/dummy/SDL_dummysensor.c",
        "src/sensor/dummy/SDL_dummysensor.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//:SDL_internal",
        ":dynapi_lib",
        ":events_lib",
        ":video_lib",
        ":video_khronos_lib",
    ] + select({
        "//:windows": [":sensor_windows_lib"],
        "//:macos": [":sensor_coremotion_lib"],
        "//conditions:default": [],
    }),
    alwayslink = True,
)

## src/cpuinfo
cc_library(
    name = "cpuinfo_lib",
    srcs = glob(["src/cpuinfo/*.c"]),
    hdrs = glob(["src/cpuinfo/*.h"]),
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

## src/file
objc_library(
    name = "file_cocoa_lib",
    srcs = glob(["src/file/cocoa/*.m"]),
    hdrs = glob(["src/file/cocoa/*.h"]),
    copts = [
        "-fno-objc-arc",
    ],
    includes = ["src/thread"],
    sdk_frameworks = ["Cocoa"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "file_lib",
    srcs = glob(["src/file/*.c"]),
    hdrs = glob(["src/file/*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        "//:SDL_internal",
        ":dynapi_lib",
    ] + select({
        "//:macos": [":file_cocoa_lib"],
    }),
    alwayslink = True,
)

## src/filesystem
cc_library(
    name = "filesystem_windows_lib",
    srcs = glob(["src/filesystem/windows/*.c"]),
    hdrs = glob(["*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
)

objc_library(
    name = "filesystem_cocoa_lib",
    srcs = glob(["src/filesystem/cocoa/*.m"]),
    hdrs = glob(["*.h"]),
    copts = ["-fno-objc-arc"],
    sdk_frameworks = ["Cocoa"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":SDL_internal",
        ":dynapi_lib",
    ],
)

cc_library(
    name = "filesystem_dummy_lib",
    srcs = glob(["src/filesystem/dummy/*.c"]),
    hdrs = glob(["*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "filesystem_unix_lib",
    srcs = glob(["src/filesystem/unix/*.c"]),
    hdrs = glob(["*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "filesystem_lib",
    deps = [
        "//:SDL_internal",
    ] + select({
        "//:windows": [
            "filesystem_windows_lib",
        ],
        "//:macos": [
            "filesystem_cocoa_lib",
        ],
        "//conditions:default": [
            "filesystem_dummy_lib",
            "filesystem_unix_lib",
        ],
    }),
    alwayslink = True,
)

## src/loadso
cc_library(
    name = "loadso_windows_lib",
    srcs = glob(["src/loadso/windows/*.c"]),
    hdrs = glob(["*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
)

cc_library(
    name = "loadso_dlopen_lib",
    srcs = glob(["src/loadso/dlopen/*.c"]),
    hdrs = glob(["*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "loadso_dummy_lib",
    srcs = glob(["src/loadso/dummy/*.c"]),
    hdrs = glob(["*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "loadso_lib",
    deps = ["//:SDL_internal"] + select({
        "//:windows": ["loadso_windows_lib"],
        "//:macos": ["loadso_dlopen_lib"],
        "//conditions:default": [
            "loadso_dlopen_lib",
            "loadso_dummy_lib",
        ],
    }),
    alwayslink = True,
)

## src/locale
cc_library(
    name = "locale_windows_lib",
    srcs = glob(["src/locale/windows/*.c"]),
    hdrs = glob(["src/locale/windows/*.h"]),
    deps = ["//:SDL_internal"],
)

cc_library(
    name = "locale_lib",
    srcs = glob(["src/locale/*.c"]),
    hdrs = glob(["src/locale/*.h"]),
    deps = ["//:SDL_internal"] + select({
        "//:windows": ["locale_windows_lib"],
        "//:macos": ["locale_macosx_lib"],
        "//conditions:default": ["locale_unix_lib"],
    }),
    alwayslink = True,
)

cc_library(
    name = "locale_unix_lib",
    srcs = glob(["src/locale/unix/*.c"]),
    hdrs = glob(["*.h"]),
    deps = [
        ":dynapi_lib",
        ":locale_headers",
        "//:SDL_internal",
    ],
)

objc_library(
    name = "locale_macosx_lib",
    srcs = glob(["src/locale/macos/*.m"]),
    copts = ["-fno-objc-arc"],
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":locale_headers",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "locale_headers",
    hdrs = glob(["src/locale/*.h"]),
)

## src/main
cc_library(
    name = "main_lib",
    deps = ["//:SDL_internal"] + select({
        "//:windows": ["main_windows_lib"],
        "//conditions:default": ["main_dummy_lib"],
    }),
    alwayslink = True,
)

cc_library(
    name = "main_dummy_lib",
    srcs = glob(["src/main/dummy/*.c"]),
    hdrs = glob(["*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "main_windows_lib",
    srcs = glob(["src/main/windows/*.c"]),
    hdrs = glob(["*.h"]),
    visibility = ["//:__subpackages__"],
    deps = ["//:SDL_internal"],
)

## src/misc
cc_library(
    name = "misc_windows_lib",
    srcs = glob(["src/misc/windows/*.c"]),
    visibility = ["//:__subpackages__"],
    deps = [
        "core_windows_lib",
        "misc_headers",
    ],
    alwayslink = True,
)

cc_library(
    name = "misc_unix_lib",
    srcs = glob(["src/misc/unix/*.c"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        ":misc_headers",
        "//:SDL_internal",
    ],
    alwayslink = True,
)

cc_library(
    name = "misc_lib",
    srcs = glob(["src/misc/*.c"]),
    includes = ["src/misc"],
    visibility = ["//visibility:public"],
    deps = [
        "//:SDL_internal",
        ":misc_headers",
    ] + select({
        "//:windows": [":misc_windows_lib"],
        "//conditions:default": ["misc_unix_lib"],
    }),
    alwayslink = True,
)

cc_library(
    name = "misc_headers",
    hdrs = glob(["src/misc/*.h"]),
    visibility = ["//:__subpackages__"],
)

## src/power
cc_library(
    name = "power_windows_lib",
    srcs = glob(["src/power/windows/*.c"]),
    hdrs = glob(["*.h"]),
    deps = ["//:SDL_internal"],
)

cc_library(
    name = "power_macosx_lib",
    srcs = glob(["src/power/macosx/*.c"]),
    hdrs = glob(["*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "power_linux_lib",
    srcs = glob(["src/power/linux/*.c"]),
    hdrs = glob(["*.h"]),
    visibility = ["//:__subpackages__"],
    deps = [
        ":dynapi_lib",
        "//:SDL_internal",
    ],
)

cc_library(
    name = "power_lib",
    srcs = glob(["src/power/*.c"]),
    hdrs = glob(["src/power/*.h"]),
    deps = [
        "//:SDL_internal",
        "dynapi_lib",
    ] + select({
        "//:windows": ["power_windows_lib"],
        "//:macos": ["power_macosx_lib"],
        "//conditions:default": ["power_linux_lib"],
    }),
    alwayslink = True,
)

## base library
cc_library(
    name = "SDL_lib",
    srcs = glob([
        "include/*.h",
        "src/*.c",
        "src/*.h",
    ]),
    hdrs = glob([
        "include/*.h",
        "src/*.h",
        "src/core/**/*.h",
    ]),
    includes = [
        "include",
        "src",
    ],
    deps = [
        "misc_lib",
        "power_lib",
        ":SDL_internal",
        ":atomic_lib",
        ":audio_lib",
        ":core_lib",
        ":cpuinfo_lib",
        ":dynapi_lib",
        ":events_lib",
        ":file_lib",
        ":filesystem_lib",
        ":haptic_lib",
        ":hidapi_lib",
        ":joystick_lib",
        ":libm_lib",
        ":loadso_lib",
        ":locale_lib",
        ":main_lib",
        ":render_lib",
        ":sensor_lib",
        ":stdlib_lib",
        ":thread_lib",
        ":timer_lib",
        ":video_lib",
    ],
    alwayslink = True,
)

## base
cc_library(
    name = "SDL_base",
    srcs = [
        "src/SDL.c",
        "src/SDL_assert.c",
        "src/SDL_dataqueue.c",
        "src/SDL_error.c",
        "src/SDL_guid.c",
        "src/SDL_hints.c",
        "src/SDL_list.c",
        "src/SDL_log.c",
        "src/SDL_utils.c",
    ],
    hdrs = glob([
        "include/**/*.h",
        "src/*.h",
    ]),
    includes = [
        "include",
        "src",
    ],
    visibility = ["//:__subpackages__"],
    deps = [":thread_lib"],
)

cc_library(
    name = "SDL_internal",
    hdrs = glob([
        "include/**/*.h",
        "src/*.h",
    ]),
    includes = [
        "include",
        "src",
    ],
    visibility = ["//:__subpackages__"],
)

apple_binary(
    name = "SDL_mac",
    binary_type = "dylib",
    minimum_os_version = "15.0",
    platform_type = "macos",
    visibility = ["//:__subpackages__"],
    deps = [":SDL_lib"],
)

genrule(
    name = "SDL_dylib",
    srcs = ["SDL_mac"],
    outs = ["SDL.dylib"],
    cmd = "cp $(location SDL_mac) $(location SDL.dylib)",
)

cc_binary(
    name = "libSDL.so",
    linkshared = True,
    deps = [":SDL_lib"],
)

cc_binary(
    name = "SDL.dll",
    linkopts = SDL_LINKOPTS,
    linkshared = True,
    deps = [":SDL_lib"],
)
