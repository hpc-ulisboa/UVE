
#include <stdint.h>

#if defined (UVE_COMPILATION)
#define v_len 16
void
uve_config(void *src1, void *src2, void* src3, uint64_t sizeN) {
    asm volatile(                       /*offset, size, stride*/ /*mod-> size, disp*/
                 //L(i,j) stream load
                 "ss.sta.ld.w           u1, %[src1], %[sndx], %[one] \t\n"   // D1: linear access (initial size: 0)
                //  "ss.cfg.vec            u1 \t\n"
                 "ss.app                u1, zero, %[snm1], %[sn] \t\n"      // D2: slide verticaly stride N access size N-1
                 "ss.end.mod.siz.inc    u1, %[snm1], %[one] \t\n"           // Modifier->D1: increment D1 size N-1


                 //x(j) stream load
                 "ss.sta.ld.w           u2, %[src3], %[sndx], %[one] \t\n"    // D1: vector - linear access (initial size: 0)
                //  "ss.cfg.vec            u2 \t\n"
                 "ss.app                u2, zero, %[snm1], zero \t\n"   // Repeat N-1 times 
                 "ss.end.mod.siz.inc    u2, %[snm1], %[one] \t\n"          // Modifier->D1: increment D1 size N-1

                 //b stream scalar load (?)
                 "ss.sta.ld.w           u3, %[src2], %[vl], zero \t\n"   // D0: scalar access 
                 "ss.end                u3, zero, %[snm1], %[one] \t\n"    // D1: vector - linear access 

                 //L(i,i) stream scalar load (?)
                 "ss.sta.ld.w               u4, %[src1], %[vl], zero \t\n"   // D0: scalar access 
                 "ss.end                    u4, zero, %[snm1], %[snp1] \t\n"     // D1: diagonal access size N

                 //x stream scalar store (?)
                 "ss.st.w               u5, %[src2], %[snm1], %[one] \t\n"   // D1: vector - linear access 

                 :
                 :[src1] "r"(src1), [src2] "r"(src2), [src3] "r"(src3), 
                 [sn] "r"(sizeN), [snm1] "r"(sizeN-1), [snp1] "r"(sizeN+1), [one] "r" (1),
                 [vl] "r" (v_len), [iv] "r" (sizeN/v_len), [sndx] "r" (sizeN/2));

    return;
}

void
uve_kernel() {
    asm volatile(
        // "so.a.div.fp    u5, u3, u4, p0  \n\t" //  x = b / L

        "fLoop1: \t\n"

          "so.v.dp.w  u6, zero, p0\n\t"
          "jloop1: \t\n"
            "so.a.mul.fp u7, u1, u2, p0\n\t"
            "so.a.sub.fp u6, u7, u6, p0\n\t"
          "so.b.ndc.1 u1, jloop1 \n\t"
          
          "so.a.adde.fp   u7, u6, p0      \n\t" // reduce vector
          "so.a.add.fp    u7, u3, u7, p0  \n\t" //  t = b + red
          "so.a.div.fp    u5, u7, u4, p0  \n\t" //  x = t / L
        
        "so.b.nc	u1, fLoop1 \n\t"
    );
}

#else

#define fDataType float
void
core_kernel(void *src1, void *src2, void* src3, uint64_t sizeN) {
  fDataType *L = (fDataType *)src1;
  fDataType *b = (fDataType *)src2;
  fDataType *x = (fDataType *)src3;

  float sum = 0.0;
  int cur_nnz;

  for (int i = 0; i < sizeN; i++)
    {
      x[i] = (fDataType)b[i];
      #pragma clang loop vectorize(enable)
      for (int j = 0; j < i; j++)
        x[i] -= L[i*sizeN+j] * x[j];

      x[i] = (fDataType)x[i] / (fDataType)L[i*sizeN+i];
    }
}
#endif

/*foreach ii in i
  t_x = b[i]
  foreach jv in j/vlen, j < i
    v16c <= L[ii][jv:jv+vlen]
    v16x <= x[jv:jv+vlen]
    t_v16x <= -= v16c * v16x
  t_x += acc(t_v16x)
  x[i] = t_x / L[i][i]


  
*/