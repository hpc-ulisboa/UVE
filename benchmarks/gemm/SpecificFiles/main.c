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

#define X {{ SX }}
#define Y {{ SY }}
#define Z {{ SZ }}
#define ALPHA 1
#define BETA 1
#define cacheSize 65536*4

#define DataType float
#define ResultType double

#if defined (UVE_COMPILATION)
extern void uve_config(void * src1, void * src2, void * src3, float alpha, float beta, uint64_t sizeI, uint64_t sizeJ, uint64_t sizeK);
extern ResultType uve_kernel(uint64_t sizeK);
#else
extern ResultType core_kernel(void* src1, void* src2, void* src3, float alpha, float beta, uint64_t sizeN, uint64_t sizeM, uint64_t sizeK);
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
    DataType *src_1 = (DataType*) malloc(sizeof(DataType)*X*Z);
    DataType *src_2 = (DataType*) malloc(sizeof(DataType)*Z*Y);
    DataType *src_3 = (DataType*) malloc(sizeof(DataType)*X*Y);
    DataType alpha = ALPHA, beta = BETA;

    if(src_1==NULL || src_2==NULL || src_3==NULL) {printf("Error allocating memory\n"); return -1;}

    // Benchmark configurations 
    printf("Test info:\n");
    printf("N_ELEMENTS %d\n", X);
    printf("N_ELEMENTS2 %d\n", Y);
    printf("N_ELEMENTS3 %d\n", Z);
    printf("ALPHA %d\n", ALPHA);
    printf("BETA %d\n", BETA);
    printf("CACHE_SIZE %d\n", cacheSize);


    // Fill arrays with datasets
    for(int i = 0; i < X*Z; i++){
    src_1[i] = static_src[i%1000];
    }
    for(int i = 0; i < Z*Y; i++){
      src_2[i] = static_src[i%1000];
    }
    for(int i = 0; i < X*Y; i++){
      src_3[i] = static_src[i%1000];
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
    uve_config(src_1, src_2, src_3, alpha, beta, X, Y, Z);
    // t22 = mysecond();
    result = uve_kernel(Z);
  #else // SVE, ARM, RISC-V, x86 code
    result = core_kernel(src_1, src_2, src_3, alpha, beta, X, Y, Z);
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
  if(X*Y < 20){
    for(int i = 0; i < X*Y; i++){
    fprintf(stdout, "\n\t%d: %lf",i, src_3[i]);
  }
  }
  else
  for(int i = 0; i < X*Y; i+=X*Y/20){
    fprintf(stdout, "\n\t%d: %lf",i, src_3[i]);
  }
  fprintf(stdout, "\n");
  return 0;
}
