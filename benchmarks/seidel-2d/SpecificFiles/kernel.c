#include <stdint.h>

#if defined (UVE_COMPILATION) //UVE Code
void
uve_config(void* src1, uint64_t sizeN) {
    asm volatile(                       /*offset, size, stride*/
                 // A stream load
                 "ss.sta.ld.d           u1, %[src1a], %[sn], %[one] \t\n"        // D1: vector - linear access size V_len
                 "ss.end                u1, zero, %[sn], %[sn] \t\n"       // D4: slide window vertically N-2 times
                 "ss.sta.ld.d           u2, %[src1b], %[sn], %[one] \t\n"
                 "ss.end                u2, zero, %[sn], %[sn] \t\n" 
                 "ss.sta.ld.d           u3, %[src1c], %[sn], %[one] \t\n"
                 "ss.end                u3, zero, %[sn], %[sn] \t\n" 
                 "ss.sta.ld.d           u4, %[src1d], %[sn], %[one] \t\n"
                 "ss.end                u4, zero, %[sn], %[sn] \t\n" 
                 "ss.sta.ld.d           u5, %[src1e], %[sn], %[one] \t\n"
                 "ss.end                u5, zero, %[sn], %[sn] \t\n" 
                 "ss.sta.ld.d           u6, %[src1f], %[sn], %[one] \t\n"
                 "ss.end                u6, zero, %[sn], %[sn] \t\n" 
                 "ss.sta.ld.d           u7, %[src1g], %[sn], %[one] \t\n"
                 "ss.end                u7, zero, %[sn], %[sn] \t\n" 
                 "ss.sta.ld.d           u8, %[src1h], %[sn], %[one] \t\n"
                 "ss.end                u8, zero, %[sn], %[sn] \t\n" 
                 "ss.sta.ld.d           u9, %[src1i], %[sn], %[one] \t\n"
                 "ss.end                u9, zero, %[sn], %[sn] \t\n" 

                // B stream store
                 "ss.sta.st.d           u10, %[src1e], %[sn], %[one] \t\n"
                 "ss.end                u10, zero, %[sn], %[sn] \t\n" 

                 
                 "so.v.dp.d  u20, %[fval], p0\t\n"
                 :
                 : [src1a] "r"  (src1),
                   [src1b] "r"  (src1+1),
                   [src1c] "r"  (src1+1+1),
                   [src1d] "r"  (src1+sizeN),
                   [src1e] "r"  (src1+sizeN+1),
                   [src1f] "r"  (src1+sizeN+1+1),
                   [src1g] "r"  (src1+sizeN+sizeN),
                   [src1h] "r"  (src1+sizeN+sizeN+1),
                   [src1i] "r"  (src1+sizeN+sizeN+1+1),
                   [sn]    "r"  (sizeN - 2),
                   [one]   "r"  (1),
                   [fval]  "r"  (9.0)
                 :);

    return;
}
void
uve_kernel() {
    asm volatile(
        "fLoop1: \t\n"
        "so.a.add.fp u21, u1,  u2,  p0  \n\t"
        "so.a.add.fp u22, u3,  u4,  p0  \n\t"
        "so.a.add.fp u23, u5,  u6,  p0  \n\t"
        "so.a.add.fp u24, u7,  u8,  p0  \n\t"
        "so.a.add.fp u21, u21, u22, p0  \n\t"
        "so.a.add.fp u23, u23, u24, p0  \n\t"
        "so.a.add.fp u21, u21, u23, p0  \n\t"
        "so.a.add.fp u21, u21, u9,  p0  \n\t"
        "so.a.div.fp u10, u21, u20, p0  \n\t"
        "so.b.nc	 u1,  fLoop1        \n\t"
        );
}

#else //SVE, Arm, RISC-V, x86 code

#define fDataType double
void
core_kernel(void* src1, uint64_t sizeN) {
    fDataType *A = (fDataType *)src1; /* NxN */

    for (int i = 1; i < sizeN - 2; i++) {
        for (int j = 1; j < sizeN - 2; j++) {
            A[(i  )*sizeN + j  ] = 
                 A[(i-1)*sizeN + j-1] +
                 A[(i-1)*sizeN + j  ] + 
                 A[(i-1)*sizeN + j+1] +
                 A[(i  )*sizeN + j-1] +
                 A[(i  )*sizeN + j  ] +
                 A[(i  )*sizeN + j+1] +
                 A[(i+1)*sizeN + j-1] +
                 A[(i+1)*sizeN + j  ] +
                 A[(i+1)*sizeN + j+1];
            A[(i  )*sizeN + j  ] = A[(i  )*sizeN + j  ]/9;
        }
    }
}

#endif
