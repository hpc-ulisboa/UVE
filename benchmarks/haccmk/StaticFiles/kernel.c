//
// Argonne Leadership Computing Facility
// Vitali Morozov morozov@anl.gov
//


#include <math.h>

// #define UVE_COMPILATION

void Step10_orig( int count1, float xxi, float yyi, float zzi, float fsrrmax2, float mp_rsm2, float *xx1, float *yy1, float *zz1, float *mass1, float *dxi, float *dyi, float *dzi )
{

#if defined (UVE_COMPILATION)
    int stride =1;
    //Configure xx1(u1), yy1(u2) and zz1(u3) streams
    asm volatile (
        "ss.ld.w  u1, %[xx1], %[sz], %[st] \t\n"
        "ss.ld.w  u2, %[yy1], %[sz], %[st] \t\n"
        "ss.ld.w  u3, %[zz1], %[sz], %[st] \t\n"
        :
        : [xx1] "r" (xx1), [yy1] "r" (yy1), [zz1] "r" (zz1), [sz] "r" (count1), [st] "r" (stride)
    );

    const float ma0 = 0.269327, ma1 = -0.0750978, ma2 = 0.0114808, ma3 = -0.00109313, ma4 = 0.0000605491, ma5 = -0.00000147177;

    float dxc, dyc, dzc, m, r2, f, xi, yi, zi;
    int j;

    xi = 0.; yi = 0.; zi = 0.;

    //Load xxi to u11, yyi to u12, zzi to u13
    asm volatile
    (
        "so.v.dp.w  u11, %[xxi], p0\n\t"
        "so.v.dp.w  u12, %[yyi], p0\n\t"
        "so.v.dp.w  u13, %[zzi], p0\n\t"
        :
        : [xxi] "r" (xxi), [yyi] "r" (yyi), [zzi] "r" (zzi)
    );

    //Load ma* to vectors u2*
    asm volatile
    (
        "so.v.dp.w  u20, %0, p0\n\t"
        "so.v.dp.w  u21, %1, p0\n\t"
        "so.v.dp.w  u22, %2, p0\n\t"
        "so.v.dp.w  u23, %3, p0\n\t"
        "so.v.dp.w  u24, %4, p0\n\t"
        "so.v.dp.w  u25, %5, p0\n\t"
        :
        : "r" (ma0), "r" (ma1), "r" (ma2), "r" (ma3), "r" (ma4), "r" (ma5)
    );

    asm volatile ("STEP_UVE_LOOP:\n\t");
    //dxc (u4) = xx1[j] (u1) - xxi (u11);
    asm volatile
    (
        "so.a.sub.fp u4, u1, u11, p0\n\t"
        "so.a.sub.fp u5, u2, u12, p0\n\t"
        "so.a.sub.fp u6, u3, u13, p0\n\t"
    );

    // r2 (u10) = dxc * dxc + dyc * dyc + dzc * dzc;
    asm volatile
    (
        "so.a.mul.fp u7, u4, u4, p0\n\t"
        "so.a.mul.fp u8, u5, u5, p0\n\t"
        "so.a.mul.fp u9, u6, u6, p0\n\t"
        "so.a.add.fp u10, u7, u8, p0\n\t"
        "so.a.add.fp u10, u10, u9, p0\n\t"
    );

    // m = ( r2 < fsrrmax2 ) ? mass1[j] : 0.0f;

    // pow( r2 + mp_rsm2, -1.5 ) -
    // f = ( ma0 + r2*(ma1 + r2*(ma2 + r2*(ma3 + r2*(ma4 + r2*ma5)))));
        //Calculate (f result in u26)
        //r2*ma5    u10*u25
        //ma4 + @
        //r2 * @
        //ma3 + @
        //r2 * @
        //ma2 + @
        //r2 * @
        //ma1 + @
        //r2 * @
        //ma0 + @

    asm volatile
    (
        "so.a.mul.fp u26, u10, u25, p0\n\t"
        "so.a.add.fp u26, u26, u24, p0\n\t"
        "so.a.mul.fp u26, u26, u10, p0\n\t"
        "so.a.add.fp u26, u26, u23, p0\n\t"
        "so.a.mul.fp u26, u26, u10, p0\n\t"
        "so.a.add.fp u26, u26, u22, p0\n\t"
        "so.a.mul.fp u26, u26, u10, p0\n\t"
        "so.a.add.fp u26, u26, u21, p0\n\t"
        "so.a.mul.fp u26, u26, u10, p0\n\t"
        "so.a.add.fp u26, u26, u20, p0\n\t"
    );


    // f = ( r2 > 0.0f ) ? m * f : 0.0f;

    // xi = xi + f (u26) * dxc (u4);
    asm volatile
    (
        "so.a.mul.fp u30, u26, u4, p0\n\t"
        "so.a.mul.fp u31, u26, u5, p0\n\t"
        "so.a.mul.fp u29, u26, u6, p0\n\t"
        "so.a.adds.fp ft0, u30, p0 \n\t"
        "so.a.adds.fp ft1, u31, p0 \n\t"
        "so.a.adds.fp ft2, u29, p0 \n\t"
        "fcvt.s.d ft0, ft0 \n\t"
        "fcvt.s.d ft1, ft1 \n\t"
        "fcvt.s.d ft2, ft2 \n\t"
        "fadd.s %[xi], %[xi], ft0 \n\t"
        "fadd.s %[yi], %[yi], ft1 \n\t"
        "fadd.s %[zi], %[zi], ft2 \n\t"
        : [xi] "+f" (xi), [yi] "+f" (yi), [zi] "+f" (zi)
        :
        : "ft0", "ft1", "ft2"
    );

    //LOOP
    asm volatile
    (
        "so.b.nc    u1, STEP_UVE_LOOP \n\t"
    );

    *dxi = xi;
    *dyi = yi;
    *dzi = zi;
#else
    const float ma0 = 0.269327, ma1 = -0.0750978, ma2 = 0.0114808, ma3 = -0.00109313, ma4 = 0.0000605491, ma5 = -0.00000147177;

    float dxc, dyc, dzc, m, r2, f, xi, yi, zi;
    int j;

    xi = 0.; yi = 0.; zi = 0.;

    for ( j = 0; j < count1; j++ )
    {
        dxc = xx1[j] - xxi;
        dyc = yy1[j] - yyi;
        dzc = zz1[j] - zzi;

        r2 = dxc * dxc + dyc * dyc + dzc * dzc;

        // m = ( r2 < fsrrmax2 ) ? mass1[j] : 0.0f;

        f = ( ma0 + r2*(ma1 + r2*(ma2 + r2*(ma3 + r2*(ma4 + r2*ma5)))));
        // pow( r2 + mp_rsm2, -1.5 ) -
        // 1/(r2 + mp_rsm2)*(r2 + mp_rsm2)*(r2+mp_rsm2)

        // f = ( r2 > 0.0f ) ? m * f : 0.0f;

        xi = xi + f * dxc;
        yi = yi + f * dyc;
        zi = zi + f * dzc;
    }

    *dxi = xi;
    *dyi = yi;
    *dzi = zi;
#endif
}
