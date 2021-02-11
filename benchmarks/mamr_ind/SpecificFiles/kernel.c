#include <stdint.h>

#if defined (UVE_COMPILATION) //UVE Code
void
uve_config(void* src1/*data*/, void*src2/*max*/, void*src3/*indexes*/, uint64_t sizeN, uint64_t sizeM) {
    asm volatile(                        //offset, size, stride
                 //Instructions
                                 
                 // A stream (A[i])
                 "ss.sta.ld.w           u1, %[src3], %[sm], %[one] \t\n" 
                //  "ss.cfg.vec            u1 \t\n"
                 "ss.cfg.ind            u1 \t\n"        
                 "ss.end                u1, zero, %[sn], %[sm] \t\n" 

                 // B stream (B[A[i]])
                 "ss.sta.ld.w           u2, %[src1], %[one], %[one] \t\n"   
                 "ss.app                u2, zero, %[sm], zero \t\n"     
                 "ss.app.indl.ofs.add   u2, u1 \t\n"
                 "ss.end                u2, zero, %[sn], zero \t\n"     
                 
                 "ss.st.w               u2, %[src2], %[sn], %[one] \t\n"

                 : // Destination operands
                 
                 : // Source operands
                 [src1] "r"(src1), [sm] "r"(sizeM), [one] "r" (1),
                 [src2] "r"(src2), [sn] "r"(sizeN),
                 [src3] "r"(src3), [snm] "r"(sizeN*sizeM)
                 : //Clobbers (explicit registers used inside the code), avoid to put here u# registers

            );

    return;
}
void
uve_kernel() {
    asm volatile(
        "iLoop:                               \n\t"
            "so.v.mv  u3, u1, p0                \n\t"
            "so.b.dc.1 u1, oLoop              \n\t"
            "jLoop:                           \n\t"
                "so.a.max.fp u3, u3, u1, p0     \n\t"
                "so.b.ndc.1 u1, jLoop         \n\t"
        "oLoop:                               \n\t"
            "so.a.maxe.fp u2, u3, p0            \n\t"
            "so.b.nc u1, iLoop                \n\t"
    );
}

// #elif defined(SVE_COMPILATION) //SVE, Arm, RISC-V, x86 code

// void
// core_kernel(void* src1, void* src2, uint64_t sizeN, uint64_t sizeM) {
//     float fpm = -99999.9;
//     asm volatile(
//         "ldr  s5, [%[fpmin]] \n\t"
//         "mov  x3, %[sn]      \n\t"
//         "orr  x13, xzr, xzr \n\t"
//         "orr  x12, xzr, xzr \n\t"
//         "iLoop2: \n\t"
//             "whilelt p0.s, x12, %[sm] \n\t"
//             "fmov s1, s5 \n\t"
//             "jLoop2: \n\t"
//                 "ld1w z0.s, p0/z, [%[src1], x12, lsl #2] \n\t"
//                 "fmaxv s2, p0, z0.s \n\t"
//                 "fmaxnm s1, s1, s2 \n\t"
//                 "incw x12 \n\t"
//                 "whilelt p0.s, x12, %[sm] \n\t"
//                 "b.first jLoop2 \n\t"
//             "str s1, [%[src2], x13, lsl #2] \n\t"
//             "add x13, x13, #1 \n\t"
//             "subs x3, x3, #1 \n\t"
//             "b.ne iLoop2 \n\t"
//     :
//     :   [src1] "r" (src1), [src2] "r" (src2), [sn] "r" (sizeN), [sm] "r" (sizeM), [fpmin] "r" (&fpm)
//     : "x3", "x12", "x13", "s1", "s2", "s5" );
// }


#else //SVE, Arm, RISC-V, x86 code

#define fDataType float
#define MAX(a,b) a > b ? a : b
void
core_kernel(void* src1, void* src2, void* src3, uint64_t sizeN, uint64_t sizeM) {
    fDataType *data = (fDataType *)src1; // MxN
    unsigned *index = (unsigned *)src3; // MxN
    fDataType *max = (fDataType *)src2;  // N

    // for (int i = 0; i < sizeN; i++) {
    //   for (int j = 0; j < sizeM; j++)
    //     max[i] = MAX(data[i*sizeM+j], max[i]);
    // }
    fDataType tmp;
    for (int i = 0; i < sizeN; i++) {
        tmp = -99999;
        for (int j = 0; j < sizeM; j++) {
          if( data[i*sizeM+j] > tmp){
              tmp = data[index[i*sizeM+j]];
          }
        }
        max[i] = tmp; 
    }
}

#endif
