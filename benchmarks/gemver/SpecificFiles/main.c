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

#define N {{ SN }}
#define calpha {{ ALPHA or 1.5 }}
#define cbeta  {{ BETA  or 1.2 }}
#define cacheSize 65536*4

#define DataType float
#define ResultType double

#if defined (UVE_COMPILATION)
extern ResultType uve_kernel(void* _A, void* _u1, void* _v1, void* _u2, void* _v2, void* _w, void* _x, void* _y, void* _z, uint64_t sizeN, float alpha, float beta);
#else
extern ResultType core_kernel(void* _A, void* _u1, void* _v1, void* _u2, void* _v2, void* _w, void* _x, void* _y, void* _z, uint64_t sizeN, float alpha, float beta);
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
    DataType *src_2 = (DataType*) malloc(sizeof(DataType)*N);
    DataType *src_3 = (DataType*) malloc(sizeof(DataType)*N);
    DataType *src_4 = (DataType*) malloc(sizeof(DataType)*N);
    DataType *src_5 = (DataType*) malloc(sizeof(DataType)*N);
    DataType *src_6 = (DataType*) malloc(sizeof(DataType)*N);
    DataType *src_7 = (DataType*) malloc(sizeof(DataType)*N);
    DataType *src_8 = (DataType*) malloc(sizeof(DataType)*N);
    DataType *src_9 = (DataType*) malloc(sizeof(DataType)*N);
    if (src_1==NULL || src_2==NULL || src_3==NULL ||
        src_4==NULL || src_5==NULL || src_6==NULL ||
        src_7==NULL || src_8==NULL || src_9==NULL)
      {printf("Error allocating memory\n"); return -1;}

    // Benchmark configurations 
    printf("Test info:\n");
    printf("N_ELEMENTS %d\n", N);


    // Fill arrays with datasets
    for(int i = 0; i < N*N; i++){
    src_1[i] = static_src[i%1000];
    }
    for(int i = 0; i < N; i++){
    src_2[i] = static_src[i%1000];
    src_3[i] = static_src[i%1000];
    src_4[i] = static_src[i%1000];
    src_5[i] = static_src[i%1000];
    src_6[i] = static_src[i%1000];
    src_7[i] = static_src[i%1000];
    src_8[i] = static_src[i%1000];
    src_9[i] = static_src[i%1000];
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
    result = uve_kernel(src_1, src_2, src_3, src_4, src_5, src_6, src_7, src_8, src_9, N, calpha, cbeta);
  #else // SVE, ARM, RISC-V, x86 code
    result = core_kernel(src_1, src_2, src_3, src_4, src_5, src_6, src_7, src_8, src_9, N, calpha, cbeta);
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
  if(N < 20){
    for (int i = 0; i < N; i++){
      fprintf(stdout, "\n\t%d: %lf",i, src_6[i]);
    }
  }
  else
  for(int i = 0; i < N; i+=N/20){
    fprintf(stdout, "\n\t%d: %lf",i, src_6[i]);
  }
  fprintf(stdout, "\n");
  return 0;
}
