from tools.build import mkcpmfs, img2osi
from build.llvm import llvmrawprogram
from config import (
    MINIMAL_APPS,
    MINIMAL_APPS_SRCS,
    BIG_APPS,
    BIG_APPS_SRCS,
    SCREEN_APPS,
    SCREEN_APPS_SRCS,
    BIG_SCREEN_APPS,
    PASCAL_APPS,
)

# ----------------------------------------------------------------------------
# 400, 500, 600 Mini-Floppy (5.25")

llvmrawprogram(
    name="osi400mf_bios",
    srcs=["./osi.S"],
    deps=[
        "include",
        "src/lib+bioslib",
        "src/arch/osi/floppy.S",
        "src/arch/osi/keyboard.S",
        "src/arch/osi/ascii.S",
    ],
    cflags=["-DOSI400"],
    linkscript="./osi.ld",
)

llvmrawprogram(
    name="osi500mf_bios",
    srcs=["./osi.S"],
    deps=[
        "include",
        "src/lib+bioslib",
        "src/arch/osi/floppy.S",
        "src/arch/osi/keyboard.S",
    ],
    cflags=["-DOSI500"],
    linkscript="./osi.ld",
)

llvmrawprogram(
    name="osi600mf_bios",
    srcs=["./osi.S"],
    deps=[
        "include",
        "src/lib+bioslib",
        "src/arch/osi/floppy.S",
        "src/arch/osi/keyboard.S",
    ],
    cflags=["-DOSI600"],
    linkscript="./osi.ld",
)

mkcpmfs(
    name="osi400mf_rawdiskimage",
    format="osi5",
    bootimage=".+osi400mf_bios",
    size=128 * 640,
    items={
        "0:ccp.sys@sr": "src+ccp",
        "0:bdos.sys@sr": "src/bdos",
    }
    | MINIMAL_APPS,
)

mkcpmfs(
    name="osi500mf_rawdiskimage",
    format="osi5",
    bootimage=".+osi500mf_bios",
    size=128 * 640,
    items={
        "0:ccp.sys@sr": "src+ccp",
        "0:bdos.sys@sr": "src/bdos",
    }
    | MINIMAL_APPS,
)

mkcpmfs(
    name="osi600mf_rawdiskimage",
    format="osi5",
    bootimage=".+osi600mf_bios",
    size=128 * 640,
    items={
        "0:ccp.sys@sr": "src+ccp",
        "0:bdos.sys@sr": "src/bdos",
    }
    | MINIMAL_APPS,
)

mkcpmfs(
    name="osimf-b_rawdiskimage",
    format="osi5",
    size=128 * 640,
    items={} | BIG_APPS | PASCAL_APPS,
)

mkcpmfs(
    name="osimf-c_rawdiskimage",
    format="osi5",
    size=128 * 640,
    items={} | MINIMAL_APPS_SRCS | BIG_APPS_SRCS,
)

mkcpmfs(
    name="osimf-d_rawdiskimage",
    format="osi5",
    size=128 * 640,
    items={
        "0:tty540b.com": "src/arch/osi/utils+tty540b",
        "0:tty630.com": "src/arch/osi/utils+tty630",
    }
    | SCREEN_APPS
    | SCREEN_APPS_SRCS
    | BIG_SCREEN_APPS,
)

img2osi(
    name="osi400mf_diskimage",
    src=".+osi400mf_rawdiskimage",
)

img2osi(
    name="osi500mf_diskimage",
    src=".+osi500mf_rawdiskimage",
)

img2osi(
    name="osi600mf_diskimage",
    src=".+osi600mf_rawdiskimage",
)

img2osi(
    name="osimf-b_diskimage",
    src=".+osimf-b_rawdiskimage",
)

img2osi(
    name="osimf-c_diskimage",
    src=".+osimf-c_rawdiskimage",
)

img2osi(
    name="osimf-d_diskimage",
    src=".+osimf-d_rawdiskimage",
)

# ----------------------------------------------------------------------------
# 400, 500, 600, Floppy (8")

llvmrawprogram(
    name="osi400f_bios",
    srcs=["./osi.S"],
    deps=[
        "include",
        "src/lib+bioslib",
        "src/arch/osi/floppy.S",
        "src/arch/osi/ascii.S",
    ],
    cflags=["-DOSI400", "-DFLOPPY8"],
    linkscript="./osi.ld",
)

llvmrawprogram(
    name="osi500f_bios",
    srcs=["./osi.S"],
    deps=[
        "include",
        "src/lib+bioslib",
        "src/arch/osi/floppy.S",
        "src/arch/osi/keyboard.S",
        "src/arch/osi/ascii.S",
    ],
    cflags=["-DOSI500", "-DFLOPPY8"],
    linkscript="./osi.ld",
)

llvmrawprogram(
    name="osi600f_bios",
    srcs=["./osi.S"],
    deps=[
        "include",
        "src/lib+bioslib",
        "src/arch/osi/floppy.S",
        "src/arch/osi/keyboard.S",
        "src/arch/osi/ascii.S",
    ],
    cflags=["-DOSI600", "-DFLOPPY8"],
    linkscript="./osi.ld",
)

