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

#define NStart {{ N_ELEMENTS_START }}
#define N {{ N_ELEMENTS }}
#define COUNT {{ ITER_COUNT }}
{% if CACHE_SIZE %}
#define cacheSize {{ CACHE_SIZE }}
{% else %}
#define cacheSize 65536*4
{%endif %}


extern void Step10_orig( int count1, float xxi, float yyi, float zzi, float fsrrmax2, float mp_rsm2, float *xx1, float *yy1, float *zz1, float *mass1, float *dxi, float *dyi, float *dzi );

extern double mysecond();

int main( )
{
  static float xx[N], yy[N], zz[N], mass[N], vx1[N], vy1[N], vz1[N];
  float fsrrmax2, mp_rsm2, fcoeff, dx1, dy1, dz1;

  char  M1[cacheSize], M2[cacheSize];
  int n, count, i, rank, nprocs;
  unsigned long long tm1, tm2, tm3, tm4, total = 0;
  double t1, t2, t3, elapsed = 0.0, validation, final;

  printf("Test info:\n");
  printf("N_ELEMENTS {{ N_ELEMENTS }}\n");
  printf("CACHE_SIZE %d\n", cacheSize);
  printf("N_ELEMENTS_START {{ N_ELEMENTS_START }}\n");
  printf("ITER_COUNT {{ ITER_COUNT }}\n");

  count = COUNT;

  final = 0.;

  for ( n = NStart; n < N; n = n + 20 ) 
  {
      /* Initial data preparation */
      fcoeff = 0.23f;
      fsrrmax2 = 0.5f;
      mp_rsm2 = 0.03f;
      dx1 = 1.0f/(float)n;
      dy1 = 2.0f/(float)n;
      dz1 = 3.0f/(float)n;
      xx[0] = 0.f;
      yy[0] = 0.f;
      zz[0] = 0.f;
      mass[0] = 2.f;

      for ( i = 1; i < n; i++ )
      {
          xx[i] = xx[i-1] + dx1;
          yy[i] = yy[i-1] + dy1;
          zz[i] = zz[i-1] + dz1;
          mass[i] = (float)i * 0.01f + xx[i];
      }
      for ( i = 0; i < n; i++ )
      {
          vx1[i] = 0.f;
          vy1[i] = 0.f;
          vz1[i] = 0.f;
      }

      /* Data preparation done */

      /* Clean L1 cache */
      for ( i = 0; i < cacheSize; i++ ) M1[i] = 4;
      for ( i = 0; i < cacheSize; i++ ) M2[i] = M1[i];
      // puts( "Running" );

      t1 = mysecond();

      M5resetstats();
      for ( i = 0; i < count; ++i)
      {
        //Start
        Step10_orig( n, xx[i], yy[i], zz[i], fsrrmax2, mp_rsm2, xx, yy, zz, mass, &dx1, &dy1, &dz1 );
    
        vx1[i] = vx1[i] + dx1 * fcoeff;
        vy1[i] = vy1[i] + dy1 * fcoeff;
        vz1[i] = vz1[i] + dz1 * fcoeff;
        //End
      }
      M5resetdumpstats();

      t2 = mysecond();

      validation = 0.;
      for ( i = 0; i < n; i++ )
      {
         validation = validation + ( vx1[i] + vy1[i] + vz1[i] );
      }

      final = final + validation;

      t3 = (t2 - t1);
      elapsed = elapsed + t3;
  }

  printf( "\nelapsed time, s: %18.8lf\n", elapsed);
  fprintf(stdout, "Result: %lf\n", final);
  return 0;
}
