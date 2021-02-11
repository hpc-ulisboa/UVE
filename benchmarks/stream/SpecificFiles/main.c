#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include "dataset.h"
#include "m5ops.h"

#ifdef TIMEBASE
extern unsigned long long timebase();
#else
extern double mysecond();
#endif

#define N {{ SN }}        //10'000'000
#define ntimes {{ TIMES }} //10
#define scalar 3.0

#define cacheSize 65536*4

#define DataType float
#define ResultType double

#if defined (UVE_COMPILATION)
extern ResultType uve_kernel(void* src1, void*src2, void*src3, uint64_t sizeN, int antimes, float cscalar, double *timea[4]);
#else
extern ResultType core_kernel(void* src1, void* src2, void* src3, uint64_t sizeN, int antimes, float cscalar, double *timea[4]);
#endif

# ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MAX
# define MAX(x,y) ((x)>(y)?(x):(y))
# endif

static char	*label[4] = {"Copy:      ", "Scale:     ",
    "Add:       ", "Triad:     "};
static double	avgtime[4] = {0}, maxtime[4] = {0},
		mintime[4] = {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX};

static double	bytes[4] = {
    2 * sizeof(DataType) * N,
    2 * sizeof(DataType) * N,
    3 * sizeof(DataType) * N,
    3 * sizeof(DataType) * N
    };

extern double mysecond();

int main( )
{
    ResultType result = 0;
    static DataType static_src[]= FpDataset;
    // static DataType static_src[]= IntDataset;
    char cacher[cacheSize], cacher2[cacheSize];
    double t1, t2, t3, t11, t12, t13, t21, t22, t23, elapsed = 0.0;

    //Allocate arrays
    double *times[4];
    
    for (int i=0; i<4;i++){
      times[i] = (double *) malloc(sizeof(double)*ntimes);
    } 

    DataType *src_1 = (DataType*) malloc(sizeof(DataType)*N);
    DataType *src_2 = (DataType*) malloc(sizeof(DataType)*N);
    DataType *src_3 = (DataType*) malloc(sizeof(DataType)*N);
    if(src_1==NULL || src_2==NULL || src_3==NULL) {printf("Error allocating memory\n"); return -1;}

    // Benchmark configurations 
    printf("Test info:\n");
    printf("N_ELEMENTS %d\n", N);
    printf("N_ELEMENTS2 %d\n", ntimes);


    // Fill arrays with datasets
    for(int i = 0; i < N; i++){
    src_1[i] = static_src[i%1000];
    src_2[i] = static_src[i%1000];
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
    result = uve_kernel(src_1, src_2, src_3, N, ntimes, scalar, times);
  #else // SVE, ARM, RISC-V, x86 code
    result = core_kernel(src_1, src_2, src_3, N, ntimes, scalar, times);
  #endif
    M5resetdumpstats();
    t2 = mysecond();

    t3 = (t2 - t1);
    elapsed = elapsed + t3;
    //End execution

  //Time statistics
  printf("\nelapsed time, s: %18.8lf\n", elapsed);

  for (int k=1; k<ntimes; k++) /* note -- skip first iteration */
    {
    for (int j=0; j<4; j++)
        {
        avgtime[j] = avgtime[j] + times[j][k];
        mintime[j] = MIN(mintime[j], times[j][k]);
        maxtime[j] = MAX(maxtime[j], times[j][k]);
        }
    }

    printf("Function    Best Rate MB/s  Avg time     Min time     Max time\n");
    for (int j=0; j<4; j++) {
      avgtime[j] = avgtime[j]/(double)(ntimes-1);

      printf("%s%12.1f  %11.6f  %11.6f  %11.6f\n", label[j],
          1.0E-06 * bytes[j]/mintime[j],
          avgtime[j],
          mintime[j],
          maxtime[j]);
    }


  //Results printing
  fprintf(stdout, "Results:");
  if(N < 20){
    for (int i = 0; i < N; i++){
      fprintf(stdout, "\n\t%d: %lf",i, src_1[i]);
    }
  }
  else
  for(int i = 0; i < N; i+=N/20){
    fprintf(stdout, "\n\t%d: %lf",i, src_1[i]);
  }
  fprintf(stdout, "\n");
  return 0;
}
