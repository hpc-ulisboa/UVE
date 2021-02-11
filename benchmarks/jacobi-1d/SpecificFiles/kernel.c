#include <stdint.h>

#if defined (UVE_COMPILATION) //UVE Code
void
uve_config(void* src1, void*src2, uint64_t sizeN) {
    double ct = 0.33333;
    asm volatile(                        //offset, size, stride 
                //Instructions 
                "ss.ld.d               u1, %[src1a], %[sn], %[one] \t\n" 
                "ss.ld.d               u2, %[src1b], %[sn], %[one] \t\n" 
                "ss.ld.d               u3, %[src1c], %[sn], %[one] \t\n" 
                "ss.st.d               u4, %[src2],  %[sn], %[one] \t\n"
                "so.v.dp.d             u20, %[ct],    p0            \t\n"
                : // Dest
                : // Source  
                [src1a] "r" (src1), [src1b] "r" (src1+1), [src1c] "r" (src1+2), [sn] "r"(sizeN - 2), [src2] "r" (src2+1), [one] "r" (1), [ct] "r" (ct)
                : //Clobbers
    ); 

    return;
}
void
uve_kernel() {
    asm volatile(                        //offset, size, stride 
                //Instructions
                "loop:\t\n" 
                "so.a.add.fp   u10, u1,  u2,  p0 \t\n"
                "so.a.add.fp   u10, u10, u3,  p0 \t\n"
                "so.a.mul.fp   u4,  u10, u20, p0 \t\n"
                "so.b.nc       u1,  loop \t\n"
    ); 
}

#else //SVE, Arm, RISC-V, x86 code

#define fDataType double
void
core_kernel(void* src1, void* src2, uint64_t sizeN) {
    fDataType *A = (fDataType *)src1; // N 
    fDataType *B = (fDataType *)src2; // N 
    int t,i;

    for (i = 1; i < sizeN - 1; i++)
    B[i] = 0.33333 * (A[i-1] + A[i] + A[i + 1]);
    for (i = 1; i < sizeN - 1; i++)
    A[i] = 0.33333 * (B[i-1] + B[i] + B[i + 1]);
    
}

#endif
