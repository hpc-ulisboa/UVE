#include <stdint.h>

#if defined (UVE_COMPILATION) //UVE Code
#if 1
void
uve_config(void * src1, void * src2, void * src3, float alpha, float beta, uint64_t sizeI, uint64_t sizeJ, uint64_t sizeK) {
    int v_len = 16;
    asm volatile(                        /*offset, size, stride*/
                 // B stream
                 "ss.sta.ld.w           u2, %[src2], %[vl], %[one] \t\n"    // D1: vector - linear access size V_len
                 "ss.app                u2, zero, %[sk], %[sj] \t\n"        // D2: slide verticaly stride N ('k' times)
                 "ss.cfg.vec            u2\n\t"
                 "ss.app                u2, zero, %[jv], %[vl] \t\n"        // D3: slide horizontaly by V_len, access size J/V_len
                 "ss.cfg.vec            u2\n\t"
                 "ss.end                u2, zero, %[siu], zero \t\n"    // repeat: for each 'i'

                // A stream 
                 "ss.sta.ld.w           u1, %[src1], %[vl], zero \t\n"  // D1: vector element - linear access size one
                 "ss.app                u1, zero, %[sk], %[one] \t\n"    // D2: slide horizontaly by 1, access size K
                 "ss.app                u1, zero, %[jv], zero \t\n"        // D3: slide horizontaly by V_len, access size J/V_len
                 "ss.end                u1, zero, %[si], %[sk] \t\n"     // D3: slide verticaly stride K access size I

                 // C stream load
                 "ss.sta.ld.w           u3, %[src3], %[vl], %[one] \t\n" // D1: vector - linear access size V_len
                 "ss.cfg.mem1           u3 \n\t"
                 "ss.app                u3, zero, %[jv], %[vl] \t\n"     // D2: slide horizontaly by V_len, access size J/V_len
                 "ss.end                u3, zero, %[si], %[sj] \t\n"     // D3: slide verticaly stride J access size I

                 // C stream store
                 "ss.sta.st.w           u4, %[src3], %[vl], %[one] \t\n" // D1: vector - linear access size V_len
                 "ss.app                u4, zero, %[jv], %[vl] \t\n"     // D2: slide horizontaly by V_len, access size J/V_len
                 "ss.end                u4, zero, %[si], %[sj] \t\n"     // D3: slide verticaly stride J access size I

                 "so.v.dp.w  u5, %[alpha], p0\t\n"
                 "so.v.dp.w  u6, %[beta], p0\t\n"
                 :
                 : [src1] "r"(src1), [src2] "r"(src2), [src3] "r"(src3), 
                 [si] "r"(sizeI), [sj] "r"(sizeJ), [sk] "r"(sizeK), [one] "r" (1), [siu] "r"(sizeI/1),
                 [alpha] "r" (alpha), [beta] "r" (beta), [vl] "r" (v_len), [jv] "r" (sizeJ/v_len));
    return;
}
void
uve_kernel(uint64_t sizeK) {
    asm volatile(
        "iLoop1: \t\n"
          
          "jvloop1: \t\n"
            "so.a.mul.fp u9, u3, u6, p0 \n\t" // get vector C[ii][jv:jv+vlen] * beta -> tmp
            
            "kloop1: \t\n"
              "so.v.mv     u10, u2, p0\n\t"     // Get B 
              "so.a.mul.fp u11, u1, u5, p0\n\t" // A * alpha 
              // "so.a.mul.fp u12, u1, u5, p0\n\t" // A+1 * alpha 
              // "so.a.mul.fp u13, u1, u5, p0\n\t" // A+1 * alpha 
              // "so.a.mul.fp u14, u1, u5, p0\n\t" // A+1 * alpha 
              // "so.a.mul.fp u15, u1, u5, p0\n\t" // A+1 * alpha 
              // "so.a.mul.fp u16, u1, u5, p0\n\t" // A+1 * alpha 
              // "so.a.mul.fp u17, u1, u5, p0\n\t" // A+1 * alpha 
              // "so.a.mul.fp u18, u1, u5, p0\n\t" // A+1 * alpha 
              "so.a.mac.fp u21, u11, u10, p0\n\t" // tmp += a(A)   * B
              // "so.a.mac.fp u22, u12, u10, p0\n\t" // tmp += a(A+1) * B
              // "so.a.mac.fp u23, u13, u10, p0\n\t" // tmp += a(A+1) * B
              // "so.a.mac.fp u24, u14, u10, p0\n\t" // tmp += a(A+1) * B
              // "so.a.mac.fp u25, u15, u10, p0\n\t" // tmp += a(A+1) * B
              // "so.a.mac.fp u26, u16, u10, p0\n\t" // tmp += a(A+1) * B
              // "so.a.mac.fp u27, u17, u10, p0\n\t" // tmp += a(A+1) * B
              // "so.a.mac.fp u28, u18, u10, p0\n\t" // tmp += a(A+1) * B
            "so.b.ndc.2 u2, kloop1 \n\t"
            "so.v.mv  u4, u21, p0 \n\t" // store tmp to C
            // "so.v.mv  u4, u22, p0 \n\t" // store tmp to C+j
            // "so.v.mv  u4, u23, p0 \n\t" // store tmp to C+j
            // "so.v.mv  u4, u24, p0 \n\t" // store tmp to C+j
            // "so.v.mv  u4, u25, p0 \n\t" // store tmp to C+j
            // "so.v.mv  u4, u26, p0 \n\t" // store tmp to C+j
            // "so.v.mv  u4, u27, p0 \n\t" // store tmp to C+j
            // "so.v.mv  u4, u28, p0 \n\t" // store tmp to C+j
          
          "so.b.ndc.3 u2, jvloop1 \n\t"
        "so.b.nc	u2, iLoop1 \n\t"
        );
}

