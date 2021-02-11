#include <stdint.h>

#if defined (UVE_COMPILATION) //UVE Code
void
uve_config(void* force_x, void* force_y, void* force_z, void* position_x, void* position_y, void* position_z, void* NL,
               uint64_t nAtoms, uint64_t maxNeighbors) {
    asm volatile(                       /*offset, size, stride*/
                 //position_x_i stream 
                 "ss.sta.ld.w              u1, %[pos_x], %[vl], zero \t\n"     // D0: scalar access 
                 "ss.end                   u1, zero, %[na], %[one] \t\n"       // D1: linear access
                 //position_y_i stream 
                 "ss.sta.ld.w              u2, %[pos_y], %[vl], zero \t\n"     // D0: scalar access 
                 "ss.end                   u2, zero, %[na], %[one] \t\n"       // D1: linear access
                 //position_z_i stream 
                 "ss.sta.ld.w              u3, %[pos_z], %[vl], zero \t\n"     // D0: scalar access 
                 "ss.end                   u3, zero, %[na], %[one] \t\n"       // D1: linear access

                 //NL stream
                 "ss.sta.ld.w              u27, %[n_list], %[mn], %[one] \t\n"  // D1: linear access size maxNeighbors
                 "ss.cfg.ind               u27 \t\n"
                 "ss.end                   u27, zero, %[na], %[mn] \t\n"        // D2: new line stride maxNeighbors size nAtoms
                 "ss.sta.ld.w              u28, %[n_list], %[mn], %[one] \t\n"  // D1: linear access size maxNeighbors
                 "ss.cfg.ind               u28 \t\n"
                 "ss.end                   u28, zero, %[na], %[mn] \t\n"        // D2: new line stride maxNeighbors size nAtoms
                 "ss.sta.ld.w              u29, %[n_list], %[mn], %[one] \t\n"  // D1: linear access size maxNeighbors
                 "ss.cfg.ind               u29 \t\n"
                 "ss.end                   u29, zero, %[na], %[mn] \t\n"        // D2: new line stride maxNeighbors size nAtoms

                 //position_x_j stream
                 "ss.sta.ld.w              u5, %[pos_x], %[one], %[one] \t\n"    // D1: linear access size 'unknown'
                 "ss.app                   u5, zero, %[na], zero \t\n"         // Repeat nAtoms times 
                 "ss.app.indl.ofs.add      u5, u27 \t\n"                        // Indirection from stream u4 -> add to base address
                 "ss.end                   u5, zero, zero, zero \t\n"         // Repeat nAtoms times 
                 //position_y_j stream
                 "ss.sta.ld.w              u6, %[pos_y], %[one], %[one] \t\n"    // D1: linear access size 'unknown'
                 "ss.app                   u6, zero, %[na], zero \t\n"         // Repeat nAtoms times 
                 "ss.app.indl.ofs.add      u6, u28 \t\n"                        // Indirection from stream u4 -> add to base address
                 "ss.end                   u6, zero, zero, zero \t\n"         // Repeat nAtoms times 
                 //position_z_j stream
                 "ss.sta.ld.w              u7, %[pos_z], %[one], %[one] \t\n"    // D1: linear access size 'unknown'
                 "ss.app                   u7, zero, %[na], zero \t\n"         // Repeat nAtoms times 
                 "ss.app.indl.ofs.add      u7, u29 \t\n"                        // Indirection from stream u4 -> add to base address
                 "ss.end                   u7, zero, zero, zero \t\n"         // Repeat nAtoms times 

                 //force_x_i stream 
                 "ss.sta.st.w              u8, %[frc_x], %[vl], zero \t\n"     // D0: scalar access 
                 "ss.end                   u8, zero, %[na], %[one] \t\n"       // D1: linear access
                 //force_y_i stream 
                 "ss.sta.st.w              u9, %[frc_y], %[vl], zero \t\n"     // D0: scalar access 
                 "ss.end                   u9, zero, %[na], %[one] \t\n"       // D1: linear access
                 //force_z_i stream 
                 "ss.sta.st.w              u10, %[frc_z], %[vl], zero \t\n"    // D0: scalar access 
                 "ss.end                   u10, zero, %[na], %[one] \t\n"      // D1: linear access

                 :
                 : [pos_x] "r"(position_x), [pos_y] "r"(position_y), [pos_z] "r"(position_z),
                 [frc_x] "r"(force_x), [frc_y] "r"(force_y), [frc_z] "r"(force_z),
                 [n_list] "r"(NL), [vl] "r" (16),
                 [na] "r"(nAtoms), [mn] "r"(maxNeighbors), [one] "r" (1));

    return;
}
void
uve_kernel() {
    asm volatile(
        
        "so.v.dp.w  u17, %[one], p0\n\t"
        "so.v.dp.w  u18, %[lj1], p0\n\t"
        "so.v.dp.w  u19, %[lj2], p0\n\t"

        "iLoop1: \t\n"
          "so.v.mv     u11, u1, p0\n\t"     // Get pos_x[i] 
          "so.v.mv     u12, u2, p0\n\t"     // Get pos_y[i] 
          "so.v.mv     u13, u3, p0\n\t"     // Get pos_z[i]

          "so.v.dp.w  u14, zero, p0\n\t"
          "so.v.dp.w  u15, zero, p0\n\t"
          "so.v.dp.w  u16, zero, p0\n\t"

          "jloop1: \t\n"

            "so.a.sub.fp u11, u5, u11, p0\n\t" //  delx = i_x - j_x;
            "so.a.sub.fp u12, u6, u12, p0\n\t" //  dely = i_y - j_y;
            "so.a.sub.fp u13, u7, u13, p0\n\t" //  delz = i_z - j_z;

            "so.a.mul.fp u20, u11, u11, p0\n\t" // delx*delx
            "so.a.mac.fp u20, u12, u12, p0\n\t" // + dely*dely
            "so.a.mac.fp u20, u13, u13, p0\n\t" // + delz*delz

            "so.a.div.fp u20, u17, u20, p0\n\t" // r2inv = 1.0 / ()

            "so.a.mul.fp u21, u20, u20, p0\n\t" // r2inv*r2inv
            "so.a.mul.fp u21, u20, u21, p0\n\t" // r6inv = ()*r2inv

            "so.a.mul.fp u22, u18, u21, p0\n\t" // lj1*r6inv
            "so.a.sub.fp u22, u22, u19, p0\n\t" // () - lj2
            "so.a.mul.fp u22, u21, u22, p0\n\t" // potential = r6inv*()

            "so.a.mul.fp u20, u20, u22, p0\n\t" // force = r2inv*potential

            "so.a.mac.fp u14, u11, u20, p0\n\t" // fx += delx * force
            "so.a.mac.fp u15, u12, u20, p0\n\t" // fy += dely * force
            "so.a.mac.fp u16, u13, u20, p0\n\t" // fz += delz * force

          "so.b.ndc.1 u1, jloop1 \n\t"
          
          "so.a.adde.fp    u8, u14, p0      \n\t" // reduce fx vector
          "so.a.adde.fp    u9, u15, p0      \n\t" // reduce fy vector
          "so.a.adde.fp   u10, u16, p0      \n\t" // reduce fz vector
        
        "so.b.nc u1, iLoop1 \n\t"
        :
        : [one] "r" (1), [lj1] "r" (1.5),  [lj2] "r" (2.0));
}


