#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "dataset.h"
#include "m5ops.h"


#ifdef TIMEBASE
extern unsigned long long timebase();
#else
extern double mysecond();
#endif

#define MAX_ITERATION {{ MAX_ITERATION }}
#define cacheSize 65536*4

#define DataType float
#define ResultType double

// Problem Constants
#define nAtoms        256
#define maxNeighbors  16
// LJ coefficients
#define lj1           1.5
#define lj2           2.0

#if defined (UVE_COMPILATION)
extern void uve_config(void* force_x, void* force_y, void* force_z, 
               void* position_x, void* position_y, void* position_z,
               void* NL, uint64_t nxAtoms, uint64_t maxxNeighbors);
extern ResultType uve_kernel();
#else
extern ResultType core_kernel(DataType* force_x, DataType* force_y, DataType* force_z, 
               DataType* position_x, DataType* position_y, DataType* position_z,
               int32_t* NL, uint64_t nxAtoms, uint64_t maxxNeighbors);
#endif

extern double mysecond();

DataType distance(
        DataType position_x[nAtoms],
        DataType position_y[nAtoms],
        DataType position_z[nAtoms],
        int i,
        int j)
{
    DataType delx, dely, delz, r2inv;
    delx = position_x[i] - position_x[j];
    dely = position_y[i] - position_y[j];
    delz = position_z[i] - position_z[j];
    r2inv = delx * delx + dely * dely + delz * delz;
    return r2inv;
}

inline void insertInOrder(DataType currDist[maxNeighbors], 
        int currList[maxNeighbors], 
        int j, 
        DataType distIJ)
{
    int dist, pos, currList_t;
    DataType currMax, currDist_t;
    pos = maxNeighbors - 1;
    currMax = currDist[pos];
    if (distIJ > currMax){
        return;
    }
    for (dist = pos; dist > 0; dist--){
        if (distIJ < currDist[dist]){
            currDist[dist] = currDist[dist - 1];
            currList[dist]  = currList[pos  - 1];
        }
        else{
            break;
        }
        pos--;
    }
    currDist[dist] = distIJ;
    currList[dist]  = j;
}


int populateNeighborList(DataType currDist[maxNeighbors], 
        int currList[maxNeighbors], 
        const int i, 
        int NL[nAtoms][maxNeighbors])
{
    int idx, validPairs, distanceIter, neighborIter;
    idx = 0; validPairs = 0; 
    for (neighborIter = 0; neighborIter < maxNeighbors; neighborIter++){
        NL[i][neighborIter] = currList[neighborIter];
        validPairs++;
    }
    return validPairs;
}

int buildNeighborList(DataType position_x[nAtoms], 
        DataType position_y[nAtoms], 
        DataType position_z[nAtoms], 
        int NL[nAtoms][maxNeighbors]
        )
{
    int totalPairs, i, j, k;
    totalPairs = 0;
    DataType distIJ;
    for (i = 0; i < nAtoms; i++){
        int currList[maxNeighbors];
        DataType currDist[maxNeighbors];
        for(k=0; k<maxNeighbors; k++){
            currList[k] = 0;
            currDist[k] = 999999999;
        }
        for (j = 0; j < maxNeighbors; j++){
            if (i == j){
                continue;
            }
            distIJ = distance(position_x, position_y, position_z, i, j);
            currList[j] = j;
            currDist[j] = distIJ;
        }
        totalPairs += populateNeighborList(currDist, currList, i, NL);
    }
    return totalPairs;
}


int main(){
    
    ResultType result = 0;
    static DataType static_src[]= FpDataset;
    // static DataType static_src[]= IntDataset;
    char cacher[cacheSize], cacher2[cacheSize];
    double t1, t2, t3, t11, t12, t13, t21, t22, t23, elapsed = 0.0;

    
    int i, iter, j, totalPairs;
    iter = 0; 

    srand(8650341L);

    printf("here");

    //Allocate arrays
    DataType * position_x = (DataType *) malloc( nAtoms *sizeof(DataType));
    DataType * position_y = (DataType *) malloc( nAtoms *sizeof(DataType));
    DataType * position_z = (DataType *) malloc( nAtoms *sizeof(DataType));
    DataType * force_x = (DataType *) malloc( nAtoms *sizeof(DataType));
    DataType * force_y = (DataType *) malloc( nAtoms *sizeof(DataType));
    DataType * force_z = (DataType *) malloc( nAtoms *sizeof(DataType));
    int NL[nAtoms][maxNeighbors];
    int * neighborList = (int *) malloc(nAtoms*maxNeighbors*sizeof(int));

    for  (i = 0; i < nAtoms; i++)
    {
        position_x[i] = static_src[i%1000];
        position_y[i] = static_src[i%1000];
        position_z[i] = static_src[i%1000];
        force_x[i] = static_src[i%1000];
        force_y[i] = static_src[i%1000];
        force_z[i] = static_src[i%1000];
    }

    for(i=0; i<nAtoms; i++){
        for(j = 0; j < maxNeighbors; ++j){
            NL[i][j] = 0;
        }
    }

    totalPairs = buildNeighborList(position_x, position_y, position_z, NL);

    for(i=0; i<nAtoms; i++){
        for(j = 0; j < maxNeighbors; ++j)
            neighborList[i*maxNeighbors + j] = NL[i][j];
    }

    printf("Clearing cache (%d)...\n", cacheSize);
    //Clear Cache
    for(int j=0; j<cacheSize; j++){
      cacher[j] = 4;
      cacher2[j] = cacher[j];
    }

    //Function Call
    
    // Application execution:
    printf("Started...\n");
    t1 = mysecond();
    M5resetstats();
  #if defined (UVE_COMPILATION) //UVE code only
    // for(iter = 0; iter < MAX_ITERATION; iter++) {
        uve_config(force_x, force_y, force_z, position_x, position_y, position_z, neighborList, nAtoms, maxNeighbors);
        result = uve_kernel();
    // }
  #else // SVE, ARM, RISC-V, x86 code
    // for(iter = 0; iter < MAX_ITERATION; iter++) {
        result = core_kernel(force_x, force_y, force_z, position_x, position_y, position_z, neighborList, nAtoms, maxNeighbors);
    // }
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
  if(nAtoms < 20){
    for (int i = 0; i < nAtoms; i++){
      fprintf(stdout, "\n\t%d: %lf",i, force_x[i]);
    }
  }
  else
  for(int i = 0; i < nAtoms; i+=nAtoms/20){
    fprintf(stdout, "\n\t%d: %lf",i, force_x[i]);
  }
  fprintf(stdout, "\n");


    return 0;
}