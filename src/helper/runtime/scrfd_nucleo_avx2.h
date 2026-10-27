#pragma once

#include <cstddef>
#include <immintrin.h>
namespace nrsc {
inline __m256i masc8_avx2(unsigned bits){const __m256i sel=_mm256_setr_epi32(1,2,4,8,16,32,64,128);return _mm256_cmpeq_epi32(_mm256_and_si256(_mm256_set1_epi32(int(bits)),sel),sel);}
__declspec(noinline) inline void sub_4x3_avx2(const float* in,std::size_t cs,const int* toff,int nt,int nci,const float* w,int ws,const float* b,
    float* out,std::size_t pso,const float* res,bool relu,const unsigned* m8){
    __m256 a00=_mm256_set1_ps(b[0]);__m256 a01=a00;__m256 a02=a00;
    __m256 a10=_mm256_set1_ps(b[1]);__m256 a11=a10;__m256 a12=a10;
    __m256 a20=_mm256_set1_ps(b[2]);__m256 a21=a20;__m256 a22=a20;
    __m256 a30=_mm256_set1_ps(b[3]);__m256 a31=a30;__m256 a32=a30;
    for(int c=0;c<nci;++c,in+=cs){const float* wt=w;
        for(int t=0;t<nt;++t,wt+=ws){const float* s=in+toff[t];
            const __m256 v0=_mm256_loadu_ps(s+0);const __m256 v1=_mm256_loadu_ps(s+8);const __m256 v2=_mm256_loadu_ps(s+16);
            {const __m256 q=_mm256_set1_ps(wt[0]);a00=_mm256_fmadd_ps(q,v0,a00);a01=_mm256_fmadd_ps(q,v1,a01);a02=_mm256_fmadd_ps(q,v2,a02);}
            {const __m256 q=_mm256_set1_ps(wt[1]);a10=_mm256_fmadd_ps(q,v0,a10);a11=_mm256_fmadd_ps(q,v1,a11);a12=_mm256_fmadd_ps(q,v2,a12);}
            {const __m256 q=_mm256_set1_ps(wt[2]);a20=_mm256_fmadd_ps(q,v0,a20);a21=_mm256_fmadd_ps(q,v1,a21);a22=_mm256_fmadd_ps(q,v2,a22);}
            {const __m256 q=_mm256_set1_ps(wt[3]);a30=_mm256_fmadd_ps(q,v0,a30);a31=_mm256_fmadd_ps(q,v1,a31);a32=_mm256_fmadd_ps(q,v2,a32);}
        }w+=std::size_t(nt)*std::size_t(ws);}
    const __m256 z=_mm256_setzero_ps();
    auto fim=[&](__m256 x,std::size_t o){if(res)x=_mm256_add_ps(x,_mm256_loadu_ps(res+o));if(relu)x=_mm256_max_ps(x,z);return x;};
    if(!m8){
        _mm256_storeu_ps(out+0*pso+0,fim(a00,0*pso+0));_mm256_storeu_ps(out+0*pso+8,fim(a01,0*pso+8));_mm256_storeu_ps(out+0*pso+16,fim(a02,0*pso+16));
        _mm256_storeu_ps(out+1*pso+0,fim(a10,1*pso+0));_mm256_storeu_ps(out+1*pso+8,fim(a11,1*pso+8));_mm256_storeu_ps(out+1*pso+16,fim(a12,1*pso+16));
        _mm256_storeu_ps(out+2*pso+0,fim(a20,2*pso+0));_mm256_storeu_ps(out+2*pso+8,fim(a21,2*pso+8));_mm256_storeu_ps(out+2*pso+16,fim(a22,2*pso+16));
        _mm256_storeu_ps(out+3*pso+0,fim(a30,3*pso+0));_mm256_storeu_ps(out+3*pso+8,fim(a31,3*pso+8));_mm256_storeu_ps(out+3*pso+16,fim(a32,3*pso+16));
    }else{
        _mm256_maskstore_ps(out+0*pso+0,masc8_avx2(m8[0]),fim(a00,0*pso+0));_mm256_maskstore_ps(out+0*pso+8,masc8_avx2(m8[1]),fim(a01,0*pso+8));_mm256_maskstore_ps(out+0*pso+16,masc8_avx2(m8[2]),fim(a02,0*pso+16));
        _mm256_maskstore_ps(out+1*pso+0,masc8_avx2(m8[0]),fim(a10,1*pso+0));_mm256_maskstore_ps(out+1*pso+8,masc8_avx2(m8[1]),fim(a11,1*pso+8));_mm256_maskstore_ps(out+1*pso+16,masc8_avx2(m8[2]),fim(a12,1*pso+16));
        _mm256_maskstore_ps(out+2*pso+0,masc8_avx2(m8[0]),fim(a20,2*pso+0));_mm256_maskstore_ps(out+2*pso+8,masc8_avx2(m8[1]),fim(a21,2*pso+8));_mm256_maskstore_ps(out+2*pso+16,masc8_avx2(m8[2]),fim(a22,2*pso+16));
        _mm256_maskstore_ps(out+3*pso+0,masc8_avx2(m8[0]),fim(a30,3*pso+0));_mm256_maskstore_ps(out+3*pso+8,masc8_avx2(m8[1]),fim(a31,3*pso+8));_mm256_maskstore_ps(out+3*pso+16,masc8_avx2(m8[2]),fim(a32,3*pso+16));
    }
}

inline void nucleo_8x3_avx2(const float* in,std::size_t cs,const int* toff,int nt,int nci,const float* w,const float* b,
    float* out,std::size_t pso,const float* res,bool relu,const __mmask16* mk){
    unsigned m8[6];if(mk)for(int j=0;j<6;++j)m8[j]=(unsigned(mk[j>>1])>>((j&1)*8))&0xFFu;
    for(int kg=0;kg<8;kg+=4)for(int pg=0;pg<6;pg+=3)
        sub_4x3_avx2(in+8*pg,cs,toff,nt,nci,w+kg,8,b+kg,out+std::size_t(kg)*pso+std::size_t(8*pg),pso,
            res?res+std::size_t(kg)*pso+std::size_t(8*pg):nullptr,relu,mk?m8+pg:nullptr);
}
__declspec(noinline) inline void sub_6x2_avx2(const float* in,std::size_t cs,const int* toff,int nt,int nci,const float* w,int ws,const float* b,
    float* out,std::size_t pso,const float* res,bool relu,const unsigned* m8){
    __m256 a00=_mm256_set1_ps(b[0]);__m256 a01=a00;
    __m256 a10=_mm256_set1_ps(b[1]);__m256 a11=a10;
    __m256 a20=_mm256_set1_ps(b[2]);__m256 a21=a20;
    __m256 a30=_mm256_set1_ps(b[3]);__m256 a31=a30;
    __m256 a40=_mm256_set1_ps(b[4]);__m256 a41=a40;
    __m256 a50=_mm256_set1_ps(b[5]);__m256 a51=a50;
    for(int c=0;c<nci;++c,in+=cs){const float* wt=w;
        for(int t=0;t<nt;++t,wt+=ws){const float* s=in+toff[t];
            const __m256 v0=_mm256_loadu_ps(s+0);const __m256 v1=_mm256_loadu_ps(s+8);
            {const __m256 q=_mm256_set1_ps(wt[0]);a00=_mm256_fmadd_ps(q,v0,a00);a01=_mm256_fmadd_ps(q,v1,a01);}
            {const __m256 q=_mm256_set1_ps(wt[1]);a10=_mm256_fmadd_ps(q,v0,a10);a11=_mm256_fmadd_ps(q,v1,a11);}
            {const __m256 q=_mm256_set1_ps(wt[2]);a20=_mm256_fmadd_ps(q,v0,a20);a21=_mm256_fmadd_ps(q,v1,a21);}
            {const __m256 q=_mm256_set1_ps(wt[3]);a30=_mm256_fmadd_ps(q,v0,a30);a31=_mm256_fmadd_ps(q,v1,a31);}
            {const __m256 q=_mm256_set1_ps(wt[4]);a40=_mm256_fmadd_ps(q,v0,a40);a41=_mm256_fmadd_ps(q,v1,a41);}
            {const __m256 q=_mm256_set1_ps(wt[5]);a50=_mm256_fmadd_ps(q,v0,a50);a51=_mm256_fmadd_ps(q,v1,a51);}
        }w+=std::size_t(nt)*std::size_t(ws);}
    const __m256 z=_mm256_setzero_ps();
    auto fim=[&](__m256 x,std::size_t o){if(res)x=_mm256_add_ps(x,_mm256_loadu_ps(res+o));if(relu)x=_mm256_max_ps(x,z);return x;};
    if(!m8){
        _mm256_storeu_ps(out+0*pso+0,fim(a00,0*pso+0));_mm256_storeu_ps(out+0*pso+8,fim(a01,0*pso+8));
        _mm256_storeu_ps(out+1*pso+0,fim(a10,1*pso+0));_mm256_storeu_ps(out+1*pso+8,fim(a11,1*pso+8));
        _mm256_storeu_ps(out+2*pso+0,fim(a20,2*pso+0));_mm256_storeu_ps(out+2*pso+8,fim(a21,2*pso+8));
        _mm256_storeu_ps(out+3*pso+0,fim(a30,3*pso+0));_mm256_storeu_ps(out+3*pso+8,fim(a31,3*pso+8));
        _mm256_storeu_ps(out+4*pso+0,fim(a40,4*pso+0));_mm256_storeu_ps(out+4*pso+8,fim(a41,4*pso+8));
        _mm256_storeu_ps(out+5*pso+0,fim(a50,5*pso+0));_mm256_storeu_ps(out+5*pso+8,fim(a51,5*pso+8));
    }else{
        _mm256_maskstore_ps(out+0*pso+0,masc8_avx2(m8[0]),fim(a00,0*pso+0));_mm256_maskstore_ps(out+0*pso+8,masc8_avx2(m8[1]),fim(a01,0*pso+8));
        _mm256_maskstore_ps(out+1*pso+0,masc8_avx2(m8[0]),fim(a10,1*pso+0));_mm256_maskstore_ps(out+1*pso+8,masc8_avx2(m8[1]),fim(a11,1*pso+8));
        _mm256_maskstore_ps(out+2*pso+0,masc8_avx2(m8[0]),fim(a20,2*pso+0));_mm256_maskstore_ps(out+2*pso+8,masc8_avx2(m8[1]),fim(a21,2*pso+8));
        _mm256_maskstore_ps(out+3*pso+0,masc8_avx2(m8[0]),fim(a30,3*pso+0));_mm256_maskstore_ps(out+3*pso+8,masc8_avx2(m8[1]),fim(a31,3*pso+8));
        _mm256_maskstore_ps(out+4*pso+0,masc8_avx2(m8[0]),fim(a40,4*pso+0));_mm256_maskstore_ps(out+4*pso+8,masc8_avx2(m8[1]),fim(a41,4*pso+8));
        _mm256_maskstore_ps(out+5*pso+0,masc8_avx2(m8[0]),fim(a50,5*pso+0));_mm256_maskstore_ps(out+5*pso+8,masc8_avx2(m8[1]),fim(a51,5*pso+8));
    }
}

inline void nucleo_6x4_avx2(const float* in,std::size_t cs,const int* toff,int nt,int nci,const float* w,const float* b,
    float* out,std::size_t pso,const float* res,bool relu,const __mmask16* mk){
    unsigned m8[8];if(mk)for(int j=0;j<8;++j)m8[j]=(unsigned(mk[j>>1])>>((j&1)*8))&0xFFu;
    for(int kg=0;kg<6;kg+=6)for(int pg=0;pg<8;pg+=2)
        sub_6x2_avx2(in+8*pg,cs,toff,nt,nci,w+kg,6,b+kg,out+std::size_t(kg)*pso+std::size_t(8*pg),pso,
            res?res+std::size_t(kg)*pso+std::size_t(8*pg):nullptr,relu,mk?m8+pg:nullptr);
}
}