#else //SVE, Arm, RISC-V, x86 code

#define lj1           1.5
#define lj2           2.0
#define fDataType float
void
core_kernel(fDataType* force_x,
               fDataType* force_y,
               fDataType* force_z,
               fDataType* position_x,
               fDataType* position_y,
               fDataType* position_z,
               int32_t* NL,
               uint64_t nAtoms,
               uint64_t maxNeighbors)
{
    fDataType delx, dely, delz, r2inv;
    fDataType r6inv, potential, force, j_x, j_y, j_z;
    fDataType i_x, i_y, i_z, fx, fy, fz;

    int32_t i, j, jidx;

loop_i : for (i = 0; i < nAtoms; i++){
             i_x = position_x[i];
            //  i_y = position_y[i];
            //  i_z = position_z[i];
             fx = 0;
            //  fy = 0;
            //  fz = 0;
loop_j : for( j = 0; j < maxNeighbors; j++){
             // Get neighbor
             jidx = NL[i*maxNeighbors + j];
             // Look up x,y,z positions
             j_x = position_x[jidx];
            //  j_y = position_y[jidx];
            //  j_z = position_z[jidx];
             // Calc distance
             delx = i_x - j_x;
            //  dely = i_y - j_y;
            //  delz = i_z - j_z;
             r2inv = 1.0/( delx*delx);// + dely*dely + delz*delz );
             // Assume no cutoff and aways account for all nodes in area
             r6inv = r2inv * r2inv * r2inv;
             potential = r6inv*(lj1*r6inv - lj2);
             // Sum changes in force
             force = r2inv*potential;
             fx += delx * force;
            //  fy += dely * force;
            //  fz += delz * force;
         }
         //Update forces after all neighbors accounted for.
         force_x[i] = fx;
        //  force_y[i] = fy;
        //  force_z[i] = fz;
         //printf("dF=%lf,%lf,%lf\n", fx, fy, fz);
         }
}

#endif