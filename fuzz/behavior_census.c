// Billion-operand behavioral census: fingerprint every MATINT operand's output,
// catalog distinct behaviors, surface novel ones. SIGILL-recovering, no fork.
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include "../amx.h"
#define ROW(p,r) (((uint64_t)(r)<<56)|(uint64_t)(uintptr_t)(p))
static _Alignas(128) int16_t x[32],y[32],zero[32];
static _Alignas(128) int32_t z[64][16];
static sigjmp_buf jb; static void onill(int s){(void)s;siglongjmp(jb,1);}
static void clearZ(void){ for(int r=0;r<64;r++) AMX_LDZ(ROW(zero,r)); }
#define CAP (1<<22)
static uint64_t *sigtab; static uint64_t *sigex; static long ndistinct=0;
static int seen(uint64_t s,uint64_t op){    // open addressing insert; 1 if new
    uint64_t h=(s*0x9E3779B97F4A7C15ULL)>>42; 
    for(;;){ h&=(CAP-1); if(sigtab[h]==0){ sigtab[h]=s?s:1; sigex[h]=op; ndistinct++; return 1; } if(sigtab[h]==(s?s:1)) return 0; h++; }
}
static int fp(uint64_t op,uint64_t*sig){
    if(sigsetjmp(jb,1)) return -1;
    memset(x,0,sizeof x); memset(y,0,sizeof y);
    for(int i=0;i<32;i++){ x[i]=i+1; y[i]=(i%5)-2; }   // distinct X, signed Y
    AMX_LDX(ROW(x,0)); AMX_LDY(ROW(y,0)); clearZ(); AMX_MATINT(op);
    for(int r=0;r<64;r++)AMX_STZ(ROW(z[r],r));
    uint64_t s=1469598103934665603ULL; int nz=0; int32_t*zi=(int32_t*)z;
    for(int i=0;i<64*16;i++){ if(zi[i]){ s^=(uint64_t)(uint32_t)zi[i]*0x100000001B3ULL + i; s*=1099511628211ULL; nz++; } }
    *sig = s ^ ((uint64_t)nz<<1);
    return 0;
}
int main(int ac,char**av){
    long N = ac>1? atol(av[1]) : 1000000000L;
    memset(zero,0,sizeof zero);
    sigtab=calloc(CAP,8); sigex=calloc(CAP,8);
    struct sigaction sa; memset(&sa,0,sizeof sa); sa.sa_handler=onill; sigemptyset(&sa.sa_mask);
    sigaction(SIGILL,&sa,0); sigaction(SIGTRAP,&sa,0);
    AMX_SET();
    srandom(0xFACE1234); long ok=0;
    for(long i=0;i<N;i++){
        uint64_t op=((uint64_t)(uint32_t)random())^((uint64_t)(uint32_t)random()<<29)^((uint64_t)(uint32_t)random()<<17);
        op&=~0xFFFFULL; op&=~(1ULL<<63);
        uint64_t sig; if(fp(op,&sig)<0) continue; ok++;
        seen(sig,op);
        if((i%50000000)==0) fprintf(stderr,"  %ldM ops, %ld valid, %ld DISTINCT behaviors\n",i/1000000,ok,ndistinct);
    }
    printf("\n=== census done: %ld ops, %ld valid, %ld distinct behaviors ===\n",N,ok,ndistinct);
    // dump a sample of distinct example operands
    printf("sample distinct-behavior operands:\n"); int sh=0;
    for(long h=0;h<CAP&&sh<60;h++) if(sigtab[h]){ printf(" 0x%llx",(unsigned long long)sigex[h]); if(++sh%6==0)printf("\n"); }
    printf("\n");
    return 0;
}
