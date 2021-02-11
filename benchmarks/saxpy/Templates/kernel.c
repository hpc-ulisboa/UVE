
#include <stdint.h>

#if defined (UVE_COMPILATION)
void
uve_kernel_word(void* src1, void* src2, float A, uint64_t size) {
    asm volatile("ss.ld.w  u1, %[src1], %[y], %[z] \t\n"
                 "ss.ld.w  u2, %[src2], %[y], %[z] \t\n"
                 "ss.st.w  u3, %[src2], %[y], %[z] \t\n"
                 "so.v.dp.w  u4, %[A], p0\t\n"
                 :
                 : [src1] "r"(src1), [src2] "r"(src2), [y] "r"(size), [z] "r" (1), [A] "r" (A));

    return;
}

void
uve_kernel_fp_loop_1() {
    asm volatile(
        "fLoop1: \t\n"
        "so.a.mul.fp u5, u1, u4, p0\n\t"
        "so.a.add.fp u3, u2, u5, p0 \n\t"
        "so.b.nc	u1, fLoop1 \n\t"
        );
}

#else

#define fDataType float
void
core_kernel_fp_1(void *src1,void *src2, float A, uint64_t size) {
    fDataType *x = (fDataType *)src1;
    fDataType *y = (fDataType *)src2;
    for (int i = 0; i < size; i++) {
        y[i] +=(fDataType) x[i] * A;
    }
    return;
}
#endif
