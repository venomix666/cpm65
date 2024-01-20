from build.llvm import llvmprogram

llvmprogram(
    name="color",
    srcs=["./color.S"],
    deps=[
        "include",
    ],
)
