// Find a GENLUT operand that actually transforms data: load ramps into X,Y,Z,
// run GENLUT, see which register changes.
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include "../amx.h"
#define ROW(p,r) (((uint64_t)(r)<<56)|(uint64_t)(uintptr_t)(p))
static _Alignas(128) uint8_t X[64],Y[64]; static _Alignas(128) uint8_t Zr[64];
static sigjmp_buf jb; static void onill(int s){(void)s;siglongjmp(jb,1);}
// load distinct ramps, run GENLUT(op), report what changed in X/Y and Z row0
static int probe(uint64_t op,char*what){
    if(sigsetjmp(jb,1)){ strcpy(what,"SIGILL"); return -1; }
    _Alignas(128) uint8_t xo[64],yo[64],zo[64];
    for(int i=0;i<64;i++){X[i]=i; Y[i]=128+i;}
    AMX_LDX(ROW(X,0)); AMX_LDY(ROW(Y,0));
    for(int i=0;i<64;i++)Zr[i]=200+ (i&15); AMX_LDZ(ROW(Zr,0));
    AMX_GENLUT(op);
    AMX_STX(ROW(xo,0)); AMX_STY(ROW(yo,0)); AMX_STZ(ROW(zo,0));
    int xc=memcmp(xo,X,64)!=0, yc=memcmp(yo,Y,64)!=0;
    int zc=0; for(int i=0;i<64;i++) if(zo[i]!=Zr[i])zc=1;
    what[0]=0;
    if(xc)strcat(what,"X ");  if(yc)strcat(what,"Y ");  if(zc)strcat(what,"Z ");
    if(!xc&&!yc&&!zc)strcpy(what,"(nothing)");
    return xc||yc||zc;
}
int main(void){
    struct sigaction sa; memset(&sa,0,sizeof sa); sa.sa_handler=onill; sigemptyset(&sa.sa_mask);
    sigaction(SIGILL,&sa,0);
    AMX_SET();
    char w[32];
    printf("GENLUT operand sweep -- what changes:\n");
    for(uint64_t op=0;op<64;op++){ int r=probe(op,w); if(r!=0) printf("  op=%2llu : %s\n",(unsigned long long)op,w); }
    // try mode field at high bits (genlut mode often bits 53-57 or 20-26)
    for(int base=20;base<=58;base++)for(int v=1;v<=3;v++){ uint64_t op=(uint64_t)v<<base; int r=probe(op,w);
        if(r>0) printf("  op=(%d<<%d): %s\n",v,base,w); }
    AMX_CLR(); return 0;
}
