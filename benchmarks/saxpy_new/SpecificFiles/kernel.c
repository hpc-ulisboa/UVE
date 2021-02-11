#include <stdint.h>

#if defined (UVE_COMPILATION) //UVE Code
void
uve_config(void* src1, void*src2, uint64_t sizeN, float A) {
    asm volatile("ss.ld.w  u1, %[src1], %[y], %[z] \t\n"
                 "ss.ld.w  u2, %[src2], %[y], %[z] \t\n"
                 "ss.st.w  u3, %[src2], %[y], %[z] \t\n"
                 "so.v.dp.w  u4, %[A], p0\t\n"
                 :
                 : [src1] "r"(src1), [src2] "r"(src2), [y] "r"(sizeN), [z] "r" (1), [A] "r" (A));

    return;

    return;
}
void
uve_kernel() {
    asm volatile(
        "fLoop1: \t\n"
        "so.a.mul.fp u5, u1, u4, p0\n\t"
        "so.a.mul.fp u6, u1, u4, p0\n\t"
        "so.a.mul.fp u7, u1, u4, p0\n\t"
        "so.a.mul.fp u8, u1, u4, p0\n\t"
        "so.a.add.fp u3, u2, u5, p0 \n\t"
        "so.a.add.fp u3, u2, u6, p0 \n\t"
        "so.a.add.fp u3, u2, u7, p0 \n\t"
        "so.a.add.fp u3, u2, u8, p0 \n\t"
        "so.b.nc	u1, fLoop1 \n\t"
        );
}

#else //SVE, Arm, RISC-V, x86 code

#define fDataType float
void
core_kernel(void* src1, void* src2, uint64_t sizeN, float A) {
    fDataType *x = (fDataType *)src1;
    fDataType *y = (fDataType *)src2;
    for (int i = 0; i < sizeN; i++) {
        y[i] +=(fDataType) x[i] * A;
    }
}

#endif
