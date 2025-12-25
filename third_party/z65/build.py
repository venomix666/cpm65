from build.ab import Rule, Target, Targets, simplerule
from build.llvm import llvmprogram
from tools.build import unixtocpm

llvmprogram(
    name="z65",
    srcs=["./z65.c"],
    deps=["lib+cpm65"],
)
