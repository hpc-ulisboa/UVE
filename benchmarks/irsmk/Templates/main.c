/*BHEADER****************************************************************
 * (c) 2006   The Regents of the University of California               *
 *                                                                      *
 * See the file COPYRIGHT_and_DISCLAIMER for a complete copyright       *
 * notice and disclaimer.                                               *
 *                                                                      *
 *EHEADER****************************************************************/


//--------------
//  A micro kernel based on IRS
//    http://www.llnl.gov/asci/purple/benchmarks/limited/irs/
//--------------


#include <stdio.h>
#include <stdlib.h>
#include "irsmk.h"
#include "m5ops.h"

extern double mysecond();

void allocMem(RadiationData_t *);
void init(Domain_t *, RadiationData_t *, double *, double *);
void readInput();
void rmatmult3(Domain_t *, RadiationData_t *, double *, double *);

int main()
{
  Domain_t domain;
  Domain_t *domain_ptr = &domain;

  RadiationData_t rblk;
  RadiationData_t *rblk_ptr = &rblk;

  double t0_cpu = 0,
          t1_cpu = 0;

  double *x;
  double *b;

  int i = 0;
  const int noIter = 1;

  printf ("\nSequoia Benchmark Version 1.0\n\n");

  readInput();

  b = (double *)malloc(i_ub*sizeof(double));
  x = (double *)malloc(x_size*sizeof(double));

  allocMem(rblk_ptr);

  init(domain_ptr, rblk_ptr, x, b);
  t0_cpu = mysecond();
  M5resetstats();
  for (i=0; i<noIter; ++i) {
     rmatmult3(domain_ptr, rblk_ptr, x, b);
  }
  M5dumpstats();
  t1_cpu = mysecond();

  printf("***** results \n");  
  for (i=0; i<i_ub; i+=i_ub/50) {
    printf("i = %10d      b[i] = %e \n", i, b[i]);
  }

  printf("\nelapsed time, s: %18.8lf\n", t1_cpu - t0_cpu);

}
