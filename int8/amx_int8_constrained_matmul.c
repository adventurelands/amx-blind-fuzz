// FINISH: bit-exact int8->int32 matmul via (3<<42)|(1<<50), strided packing.
//   Y-lane 2i = A[i][p] ; X-lane 4j = B[p][j] ; C[i][j] = Z[2i][j]
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../amx.h"
#define ROW(p,r) (((uint64_t)(r)<<56)|(uint64_t)(uintptr_t)(p))
#define OP ((3ULL<<42)|(1ULL<<50))
#define T 16
static _Alignas(128) int8_t zero[64];
static void clearZ(void){ for(int r=0;r<64;r++) AMX_LDZ(ROW(zero,r)); }
int main(int ac,char**av){
    int K=ac>1?atoi(av[1]):256;
    memset(zero,0,sizeof zero);
    srandom(7);
    uint8_t *A=malloc(T*K),*B=malloc(K*T);
    for(int i=0;i<T*K;i++)A[i]=random()%200; for(int i=0;i<K*T;i++)B[i]=random()%200;
    static int32_t ref[T][T];
    for(int i=0;i<T;i++)for(int j=0;j<T;j++){int32_t s=0;for(int p=0;p<K;p++)s+=(int32_t)A[i*K+p]*B[p*T+j];ref[i][j]=s;}
    // packed per-p strided buffers
    _Alignas(128) int8_t Xv[64],Yv[64]; _Alignas(128) int32_t z[64][16];
    AMX_SET(); clearZ();
    for(int p=0;p<K;p++){
        memset(Xv,0,sizeof Xv); memset(Yv,0,sizeof Yv);
        for(int i=0;i<T;i++) Yv[2*i]=A[i*K+p];     // Y-lane 2i
        for(int j=0;j<T;j++) Xv[4*j]=B[p*T+j];     // X-lane 4j
        AMX_LDY(ROW(Yv,0)); AMX_LDX(ROW(Xv,0)); AMX_MATINT(OP);
    }
    for(int r=0;r<64;r++)AMX_STZ(ROW(z[r],r));
    AMX_CLR();
    int bad=0; long cs=0;
    for(int i=0;i<T;i++)for(int j=0;j<T;j++){ int g=z[2*i][j]; if(g!=ref[i][j])bad++; cs+=g; }
    printf("int8->int32 %dx%d K=%d matmul via bit-50: %d/%d mismatch  checksum=%ld  %s\n",
        T,T,K,bad,T*T,cs,bad==0?"BIT-EXACT int8 matmul":"FAIL");
    printf("  C[0][0]=%d/%d  C[9][13]=%d/%d  C[15][15]=%d/%d\n",
        z[0][0],ref[0][0], z[18][13],ref[9][13], z[30][15],ref[15][15]);
    return 0;
}