mkcpmfs(
    name="osi400f_rawdiskimage",
    format="osi8",
    bootimage=".+osi400f_bios",
    size=128 * 1848,
    items={
        "0:ccp.sys@sr": "src+ccp",
        "0:bdos.sys@sr": "src/bdos",
    }
    | MINIMAL_APPS
    | BIG_APPS
    | PASCAL_APPS
    | MINIMAL_APPS_SRCS
    | BIG_APPS_SRCS,
)

mkcpmfs(
    name="osi500f_rawdiskimage",
    format="osi8",
    bootimage=".+osi500f_bios",
    size=128 * 1848,
    items={
        "0:ccp.sys@sr": "src+ccp",
        "0:bdos.sys@sr": "src/bdos",
        "0:tty540b.com": "src/arch/osi/utils+tty540b",
    }
    | MINIMAL_APPS
    | BIG_APPS
    | PASCAL_APPS
    | MINIMAL_APPS_SRCS
    | BIG_APPS_SRCS
    | SCREEN_APPS
    | SCREEN_APPS_SRCS
    | BIG_SCREEN_APPS,
)

mkcpmfs(
    name="osi600f_rawdiskimage",
    format="osi8",
    bootimage=".+osi600f_bios",
    size=128 * 1848,
    items={
        "0:ccp.sys@sr": "src+ccp",
        "0:bdos.sys@sr": "src/bdos",
        "0:tty630.com": "src/arch/osi/utils+tty630",
    }
    | MINIMAL_APPS
    | BIG_APPS
    | PASCAL_APPS
    | MINIMAL_APPS_SRCS
    | BIG_APPS_SRCS,
)

mkcpmfs(name="osif-b_rawdiskimage", format="osi8", size=128 * 1848, items={})

img2osi(
    name="osi400f_diskimage",
    src=".+osi400f_rawdiskimage",
)

img2osi(
    name="osi500f_diskimage",
    src=".+osi500f_rawdiskimage",
)

img2osi(
    name="osi600f_diskimage",
    src=".+osi600f_rawdiskimage",
)

img2osi(
    name="osif-b_diskimage",
    src=".+osif-b_rawdiskimage",
)

# ----------------------------------------------------------------------------
# Serial system with 8" floppy

llvmrawprogram(
    name="osiserf_bios",
    srcs=["./osi.S"],
    deps=[
        "include",
        "src/lib+bioslib",
        "src/arch/osi/floppy.S",
        "src/arch/osi/serial.S",
    ],
    cflags=["-DOSISERIAL", "-DFLOPPY8"],
    linkscript="./osi.ld",
)

mkcpmfs(
    name="osiserf_rawdiskimage",
    format="osi8",
    bootimage=".+osiserf_bios",
    size=128 * 1848,
    items={
        "0:ccp.sys@sr": "src+ccp",
        "0:bdos.sys@sr": "src/bdos",
        "0:scrvt100.com": "apps+scrvt100",
    }
    | MINIMAL_APPS
    | BIG_APPS
    | PASCAL_APPS
    | MINIMAL_APPS_SRCS
    | BIG_APPS_SRCS
    | SCREEN_APPS
    | SCREEN_APPS_SRCS
    | BIG_SCREEN_APPS,
)

img2osi(
    name="osiserf_diskimage",
    src=".+osiserf_rawdiskimage",
)

# ----------------------------------------------------------------------------
# 600 series with 5.25" (or 3.5") floppy, 80 tracks

llvmrawprogram(
    name="osi600mf80_bios",
    srcs=["./osi.S"],
    deps=[
        "include",
        "src/lib+bioslib",
        "src/arch/osi/floppy.S",
        "src/arch/osi/keyboard.S",
    ],
    cflags=["-DOSI600 -DTRACKS80"],
    linkscript="./osi.ld",
)

mkcpmfs(
    name="osi600mf80_rawdiskimage",
    format="osi5_80",
    bootimage=".+osi600mf80_bios",
    size=128 * 1280,
    items={
        "0:ccp.sys@sr": "src+ccp",
        "0:bdos.sys@sr": "src/bdos",
        "0:tty630.com": "src/arch/osi/utils+tty630",
        "0:pint.com": "third_party/pascal-m+pint",
        "0:pasc.obb": "third_party/pascal-m+pasc-obb",
        "0:pload.com": "third_party/pascal-m+loader",
    }
    | MINIMAL_APPS
    | BIG_APPS
    | SCREEN_APPS,
)

mkcpmfs(
    name="osimf80-b_rawdiskimage",
    format="osi5_80",
    size=128 * 1280,
    items={
        "0:hello.pas": "cpmfs+hello_pas_cpm",
    }
    | MINIMAL_APPS_SRCS
    | BIG_APPS_SRCS
    | SCREEN_APPS_SRCS
    | BIG_SCREEN_APPS,
)

mkcpmfs(
    name="osimf80-c_rawdiskimage", format="osi5_80", size=128 * 1280, items={}
)

mkcpmfs(
    name="osimf80-d_rawdiskimage", format="osi5_80", size=128 * 1280, items={}
)

img2osi(
    name="osi600mf80_diskimage",
    src=".+osi600mf80_rawdiskimage",
)

img2osi(
    name="osimf80-b_diskimage",
    src=".+osimf80-b_rawdiskimage",
)

img2osi(
    name="osimf80-c_diskimage",
    src=".+osimf80-c_rawdiskimage",
)

img2osi(
    name="osimf80-d_diskimage",
    src=".+osimf80-d_rawdiskimage",
)
