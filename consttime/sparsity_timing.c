// Does AMX timing scale with operand SPARSITY? Sweep the number of zero lanes in X
// (rest = 1.5), Y dense. If time tracks nonzero count, we can read activation
// sparsity -- which-neurons-fired = content -- continuously off the clock.
#include <stdio.h>
#include <stdint.h>
#include <mach/mach_time.h>
#include "../amx.h"
#define ROW(p,r) (((uint64_t)(r)<<56)|(uint64_t)(uintptr_t)(p))
#define SKIP_Z (1ull<<27)
static double TBN,TBD;
static double bench_k(int nz){    // nz zero lanes in X
    _Alignas(128) float x[16],y[16];
    for(int i=0;i<16;i++){ x[i]=(i<nz)?0.0f:1.5f; y[i]=1.5f; }
    AMX_LDX(ROW(x,0)); AMX_LDY(ROW(y,0));
    for(int i=0;i<200000;i++) AMX_FMA32(SKIP_Z);
    double best=1e18;
    for(int rep=0;rep<25;rep++){
        uint64_t a=mach_absolute_time();
        for(int i=0;i<3000000;i++) AMX_FMA32(SKIP_Z);
        uint64_t b=mach_absolute_time(); double ns=(b-a)*TBN/TBD; if(ns<best)best=ns;
    }
    return best/3e6;
}
int main(){
    mach_timebase_info_data_t tb; mach_timebase_info(&tb); TBN=tb.numer; TBD=tb.denom;
    AMX_SET();
    printf("zeros_in_X  ns/op\n");
    double t0=0,t16=0,vals[17];
    for(int nz=0;nz<=16;nz++){ double t=bench_k(nz); vals[nz]=t; if(nz==0)t0=t; if(nz==16)t16=t;
        printf("  %2d/16    %.4f\n", nz, t); }
    AMX_CLR();
    // monotonic? correlation of nz vs time
    double sx=0,sy=0,sxy=0,sxx=0; for(int k=0;k<=16;k++){sx+=k;sy+=vals[k];sxy+=k*vals[k];sxx+=k*k;}
    double n=17, r=(n*sxy-sx*sy)/((n*sxx-sx*sx));
    printf("\ndense(0 zeros)=%.4f  full-sparse(16 zeros)=%.4f  span=%.1f%%\n",t0,t16,100*(t0-t16)/t16);
    printf("slope %.5f ns per zero-lane.  %s\n", r,
        (t0-t16)/t16>0.03 ? "==> timing tracks sparsity. we can READ activation sparsity off the clock."
                          : "==> weak/non-monotonic; need power.");
    return 0;
}
