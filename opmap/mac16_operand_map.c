// Empirically decode MAC16's int16 Z-layout with one-hot probes:
// set X[k]=1, Y[m]=1, run MAC16, find which Z[row][col] becomes 1.
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../amx.h"
#define ROW(p,r) (((uint64_t)(r)<<56)|(uint64_t)(uintptr_t)(p))
static _Alignas(128) int16_t x[32], y[32], zero[32];
static _Alignas(128) int16_t z[64][32];

static void clearZ(void){ for(int r=0;r<64;r++) AMX_LDZ(ROW(zero,r)); }

static void probe(int k,int m){
    memset(x,0,sizeof x); memset(y,0,sizeof y);
    x[k]=1; y[m]=1;
    AMX_LDX(ROW(x,0)); AMX_LDY(ROW(y,0));
    clearZ();
    AMX_MAC16(0);
    for(int r=0;r<64;r++) AMX_STZ(ROW(z[r],r));
    printf("X[%2d]=1, Y[%2d]=1  ->",k,m);
    int found=0;
    for(int r=0;r<64;r++) for(int c=0;c<32;c++) if(z[r][c]){
        printf("  Z[row %d][col %d]=%d",r,c,z[r][c]); found++;
    }
    if(!found) printf("  (nothing)");
    printf("\n");
}
int main(void){
    memset(zero,0,sizeof zero);
    AMX_SET();
    probe(0,0); probe(1,0); probe(2,0); probe(0,1); probe(0,2);
    probe(3,0); probe(0,3); probe(5,7); probe(7,5); probe(31,31);
    AMX_CLR();
    return 0;
}
