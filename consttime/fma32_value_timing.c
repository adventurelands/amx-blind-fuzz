// HIGHEST-PRIORITY: is AMX FMA32 timing VALUE-dependent? Denormals are the classic
// FPU slow-path leak. Back-to-back FMA32 accumulate (Z-dependency chain exposes
// per-op latency); load X,Y with value regimes; compare ns/op.
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread/qos.h>
#include <mach/mach_time.h>
#include "../amx.h"
#define ROW(p,r) (((uint64_t)(r)<<56)|(uint64_t)(uintptr_t)(p))
static _Alignas(128) float X[16],Y[16];
static mach_timebase_info_data_t tb;
static double ns(uint64_t t){ return t*(double)tb.numer/tb.denom; }
#define N 3000000
static double chain(void){            // back-to-back FMA32 accumulate, min ns/op
    double best=1e9;
    for(int t=0;t<25;t++){ AMX_LDX(ROW(X,0)); AMX_LDY(ROW(Y,0));
        uint64_t t0=mach_absolute_time();
        for(long i=0;i<N;i++) AMX_FMA32(0);
        double v=ns(mach_absolute_time()-t0)/N; if(v<best)best=v; }
    return best;
}
static void fill(float v){ for(int i=0;i<16;i++){X[i]=v;Y[i]=v;} }
static void fillxy(float a,float b){ for(int i=0;i<16;i++){X[i]=a;Y[i]=b;} }
int main(void){
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE,0);
    mach_timebase_info(&tb); AMX_SET();
    fill(1.0f); chain();              // warmup
    struct{const char*n;float a,b;} T[]={
        {"normal 1.0",        1.0f, 1.0f},
        {"normal small 1e-10",1e-10f,1e-10f},
        {"DENORMAL 1e-40",    1e-40f,1e-40f},
        {"DENORMAL x normal", 1e-40f,1.0f},
        {"zero",              0.0f, 0.0f},
        {"large 1e30",        1e30f,1e30f},
        {"inf (1e30*1e30)",   1e38f,1e38f},
        {"tiny normal FLT_MIN",1.2e-38f,1.2e-38f},
    };
    printf("AMX FMA32 value-dependent timing (min ns/op, back-to-back accumulate):\n");
    double base=0;
    for(int i=0;i<8;i++){ fillxy(T[i].a,T[i].b); double v=chain();
        if(i==0)base=v;
        printf("  %-22s : %.4f ns  (%+.1f%% vs normal)\n",T[i].n,v,100*(v-base)/base); }
    printf("\n=> any regime >> normal = VALUE-DEPENDENT timing leak (esp. denormals)\n");
    AMX_CLR(); return 0;
}
