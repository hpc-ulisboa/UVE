#include <stdint.h>

#ifdef TIMEBASE
extern unsigned long long timebase();
#else
extern double mysecond();
#endif

#if defined (UVE_COMPILATION) //UVE Code

static inline void __kernel_copy(void * c, void * a, uint64_t N){
    asm volatile(                        /*offset, size, stride*/
        "ss.ld.w  u1,  %[a],  %[sn],  %[one] \t\n" //z[i]
        "ss.st.w  u30, %[c],  %[sn],  %[one] \t\n" //x[i]

        "1: \t\n"
          "so.v.mv  u30,u1 ,p0  \n\t" // c[] = a[]
        "so.b.nc	u1, 1b  \n\t"
        ::  [a]"r"(a), [c]"r"(c),
            [sn]"r"(N), [one]"r"(1)
        );
}


static inline void __kernel_scale(void * b, void * c, uint64_t N, float scalar){
    asm volatile(                        /*offset, size, stride*/
        "ss.ld.w  u1,  %[c],  %[sn],  %[one] \t\n" //z[i]
        "ss.st.w  u30, %[b],  %[sn],  %[one] \t\n" //x[i]
        "so.v.dp.w    u10, %[sc], p0 \t\n"

        "1: \t\n"
          "so.a.mul.fp  u30, u1,u10,p0  \n\t" // b[] = scalar * c[]
        "so.b.nc	u1, 1b  \n\t"
        ::  [c]"r"(c), [b]"r"(b),
            [sc]"r"(scalar),
            [sn]"r"(N), [one]"r"(1)
        );
}


static inline void __kernel_add(void * c, void * a, void * b, uint64_t N){
    asm volatile(                        /*offset, size, stride*/
        "ss.ld.w  u1,  %[a],  %[sn],  %[one] \t\n" //z[i]
        "ss.ld.w  u2,  %[b],  %[sn],  %[one] \t\n" //z[i]
        "ss.st.w  u30, %[c],  %[sn],  %[one] \t\n" //x[i]

        "1: \t\n"
          "so.a.add.fp  u30,u1 ,u2 ,p0  \n\t" // b[] = scalar * c[]
        "so.b.nc	u1, 1b  \n\t"
        ::  [c]"r"(c), [a]"r"(a), [b]"r"(b),
            [sn]"r"(N), [one]"r"(1)
        );
}

static inline void __kernel_triad(void * a, void * b, void * c, uint64_t N, float scalar){
    asm volatile(                        /*offset, size, stride*/
        "ss.ld.w  u1,  %[b],  %[sn],  %[one] \t\n" //z[i]
        "ss.ld.w  u2,  %[c],  %[sn],  %[one] \t\n" //z[i]
        "ss.st.w  u30, %[a],  %[sn],  %[one] \t\n" //x[i]
        "so.v.dp.w    u10, %[sc], p0 \t\n"

        "1: \t\n"
          "so.a.mul.fp  u20,u2 ,u10 ,p0  \n\t" // b[] = scalar * c[]
          "so.a.add.fp  u30,u1 ,u20,p0  \n\t" // b[] = scalar * c[]
        "so.b.nc	u1, 1b  \n\t"
        ::  [c]"r"(c), [a]"r"(a), [b]"r"(b),
            [sc]"r"(scalar),
            [sn]"r"(N), [one]"r"(1)
        );
}


#define fDataType float


void
uve_kernel(void* _a, void* _b, void* _c, uint64_t sizeN, int ntimes, float scalar, double *times[4]) {
    fDataType *a = (fDataType *)_a; /* N */
    fDataType *b = (fDataType *)_b; /* N */
    fDataType *c = (fDataType *)_c; /* N */
    int k, j;
    for (k=0; k<ntimes; k++)
	{
        times[0][k] = mysecond();
        
        __kernel_copy(c, a, sizeN);

        times[0][k] = mysecond() - times[0][k];
        times[1][k] = mysecond();
        
        __kernel_scale(b, c, sizeN, scalar);

        times[1][k] = mysecond() - times[1][k];
        times[2][k] = mysecond();
        
        __kernel_add(c, a, b, sizeN);
        
        times[2][k] = mysecond() - times[2][k];
        times[3][k] = mysecond();

        __kernel_triad(a, b, c, sizeN, scalar);
        
        times[3][k] = mysecond() - times[3][k];
	}
}

#else //SVE, Arm, RISC-V, x86 code

#define fDataType float
void
core_kernel(void* _a, void* _b, void* _c, uint64_t sizeN, int ntimes, float scalar, double *times[4]) {
    fDataType *a = (fDataType *)_a; /* N */
    fDataType *b = (fDataType *)_b; /* N */
    fDataType *c = (fDataType *)_c; /* N */
    int k, j;
    for (k=0; k<ntimes; k++)
	{
        times[0][k] = mysecond();
        
        for (j=0; j<sizeN; j++)
            c[j] = a[j];

        times[0][k] = mysecond() - times[0][k];
        times[1][k] = mysecond();
        
        for (j=0; j<sizeN; j++)
            b[j] = scalar*c[j];

        times[1][k] = mysecond() - times[1][k];
        times[2][k] = mysecond();
        
        for (j=0; j<sizeN; j++)
            c[j] = a[j]+b[j];
        
        times[2][k] = mysecond() - times[2][k];
        times[3][k] = mysecond();

        for (j=0; j<sizeN; j++)
            a[j] = b[j]+scalar*c[j];
        
        times[3][k] = mysecond() - times[3][k];
	}
}

#endif
