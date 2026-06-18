// Keep data varied: loop {LDX(pattern); GENLUT}. LDX after GENLUT waits on GENLUT's
// X-write (WAW) -> exposes GENLUT latency. Pattern reloaded fresh each iter -> data
// stays = pattern. LDX cost identical across patterns, so timing delta = GENLUT.
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread/qos.h>
#include <mach/mach_time.h>
#include "../amx.h"
#define ROW(p,r) (((uint64_t)(r)<<56)|(uint64_t)(uintptr_t)(p))
static _Alignas(128) uint8_t X[64],Y[64];
static mach_timebase_info_data_t tb;
static double ns(uint64_t t){ return t*(double)tb.numer/tb.denom; }
#define N 3000000
static double meas(void){           // min ns/iter of {LDX(X);GENLUT} over trials
    double best=1e9;
    for(int t=0;t<25;t++){ uint64_t t0=mach_absolute_time();
        for(long i=0;i<N;i++){ AMX_LDX(ROW(X,0)); AMX_GENLUT(0); }
        double v=ns(mach_absolute_time()-t0)/N; if(v<best)best=v; }
    return best;
}
int main(void){
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE,0);
    mach_timebase_info(&tb); srand(3); AMX_SET();
    for(int i=0;i<64;i++)Y[i]=i*7; AMX_LDY(ROW(Y,0));
    memset(X,0,64); meas();              // warmup
    struct{const char*n; void(*f)(void);} ; 
    const char*names[]={"all-0x00","all-0xFF","all-0x3F","ramp 0..63","random","one-hot lane0","half 0/half FF","0x01 (idx 1)"};
    printf("GENLUT data-dependent timing (min ns/iter of LDX+GENLUT):\n");
    double mn=1e9,mx=0; double vals[8];
    for(int p=0;p<8;p++){
        memset(X,0,64);
        if(p==1)memset(X,0xFF,64); else if(p==2)memset(X,0x3F,64);
        else if(p==3)for(int i=0;i<64;i++)X[i]=i;
        else if(p==4)for(int i=0;i<64;i++)X[i]=rand();
        else if(p==5)X[0]=0x3F;
        else if(p==6){for(int i=0;i<32;i++)X[i]=0; for(int i=32;i<64;i++)X[i]=0xFF;}
        else if(p==7)memset(X,1,64);
        double v=meas(); vals[p]=v;
        printf("  %-16s : %.4f ns\n",names[p],v);
        if(v<mn)mn=v; if(v>mx)mx=v;
    }
    printf("spread: %.4f ns (%.2f%%)  %s\n",mx-mn,100*(mx-mn)/mn,
        (mx-mn)/mn>0.015?"<-- DATA-DEPENDENT TIMING (leak candidate!)":"flat = constant-time, no leak");
    AMX_CLR(); return 0;
}
