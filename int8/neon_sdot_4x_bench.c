// The real int8 4x on ARM: NEON SDOT (int8 dot->int32) GEMV vs fp32 NEON GEMV.
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <arm_neon.h>
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
int main(void){
    int M=4096,K=4096;
    float  *W32=aligned_alloc(128,(size_t)M*K*4), *x=aligned_alloc(128,K*4);
    int8_t *W8=aligned_alloc(128,(size_t)M*K), *x8=aligned_alloc(128,K);
    float *y32=malloc(M*4),*y8=malloc(M*4),*sc=malloc(M*4); float sx=0.02f;
    srand(1);
    for(int k=0;k<K;k++){ float v=((rand()%2001)-1000)/1000.0f; x[k]=v; int q=lrintf(v/sx); x8[k]=q>127?127:q<-128?-128:q; }
    for(int i=0;i<M;i++){ sc[i]=0.015f;
        for(int k=0;k<K;k++){ float w=((rand()%2001)-1000)/1000.0f; W32[(size_t)i*K+k]=w; int q=lrintf(w/sc[i]); W8[(size_t)i*K+k]=q>127?127:q<-128?-128:q; } }
    int R=30;
    // fp32 NEON GEMV (4 accumulators)
    double t0=now();
    for(int r=0;r<R;r++) for(int i=0;i<M;i++){ const float*w=&W32[(size_t)i*K];
        float32x4_t a0=vdupq_n_f32(0),a1=a0,a2=a0,a3=a0;
        for(int k=0;k<K;k+=16){ a0=vfmaq_f32(a0,vld1q_f32(w+k),vld1q_f32(x+k));
            a1=vfmaq_f32(a1,vld1q_f32(w+k+4),vld1q_f32(x+k+4));
            a2=vfmaq_f32(a2,vld1q_f32(w+k+8),vld1q_f32(x+k+8));
            a3=vfmaq_f32(a3,vld1q_f32(w+k+12),vld1q_f32(x+k+12)); }
        y32[i]=vaddvq_f32(vaddq_f32(vaddq_f32(a0,a1),vaddq_f32(a2,a3))); }
    double t1=now();
    // int8 SDOT GEMV
    double t2=now();
    for(int r=0;r<R;r++) for(int i=0;i<M;i++){ const int8_t*w=&W8[(size_t)i*K];
        int32x4_t a0=vdupq_n_s32(0),a1=a0,a2=a0,a3=a0;
        for(int k=0;k<K;k+=64){ a0=vdotq_s32(a0,vld1q_s8(w+k),vld1q_s8(x8+k));
            a1=vdotq_s32(a1,vld1q_s8(w+k+16),vld1q_s8(x8+k+16));
            a2=vdotq_s32(a2,vld1q_s8(w+k+32),vld1q_s8(x8+k+32));
            a3=vdotq_s32(a3,vld1q_s8(w+k+48),vld1q_s8(x8+k+48)); }
        int32_t s=vaddvq_s32(vaddq_s32(vaddq_s32(a0,a1),vaddq_s32(a2,a3))); y8[i]=s*sc[i]*sx; }
    double t3=now();
    double err=0,mag=0; for(int i=0;i<M;i++){err+=fabs(y8[i]-y32[i]);mag+=fabs(y32[i]);}
    double tf=t1-t0,ti=t3-t2;
    printf("GEMV %dx%d x%d reps:\n",M,K,R);
    printf("  fp32 NEON : %.3fs  %.1f GFLOP/s  %.1f GB/s\n",tf,(double)M*K*2*R/tf/1e9,(double)M*K*4*R/tf/1e9);
    printf("  int8 SDOT : %.3fs  %.1f GOP/s    %.1f GB/s\n",ti,(double)M*K*2*R/ti/1e9,(double)M*K*1*R/ti/1e9);
    printf("  SPEEDUP: %.2fx   (quant rel-error %.2f%%)\n",tf/ti,100*err/mag);
    return 0;
}
