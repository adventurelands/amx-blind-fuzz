// BIG hunt: 6M operands. One probe reveals X-sign + Y-sign + int32 + footprint.
//   X[0]=-1 X[5]=200 ; Y[0]=1 Y[3]=-200
//   has(-1)   => X signed (X0*Y0 = -1*1)
//   has(-40000) => Y signed AND int32 (X5*Y3 = 200*-200, can't fit int16)
//   fully-signed int32 = has(-1) AND has(-40000)
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
static sigjmp_buf jb;
static void onill(int s){ (void)s; siglongjmp(jb,1); }
static void clearZ(void){ for(int r=0;r<64;r++) AMX_LDZ(ROW(zero,r)); }
// bits: 1=has-1(Xsigned) 2=has-40000(Ysigned+i32). nz via *pnz. ok via return>=0
static int probe(uint64_t op,int*pnz){
    if(sigsetjmp(jb,1)){ return -1; }
    memset(x,0,sizeof x); memset(y,0,sizeof y); x[0]=-1; x[5]=200; y[0]=1; y[3]=-200;
    AMX_LDX(ROW(x,0)); AMX_LDY(ROW(y,0)); clearZ(); AMX_MATINT(op);
    for(int r=0;r<64;r++) AMX_STZ(ROW(z[r],r));
    int n=0,f=0; int32_t*zi=(int32_t*)z;
    for(int i=0;i<64*16;i++){ int32_t v=zi[i]; if(v)n++; if(v==-1)f|=1; if(v==-40000)f|=2; }
    *pnz=n; return f;
}
static int has40k_signed(uint64_t op){ int nz; return probe(op,&nz)==3; }
int main(void){
    memset(zero,0,sizeof zero);
    struct sigaction sa; memset(&sa,0,sizeof sa); sa.sa_handler=onill; sigemptyset(&sa.sa_mask);
    sigaction(SIGILL,&sa,0); sigaction(SIGTRAP,&sa,0);
    AMX_SET();
    long N=6000000; srandom(0xC0DE);
    long fully=0, ysig=0, xsig=0; uint64_t firstfull=0;
    int nzhist[1100]; memset(nzhist,0,sizeof nzhist);
    uint64_t nz_example[1100]; memset(nz_example,0,sizeof nz_example);
    for(long i=0;i<N;i++){
        uint64_t op = ((uint64_t)(uint32_t)random()) ^ ((uint64_t)(uint32_t)random()<<29);
        op &= ~0xFFFFULL;        // clear low 16 (read offsets) so inputs are seen
        op &= ~(1ULL<<63);       // matrix mode
        int nz; int f=probe(op,&nz);
        if(f<0) continue;
        if(f&1) xsig++;
        if(f&2) ysig++;
        if(f==3){ fully++; if(!firstfull) firstfull=op; }
        if(nz>0 && nz<1100 && !nzhist[nz]) nz_example[nz]=op;   // first op for each footprint size
        if(nz>0 && nz<1100) nzhist[nz]++;
        if((i%1000000)==0) fprintf(stderr,"  %ldM... fully=%ld\n",i/1000000,fully);
    }
    printf("\n=== 6M operand hunt ===\n");
    printf("X-signed observed: %ld   Y-signed observed: %ld   FULLY-SIGNED int32: %ld\n",xsig,ysig,fully);
    if(firstfull){
        uint64_t op=firstfull;
        for(int b=0;b<64;b++){ uint64_t t=op&~(1ULL<<b); if((op&(1ULL<<b))&&has40k_signed(t))op=t; }
        printf("FULLY-SIGNED minimized: op=0x%llx  bits:",(unsigned long long)op);
        for(int b=63;b>=0;b--) if(op&(1ULL<<b))printf(" %d",b); printf("\n");
    }
    // footprint catalog (tile structures): show distinct nz sizes
    printf("\ndistinct output footprints (nz -> count, example op):\n");
    int shown=0; for(int n=1;n<1100&&shown<25;n++) if(nzhist[n]){ printf("  nz=%-4d x%-8d op=0x%llx\n",n,nzhist[n],(unsigned long long)nz_example[n]); shown++; }
    AMX_CLR(); return 0;
}
