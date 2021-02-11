#include <stdint.h>

#if defined (UVE_COMPILATION)
void
uve_config_1(void* src1/*data*/, void*src3/*mean*/, uint64_t sizeN, uint64_t sizeM) {
    int v_len = 16;
    asm volatile(                        /*offset, size, stride*/
                 // data stream load
                 "ss.sta.ld.w           u1, %[src1], %[vl], %[one] \t\n"    // D1: vector - linear access size V_len
                 "ss.app                u1, %[zero], %[sn], %[sm] \t\n"     // D2: slide verticaly stride M access size N
                 "ss.end                u1, %[zero], %[mv], %[vl] \t\n"     // D3: slide horizontaly by V_len, access size M/V_len
                 
                 // mean stream store
                 "ss.st.w               u2, %[src3], %[sm], %[one] \t\n"    // D1: linear access size M (should build vector automatically)
                 
                 "so.v.dp.w  u3, %[floatN], p0\t\n"
                 :
                 : [src1] "r"(src1), [src3] "r"(src3), 
                 [sn] "r"(sizeN), [sm] "r"(sizeM), [zero] "r" (0), [one] "r" (1),
                 [vl] "r" (v_len), [mv] "r" (sizeM/v_len),
                 [floatN] "r" ((float)sizeN));

    return;
}


void
uve_kernel_1(uint64_t sizeN) {
    asm volatile(
        "aLoop1: \t\n"
        "so.v.dp.w  u4, %[zero], p0\n\t"

        "add x12, x0, %[sn]\t\n"
        "bloop1: \t\n"
          "so.a.add.fp u4, u4, u1, p0\n\t"
          "sub x12, x12, %[one] \n\t"
          "bne x12, %[zero], bloop1 \n\t"
          
        "so.a.div.fp u2, u4, u3, p0\n\t"
        "so.b.nc	u1, aLoop1 \n\t"
        :
        : [sn] "r"(sizeN), [zero] "r" (0), [one] "r" (1));

}


void
uve_config_2(void* src1/*data*/, void*src3/*mean*/, uint64_t sizeN, uint64_t sizeM) {
    int v_len = 16;
    // NOTE: Should vectorize automatically
    asm volatile(                        /*offset, size, stride*/
                 // mean stream load
                 "ss.sta.ld.w           u1, %[src3], %[sm], %[one] \t\n"    // D1: linear access size M 
                 "ss.end                u1, %[zero], %[sn], %[zero] \t\n"   // repeat N times

                // data stream load
                 "ss.sta.ld.w           u2, %[src1], %[sm], %[one] \t\n"    // D1: linear access size M
                 "ss.end                u2, %[zero], %[sm], %[sm] \t\n"     // D2: strided M access size N

                 // data stream store
                 "ss.sta.st.w           u3, %[src1], %[sm], %[one] \t\n"    // D1: linear access size M
                 "ss.end                u3, %[zero], %[sm], %[sm] \t\n"     // D2: strided M access size N
                 
                 :
                 : [src1] "r"(src1), [src3] "r"(src3), 
                 [sn] "r"(sizeN), [sm] "r"(sizeM), [zero] "r" (0), [one] "r" (1),
                 [vl] "r" (v_len));

    return;
}


void
uve_kernel_2() {
    asm volatile(
        "cLoop1: \t\n"
        "so.a.sub.fp u3, u2, u1, p0\n\t"
        "so.b.nc	u3, cLoop1 \n\t"
    );

}


void
uve_config_3(void* src1/*data*/,  uint64_t sizeN, uint64_t sizeM) {
    int v_len = 16;
    asm volatile(                       //offset, size, stride 
                 // data1 stream load - left column repeat 
                 "ss.sta.ld.w           u1, %[src1], %[vl], %[sm] \t\n"     // d1: strided m access size v_len 
                 "ss.app                u1, %[zero], %[mv], %[vm] \t\n"     // d2: slide vertically m/v_len times by m*v_len 
                 "ss.app                u1, %[zero], %[sm], %[zero] \t\n"   // repeat j = m...0 times 
                 "ss.app                u1, %[zero], %[sm], %[one] \t\n"    // d3: new column stride 1 
                 "ss.end.mod.siz.dec    u1, %[sm], %[one] \t\n"             // decrement 'j' 
                 
                 // data2 stream load - swipe all right columns 
                 "ss.sta.ld.w           u2, %[src1], %[vl], %[sm] \t\n"     // d1: strided m access size v_len 
                 "ss.app                u2, %[zero], %[mv], %[vm] \t\n"     // d2: slide vertically m/v_len times by m*v_len 
                 "ss.app                u2, %[zero], %[sm], %[one] \t\n"    // d3: linear access size j = m...0 
                 "ss.app                u2, %[zero], %[sm], %[one] \t\n"    // new column start stride 1 
                 "ss.end.mod.siz.dec    u2, %[sm], %[one] \t\n"             // decrement 'j' 

                 : 
                 : [src1] "r"(src1), [sn] "r"(sizeN), [sm] "r"(sizeM), 
                 [vl] "r" (v_len), [mv] "r" (sizeM/v_len) , [vm] "r" (sizeM*v_len),
                 [zero] "r" (0), [one] "r" (1)); 

    return;
}


