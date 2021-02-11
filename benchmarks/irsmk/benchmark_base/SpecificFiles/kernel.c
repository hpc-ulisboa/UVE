#include <stdint.h>

#if defined (UVE_COMPILATION) //UVE Code
void
uve_config(void* src1/*data*/, void*src3/*mean*/, uint64_t sizeN, uint64_t sizeM) {
    /* asm volatile(                        //offset, size, stride */
    /*              //Instructions */
    /*              "ss.st.w               u2, %[src3], %[sm], %[one] \t\n" */ 
    /*              : // Destination operands */
                 
    /*              : // Source operands */
    /*              [src3] "r"(src3), [sm] "r"(sizeM), [one] "r" (1) */
    /*              : //Clobbers (explicit registers used inside the code), avoid to put here u# registers */

    /*         ); */

    return;
}
void
uve_kernel() {
    /* asm volatile( */
    /*     "aLoop1: \t\n" */
    /*     "so.v.dp.w  u4, %[zero], p0\n\t" */
    /*     "add x12, x0, %[sn]\t\n" */
    /*     "so.a.div.fp u2, u4, u3, p0\n\t" */
    /*     "so.b.nc	u1, aLoop1 \n\t" */
    /*     : */
    /*     : [sn] "r"(sizeN), [zero] "r" (0), [one] "r" (1)); */
    /*     : "x12", "x0" */ 
}

#else //SVE, Arm, RISC-V, x86 code

#define fDataType float
void
core_kernel(void* src1, void* src2, void* src3, uint64_t sizeN, uint64_t sizeM) {
    /* fDataType *data = (fDataType *)src1; // NxM */
    /* fDataType *cov = (fDataType *)src2;  // MxM */
    /* fDataType *mean = (fDataType *)src3; // M */ 

    /* fDataType float_n = (fDataType) sizeN; */

    /* for (int j = 0; j < sizeM; j++) { */
    /*   mean[j] = 0.0; */
    /*   for (int i = 0; i < sizeN; i++) */
    /*     mean[j] +=  data[i*sizeM+j]; */
    /*   mean[j] /= float_n; */
    /* } */
}

#endif
