// Verify signed int16->int32 32x32 matmul. Mode (3<<42)|(1<<26)|(1<<63): int32
// accumulate + X-signed (bit 26) + Y-signed (bit 63). Auto-recovers Z layout.
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../amx.h"
#define ROW(p,r) (((uint64_t)(r)<<56)|(uint64_t)(uintptr_t)(p))
#define MODE ((3ULL<<42)|(1ULL<<26)|(1ULL<<63))
#define T 32
static _Alignas(128) int16_t zero[32];
static void clearZ(void){ for(int r=0;r<64;r++) AMX_LDZ(ROW(zero,r)); }
static int rbase[T],roff[T],col[T];
static void onehot(int xk,int ym,int*orow,int*ocol){
    _Alignas(128) int16_t x[32],y[32]; _Alignas(128) int32_t z[64][16];
    memset(x,0,sizeof x); memset(y,0,sizeof y); x[xk]=1; y[ym]=1;
    AMX_LDX(ROW(x,0)); AMX_LDY(ROW(y,0)); clearZ(); AMX_MATINT(MODE);
    for(int r=0;r<64;r++)AMX_STZ(ROW(z[r],r));
    for(int r=0;r<64;r++)for(int c=0;c<16;c++) if(z[r][c]==1){*orow=r;*ocol=c;return;}
    *orow=-1;*ocol=-1;
}
int main(int ac,char**av){
    int K=ac>1?atoi(av[1]):512;
    memset(zero,0,sizeof zero);
    srandom(99);
    int16_t *A=malloc(2*T*K),*B=malloc(2*K*T);
    for(int i=0;i<T*K;i++)A[i]=(random()%201)-100;
    for(int i=0;i<K*T;i++)B[i]=(random()%201)-100;
    static int32_t ref[T][T];
    for(int i=0;i<T;i++)for(int j=0;j<T;j++){int32_t s=0;for(int p=0;p<K;p++)s+=(int32_t)A[i*K+p]*B[p*T+j];ref[i][j]=s;}
    int16_t *Acol=malloc(2*K*T),*Brow=malloc(2*K*T);
    for(int p=0;p<K;p++)for(int i=0;i<T;i++)Acol[p*T+i]=A[i*K+p];
    for(int p=0;p<K;p++)for(int j=0;j<T;j++)Brow[p*T+j]=B[p*T+j];
    AMX_SET();
    int d;
    for(int i=0;i<T;i++) onehot(0,i,&rbase[i],&d);
    for(int j=0;j<T;j++) onehot(j,0,&roff[j],&col[j]);
    _Alignas(128) int32_t z[64][16];
    clearZ();
    for(int p=0;p<K;p++){ AMX_LDY(ROW(&Acol[p*T],0)); AMX_LDX(ROW(&Brow[p*T],0)); AMX_MATINT(MODE); }
    for(int r=0;r<64;r++)AMX_STZ(ROW(z[r],r));
    AMX_CLR();
    int bad=0;
    for(int i=0;i<T;i++)for(int j=0;j<T;j++){int rr=rbase[i]+roff[j]-roff[0];
        int got=(rr>=0&&rr<64)?z[rr][col[j]]:0xDEAD; if(got!=ref[i][j])bad++;}
    printf("signed 32x32 int16->int32 matmul K=%d: %d/%d mismatch  %s\n",K,bad,T*T,bad==0?"BIT-EXACT":"FAIL");
    printf("  layout: row=2i+(j&1)? rbase[1]=%d roff[1]=%d col[1]=%d  (i.e. C[i][j]=z[%d*i+roff_j][col_j])\n",
        rbase[1],roff[1],col[1], rbase[1]);
    return 0;
}
