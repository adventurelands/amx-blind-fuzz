// Verified int16 -> int32 matmul on AMX MATINT. C[MxN] = A[MxK] * B[KxN].
// Per p: Y = column A[:,p] (rows i), X = row B[p,:] (cols j); MATINT accumulates
// outer products in int32. Tile is 32x32. Read C[i][j]=Z32[2i+(j&1)][j/2].
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../amx.h"
#define ROW(p,r) (((uint64_t)(r)<<56)|(uint64_t)(uintptr_t)(p))
// Mode: 3<<42 = int32 accumulate; bit 26 = X operand signed; bit 63 = Y operand
// signed. Both sign bits are required: without them MATINT reads signed int16
// inputs as unsigned, polluting the high 16 bits of each int32 lane (off by
// multiples of 65536). Recovered by fuzzing the mode field on a negative-input
// known-answer case (see RESULTS.md, "MATINT signed-mode recovery").
#define I32MODE ((3ULL<<42)|(1ULL<<26)|(1ULL<<63))
#define T 32
static void clearZ(int16_t*zero){ for(int r=0;r<64;r++) AMX_LDZ(ROW(zero,r)); }

// one 32x32 tile, K-deep, int16 in -> int32 out
static void amx_tile(const int16_t*Acol,const int16_t*Brow,int K,int32_t C[T][T]){
    _Alignas(128) int16_t zero[T]={0};
    _Alignas(128) int32_t z[64][16];
    AMX_SET(); clearZ(zero);
    for(int p=0;p<K;p++){
        AMX_LDY(ROW(&Acol[p*T],0));   // A[:,p], 32 int16 contiguous
        AMX_LDX(ROW(&Brow[p*T],0));   // B[p,:], 32 int16 contiguous
        AMX_MATINT(I32MODE);
    }
    for(int r=0;r<64;r++) AMX_STZ(ROW(z[r],r));
    AMX_CLR();
    for(int i=0;i<T;i++) for(int j=0;j<T;j++) C[i][j]=z[2*i+(j&1)][j/2];
}
int main(void){
    int K=512;
    srandom(1234);
    int16_t *A=malloc(sizeof(int16_t)*T*K), *B=malloc(sizeof(int16_t)*K*T);
    for(int i=0;i<T*K;i++) A[i]=(random()%201)-100;   // -100..100
    for(int i=0;i<K*T;i++) B[i]=(random()%201)-100;
    // reference (scalar int32)
    static int32_t ref[T][T];
    for(int i=0;i<T;i++)for(int j=0;j<T;j++){ int32_t s=0; for(int p=0;p<K;p++) s+=(int32_t)A[i*K+p]*B[p*T+j]; ref[i][j]=s; }
    // pack: Acol[p*T+i]=A[i][p],  Brow[p*T+j]=B[p][j]
    int16_t *Acol=malloc(sizeof(int16_t)*K*T), *Brow=malloc(sizeof(int16_t)*K*T);
    for(int p=0;p<K;p++)for(int i=0;i<T;i++) Acol[p*T+i]=A[i*K+p];
    for(int p=0;p<K;p++)for(int j=0;j<T;j++) Brow[p*T+j]=B[p*T+j];
    static int32_t C[T][T];
    amx_tile(Acol,Brow,K,C);
    int bad=0; long checksum=0;
    for(int i=0;i<T;i++)for(int j=0;j<T;j++){ if(C[i][j]!=ref[i][j]) bad++; checksum+=C[i][j]; }
    printf("32x32 int16 matmul, K=%d:  mismatches=%d / %d   checksum=%ld\n",K,bad,T*T,checksum);
    printf("sample C[0][0]=%d (ref %d)   C[5][7]=%d (ref %d)   C[31][31]=%d (ref %d)\n",
        C[0][0],ref[0][0],C[5][7],ref[5][7],C[31][31],ref[31][31]);
    printf(bad==0?"\nBIT-EXACT int16->int32 matmul on AMX. \xe2\x9c\x93\n":"\nMISMATCH\n");
    return 0;
}
