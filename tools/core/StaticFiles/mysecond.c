/* A gettimeofday routine to give access to the wall
   clock timer on most UNIX-like systems.

   This version defines two entry points -- with 
   and without appended underscores, so it *should*
   automagically link with FORTRAN */

// #if !defined (__riscv)
        #include <time.h>
// #endif


double volatile mysecond()
{
/* struct timeval { long        tv_sec;
            long        tv_usec;        };

struct timezone { int   tz_minuteswest;
             int        tz_dsttime;      };     */

#define read_csr(reg) ({ register long __tmp asm("a0"); \
                        asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
                        __tmp; })

#if defined (__riscv)
        // unsigned long cycles;
        // asm volatile ("rdcycle %0" : "=r" (cycles));
        // unsigned long cycles;
        // asm volatile ("rdcycle %0" : "=r" (cycles));
        // asm volatile ("csrr %0, mtime" : "=r" (cycles));
        // return (double)cycles;// * (double)CLOCKS_PER_SEC;
        return ((double)read_csr(cycle)) / ((double)CLOCKS_PER_SEC);
#else
        struct timespec tp;
        clock_gettime(CLOCK_MONOTONIC,&tp);
        return ( (double) tp.tv_sec + (double) tp.tv_nsec * 1.e-9 ) * (double)1000;
#endif
}

double mysecond_() {return mysecond();}

