from build.ab import Rule, Target, Targets, simplerule
from build.llvm import llvmprogram
from tools.build import unixtocpm

llvmprogram(
    name="z65",
    cflags=["-Ithird_party/zmalloc"],
    srcs=["./z65.c"],
    deps=["lib+cpm65", "third_party/zmalloc+zmalloc"],
)
