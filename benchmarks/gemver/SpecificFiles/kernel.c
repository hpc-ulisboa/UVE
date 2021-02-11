#include <stdint.h>

#if defined (UVE_COMPILATION) //UVE Code
#define v_len 4

static inline
void __config_1(void* A, void* u1, void* v1, void* u2, void* v2, uint64_t N){
    asm volatile(                 /*offset, size, stride*/
        "ss.sta.ld.w  u1, %[A],  %[sn],  %[one] \t\n" //A[i][j]
        "ss.end       u1, zero,  %[sn],  %[sn]  \t\n" 
        "ss.sta.ld.w  u2, %[u1], %[vl],  zero   \t\n" //.u1[i]
        "ss.end       u2, zero,  %[sn],  %[one] \t\n"
        "ss.sta.ld.w  u3, %[v1], %[sn],  %[one] \t\n" //v1*[j]
        "ss.end       u3, zero,  %[sn],  zero   \t\n" 
        "ss.sta.ld.w  u4, %[u2], %[vl],  zero   \t\n" //.u2[i]
        "ss.end       u4, zero,  %[sn],  %[one] \t\n"
        "ss.sta.ld.w  u5, %[v2], %[sn],  %[one] \t\n" //v2*[j]
        "ss.end       u5, zero,  %[sn],  zero   \t\n" 
        "ss.sta.st.w  u30, %[A],  %[sn],  %[one] \t\n" //A[i][j]
        "ss.end       u30, zero,  %[sn],  %[sn]  \t\n" 
        :: [A]"r"(A), [u1]"r"(u1), [v1]"r"(v1), [u2]"r"(u2), [v2]"r"(v2),
           [sn]"r"(N), [one]"r"(1), [vl]"r"(v_len)
        );
}
static inline
void __execute_1(){
    asm volatile(
        "1: \t\n"
          "so.v.mv      u20,u2 ,p0  \n\t" //.u1[i]
          "so.v.mv      u21,u4 ,p0  \n\t" //.u2[i]
        "2: \t\n"
          "so.a.mul.fp  u22,u20, u3,p0  \n\t"  // uv1[i] = .u1[i] + v1[]
          "so.a.mul.fp  u23,u21, u5,p0  \n\t"  // uv2[i] = .u2[i] + v2[]
          "so.a.mul.fp  u24,u22,u23,p0  \n\t"  // uv12[i] = uv1[i] + uv2[i]
          "so.a.add.fp  u30,u1 ,u24,p0  \n\t"  // A[i][] += uv12[i]
        "so.b.ndc.1	u1, 2b \n\t"
        "so.b.nc	u1, 1b \n\t"
    );
}


static inline
void __config_2(void* A, void* x, void* y, uint64_t N, float b){
    asm volatile(                        /*offset, size, stride*/
        "ss.sta.ld.w  u1, %[A],  %[sn],  %[sn] \t\n" //A[i][]
        "ss.end       u1, zero,  %[sn],  %[one]  \t\n" 
        "ss.sta.ld.w  u2, %[y],  %[sn],  %[one] \t\n" //y*[j]
        "ss.end       u2, zero,  %[sn],  zero   \t\n"     
        "ss.sta.ld.w  u3, %[x],  %[vl],  zero   \t\n" //.x[i]
        "ss.end       u3, zero,  %[sn],  %[one] \t\n"   
        "ss.st.w      u30, %[x],  %[sn],  %[one] \t\n" //.x[i]
        "so.v.dp.w    u10, %[cb], p0 \t\n"
        ::  [A]"r"(A), [x]"r"(x), [y]"r"(y),
            [cb]"r"(b),
            [sn]"r"(N), [one]"r"(1), [vl]"r"(v_len)
        );
}
static inline
void __execute_2(){
    asm volatile(
        "1: \t\n"
        "2: \t\n"
          "so.a.mac.fp  u20,u2 ,u1,p0  \n\t" // Ay["][i] += y[] * A[][i]
          "so.b.ndc.1   u1, 2b  \n\t"

        "so.a.mul.fp  u21,u10 ,u20,p0  \n\t" // bAy["][i] = b * Ay["][i]
        "so.a.adde.fp   u22, u21, p0      \n\t" // reduce vector
        "so.a.add.fp  u30,u3  ,u22,p0  \n\t" // .x[i] += bAy[][i]
        "so.b.nc	u1, 1b  \n\t"
    );
}


