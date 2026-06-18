#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../amx.h"
#define ROW(p,r) (((uint64_t)(r)<<56)|(uint64_t)(uintptr_t)(p))
#define I32 (3ULL<<42)
static _Alignas(128) int16_t zero[32];
static void clearZ(void){ for(int r=0;r<64;r++) AMX_LDZ(ROW(zero,r)); }
static void run(int SZ,int K,int16_t*A,int16_t*B){
    int32_t ref[8][8];
    for(int i=0;i<SZ;i++)for(int j=0;j<SZ;j++){int s=0;for(int p=0;p<K;p++)s+=A[i*K+p]*B[p*SZ+j];ref[i][j]=s;}
    _Alignas(128) int16_t Xv[32],Yv[32]; _Alignas(128) int32_t z[64][16];
    AMX_SET(); clearZ();
    for(int p=0;p<K;p++){ memset(Xv,0,sizeof Xv);memset(Yv,0,sizeof Yv);
        for(int i=0;i<SZ;i++)Yv[i]=A[i*K+p]; for(int j=0;j<SZ;j++)Xv[j]=B[p*SZ+j];
        AMX_LDY(ROW(Yv,0)); AMX_LDX(ROW(Xv,0)); AMX_MATINT(I32); }
    for(int r=0;r<64;r++)AMX_STZ(ROW(z[r],r));
    AMX_CLR();
    printf("SZ=%d K=%d:\n",SZ,K);
    for(int i=0;i<SZ;i++){for(int j=0;j<SZ;j++){int g=z[2*i+(j&1)][j/2];printf(" %d/%d%s",g,ref[i][j],g==ref[i][j]?"":"!");}printf("\n");}
}
int main(void){
    memset(zero,0,sizeof zero);
    int16_t A1[]={1,2,3, 4,5,6, 7,8,9, 10,11,12}, B1[]={1,0,2,1, 0,1,1,2, 3,1,0,1};
    run(4,3,A1,B1);                       // exactly tiny (worked before)
    int16_t A2[16],B2[16]; for(int i=0;i<16;i++){A2[i]=(i%7)-3;B2[i]=(i%5)-2;}
    run(4,4,A2,B2);                       // size-style K=4
    int16_t A3[12],B3[12]; for(int i=0;i<12;i++){A3[i]=(i%7)-3;B3[i]=(i%5)-2;}
    run(4,3,A3,B3);                       // size-style but K=3
    return 0;
}
