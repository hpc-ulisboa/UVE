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

#define N {{ N_ELEMENTS }}
{% if CACHE_SIZE %}
#define cacheSize {{ CACHE_SIZE }}
{% else %}
#define cacheSize 65536*4
{%endif %}
#define DataType int32_t

#if defined (UVE_COMPILATION)
extern void uve_memcpy_word(void * dest, void * src, uint64_t size);
extern void uve_memcpy_loop();
#else
extern void core_memcpy(void * dest, void * src, uint64_t size);
#endif

extern double mysecond();

int main( )
{
	static DataType static_src[]= IntDataset;
  char cacher[cacheSize], cacher2[cacheSize];
  DataType *src = (DataType*) malloc(sizeof(DataType)*N);
  DataType *dest = (DataType*) malloc(sizeof(DataType)*N);
  if(src==NULL) {printf("Error allocating memory\n"); return -1;}
  double t1, t2, t3, elapsed = 0.0;

  printf("Test info:\n");
  printf("N_ELEMENTS {{ N_ELEMENTS }}\n");
  printf("CACHE_SIZE %d\n", cacheSize);

  for(int i = 0; i < N; i++){
    src[i] = static_src[i%(sizeof(static_src)/sizeof(DataType))];
  }

    //Clear Cache
    for(int j=0; j<cacheSize; j++){
      cacher[j] = 4;
      cacher2[j] = cacher[j];
    }

    M5resetstats();
    t1 = mysecond();

  #if defined (UVE_COMPILATION)
    uve_memcpy_word(dest, src, N);
  #endif
  #if defined (UVE_COMPILATION)
    uve_memcpy_loop();
  #else
    core_memcpy(dest, src, N*sizeof(DataType));
  #endif

    t2 = mysecond();
    M5dumpstats();

    t3 = (t2 - t1);
    elapsed = elapsed + t3;

  uint64_t errors = 0;
  printf( "\nelapsed time, s: %18.8lf\n", elapsed);
  for(uint64_t i = 0; i<N; i++){
		if(dest[i] != src[i]){
			errors++;
		}
	}
  fprintf(stdout, "Result: %d\n", errors);
  return 0;
}
