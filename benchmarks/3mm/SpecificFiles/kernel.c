#include <stdint.h>

#if defined (UVE_COMPILATION) //UVE Code
void
uve_config(void* src1, void* src2, void* src3, uint64_t sizeI, uint64_t sizeJ, uint64_t sizeK) {
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

                 // C stream store
                 "ss.sta.st.w           u4, %[src3], %[vl], %[one] \t\n" // D1: vector - linear access size V_len
                 "ss.app                u4, zero, %[jv], %[vl] \t\n"     // D2: slide horizontaly by V_len, access size J/V_len
                 "ss.end                u4, zero, %[si], %[sj] \t\n"     // D3: slide verticaly stride J access size I

                 :
                 : [src1] "r"(src1), [src2] "r"(src2), [src3] "r"(src3), 
                 [si] "r"(sizeI), [sj] "r"(sizeJ), [sk] "r"(sizeK), [one] "r" (1), [vl] "r" (v_len), [jv] "r" (sizeJ/v_len));

    return;
}

void
uve_kernel() {
    asm volatile(
        "iLoop1: \t\n"    
            "so.v.dp.w u21, zero, p0 \t\n"
            "kloop1: \t\n"
              "so.a.mac.fp u21, u1, u2, p0\n\t" // tmp += (.A) * B
            "so.b.ndc.2 u2, kloop1 \n\t"
            "so.v.mv  u4, u21, p0 \n\t" // store tmp to C 
        "so.b.nc	u2, iLoop1 \n\t"
        );
}

#else //SVE, Arm, RISC-V, x86 code

#define fDataType float

void
core_kernel(void* src1, void* src2, void* src3, uint64_t sizeI, uint64_t sizeJ, uint64_t sizeK) {
    fDataType *C = (fDataType *)src1; /* IxJ */
    fDataType *A = (fDataType *)src2; /* IxK */
    fDataType *B = (fDataType *)src3; /* KxJ */

    for (int i = 0; i < sizeI; i++) {
        for (int j = 0; j < sizeJ; j++)
        C[i*sizeJ+j] = 0;
        for (int k = 0; k < sizeK; k++) {
        for (int j = 0; j < sizeJ; j++)
            C[i*sizeJ+j] += (fDataType) A[i*sizeK+k] * (fDataType) B[k*sizeJ+j];
        }
    } 
}

    // /* E := A*B */
    // for (int i = 0; i < sizeI; i++)
    //   for (int j = 0; j < sizeJ; j++) {
    //     E[i][j] = 0.0;
    //     for (int k = 0; k < sizeK; k++)
    //       E[i][j] += A[i][k] * B[k][j];
    //   }

    // /* F := C*D */
    // for (int j = 0; j < sizeJ; j++)
    //   for (int l = 0; l < sizeL; l++) {
    //     F[j][l] = 0.0;
    //     for (int m = 0; m < sizeM; m++)
    //       F[j][l] += C[j][m] * D[m][l];
    //   }

    // /* G := E*F */
    // for (int i = 0; i < sizeI; i++)
    //   for (int l = 0; l < sizeL; l++){
    //     G[i][l] = 0.0;
    //     for (int j = 0; j < sizeJ; j++)
    //       G[i][l] += E[i][j] * F[j][l];
    //   }

#endif
