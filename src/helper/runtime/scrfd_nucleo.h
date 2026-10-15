#pragma once

#include <cstddef>
#include <immintrin.h>
namespace nrsc {
inline void nucleo_8x3(const float* in,std::size_t cs,const int* toff,int nt,int nci,const float* w,const float* b,
    float* out,std::size_t pso,const float* res,bool relu,const __mmask16* mk){
    __m512 a00=_mm512_set1_ps(b[0]);__m512 a01=a00;__m512 a02=a00;
    __m512 a10=_mm512_set1_ps(b[1]);__m512 a11=a10;__m512 a12=a10;
    __m512 a20=_mm512_set1_ps(b[2]);__m512 a21=a20;__m512 a22=a20;
    __m512 a30=_mm512_set1_ps(b[3]);__m512 a31=a30;__m512 a32=a30;
    __m512 a40=_mm512_set1_ps(b[4]);__m512 a41=a40;__m512 a42=a40;
    __m512 a50=_mm512_set1_ps(b[5]);__m512 a51=a50;__m512 a52=a50;
    __m512 a60=_mm512_set1_ps(b[6]);__m512 a61=a60;__m512 a62=a60;
    __m512 a70=_mm512_set1_ps(b[7]);__m512 a71=a70;__m512 a72=a70;
    for(int c=0;c<nci;++c,in+=cs){
        for(int t=0;t<nt;++t,w+=8){const float* s=in+toff[t];
            const __m512 v0=_mm512_loadu_ps(s+0);const __m512 v1=_mm512_loadu_ps(s+16);const __m512 v2=_mm512_loadu_ps(s+32);
            {const __m512 q=_mm512_set1_ps(w[0]);a00=_mm512_fmadd_ps(q,v0,a00);a01=_mm512_fmadd_ps(q,v1,a01);a02=_mm512_fmadd_ps(q,v2,a02);}
            {const __m512 q=_mm512_set1_ps(w[1]);a10=_mm512_fmadd_ps(q,v0,a10);a11=_mm512_fmadd_ps(q,v1,a11);a12=_mm512_fmadd_ps(q,v2,a12);}
            {const __m512 q=_mm512_set1_ps(w[2]);a20=_mm512_fmadd_ps(q,v0,a20);a21=_mm512_fmadd_ps(q,v1,a21);a22=_mm512_fmadd_ps(q,v2,a22);}
            {const __m512 q=_mm512_set1_ps(w[3]);a30=_mm512_fmadd_ps(q,v0,a30);a31=_mm512_fmadd_ps(q,v1,a31);a32=_mm512_fmadd_ps(q,v2,a32);}
            {const __m512 q=_mm512_set1_ps(w[4]);a40=_mm512_fmadd_ps(q,v0,a40);a41=_mm512_fmadd_ps(q,v1,a41);a42=_mm512_fmadd_ps(q,v2,a42);}
            {const __m512 q=_mm512_set1_ps(w[5]);a50=_mm512_fmadd_ps(q,v0,a50);a51=_mm512_fmadd_ps(q,v1,a51);a52=_mm512_fmadd_ps(q,v2,a52);}
            {const __m512 q=_mm512_set1_ps(w[6]);a60=_mm512_fmadd_ps(q,v0,a60);a61=_mm512_fmadd_ps(q,v1,a61);a62=_mm512_fmadd_ps(q,v2,a62);}
            {const __m512 q=_mm512_set1_ps(w[7]);a70=_mm512_fmadd_ps(q,v0,a70);a71=_mm512_fmadd_ps(q,v1,a71);a72=_mm512_fmadd_ps(q,v2,a72);}
        }}
    const __m512 z=_mm512_setzero_ps();
    auto fim=[&](__m512 x,std::size_t o){if(res)x=_mm512_add_ps(x,_mm512_loadu_ps(res+o));if(relu)x=_mm512_max_ps(x,z);return x;};
    if(!mk){
        _mm512_storeu_ps(out+0*pso+0,fim(a00,0*pso+0));_mm512_storeu_ps(out+0*pso+16,fim(a01,0*pso+16));_mm512_storeu_ps(out+0*pso+32,fim(a02,0*pso+32));
        _mm512_storeu_ps(out+1*pso+0,fim(a10,1*pso+0));_mm512_storeu_ps(out+1*pso+16,fim(a11,1*pso+16));_mm512_storeu_ps(out+1*pso+32,fim(a12,1*pso+32));
        _mm512_storeu_ps(out+2*pso+0,fim(a20,2*pso+0));_mm512_storeu_ps(out+2*pso+16,fim(a21,2*pso+16));_mm512_storeu_ps(out+2*pso+32,fim(a22,2*pso+32));
        _mm512_storeu_ps(out+3*pso+0,fim(a30,3*pso+0));_mm512_storeu_ps(out+3*pso+16,fim(a31,3*pso+16));_mm512_storeu_ps(out+3*pso+32,fim(a32,3*pso+32));
        _mm512_storeu_ps(out+4*pso+0,fim(a40,4*pso+0));_mm512_storeu_ps(out+4*pso+16,fim(a41,4*pso+16));_mm512_storeu_ps(out+4*pso+32,fim(a42,4*pso+32));
        _mm512_storeu_ps(out+5*pso+0,fim(a50,5*pso+0));_mm512_storeu_ps(out+5*pso+16,fim(a51,5*pso+16));_mm512_storeu_ps(out+5*pso+32,fim(a52,5*pso+32));
        _mm512_storeu_ps(out+6*pso+0,fim(a60,6*pso+0));_mm512_storeu_ps(out+6*pso+16,fim(a61,6*pso+16));_mm512_storeu_ps(out+6*pso+32,fim(a62,6*pso+32));
        _mm512_storeu_ps(out+7*pso+0,fim(a70,7*pso+0));_mm512_storeu_ps(out+7*pso+16,fim(a71,7*pso+16));_mm512_storeu_ps(out+7*pso+32,fim(a72,7*pso+32));
    }else{
        _mm512_mask_storeu_ps(out+0*pso+0,mk[0],fim(a00,0*pso+0));_mm512_mask_storeu_ps(out+0*pso+16,mk[1],fim(a01,0*pso+16));_mm512_mask_storeu_ps(out+0*pso+32,mk[2],fim(a02,0*pso+32));
        _mm512_mask_storeu_ps(out+1*pso+0,mk[0],fim(a10,1*pso+0));_mm512_mask_storeu_ps(out+1*pso+16,mk[1],fim(a11,1*pso+16));_mm512_mask_storeu_ps(out+1*pso+32,mk[2],fim(a12,1*pso+32));
        _mm512_mask_storeu_ps(out+2*pso+0,mk[0],fim(a20,2*pso+0));_mm512_mask_storeu_ps(out+2*pso+16,mk[1],fim(a21,2*pso+16));_mm512_mask_storeu_ps(out+2*pso+32,mk[2],fim(a22,2*pso+32));
        _mm512_mask_storeu_ps(out+3*pso+0,mk[0],fim(a30,3*pso+0));_mm512_mask_storeu_ps(out+3*pso+16,mk[1],fim(a31,3*pso+16));_mm512_mask_storeu_ps(out+3*pso+32,mk[2],fim(a32,3*pso+32));
        _mm512_mask_storeu_ps(out+4*pso+0,mk[0],fim(a40,4*pso+0));_mm512_mask_storeu_ps(out+4*pso+16,mk[1],fim(a41,4*pso+16));_mm512_mask_storeu_ps(out+4*pso+32,mk[2],fim(a42,4*pso+32));
        _mm512_mask_storeu_ps(out+5*pso+0,mk[0],fim(a50,5*pso+0));_mm512_mask_storeu_ps(out+5*pso+16,mk[1],fim(a51,5*pso+16));_mm512_mask_storeu_ps(out+5*pso+32,mk[2],fim(a52,5*pso+32));
        _mm512_mask_storeu_ps(out+6*pso+0,mk[0],fim(a60,6*pso+0));_mm512_mask_storeu_ps(out+6*pso+16,mk[1],fim(a61,6*pso+16));_mm512_mask_storeu_ps(out+6*pso+32,mk[2],fim(a62,6*pso+32));
        _mm512_mask_storeu_ps(out+7*pso+0,mk[0],fim(a70,7*pso+0));_mm512_mask_storeu_ps(out+7*pso+16,mk[1],fim(a71,7*pso+16));_mm512_mask_storeu_ps(out+7*pso+32,mk[2],fim(a72,7*pso+32));
    }
}
inline void nucleo_6x4(const float* in,std::size_t cs,const int* toff,int nt,int nci,const float* w,const float* b,
    float* out,std::size_t pso,const float* res,bool relu,const __mmask16* mk){
    __m512 a00=_mm512_set1_ps(b[0]);__m512 a01=a00;__m512 a02=a00;__m512 a03=a00;
    __m512 a10=_mm512_set1_ps(b[1]);__m512 a11=a10;__m512 a12=a10;__m512 a13=a10;
    __m512 a20=_mm512_set1_ps(b[2]);__m512 a21=a20;__m512 a22=a20;__m512 a23=a20;
    __m512 a30=_mm512_set1_ps(b[3]);__m512 a31=a30;__m512 a32=a30;__m512 a33=a30;
    __m512 a40=_mm512_set1_ps(b[4]);__m512 a41=a40;__m512 a42=a40;__m512 a43=a40;
    __m512 a50=_mm512_set1_ps(b[5]);__m512 a51=a50;__m512 a52=a50;__m512 a53=a50;
    for(int c=0;c<nci;++c,in+=cs){
        for(int t=0;t<nt;++t,w+=6){const float* s=in+toff[t];
            const __m512 v0=_mm512_loadu_ps(s+0);const __m512 v1=_mm512_loadu_ps(s+16);const __m512 v2=_mm512_loadu_ps(s+32);const __m512 v3=_mm512_loadu_ps(s+48);
            {const __m512 q=_mm512_set1_ps(w[0]);a00=_mm512_fmadd_ps(q,v0,a00);a01=_mm512_fmadd_ps(q,v1,a01);a02=_mm512_fmadd_ps(q,v2,a02);a03=_mm512_fmadd_ps(q,v3,a03);}
            {const __m512 q=_mm512_set1_ps(w[1]);a10=_mm512_fmadd_ps(q,v0,a10);a11=_mm512_fmadd_ps(q,v1,a11);a12=_mm512_fmadd_ps(q,v2,a12);a13=_mm512_fmadd_ps(q,v3,a13);}
            {const __m512 q=_mm512_set1_ps(w[2]);a20=_mm512_fmadd_ps(q,v0,a20);a21=_mm512_fmadd_ps(q,v1,a21);a22=_mm512_fmadd_ps(q,v2,a22);a23=_mm512_fmadd_ps(q,v3,a23);}
            {const __m512 q=_mm512_set1_ps(w[3]);a30=_mm512_fmadd_ps(q,v0,a30);a31=_mm512_fmadd_ps(q,v1,a31);a32=_mm512_fmadd_ps(q,v2,a32);a33=_mm512_fmadd_ps(q,v3,a33);}
            {const __m512 q=_mm512_set1_ps(w[4]);a40=_mm512_fmadd_ps(q,v0,a40);a41=_mm512_fmadd_ps(q,v1,a41);a42=_mm512_fmadd_ps(q,v2,a42);a43=_mm512_fmadd_ps(q,v3,a43);}
            {const __m512 q=_mm512_set1_ps(w[5]);a50=_mm512_fmadd_ps(q,v0,a50);a51=_mm512_fmadd_ps(q,v1,a51);a52=_mm512_fmadd_ps(q,v2,a52);a53=_mm512_fmadd_ps(q,v3,a53);}
        }}
    const __m512 z=_mm512_setzero_ps();
    auto fim=[&](__m512 x,std::size_t o){if(res)x=_mm512_add_ps(x,_mm512_loadu_ps(res+o));if(relu)x=_mm512_max_ps(x,z);return x;};
    if(!mk){
        _mm512_storeu_ps(out+0*pso+0,fim(a00,0*pso+0));_mm512_storeu_ps(out+0*pso+16,fim(a01,0*pso+16));_mm512_storeu_ps(out+0*pso+32,fim(a02,0*pso+32));_mm512_storeu_ps(out+0*pso+48,fim(a03,0*pso+48));
        _mm512_storeu_ps(out+1*pso+0,fim(a10,1*pso+0));_mm512_storeu_ps(out+1*pso+16,fim(a11,1*pso+16));_mm512_storeu_ps(out+1*pso+32,fim(a12,1*pso+32));_mm512_storeu_ps(out+1*pso+48,fim(a13,1*pso+48));
        _mm512_storeu_ps(out+2*pso+0,fim(a20,2*pso+0));_mm512_storeu_ps(out+2*pso+16,fim(a21,2*pso+16));_mm512_storeu_ps(out+2*pso+32,fim(a22,2*pso+32));_mm512_storeu_ps(out+2*pso+48,fim(a23,2*pso+48));
        _mm512_storeu_ps(out+3*pso+0,fim(a30,3*pso+0));_mm512_storeu_ps(out+3*pso+16,fim(a31,3*pso+16));_mm512_storeu_ps(out+3*pso+32,fim(a32,3*pso+32));_mm512_storeu_ps(out+3*pso+48,fim(a33,3*pso+48));
        _mm512_storeu_ps(out+4*pso+0,fim(a40,4*pso+0));_mm512_storeu_ps(out+4*pso+16,fim(a41,4*pso+16));_mm512_storeu_ps(out+4*pso+32,fim(a42,4*pso+32));_mm512_storeu_ps(out+4*pso+48,fim(a43,4*pso+48));
        _mm512_storeu_ps(out+5*pso+0,fim(a50,5*pso+0));_mm512_storeu_ps(out+5*pso+16,fim(a51,5*pso+16));_mm512_storeu_ps(out+5*pso+32,fim(a52,5*pso+32));_mm512_storeu_ps(out+5*pso+48,fim(a53,5*pso+48));
    }else{
        _mm512_mask_storeu_ps(out+0*pso+0,mk[0],fim(a00,0*pso+0));_mm512_mask_storeu_ps(out+0*pso+16,mk[1],fim(a01,0*pso+16));_mm512_mask_storeu_ps(out+0*pso+32,mk[2],fim(a02,0*pso+32));_mm512_mask_storeu_ps(out+0*pso+48,mk[3],fim(a03,0*pso+48));
        _mm512_mask_storeu_ps(out+1*pso+0,mk[0],fim(a10,1*pso+0));_mm512_mask_storeu_ps(out+1*pso+16,mk[1],fim(a11,1*pso+16));_mm512_mask_storeu_ps(out+1*pso+32,mk[2],fim(a12,1*pso+32));_mm512_mask_storeu_ps(out+1*pso+48,mk[3],fim(a13,1*pso+48));
        _mm512_mask_storeu_ps(out+2*pso+0,mk[0],fim(a20,2*pso+0));_mm512_mask_storeu_ps(out+2*pso+16,mk[1],fim(a21,2*pso+16));_mm512_mask_storeu_ps(out+2*pso+32,mk[2],fim(a22,2*pso+32));_mm512_mask_storeu_ps(out+2*pso+48,mk[3],fim(a23,2*pso+48));
        _mm512_mask_storeu_ps(out+3*pso+0,mk[0],fim(a30,3*pso+0));_mm512_mask_storeu_ps(out+3*pso+16,mk[1],fim(a31,3*pso+16));_mm512_mask_storeu_ps(out+3*pso+32,mk[2],fim(a32,3*pso+32));_mm512_mask_storeu_ps(out+3*pso+48,mk[3],fim(a33,3*pso+48));
        _mm512_mask_storeu_ps(out+4*pso+0,mk[0],fim(a40,4*pso+0));_mm512_mask_storeu_ps(out+4*pso+16,mk[1],fim(a41,4*pso+16));_mm512_mask_storeu_ps(out+4*pso+32,mk[2],fim(a42,4*pso+32));_mm512_mask_storeu_ps(out+4*pso+48,mk[3],fim(a43,4*pso+48));
        _mm512_mask_storeu_ps(out+5*pso+0,mk[0],fim(a50,5*pso+0));_mm512_mask_storeu_ps(out+5*pso+16,mk[1],fim(a51,5*pso+16));_mm512_mask_storeu_ps(out+5*pso+32,mk[2],fim(a52,5*pso+32));_mm512_mask_storeu_ps(out+5*pso+48,mk[3],fim(a53,5*pso+48));
    }
}
}
