#include <stdint.h>

#if defined (UVE_COMPILATION) //UVE Code
void
uve_config(void* src1, void*src2, uint64_t sizeN) {
    asm volatile(                       /*offset, size, stride*/
                 // A1 stream load
                 "ss.sta.ld.d           u1, %[src1a], %[sn], %[one] \t\n"        // D1: vector - linear access size V_len
                 "ss.end                u1, zero, %[sn], %[sn] \t\n"       // D4: slide window vertically N-2 times
                 // A2 stream load
                 "ss.sta.ld.d           u2, %[src1b], %[sn], %[one] \t\n"
                 "ss.end                u2, zero, %[sn], %[sn] \t\n" 
                 // A3 stream load
                 "ss.sta.ld.d           u3, %[src1c], %[sn], %[one] \t\n"
                 "ss.end                u3, zero, %[sn], %[sn] \t\n" 
                 // A4 stream load
                 "ss.sta.ld.d           u4, %[src1d], %[sn], %[one] \t\n"
                 "ss.end                u4, zero, %[sn], %[sn] \t\n" 
                 // A5 stream load
                 "ss.sta.ld.d           u5, %[src1e], %[sn], %[one] \t\n"
                 "ss.end                u5, zero, %[sn], %[sn] \t\n" 

                // B stream store
                 "ss.sta.st.d           u6, %[src2], %[sn], %[one] \t\n"
                 "ss.end                u6, zero, %[sn], %[sn] \t\n" 

                 
                 "so.v.dp.d  u7, %[fval], p0\t\n"
                 :
                 : [src1a] "r" (src1+sizeN+1),
                   [src1b] "r" (src1+sizeN),
                   [src1c] "r" (src1+sizeN+2),
                   [src1d] "r" (src1+sizeN+sizeN+1),
                   [src1e] "r" (src1+1),
                   [src2] "r" (src2+1+sizeN),
                   [sn] "r"(sizeN - 2),
                   [one] "r" (1), [fval] "r" (0.2)
                 :);

    return;
}
void
uve_kernel() {
    asm volatile(
        "fLoop1: \t\n"
        "so.a.add.fp u8, u1, u2, p0\n\t"
        "so.a.add.fp u9, u3, u4, p0\n\t"
        "so.a.add.fp u10, u8, u5, p0\n\t"
        "so.a.add.fp u11, u10, u9, p0\n\t"
        "so.a.mul.fp u6, u11, u7, p0\n\t"
        "so.b.nc	u1, fLoop1 \n\t"
        );
}

#else //SVE, Arm, RISC-V, x86 code

#define fDataType double
void
core_kernel(void* src1, void* src2, uint64_t sizeN) {
    fDataType *A = (fDataType *)src1; /* NxN */
    fDataType *B = (fDataType *)src2; /* NxN */

    for (int i = 1; i < sizeN - 1; i++)
        for (int j = 1; j < sizeN - 1; j++)
          B[i*sizeN+j] = 0.2  * ((fDataType) A[i*sizeN+j] + (fDataType) A[i*sizeN+j-1] + (fDataType) A[i*sizeN+j+1] + (fDataType) A[(i+1)*sizeN+j] + (fDataType) A[(i-1)*sizeN+j]);
      
    for (int i = 1; i < sizeN - 1; i++)
        for (int j = 1; j < sizeN - 1; j++)
            A[i*sizeN+j] = 0.2  * ((fDataType) B[i*sizeN+j] + (fDataType) B[i*sizeN+j-1] + (fDataType) B[i*sizeN+j+1] + (fDataType) B[(i+1)*sizeN+j] + (fDataType) B[(i-1)*sizeN+j]);
}

#endif