#else
void
uve_config(void * src1, void * src2, void * src3, float alpha, float beta, uint64_t sizeI, uint64_t sizeJ, uint64_t sizeK) {
    int v_len = 16;
    asm volatile(                        /*offset, size, stride*/
                 // B stream
                 "ss.sta.ld.w           u2, %[src2], %[vl], %[one] \t\n"    // D1: vector - linear access size V_len
                 "ss.app                u2, zero, %[sk], %[sj] \t\n"        // D2: slide verticaly stride N ('k' times)
                 "ss.cfg.vec            u2\n\t"
                 "ss.app                u2, zero, %[jv], %[vl] \t\n"        // D3: slide horizontaly by V_len, access size J/V_len
                 "ss.cfg.vec            u2\n\t"
                 "ss.end                u2, zero, %[si], zero \t\n"    // repeat: for each 'i'

                // A stream 
                 "ss.sta.ld.w           u1, %[src1], %[vl], zero \t\n"  // D1: vector element - linear access size one
                 "ss.app                u1, zero, %[sk], %[one] \t\n"    // D2: slide horizontaly by 1, access size K
                 "ss.app                u1, zero, %[jv], zero \t\n"        // D3: slide horizontaly by V_len, access size J/V_len
                 "ss.end                u1, zero, %[si], %[sk] \t\n"     // D3: slide verticaly stride K access size I

                 // C stream load
                 "ss.sta.ld.w           u3, %[src3], %[vl], %[one] \t\n" // D1: vector - linear access size V_len
                 "ss.app                u3, zero, %[jv], %[vl] \t\n"     // D2: slide horizontaly by V_len, access size J/V_len
                 "ss.end                u3, zero, %[si], %[sj] \t\n"     // D3: slide verticaly stride J access size I

                 // C stream store
                 "ss.sta.st.w           u4, %[src3], %[vl], %[one] \t\n" // D1: vector - linear access size V_len
                 "ss.app                u4, zero, %[jv], %[vl] \t\n"     // D2: slide horizontaly by V_len, access size J/V_len
                 "ss.end                u4, zero, %[si], %[sj] \t\n"     // D3: slide verticaly stride J access size I

                 "so.v.dp.w  u5, %[alpha], p0\t\n"
                 "so.v.dp.w  u6, %[beta], p0\t\n"
                 :
                 : [src1] "r"(src1), [src2] "r"(src2), [src3] "r"(src3), 
                 [si] "r"(sizeI), [sj] "r"(sizeJ), [sk] "r"(sizeK), [one] "r" (1),
                 [alpha] "r" (alpha), [beta] "r" (beta), [vl] "r" (v_len), [jv] "r" (sizeJ/v_len));
    return;
}
void
uve_kernel(uint64_t sizeK) {
    int v_len = 16;
    asm volatile(
        "iLoop1: \t\n"
          
          "jvloop1: \t\n"
            "so.a.mul.fp u9, u3, u6, p0 \n\t" // get vector C[ii][jv:jv+vlen] * beta -> tmp
            
            "kloop1: \t\n"
              "so.a.mul.fp u8, u1, u5, p0\n\t" // A * alpha 
              "so.a.mac.fp u9, u8, u2, p0\n\t" // tmp += () * B
            "so.b.ndc.2 u2, kloop1 \n\t"
            "so.v.mv  u4, u9, p0 \n\t" // store tmp to C
          
          "so.b.ndc.3 u2, jvloop1 \n\t"
        "so.b.nc	u2, iLoop1 \n\t"
        );
}

#endif

#else //SVE, Arm, RISC-V, x86 code

#define fDataType float
void
core_kernel(void * src1, void * src2, void * src3, float alpha, float beta, uint64_t sizeI, uint64_t sizeJ, uint64_t sizeK) {
    fDataType *A = (fDataType *)src1; /* IxK */
    fDataType *B = (fDataType *)src2; /* KxJ */
    fDataType *C = (fDataType *)src3; /* IxJ */

    for (int i = 0; i < sizeI; i++) {
        for (int j = 0; j < sizeJ; j++)
        C[i*sizeJ+j] *= beta;
        for (int k = 0; k < sizeK; k++) {
        for (int j = 0; j < sizeJ; j++)
            C[i*sizeJ+j] += alpha * (fDataType) A[i*sizeK+k] * (fDataType) B[k*sizeJ+j];
        }
    } 
}

#endif
