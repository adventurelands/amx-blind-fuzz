// Does any int8->int32 mode give a 2D outer product (Y spans: Y[0],Y[1] differ)?
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include "../amx.h"
#define ROW(p,r) (((uint64_t)(r)<<56)|(uint64_t)(uintptr_t)(p))
static _Alignas(128) int8_t x[64],y[64]; static _Alignas(128) int16_t zero[32];
static _Alignas(128) int32_t z[64][16];
static sigjmp_buf jb; static void onill(int s){(void)s;siglongjmp(jb,1);}
static void clearZ(void){ for(int r=0;r<64;r++) AMX_LDZ(ROW(zero,r)); }
static int loc(int xk,int ym,uint64_t op){    // first nonzero linear pos, -1 none
    memset(x,0,sizeof x); memset(y,0,sizeof y); x[xk]=1; y[ym]=1;
    AMX_LDX(ROW(x,0)); AMX_LDY(ROW(y,0)); clearZ(); AMX_MATINT(op);
    for(int r=0;r<64;r++)AMX_STZ(ROW(z[r],r));
    int32_t*zi=(int32_t*)z; for(int i=0;i<64*16;i++) if(zi[i]==1) return i; return -1;
}
static int yspans(uint64_t op){       // int8-in AND Y[0]!=Y[1] location (2D)
    if(sigsetjmp(jb,1)) return -1;
    // int8 check: X bytes 2,3 separate, not merged 770
    memset(x,0,sizeof x); memset(y,0,sizeof y); x[0]=2;x[1]=3; y[0]=1;
    AMX_LDX(ROW(x,0)); AMX_LDY(ROW(y,0)); clearZ(); AMX_MATINT(op);
    for(int r=0;r<64;r++)AMX_STZ(ROW(z[r],r));
    int h2=0,h3=0,h770=0; int32_t*zi=(int32_t*)z;
    for(int i=0;i<64*16;i++){if(zi[i]==2)h2=1;if(zi[i]==3)h3=1;if(zi[i]==770)h770=1;}
    if(!(h2&&h3&&!h770)) return 0;
    int a=loc(0,0,op), b=loc(0,1,op);
    return (a>=0 && b>=0 && a!=b)?1:0;
}
int main(void){
    memset(zero,0,sizeof zero);
    struct sigaction sa; memset(&sa,0,sizeof sa); sa.sa_handler=onill; sigemptyset(&sa.sa_mask);
    sigaction(SIGILL,&sa,0); sigaction(SIGTRAP,&sa,0);
    AMX_SET();
    long N=40000000; srandom(0x2D2D); long hits=0; uint64_t first=0;
    for(long i=0;i<N;i++){
        uint64_t op=((uint64_t)(uint32_t)random())^((uint64_t)(uint32_t)random()<<29);
        op&=~0xFFFFULL; op&=~(1ULL<<63);
        if(yspans(op)==1){ hits++; if(!first)first=op; }
        if((i%10000000)==0) fprintf(stderr,"  %ldM int8-2D hits=%ld\n",i/10000000,hits);
    }
    printf("int8 2D-outer-product hits: %ld\n",hits);
    if(first) printf("  example op=0x%llx\n",(unsigned long long)first);
    else printf("  => no symmetric int8 outer product; AMX int8 is a specialized (matrix x vector / packed-K) dataflow\n");
    AMX_CLR(); return 0;
}
