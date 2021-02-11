/*BHEADER****************************************************************
 * (c) 2006   The Regents of the University of California               *
 *                                                                      *
 * See the file COPYRIGHT_and_DISCLAIMER for a complete copyright       *
 * notice and disclaimer.                                               *
 *                                                                      *
 *EHEADER****************************************************************/

#include "irsmk.h"
#include "stdio.h"

void
rmatmult3(Domain_t *domain, RadiationData_t *rblk, double *x, double *b) {
    char *me = "rmatmult3";
    int i, ii, jj, kk;
    int imin = domain->imin;
    int imax = domain->imax;
    int jmin = domain->jmin;
    int jmax = domain->jmax;
    int kmin = domain->kmin;
    int kmax = domain->kmax;
    int jp = domain->jp;
    int kp = domain->kp;
    double *dbl = rblk->dbl;
    double *dbc = rblk->dbc;
    double *dbr = rblk->dbr;
    double *dcl = rblk->dcl;
    double *dcc = rblk->dcc;
    double *dcr = rblk->dcr;
    double *dfl = rblk->dfl;
    double *dfc = rblk->dfc;
    double *dfr = rblk->dfr;
    double *cbl = rblk->cbl;
    double *cbc = rblk->cbc;
    double *cbr = rblk->cbr;
    double *ccl = rblk->ccl;
    double *ccc = rblk->ccc;
    double *ccr = rblk->ccr;
    double *cfl = rblk->cfl;
    double *cfc = rblk->cfc;
    double *cfr = rblk->cfr;
    double *ubl = rblk->ubl;
    double *ubc = rblk->ubc;
    double *ubr = rblk->ubr;
    double *ucl = rblk->ucl;
    double *ucc = rblk->ucc;
    double *ucr = rblk->ucr;
    double *ufl = rblk->ufl;
    double *ufc = rblk->ufc;
    double *ufr = rblk->ufr;
    double *xdbl = x - kp - jp - 1;
    double *xdbc = x - kp - jp;
    double *xdbr = x - kp - jp + 1;
    double *xdcl = x - kp - 1;
    double *xdcc = x - kp;
    double *xdcr = x - kp + 1;
    double *xdfl = x - kp + jp - 1;
    double *xdfc = x - kp + jp;
    double *xdfr = x - kp + jp + 1;
    double *xcbl = x - jp - 1;
    double *xcbc = x - jp;
    double *xcbr = x - jp + 1;
    double *xccl = x - 1;
    double *xccc = x;
    double *xccr = x + 1;
    double *xcfl = x + jp - 1;
    double *xcfc = x + jp;
    double *xcfr = x + jp + 1;
    double *xubl = x + kp - jp - 1;
    double *xubc = x + kp - jp;
    double *xubr = x + kp - jp + 1;
    double *xucl = x + kp - 1;
    double *xucc = x + kp;
    double *xucr = x + kp + 1;
    double *xufl = x + kp + jp - 1;
    double *xufc = x + kp + jp;
    double *xufr = x + kp + jp + 1;

#if defined(UVE_COMPILATION)

    double *bi = (double *)malloc(i_ub * sizeof(double));

    unsigned long offset = 0;
    for (kk = kmin; kk < kmax; kk++) {
        offset = (kk * kp + imin);
        // printf("dbl %p\n", dbl);
        // printf("dbl + offset %p\n", dbl+offset);
        // printf("offset %p\n", offset);
        asm volatile(
            "ss.sta.ld.d  u1, %0, %[sz], %[st] \t\n"
            "ss.end       u1, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u2, %1, %[sz], %[st] \t\n"
            "ss.end       u2, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u3, %2, %[sz], %[st] \t\n"
            "ss.end       u3, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u4, %3, %[sz], %[st] \t\n"
            "ss.end       u4, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u5, %4, %[sz], %[st] \t\n"
            "ss.end       u5, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u6, %5, %[sz], %[st] \t\n"
            "ss.end       u6, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u7, %6, %[sz], %[st] \t\n"
            "ss.end       u7, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u8, %7, %[sz], %[st] \t\n"
            "ss.end       u8, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u9, %8, %[sz], %[st] \t\n"
            "ss.end       u9, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u10, %9, %[sz], %[st] \t\n"
            "ss.end       u10, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u11, %10, %[sz], %[st] \t\n"
            "ss.end       u11, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u12, %11, %[sz], %[st] \t\n"
            "ss.end       u12, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u13, %12, %[sz], %[st] \t\n"
            "ss.end       u13, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u14, %13, %[sz], %[st] \t\n"
            "ss.end       u14, %[off2], %[sz2], %[st2] \t\n"
            :
            : "r"(dbl + offset), "r"(dbc + offset), "r"(dbr + offset),
              "r"(dcl + offset), "r"(dcc + offset), "r"(dcr + offset),
              "r"(dfl + offset), "r"(dfc + offset), "r"(dfr + offset),
              "r"(cbl + offset), "r"(cbc + offset), "r"(cbr + offset),
              "r"(ccl + offset), "r"(ccc + offset), [sz] "r"(imax - imin),
              [st] "r"(1), [off2] "r"(jmin), [sz2] "r"(jmax - jmin),
              [st2] "r"(jp));
        asm volatile(
            "ss.sta.ld.d  u15, %0, %[sz], %[st] \t\n"
            "ss.end       u15, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u16, %1, %[sz], %[st] \t\n"
            "ss.end       u16, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u17, %2, %[sz], %[st] \t\n"
            "ss.end       u17, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u18, %3, %[sz], %[st] \t\n"
            "ss.end       u18, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u19, %4, %[sz], %[st] \t\n"
            "ss.end       u19, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u20, %5, %[sz], %[st] \t\n"
            "ss.end       u20, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u21, %6, %[sz], %[st] \t\n"
            "ss.end       u21, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u22, %7, %[sz], %[st] \t\n"
            "ss.end       u22, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u23, %8, %[sz], %[st] \t\n"
            "ss.end       u23, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u24, %9, %[sz], %[st] \t\n"
            "ss.end       u24, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u25, %10, %[sz], %[st] \t\n"
            "ss.end       u25, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u26, %11, %[sz], %[st] \t\n"
            "ss.end       u26, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u27, %12, %[sz], %[st] \t\n"
            "ss.end       u27, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u28, %13, %[sz], %[st] \t\n"
            "ss.end       u28, %[off2], %[sz2], %[st2] \t\n"
            :
            : "r"(xdbl + offset), "r"(xdbc + offset), "r"(xdbr + offset),
              "r"(xdcl + offset), "r"(xdcc + offset), "r"(xdcr + offset),
              "r"(xdfl + offset), "r"(xdfc + offset), "r"(xdfr + offset),
              "r"(xcbl + offset), "r"(xcbc + offset), "r"(xcbr + offset),
              "r"(xccl + offset), "r"(xccc + offset), [sz] "r"(imax - imin),
              [st] "r"(1), [off2] "r"(jmin), [sz2] "r"(jmax - jmin),
              [st2] "r"(jp));

        asm volatile(
            "ss.sta.st.d  u30, %0, %[sz], %[st] \t\n"
            "ss.end       u30, %[off2], %[sz2], %[st2] \t\n"
            :
            : "r"(bi + offset), [sz] "r"(imax - imin), [st] "r"(1),
              [off2] "r"(jmin), [sz2] "r"(jmax - jmin), [st2] "r"(jp));
        asm volatile(
            "LoopA:"
            "so.a.mul.fp u29, u1 , u15, p0\n\t"  // dbl
            "so.a.mul.fp u31, u2 , u16, p0\n\t"  // dbc
            "so.a.mac.fp u29, u3 , u17, p0\n\t"  // dbr
            "so.a.mac.fp u31, u4 , u18, p0\n\t"  // dcl
            "so.a.mac.fp u29, u5 , u19, p0\n\t"  // dcc
            "so.a.mac.fp u31, u6 , u20, p0\n\t"  // dcr
            "so.a.mac.fp u29, u7 , u21, p0\n\t"  // dfl
            "so.a.mac.fp u31, u8 , u22, p0\n\t"  // dfc
            "so.a.mac.fp u29, u9 , u23, p0\n\t"  // dfr
            "so.a.mac.fp u31, u10, u24, p0\n\t"  // cbl
            "so.a.mac.fp u29, u11, u25, p0\n\t"  // cbc
            "so.a.mac.fp u31, u12, u26, p0\n\t"  // cbr
            "so.a.mac.fp u29, u13, u27, p0\n\t"  // ccl
            "so.a.mac.fp u31, u14, u28, p0\n\t"  // ccc
            "so.a.add.fp u30, u29, u31, p0\n\t"  // Save to bi
            "so.b.nc u1, LoopA\n\t");

        //---------------------------------- Down
        // dbl - u1     dcl - u4     dfl - u7
        // dbc - u2     dcc - u5     dfc - u8
        // dbr - u3     dcr - u6     dfr - u9
        //---------------------------------- Center
        // cbl - u10    ccl - u13
        // cbc - u11
        // cbr - u12
        //---------------------------------- XDown
        // xdbl - u14   xdcl - u17    xdfl - u20
        // xdbc - u15   xdcc - u18    xdfc - u21
        // xdbr - u16   xdcr - u19    xdfr - u22
        //---------------------------------- XCenter
        // xcbl - u23   xccl - u26
        // xcbc - u24
        // xcbr - u25

        // Itermediate Result
        // bi - u30

        asm volatile(
            "ss.sta.ld.d  u1, %0, %[sz], %[st] \t\n"
            "ss.end       u1, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u2, %1, %[sz], %[st] \t\n"
            "ss.end       u2, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u3, %2, %[sz], %[st] \t\n"
            "ss.end       u3, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u4, %3, %[sz], %[st] \t\n"
            "ss.end       u4, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u5, %4, %[sz], %[st] \t\n"
            "ss.end       u5, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u6, %5, %[sz], %[st] \t\n"
            "ss.end       u6, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u7, %6, %[sz], %[st] \t\n"
            "ss.end       u7, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u8, %7, %[sz], %[st] \t\n"
            "ss.end       u8, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u9, %8, %[sz], %[st] \t\n"
            "ss.end       u9, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u10, %9, %[sz], %[st] \t\n"
            "ss.end       u10, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u11, %10, %[sz], %[st] \t\n"
            "ss.end       u11, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u12, %11, %[sz], %[st] \t\n"
            "ss.end       u12, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u13, %12, %[sz], %[st] \t\n"
            "ss.end       u13, %[off2], %[sz2], %[st2] \t\n"
            :
            : "r"(ccr + offset), "r"(cfl + offset), "r"(cfc + offset),
              "r"(cfr + offset), "r"(ubl + offset), "r"(ubc + offset),
              "r"(ubr + offset), "r"(ucl + offset), "r"(ucc + offset),
              "r"(ucr + offset), "r"(ufl + offset), "r"(ufc + offset),
              "r"(ufr + offset), [sz] "r"(imax - imin), [st] "r"(1),
              [off2] "r"(jmin), [sz2] "r"(jmax - jmin), [st2] "r"(jp));
        asm volatile(
            "ss.sta.ld.d  u14, %0, %[sz], %[st] \t\n"
            "ss.end       u14, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u15, %1, %[sz], %[st] \t\n"
            "ss.end       u15, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u16, %2, %[sz], %[st] \t\n"
            "ss.end       u16, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u17, %3, %[sz], %[st] \t\n"
            "ss.end       u17, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u18, %4, %[sz], %[st] \t\n"
            "ss.end       u18, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u19, %5, %[sz], %[st] \t\n"
            "ss.end       u19, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u20, %6, %[sz], %[st] \t\n"
            "ss.end       u20, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u21, %7, %[sz], %[st] \t\n"
            "ss.end       u21, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u22, %8, %[sz], %[st] \t\n"
            "ss.end       u22, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u23, %9, %[sz], %[st] \t\n"
            "ss.end       u23, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u24, %10, %[sz], %[st] \t\n"
            "ss.end       u24, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u25, %11, %[sz], %[st] \t\n"
            "ss.end       u25, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u26, %12, %[sz], %[st] \t\n"
            "ss.end       u26, %[off2], %[sz2], %[st2] \t\n"
            :
            : "r"(xccr + offset), "r"(xcfl + offset), "r"(xcfc + offset),
              "r"(xcfr + offset), "r"(xubl + offset), "r"(xubc + offset),
              "r"(xubr + offset), "r"(xucl + offset), "r"(xucc + offset),
              "r"(xucr + offset), "r"(xufl + offset), "r"(xufc + offset),
              "r"(xufr + offset), [sz] "r"(imax - imin), [st] "r"(1),
              [off2] "r"(jmin), [sz2] "r"(jmax - jmin), [st2] "r"(jp));

        asm volatile(
            "ss.sta.st.d  u31, %0, %[sz], %[st] \t\n"
            "ss.end       u31, %[off2], %[sz2], %[st2] \t\n"
            "ss.sta.ld.d  u30, %1, %[sz], %[st] \t\n"
            "ss.end       u30, %[off2], %[sz2], %[st2] \t\n"
            :
            : "r"(b + offset), "r"(bi + offset), [sz] "r"(imax - imin),
              [st] "r"(1), [off2] "r"(jmin), [sz2] "r"(jmax - jmin),
              [st2] "r"(jp));

        asm volatile(
            "LoopB:"
            "so.a.mul.fp u27, u1 , u14, p0\n\t"  // ccc
            "so.a.mul.fp u28, u2 , u15, p0\n\t"  // ccr
            "so.a.mul.fp u29, u3 , u16, p0\n\t"  // cfl
            "so.a.mac.fp u27, u4 , u17, p0\n\t"  // cfc
            "so.a.mac.fp u28, u5 , u18, p0\n\t"  // cfr
            "so.a.mac.fp u29, u6 , u19, p0\n\t"  // ubl
            "so.a.mac.fp u27, u7 , u20, p0\n\t"  // ubc
            "so.a.mac.fp u28, u8 , u21, p0\n\t"  // ubr
            "so.a.mac.fp u29, u9 , u22, p0\n\t"  // ucl
            "so.a.mac.fp u27, u10, u23, p0\n\t"  // ucc
            "so.a.mac.fp u28, u11, u24, p0\n\t"  // ucr
            "so.a.mac.fp u29, u12, u25, p0\n\t"  // ufl
            "so.a.mac.fp u27, u13, u26, p0\n\t"  // ufc

            "so.a.add.fp u28, u28, u27, p0\n\t"
            "so.a.add.fp u29, u29, u28, p0\n\t"
            "so.a.add.fp u31, u30, u29, p0\n\t"  // Save to b; Load from bi
            "so.b.nc u1, LoopB\n\t");
    }

    //---------------------------------- Center
    //                           cfl - u3
    //              ccc - u1     cfc - u4
    //              ccr - u2     cfr - u5
    //---------------------------------- Up
    // ubl - u6     ucl - u9     ufl - u12
    // ubc - u7     ucc - u10    ufc - u13
    // ubr - u8     ucr - u11    ufr - u14
    //---------------------------------- XCenter
    //                           xcfl - u17
    //              xccc - u15   xcfc - u18
    //              xccr - u16   xcfr - u19
    //---------------------------------- XUp
    // xubl - u20   xucl - u23   xufl - u26
    // xubc - u21   xucc - u24   xufc - u27
    // xubr - u22   xucr - u25   xufr - u28

    // Final Result
    // b - u31

#else

    // printf("dbl[min] %p\n", &dbl[kmin * kp + jmin*jp + imin]);
    // printf("dbl %p\n", dbl);
    // printf("min %p\n", kmin * kp + jmin*jp + imin);
    for (kk = kmin; kk < kmax; kk++) {
        for (jj = jmin; jj < jmax; jj++) {
#pragma clang loop vectorize(enable)
            for (ii = imin; ii < imax; ii++) {
                i = ii + jj * jp + kk * kp;
                b[i] = dbl[i] * xdbl[i] + dbc[i] * xdbc[i] + dbr[i] * xdbr[i] +
                       dcl[i] * xdcl[i] + dcc[i] * xdcc[i] + dcr[i] * xdcr[i] +
                       dfl[i] * xdfl[i] + dfc[i] * xdfc[i] + dfr[i] * xdfr[i] +
                       cbl[i] * xcbl[i] + cbc[i] * xcbc[i] + cbr[i] * xcbr[i] +
                       ccl[i] * xccl[i] + ccc[i] * xccc[i] + ccr[i] * xccr[i] +
                       cfl[i] * xcfl[i] + cfc[i] * xcfc[i] + cfr[i] * xcfr[i] +
                       ubl[i] * xubl[i] + ubc[i] * xubc[i] + ubr[i] * xubr[i] +
                       ucl[i] * xucl[i] + ucc[i] * xucc[i] + ucr[i] * xucr[i] +
                       ufl[i] * xufl[i] + ufc[i] * xufc[i] + ufr[i] * xufr[i];
            }
        }
    }

#endif
}