static inline
void __config_3(void* x, void* z, uint64_t N){
    asm volatile(                        /*offset, size, stride*/
        "ss.ld.w  u1,  %[z],  %[sn],  %[one] \t\n" //z[i]
        "ss.ld.w  u2,  %[x],  %[sn],  %[one] \t\n" //x[i]
        "ss.st.w  u30, %[x],  %[sn],  %[one] \t\n" //x[i]
        ::  [x]"r"(x), [z]"r"(z),
            [sn]"r"(N), [one]"r"(1)
        );
}
static inline
void __execute_3(){
    asm volatile(
        "1: \t\n"
          "so.a.add.fp  u30,u2 ,u1,p0  \n\t" // x[] += z[]
        "so.b.nc	u1, 1b  \n\t"
    );
}


static inline
void __config_4(void* A, void* x, void* w, uint64_t N, float a){
    asm volatile(                        /*offset, size, stride*/
        "ss.sta.ld.w  u1, %[A],  %[sn],  %[one] \t\n" //A[i][]
        "ss.end       u1, zero,  %[sn],  %[sn]  \t\n" 
        "ss.sta.ld.w  u2, %[x],  %[sn],  %[one] \t\n" //y*[j]
        "ss.end       u2, zero,  %[sn],  zero   \t\n"     
        "ss.sta.ld.w  u3, %[w],  %[vl],  zero   \t\n" //.x[i]
        "ss.end       u3, zero,  %[sn],  %[one] \t\n"   
        "ss.st.w      u30, %[w],  %[sn],  %[one] \t\n" //.x[i]
        "so.v.dp.w    u10, %[ca], p0 \t\n"
        ::  [A]"r"(A), [x]"r"(x), [w]"r"(w),
            [ca]"r"(a),
            [sn]"r"(N), [one]"r"(1), [vl]"r"(v_len)
        );
}
static inline
void __execute_4(){
    asm volatile(
        "1: \t\n"
        "2: \t\n"
          "so.a.mac.fp  u20,u2 ,u1,p0  \n\t" // Ax["][i] += x[] * A[i][j]
          "so.b.ndc.1   u1, 2b  \n\t"

        "so.a.mul.fp  u21,u10 ,u20,p0  \n\t" // aAx["][i] = b * Ax[i][""]
        "so.a.adde.fp   u22, u21, p0      \n\t" // reduce vector
        "so.a.add.fp  u30,u3  ,u22,p0  \n\t" // .w[i] += bAx[i][]
        "so.b.nc	u1, 1b  \n\t"
    );
}


void
uve_kernel(void* A, void* u1, void* v1, void* u2, void* v2, void* w, void* x, void* y, void* z, uint64_t N, float a, float b) {

    __config_1(A, u1, v1, u2, v2 ,N);
    __execute_1();
    
    __config_2(A, x, y ,N, b);
    __execute_2();
    
    __config_3(x, z, N);
    __execute_3();

    __config_4(A, x, w, N, a);
    __execute_4();

}

#else //SVE, Arm, RISC-V, x86 code

#define fDataType float
void
core_kernel(void* _A, void* _u1, void* _v1, void* _u2, void* _v2, void* _w, void* _x, void* _y, void* _z, uint64_t sizeN, float alpha, float beta) {
    fDataType *A   = (fDataType *)_A;  /* NxN */
    fDataType *u1  = (fDataType *)_u1; /* N   */
    fDataType *v1  = (fDataType *)_v1; /* N   */
    fDataType *u2  = (fDataType *)_u2; /* N   */
    fDataType *v2  = (fDataType *)_v2; /* N   */
    fDataType *w   = (fDataType *)_w;  /* N   */
    fDataType *x   = (fDataType *)_x;  /* N   */
    fDataType *y   = (fDataType *)_y;  /* N   */
    fDataType *z   = (fDataType *)_z;  /* N   */
    int i,j;
    
    //Simple Vectorization in J
    // A[i][] = A[i][] + .u1 * v1[] + .u2 * v2[]
    for (i = 0; i < sizeN; i++)
        for (j = 0; j < sizeN; j++)
        A[i*sizeN + j] = A[i*sizeN + j] + u1[i] * v1[j] + u2[i] * v2[j];

    //Same as MVT (Matrix Vector mult) -- can be optimized with band processing
    // .x = .x + beta*red(A[][i]*x[])
    for (i = 0; i < sizeN; i++)
        for (j = 0; j < sizeN; j++)
        x[i] = x[i] + beta * A[j*sizeN + i] * y[j];

    //Simple Vectorization ... merge with above?
    // x[] = x[] + z[] 
    for (i = 0; i < sizeN; i++)
        x[i] = x[i] + z[i];

    //Same as MVT (Matrix Vector mult):
    // .w = .w + alpha*red(A[i][]*x[])
    for (i = 0; i < sizeN; i++)
        for (j = 0; j < sizeN; j++)
        w[i] = w[i] + alpha * A[i*sizeN + j] * x[j];
}

#endif
