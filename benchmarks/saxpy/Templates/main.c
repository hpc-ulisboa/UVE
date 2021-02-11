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

#define STRIDE 1

#define N {{ SIZE }}*STRIDE
#define cacheSize 65536*4

#define DataType float
#define ResultType double

#if defined (UVE_COMPILATION)
extern void uve_kernel_word(void * src_1, void * src_2, float A, uint64_t size);
extern ResultType uve_kernel_fp_loop_1();
#else
extern ResultType core_kernel_fp_1(void * src_1, void * src_2, float A, uint64_t size);
#endif

extern double mysecond();

int main( )
{
  ResultType result = 0;
	static DataType static_src[]= FpDataset;
	// static DataType static_src[]= IntDataset;
  char cacher[cacheSize], cacher2[cacheSize];
  DataType *src_1 = (DataType*) malloc(sizeof(DataType)*N);
  DataType *src_2 = (DataType*) malloc(sizeof(DataType)*N);
  DataType A = 5;
  if(src_1==NULL || src_2==NULL) {printf("Error allocating memory\n"); return -1;}
  double t1, t2, t3, elapsed = 0.0;

  printf("Test info:\n");
  printf("N_ELEMENTS %d\n", N);
  printf("CACHE_SIZE %d\n", cacheSize);
  printf("KERNEL_SIZE 1\n");
  printf("PATTERN {{ PATTERN }}\n");

  for(int i = 0; i < N; i++){
    src_1[i] = static_src[i%1000];
    src_2[i] = static_src[i%1000];
  }

    //Clear Cache
    for(int j=0; j<cacheSize; j++){
      cacher[j] = 4;
      cacher2[j] = cacher[j];
    }

    t1 = mysecond();

  M5resetstats();
  #if defined (UVE_COMPILATION)
    uve_kernel_word(src_1, src_2, A, N);
  #endif
  #if defined (UVE_COMPILATION)
    result = uve_kernel_fp_loop_1();
  #else
    result = core_kernel_fp_1(src_1, src_2, A, N);
  #endif
  M5resetdumpstats();

    t2 = mysecond();

    t3 = (t2 - t1);
    elapsed = elapsed + t3;

  printf("\nelapsed time, s: %18.8lf\n", elapsed);
  fprintf(stdout, "Result: %lf\n", result);
  return 0;
}
