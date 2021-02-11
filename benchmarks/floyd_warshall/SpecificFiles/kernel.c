#include <stdint.h>

#if defined (UVE_COMPILATION) //UVE Code
void
uve_config(void* src1/*path*/,  uint64_t sizeN) {

    int v_len = 16;
    asm volatile(                        /*offset, size, stride*/
                 // path_ij stream load
                 "ss.sta.ld.w           u1, %[src1], %[vl], %[one] \t\n"    // D1: vector - linear access size V_len
                 "ss.app                u1, zero, %[nv], %[vl] \t\n"     // D2: slide verticaly stride N ('' times)
                 "ss.app                u1, zero, %[sn], %[sn] \t\n"     // D2: slide verticaly stride N ('' times)
                 "ss.end                u1, zero, %[sn], zero \t\n"      // repeat: for each 'k'

                // path_ik stream load 
                 "ss.sta.ld.w           u2, %[src1], %[vl], zero \t\n"      // D1: vector element - replicate 
                 "ss.app                u2, zero, %[sn], %[sn] \t\n"     // D2: slide verticaly stride N access size N
                 "ss.end                u2, zero, %[sn], %[one] \t\n"    // D3: slide horizontaly N times (for k)

                 // path_kj stream load 
                 "ss.sta.ld.w           u3, %[src1], %[vl], %[one] \t\n"    // D1: vector - linear access size V_len
                 "ss.app                u3, zero, %[nv], %[vl] \t\n"     // D2: slide verticaly stride N ('' times)
                 "ss.cfg.vec            u3 \t\n"
                 "ss.app                u3, zero, %[sn], zero \t\n"   // repeat: for each 'i'
                 "ss.end                u3, zero, %[sn], %[sn] \t\n"     // D3: slide verticaly stride N access size N (for k)

                 // path_ij stream store
                 "ss.sta.st.w           u4, %[src1], %[vl], %[one] \t\n"    // D1: vector - linear access size V_len
                 "ss.app                u4, zero, %[nv], %[vl] \t\n"     // D2: slide verticaly stride N ('' times)
                 "ss.app                u4, zero, %[sn], %[sn] \t\n"     // D2: slide verticaly stride N ('' times)
                 "ss.end                u4, zero, %[sn], zero \t\n"      // repeat: for each 'k'

                 :
                 : [src1] "r"(src1), [sn] "r"(sizeN),
                   [one] "r" (1), [vl] "r" (v_len), [nv] "r" (sizeN/v_len) );

    return;
}

void
uve_kernel() {
    asm volatile(
        "fLoop1: \t\n"
        "so.v.mv u5, u2, p0 \n\t"    //load path_ik
         
          "jLoop1: \t\n"
            "so.v.mv u6, u1, p0 \n\t"       //load path_ij
            "so.a.add.fp u7, u5, u3, p0 \n\t" //load and add path_ik + path_kj
            //"so.p.lt.fp p1, u6, u7, p0 \n\t"  // if path_ij < path_ik + path_kj
            //"so.v.mv.fp.w u7, u6, p1 \n\t"    // path_ij <= min(path_ij, path_ik + path_kj)
            "so.a.min.fp u4, u6, u7, p0 \n\t"
            //"so.v.mv.fp.w u4, u7, p0 \n\t"    // store path_ij
          "so.b.ndc.2 u3, jLoop1 \n\t"

        "so.b.nc	u1, fLoop1 \n\t"
        );
}

#else //SVE, Arm, RISC-V, x86 code

#define fDataType float
void
core_kernel(void* src1, uint64_t sizeN) {
    fDataType *path = (fDataType *)src1; /* NxN */
    uint64_t N = sizeN;
  for (int k = 0; k < sizeN; k++)
    for(int i = 0; i < sizeN; i++)
      for (int j = 0; j < sizeN; j++)
        path[i*N+j] = path[i*N+j] < path[i*N+k] + path[k*N+j] ? path[i*N+j] : path[i*N+k] + path[k*N+j];

}

#endif
