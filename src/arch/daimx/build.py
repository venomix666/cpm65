from build.ab import normalrule
from tools.build import mkdfs, mkcpmfs
from build.llvm import llvmrawprogram
from config import (
    MINIMAL_APPS,
    MINIMAL_APPS_SRCS,
    BIG_APPS,
    BIG_APPS_SRCS,
)

llvmrawprogram(
    name="daimx",
    srcs=["./daimx.S"],
    deps=["include", "src/lib+bioslib"],
    linkscript="./daimx.ld",
)

mkcpmfs(
    name="cpmfs",
    format="generic-1m",
    items={"0:ccp.sys@sr": "src+ccp"}
    | MINIMAL_APPS
    | MINIMAL_APPS_SRCS
    | BIG_APPS
    | BIG_APPS_SRCS,
)

normalrule(
    name="diskimage",
    ins=[
        ".+cpmfs",
        ".+daimx",
        "src+bdos",
    ],
    outs=["daimx.zip"],
    commands=[
        "zip -9qj {outs[0]} {ins}",
        r'printf "@ src+bdos\n@=BDOS\n" | zipnote -w {outs[0]}',
        r'printf "@ daimx+daimx\n@=CPM.DMX\n" | zipnote -w {outs[0]}',
        r'printf "@ daimx+cpmfs.img\n@=CPMFS\n" | zipnote -w {outs[0]}',
    ],
    label="ZIP",
)
