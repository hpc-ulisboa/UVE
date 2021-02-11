#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "dataset.h"
#include "m5ops.h"

#ifdef TIMEBASE
extern unsigned long long timebase();
#else
extern double mysecond();
#endif

#define I {{ NI }}
#define J {{ NJ }}
#define K {{ NK }}
#define L {{ NL }}
#define M {{ NM }}
#define cacheSize 65536*4

#define DataType float
#define ResultType double

#if defined (UVE_COMPILATION)
extern void uve_config(void* src1, void* src2, void* src3, uint64_t sizeI, uint64_t sizeJ, uint64_t sizeK);
extern ResultType uve_kernel();
#else
extern ResultType core_kernel(void* src1, void* src2, void* src3, uint64_t sizeI, uint64_t sizeJ, uint64_t sizeK);
#endif

extern double mysecond();

int main( )
{
    ResultType result = 0;
    static DataType static_src[]= FpDataset;
    // static DataType static_src[]= IntDataset;
    char cacher[cacheSize], cacher2[cacheSize];
    double t1, t2, t3, t11, t12, t13, t21, t22, t23, elapsed = 0.0;

    //Allocate arrays
    DataType *sA = (DataType*) malloc(sizeof(DataType)*I*K);
    DataType *sB = (DataType*) malloc(sizeof(DataType)*K*J);
    DataType *sC = (DataType*) malloc(sizeof(DataType)*J*M);
    DataType *sD = (DataType*) malloc(sizeof(DataType)*M*L);
    DataType *sE = (DataType*) malloc(sizeof(DataType)*I*J);
    DataType *sF = (DataType*) malloc(sizeof(DataType)*J*L);
    DataType *sG = (DataType*) malloc(sizeof(DataType)*I*L);
    if(sA==NULL || sB==NULL || sC==NULL || sD==NULL || sE==NULL || sF==NULL || sG==NULL ) {printf("Error allocating memory\n"); return -1;}

    // Benchmark configurations 
    printf("Test info:\n");
    printf("N_ELEMENTS %d\n",  I);
    printf("N_ELEMENTS2 %d\n", J);
    printf("N_ELEMENTS3 %d\n", K);
    printf("N_ELEMENTS4 %d\n", L);
    printf("N_ELEMENTS5 %d\n", M);


    // Fill arrays with datasets
    for(int i = 0; i < I*K; i++){
      sA[i] = static_src[i%1000];
    }
    for(int i = 0; i < K*J; i++){
      sB[i] = static_src[i%1000];
    }
    for(int i = 0; i < J*M; i++){
      sC[i] = static_src[i%1000];
    }
    for(int i = 0; i < M*L; i++){
      sD[i] = static_src[i%1000];
    }

    printf("Clearing cache (%d)...\n", cacheSize);
    //Clear Cache
    for(int j=0; j<cacheSize; j++){
      cacher[j] = 4;
      cacher2[j] = cacher[j];
    }


    // Application execution:
    printf("Started...\n");
    t1 = mysecond();
    M5resetstats();
  #if defined (UVE_COMPILATION) //UVE code only
    uve_config(sE, sA, sB, I, J, K);
    result = uve_kernel();
    uve_config(sF, sC, sD, J, L, M);
    result = uve_kernel();
    uve_config(sG, sE, sF, I, L, J);
    result = uve_kernel();
  #else // SVE, ARM, RISC-V, x86 code
    result = core_kernel(sE, sA, sB, I, J, K);
    result = core_kernel(sF, sC, sD, J, L, M);
    result = core_kernel(sG, sE, sF, I, L, J);
  #endif
    M5resetdumpstats();
    t2 = mysecond();

    t3 = (t2 - t1);
    elapsed = elapsed + t3;
    //End execution



  //Time statistics
  printf("\nelapsed time, s: %18.8lf\n", elapsed);
  // printf("\npartial time, s: %18.8lf/%18.8lf/%18.8lf\n", t21-t1, t22-t21, t3-t22);

  //Results printing
  fprintf(stdout, "Results:");
  if(I*L < 20){
    for (int i = 0; i < I*L; i++){
      fprintf(stdout, "\n\t%d: %lf",i, sC[i]);
    }
  }
  else
  for(int i = 0; i < I*L; i+=I*L/20){
    fprintf(stdout, "\n\t%d: %lf",i, sC[i]);
  }
  fprintf(stdout, "\n");
  return 0;
}
