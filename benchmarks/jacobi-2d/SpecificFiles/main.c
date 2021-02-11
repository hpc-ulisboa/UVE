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

#define N {{ SX }}
#define steps {{STEPS}}
#define cacheSize 65536*4

#define DataType double
#define ResultType double

#if defined (UVE_COMPILATION)
extern void uve_config(void* src1, void*src2, uint64_t sizeN);
extern ResultType uve_kernel();
#else
extern ResultType core_kernel(void* src1, void* src2, uint64_t sizeN);
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
    DataType *src_1 = (DataType*) malloc(sizeof(DataType)*N*N);
    DataType *src_2 = (DataType*) malloc(sizeof(DataType)*N*N);
    if(src_1==NULL || src_2==NULL) {printf("Error allocating memory\n"); return -1;}

    // Benchmark configurations 
    printf("Test info:\n");
    printf("N_ELEMENTS %d\n", N);
    printf("N_ELEMENTS2 %d\n", steps);

    // Fill arrays with datasets
    for(int i = 0; i < N*N; i++){
      src_1[i] = static_src[i%1000];
      src_2[i] = static_src[i%1000];
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
    for (int i = 0 ; i < steps ; i++){
  #if defined (UVE_COMPILATION) //UVE code only
      uve_config(src_1, src_2, N);
      result = uve_kernel();
      uve_config(src_2, src_1, N);
      result = uve_kernel();
  #else // SVE, ARM, RISC-V, x86 code
      result = core_kernel(src_1, src_2, N);
  #endif
    }
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
  if(N*N < 20){
    for (int i = 1; i < N*N-N; i++){
      fprintf(stdout, "\n\t%d: %lf",i, src_1[i]);
    }
  }
  else
  for(int i = 1; i < N*N-N; i+=N*N/20){
    fprintf(stdout, "\n\t%d: %lf",i, src_1[i]);
  }
  fprintf(stdout, "\n");
  return 0;
}