#define fDataType float
// #pragma GCC push_options
// #pragma GCC optimize ("-O3")
void
uve_kernel_3 (void* src1, void* src2/*cov*/, uint64_t sizeN, uint64_t sizeM) {
    int v_len = 16;
    // fDataType *data = (fDataType *)src1; /* NxM*/
    // fDataType *cov = (fDataType *)src2; /* MxM */
    // fDataType float_n = (fDataType) sizeN;
    // for (int i = 0; i < sizeM; i++)
    //   for (int j = i; j < sizeM; j++) {

    //     cov[i*sizeM+j] = 0.0;
    //     for (int k = 0; k < sizeN; k++)
    //       cov[i*sizeM+j] += data[k*sizeM+i] * data[k*sizeM+j];

    //     cov[i*sizeM+j] /= (float_n - 1.0);
    //     cov[j*sizeM+i] = cov[i*sizeM+j];
    //   }
     asm volatile( 
         "add x10, zero, %[src2]\t\n" 
         "add x11, zero, %[sm]\t\n" 
         "add x12, zero, %[zero]\t\n" //i 
         "add x13, zero, %[zero]\t\n" //j 
         "fLoop1: \t\n" 

         "so.v.dp.w  u7, %[zero], p0\n\t" 
         "add x9, zero, %[mv]\t\n" 
         "kloop1: \t\n" 
           "so.a.mac.fp u7, u1, u2, p0\n\t" 
           "sub x9, x9, %[one] \n\t" 
           "bne x9, %[zero], kloop1 \n\t" 
        
         "so.a.adds.fp f10, u7, p0 \n\t" 

         "fdiv.s f10, f10, %[fn] \n\t" 

         "mul x14, x12, x11 \n\t" // i*M 
         "add x14, x14, x13 \n\t" // i*M + j 
         "add x14, x10, x14 \n\t" // *cov + i*M + j 
         "fsw f10, 0(x14) \n\t"   // store cov[i][j] 

         "mul x14, x13, x11 \n\t" // j*M 
         "add x14, x14, x12 \n\t" // j*M + i 
         "add x14, x10, x14 \n\t" // *cov + j*M + i 
         "fsw f10, 0(x14) \n\t"   // store cov[j][i] 
        
         "add x13, x13, %[one] \n\t" // inc j
         "bne x13, x11, skip1 \n\t"     // detect counter end
           "add x12, x12, %[one] \n\t" // inc i
           "add x13, x12, %[zero] \n\t" // reset j=i
           "sub x11, x11, %[one] \n\t"      // dec row size
         "skip1:"

         "so.b.nc	u1, fLoop1 \n\t" 
         : 
         : [src2] "r"(src2), [sn] "r"(sizeN), [sm] "r"(sizeM), 
         [vl] "r" (v_len), [mv] "r" (sizeM/v_len), [sm1] "r"(sizeM+1), [fsize] "r" (sizeof(float)), 
         [zero] "r" (0), [one] "r" (1), [fn] "f" ((float)sizeN - 1.0) 
         : "x10", "x11", "x12", "x13", "x9", "x14");
}


#else


#define fDataType float
void
core_kernel_1(void* src1, void* src2, void* src3, uint64_t sizeN, uint64_t sizeM) {
    fDataType *data = (fDataType *)src1; /* NxM*/
    fDataType *cov = (fDataType *)src2; /* MxM */
    fDataType *mean = (fDataType *)src3; /* M */

    fDataType float_n = (fDataType) sizeN;

    for (int j = 0; j < sizeM; j++) {
      mean[j] = 0.0;
      for (int i = 0; i < sizeN; i++)
        mean[j] +=  data[i*sizeM+j];
      mean[j] /= float_n;
    }
}
void
core_kernel_2(void* src1, void* src2, void* src3, uint64_t sizeN, uint64_t sizeM) {
    fDataType *data = (fDataType *)src1; /* NxM*/
    fDataType *cov = (fDataType *)src2; /* MxM */
    fDataType *mean = (fDataType *)src3; /* M */

    fDataType float_n = (fDataType) sizeN;

    for (int i = 0; i < sizeN; i++)
      for (int j = 0; j < sizeM; j++)
        data[i*sizeM+j] -= mean[j];
}
    
void
core_kernel(void* src1, void* src2, void* src3, uint64_t sizeN, uint64_t sizeM) {
    fDataType *data = (fDataType *)src1; /* NxM*/
    fDataType *cov = (fDataType *)src2; /* MxM */
    fDataType *mean = (fDataType *)src3; /* M */

    fDataType float_n = (fDataType) sizeN;

    for (int i = 0; i < sizeM; i++)
      for (int j = i; j < sizeM; j++) {

        cov[i*sizeM+j] = 0.0;
        for (int k = 0; k < sizeN; k++)
          cov[i*sizeM+j] += data[k*sizeM+i] * data[k*sizeM+j];

        cov[i*sizeM+j] /= (float_n - 1.0);
        cov[j*sizeM+i] = cov[i*sizeM+j];
      }
 
}


/*

for (int i = 0; i < sizeM; i++)
      for (int j = i; j < sizeM; j++) {
        v(tmp) = 0.0;

        for (int k = 0; k < sizeN; k+=V)
          v(tmp) += data[k:k+V][i] * v(data[k:k+V][j]);

        t = reduce(tmp)
        
        cov[i][j] = t / (float_n - 1.0);
        cov[j][i] = cov[i][j];
      }



*/
#endif
