// YuNet face detector (opencv_zoo, face_detection_yunet_2023mar) -- MIT License
// Copyright (c) 2020 Shiqi Yu <shiqi.yu@gmail.com>
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include <immintrin.h>
#include <intrin.h>
#include "yunet_pesos.h"

namespace nryn {
struct Rosto {float x,y,w,h;float lm[10];float nota;};

inline void tom_bgr(const float* rgb,int w,int h,std::vector<std::uint8_t>& bgr){
    const size_t n=size_t(w)*size_t(h)*3;bgr.assign(n,0);if(!n)return;
    std::vector<float> v(rgb,rgb+n);for(auto& x:v)if(!std::isfinite(x))x=0.f;
    const size_t m=n/2;std::nth_element(v.begin(),v.begin()+std::ptrdiff_t(m),v.end());
    float med=v[m];if(!(n&1u))med=.5f*(med+*std::max_element(v.begin(),v.begin()+std::ptrdiff_t(m)));
    const float k=.18f/(std::max)(1e-6f,med);
    for(size_t i=0;i<size_t(w)*size_t(h);++i)for(int c=0;c<3;++c){
        float x=rgb[i*3+size_t(c)];if(!std::isfinite(x))x=0.f;x*=k;
        x=(x*(2.51f*x+.03f))/(x*(2.43f*x+.59f)+.14f);x=std::clamp(x,0.f,1.f);x=std::pow(x,1.f/2.2f);
        bgr[i*3+size_t(2-c)]=std::uint8_t(std::clamp(std::floor(x*255.f+.5f),0.f,255.f));}
}

template<class Meia> inline void tom_bgr_meia(const std::uint16_t* hb,int w,int h,std::vector<std::uint8_t>& bgr,Meia meia){
    const size_t np=size_t(w)*size_t(h),n=np*3;bgr.assign(n,0);if(!n)return;
    auto val=[&](std::uint16_t c){float x=meia(c);return std::isfinite(x)?x:0.f;};
    static const std::vector<std::uint16_t> ordem=[&]{std::vector<std::uint16_t> o(65536);for(unsigned i=0;i<65536u;++i)o[i]=std::uint16_t(i);
        std::stable_sort(o.begin(),o.end(),[&](std::uint16_t a,std::uint16_t b){return val(a)<val(b);});return o;}();
    std::vector<std::uint32_t> hist(65536,0u);
    for(size_t i=0;i<np;++i){const std::uint16_t* p=hb+i*4;++hist[p[0]];++hist[p[1]];++hist[p[2]];}

    const size_t m=n/2;float vm=0.f,vm1=0.f;bool tem1=(n&1u)!=0;size_t acc=0;
    for(const std::uint16_t c:ordem){const std::uint32_t k=hist[c];if(!k)continue;
        if(!tem1&&acc+k>m-1){vm1=val(c);tem1=true;}
        if(acc+k>m){vm=val(c);break;}
        acc+=k;}
    float med=vm;if(!(n&1u))med=.5f*(med+vm1);
    const float k=.18f/(std::max)(1e-6f,med);
    std::vector<std::uint8_t> lut(65536,0);
    for(unsigned c=0;c<65536u;++c){if(!hist[c])continue;
        float x=meia(std::uint16_t(c));if(!std::isfinite(x))x=0;x*=k;
        x=(x*(2.51f*x+.03f))/(x*(2.43f*x+.59f)+.14f);x=std::clamp(x,0.f,1.f);x=std::pow(x,1.f/2.2f);
        lut[c]=std::uint8_t(std::clamp(std::floor(x*255.f+.5f),0.f,255.f));}
    for(size_t i=0;i<np;++i)for(int c=0;c<3;++c)bgr[i*3+size_t(2-c)]=lut[hb[i*4+size_t(c)]];
}

inline int cpu_simd() noexcept {
    int r[4]{};__cpuid(r,1);const bool osx=(r[2]&(1<<27))!=0,avx=(r[2]&(1<<28))!=0;if(!osx||!avx)return 0;
    const unsigned long long x=_xgetbv(0);if((x&6u)!=6u)return 0;
    int m[4]{};__cpuid(m,0);if(m[0]<7)return 1;
    int e[4]{};__cpuidex(e,7,0);return ((e[1]&(1<<16))!=0&&(x&0xE6u)==0xE6u)?2:1;
}

class Rede {
public:

    void usar_simd(int modo) noexcept {const int c=cpu_simd();simd_=modo<=0?0:modo==2?(c>=1?1:0):c;}
    int simd() const noexcept {return simd_;}

    const std::vector<Rosto>& candidatos() const noexcept {return cand_;}
    size_t n_tensores() const noexcept {return ts.size();}
    const std::vector<float>& tensor(size_t i) const {return ts[i].v;}

    void detectar(const std::uint8_t* bgr,int w,int h,int passo,std::vector<Rosto>& out,float limiar=.6f,float nms=.3f,int top=5000){
        out.clear();if(!bgr||w<32||h<32)return;
        if(ts.size()<64)ts.resize(64);
        const int W=((w-1)/32+1)*32,H=((h-1)/32+1)*32;
        T& x=t(0,3,H,W);std::fill(x.v.begin(),x.v.end(),0.f);
        for(int y=0;y<h;++y){const std::uint8_t* r=bgr+size_t(y)*size_t(passo);for(int c=0;c<3;++c){float* o=x.p(c)+size_t(y)*W;for(int i=0;i<w;++i)o[i]=float(r[size_t(i)*3+size_t(c)]);}}
        if(simd_==2){conv0_v<V16>(x,t(1,16,H/2,W/2),G_);relu_v<V16>(t(1));}else if(simd_==1){conv0_v<V8>(x,t(1,16,H/2,W/2),G_);relu_v<V8>(t(1));}else{conv0(x,t(1,16,H/2,W/2));relu(t(1));}
        int a=1;
        auto unidade=[&](int ci,int d,bool r){T& m=t(d,convs[ci].co,t(a).h,t(a).w);if(simd_==2)pw_v<V16>(t(a),m,convs[ci]);else if(simd_==1)pw_v<V8>(t(a),m,convs[ci]);else pw(t(a),m,convs[ci]);
            T& o=t(d+1,m.c,m.h,m.w);if(simd_==2)dw_v<V16>(m,o,convs[ci+1],r);else if(simd_==1)dw_v<V8>(m,o,convs[ci+1],r);else dw(m,o,convs[ci+1],r);a=d+1;};
        auto pool=[&](int d){T& o=t(d,t(a).c,t(a).h/2,t(a).w/2);if(simd_==2)maxpool16(t(a),o);else maxpool(t(a),o);a=d;};
        unidade(1,2,true);pool(4);unidade(3,5,true);unidade(5,7,true);unidade(7,9,true);unidade(9,11,true);pool(13);
        unidade(11,14,true);unidade(13,16,true);const int p3=a;pool(18);
        unidade(15,19,true);unidade(17,21,true);const int p4=a;pool(23);
        unidade(19,24,true);unidade(21,26,true);unidade(23,28,true);const int p5=a;
        {T& s=t(30,t(p4).c,t(p4).h,t(p4).w);sobe_soma(t(p5),t(p4),s);a=30;unidade(25,31,true);}const int n4=a;
        {T& s=t(33,t(p3).c,t(p3).h,t(p3).w);sobe_soma(t(n4),t(p3),s);a=33;unidade(27,34,true);}const int n3=a;
        const int feat[3]{n3,n4,p5},passos[3]{8,16,32};
        std::vector<Rosto>& cand=cand_;cand.clear();
        for(int i=0;i<3;++i){
            T* cab[4];
            for(int kk=0;kk<4;++kk){const int ci=29+kk*6+i*2;a=feat[i];unidade(ci,40+kk*2,false);cab[kk]=&t(a);}
            const T& cls=*cab[0];const T& bb=*cab[1];const T& obj=*cab[2];const T& kp=*cab[3];
            const int R=cls.h,C=cls.w,P=R*C;const float s=float(passos[i]);
            for(int r=0;r<R;++r)for(int c=0;c<C;++c){const int id=r*C+c;
                const float sc=std::clamp(sig(cls.v[size_t(id)]),0.f,1.f),so=std::clamp(sig(obj.v[size_t(id)]),0.f,1.f);
                const float nota=std::sqrt(sc*so);if(!(nota>=limiar))continue;
                Rosto f{};const float cx=(float(c)+bb.v[size_t(id)])*s,cy=(float(r)+bb.v[size_t(P+id)])*s;
                f.w=std::exp(bb.v[size_t(2*P+id)])*s;f.h=std::exp(bb.v[size_t(3*P+id)])*s;f.x=cx-f.w/2.f;f.y=cy-f.h/2.f;
                for(int k=0;k<5;++k){f.lm[2*k]=(kp.v[size_t((2*k)*P+id)]+float(c))*s;f.lm[2*k+1]=(kp.v[size_t((2*k+1)*P+id)]+float(r))*s;}
                f.nota=nota;cand.push_back(f);}
        }

        std::vector<int> ord;for(int i=0;i<int(cand.size());++i)if(cand[size_t(i)].nota>limiar)ord.push_back(i);
        std::stable_sort(ord.begin(),ord.end(),[&](int u,int v){return cand[size_t(u)].nota>cand[size_t(v)].nota;});
        if(top>0&&int(ord.size())>top)ord.resize(size_t(top));
        auto ri=[](const Rosto& f,int* q){q[0]=int(f.x);q[1]=int(f.y);q[2]=int(f.w);q[3]=int(f.h);};
        for(const int i:ord){int u[4];ri(cand[size_t(i)],u);bool ok=true;
            for(const auto& k:out){int v[4];ri(k,v);
                const int ix=(std::max)(0,(std::min)(u[0]+u[2],v[0]+v[2])-(std::max)(u[0],v[0])),iy=(std::max)(0,(std::min)(u[1]+u[3],v[1]+v[3])-(std::max)(u[1],v[1]));
                const double in=double(ix)*iy,un=double(u[2])*u[3]+double(v[2])*v[3]-in;
                if(un>0&&in/un>double(nms)){ok=false;break;}}
            if(ok)out.push_back(cand[size_t(i)]);}
    }
private:
    struct T {int c=0,h=0,w=0;std::vector<float> v;float* p(int k){return v.data()+size_t(k)*size_t(h)*size_t(w);}const float* p(int k)const{return v.data()+size_t(k)*size_t(h)*size_t(w);}};
    std::vector<T> ts;int simd_=0;std::vector<Rosto> cand_;std::vector<float> G_;
    T& t(int i){return ts[size_t(i)];}

    struct V8 {using R=__m256;static constexpr int N=8;
        static R ld(const float* p){return _mm256_loadu_ps(p);}static void st(float* p,R v){_mm256_storeu_ps(p,v);}
        static R add(R a,R b){return _mm256_add_ps(a,b);}static R mul(R a,R b){return _mm256_mul_ps(a,b);}
        static R set(float x){return _mm256_set1_ps(x);}static R bc(const float* p){return _mm256_broadcast_ss(p);}
        static R relu(R v){return _mm256_and_ps(v,_mm256_cmp_ps(v,_mm256_setzero_ps(),_CMP_GT_OQ));}
        static void fim(){_mm256_zeroupper();}};
    struct V16 {using R=__m512;static constexpr int N=16;
        static R ld(const float* p){return _mm512_loadu_ps(p);}static void st(float* p,R v){_mm512_storeu_ps(p,v);}
        static R add(R a,R b){return _mm512_add_ps(a,b);}static R mul(R a,R b){return _mm512_mul_ps(a,b);}
        static R set(float x){return _mm512_set1_ps(x);}static R bc(const float* p){return _mm512_set1_ps(*p);}
        static R relu(R v){return _mm512_maskz_mov_ps(_mm512_cmp_ps_mask(v,_mm512_setzero_ps(),_CMP_GT_OQ),v);}
        static void fim(){_mm256_zeroupper();}};
    template<class V> static void relu_v(T& a){
        float* p=a.v.data();const size_t n=a.v.size();size_t i=0;
        for(;i+size_t(V::N)<=n;i+=size_t(V::N))V::st(p+i,V::relu(V::ld(p+i)));
        for(;i<n;++i)p[i]=p[i]>0.f?p[i]:0.f;
        V::fim();
    }

    template<class V> static void conv0_v(const T& in,T& o,std::vector<float>& G){
        const ConvPesos& c=convs[0];const int OW=o.w,OH=o.h,CO=o.c;G.resize(size_t(27)*size_t(OW));
        for(int oy=0;oy<OH;++oy){
            int tap[27];int nt=0;
            for(int ci=0;ci<3;++ci)for(int ky=0;ky<3;++ky){const int iy=2*oy+ky-1;if(iy<0||iy>=in.h)continue;const float* r=in.p(ci)+size_t(iy)*in.w;
                for(int kx=0;kx<3;++kx){float* g=G.data()+size_t(nt)*OW;for(int ox=0;ox<OW;++ox){const int ix=2*ox+kx-1;g[ox]=(ix>=0&&ix<in.w)?r[ix]:0.f;}tap[nt++]=(ci*3+ky)*3+kx;}}
            int k=0;
            for(;k+8<=CO;k+=8){int ox=0;
                for(;ox+V::N<=OW;ox+=V::N){
                    typename V::R a0=V::set(c.b[k]),a1=V::set(c.b[k+1]),a2=V::set(c.b[k+2]),a3=V::set(c.b[k+3]),a4=V::set(c.b[k+4]),a5=V::set(c.b[k+5]),a6=V::set(c.b[k+6]),a7=V::set(c.b[k+7]);
                    for(int j=0;j<nt;++j){const typename V::R g=V::ld(G.data()+size_t(j)*OW+size_t(ox));const float* w=c.w+size_t(k)*27+size_t(tap[j]);
                        a0=V::add(a0,V::mul(V::bc(w),g));a1=V::add(a1,V::mul(V::bc(w+27),g));a2=V::add(a2,V::mul(V::bc(w+54),g));a3=V::add(a3,V::mul(V::bc(w+81),g));
                        a4=V::add(a4,V::mul(V::bc(w+108),g));a5=V::add(a5,V::mul(V::bc(w+135),g));a6=V::add(a6,V::mul(V::bc(w+162),g));a7=V::add(a7,V::mul(V::bc(w+189),g));}
                    float* q=o.p(k)+size_t(oy)*OW+size_t(ox);const size_t pl=size_t(OW)*OH;
                    V::st(q,a0);V::st(q+pl,a1);V::st(q+2*pl,a2);V::st(q+3*pl,a3);V::st(q+4*pl,a4);V::st(q+5*pl,a5);V::st(q+6*pl,a6);V::st(q+7*pl,a7);}
                for(;ox<OW;++ox)for(int kk=k;kk<k+8;++kk){float e=c.b[kk];for(int j=0;j<nt;++j)e+=c.w[size_t(kk)*27+size_t(tap[j])]*G[size_t(j)*OW+size_t(ox)];o.p(kk)[size_t(oy)*OW+size_t(ox)]=e;}}
            for(;k<CO;++k)for(int ox=0;ox<OW;++ox){float e=c.b[k];for(int j=0;j<nt;++j)e+=c.w[size_t(k)*27+size_t(tap[j])]*G[size_t(j)*OW+size_t(ox)];o.p(k)[size_t(oy)*OW+size_t(ox)]=e;}
        }
        V::fim();
    }
    template<class V> static void pw_v(const T& in,T& o,const ConvPesos& c){
        const int P=in.h*in.w,CI=c.ci,CO=c.co;constexpr int B=256,N=V::N;const float* S=in.v.data();float* D=o.v.data();
        for(int p0=0;p0<P;p0+=B){const int n=(std::min)(B,P-p0);int k=0;
            for(;k+4<=CO;k+=4){const float* w0=c.w+size_t(k)*CI;const float* w1=w0+CI;const float* w2=w1+CI;const float* w3=w2+CI;
                float* q0=D+size_t(k)*P+p0;float* q1=q0+P;float* q2=q1+P;float* q3=q2+P;int i=0;
                for(;i+2*N<=n;i+=2*N){
                    typename V::R a0=V::set(c.b[k]),a1=V::set(c.b[k+1]),a2=V::set(c.b[k+2]),a3=V::set(c.b[k+3]);
                    typename V::R b0=a0,b1=a1,b2=a2,b3=a3;const float* s=S+size_t(p0)+size_t(i);
                    for(int ci=0;ci<CI;++ci,s+=P){const typename V::R v=V::ld(s),u=V::ld(s+N);
                        typename V::R W=V::bc(w0+ci);a0=V::add(a0,V::mul(W,v));b0=V::add(b0,V::mul(W,u));
                        W=V::bc(w1+ci);a1=V::add(a1,V::mul(W,v));b1=V::add(b1,V::mul(W,u));
                        W=V::bc(w2+ci);a2=V::add(a2,V::mul(W,v));b2=V::add(b2,V::mul(W,u));
                        W=V::bc(w3+ci);a3=V::add(a3,V::mul(W,v));b3=V::add(b3,V::mul(W,u));}
                    V::st(q0+i,a0);V::st(q0+i+N,b0);V::st(q1+i,a1);V::st(q1+i+N,b1);V::st(q2+i,a2);V::st(q2+i+N,b2);V::st(q3+i,a3);V::st(q3+i+N,b3);}
                for(;i+N<=n;i+=N){
                    typename V::R a0=V::set(c.b[k]),a1=V::set(c.b[k+1]),a2=V::set(c.b[k+2]),a3=V::set(c.b[k+3]);const float* s=S+size_t(p0)+size_t(i);
                    for(int ci=0;ci<CI;++ci,s+=P){const typename V::R v=V::ld(s);
                        a0=V::add(a0,V::mul(V::bc(w0+ci),v));a1=V::add(a1,V::mul(V::bc(w1+ci),v));a2=V::add(a2,V::mul(V::bc(w2+ci),v));a3=V::add(a3,V::mul(V::bc(w3+ci),v));}
                    V::st(q0+i,a0);V::st(q1+i,a1);V::st(q2+i,a2);V::st(q3+i,a3);}
                for(;i<n;++i){float r0=c.b[k],r1=c.b[k+1],r2=c.b[k+2],r3=c.b[k+3];const float* s=S+size_t(p0)+size_t(i);
                    for(int ci=0;ci<CI;++ci,s+=P){const float v=*s;r0+=w0[ci]*v;r1+=w1[ci]*v;r2+=w2[ci]*v;r3+=w3[ci]*v;}
                    q0[i]=r0;q1[i]=r1;q2[i]=r2;q3[i]=r3;}}
            for(;k<CO;++k){const float* w0=c.w+size_t(k)*CI;float* q=D+size_t(k)*P+p0;int i=0;
                for(;i+2*N<=n;i+=2*N){typename V::R a0=V::set(c.b[k]),b0=a0;const float* s=S+size_t(p0)+size_t(i);
                    for(int ci=0;ci<CI;++ci,s+=P){const typename V::R W=V::bc(w0+ci);a0=V::add(a0,V::mul(W,V::ld(s)));b0=V::add(b0,V::mul(W,V::ld(s+N)));}
                    V::st(q+i,a0);V::st(q+i+N,b0);}
                for(;i+N<=n;i+=N){typename V::R a0=V::set(c.b[k]);const float* s=S+size_t(p0)+size_t(i);
                    for(int ci=0;ci<CI;++ci,s+=P)a0=V::add(a0,V::mul(V::bc(w0+ci),V::ld(s)));
                    V::st(q+i,a0);}
                for(;i<n;++i){float r0=c.b[k];const float* s=S+size_t(p0)+size_t(i);for(int ci=0;ci<CI;++ci,s+=P)r0+=w0[ci]*(*s);q[i]=r0;}}
        }
        V::fim();
    }

    template<class V,int NL> static int dw_miolo(const float* const* L,const float* const* F,float bk,float* d,int W,bool r,int x){
        typename V::R f0[NL],f1[NL],f2[NL];for(int j=0;j<NL;++j){f0[j]=V::set(F[j][0]);f1[j]=V::set(F[j][1]);f2[j]=V::set(F[j][2]);}
        const typename V::R B=V::set(bk);
        for(;x+V::N<=W-1;x+=V::N){typename V::R acc=B;
            for(int j=0;j<NL;++j){const float* l=L[j];
                acc=V::add(acc,V::add(V::add(V::mul(f0[j],V::ld(l+x-1)),V::mul(f1[j],V::ld(l+x))),V::mul(f2[j],V::ld(l+x+1))));}
            if(r)acc=V::relu(acc);
            V::st(d+x,acc);}
        return x;
    }
    template<class V,int NL> static void dw_linha(const float* const* L,const float* const* F,float bk,float* d,int W,bool r){
        int x=dw_miolo<V,NL>(L,F,bk,d,W,r,1);
        if constexpr(V::N==16)x=dw_miolo<V8,NL>(L,F,bk,d,W,r,x);
        for(;x<W-1;++x){float e=bk;for(int j=0;j<NL;++j){const float* l=L[j];const float* f=F[j];e+=f[0]*l[x-1]+f[1]*l[x]+f[2]*l[x+1];}d[x]=(!r||e>0.f)?e:0.f;}
    }
    template<class V> static void dw_v(const T& in,T& o,const ConvPesos& c,bool r){
        const int W=in.w,H=in.h;
        for(int k=0;k<in.c;++k){const float* wk=c.w+k*9;const float* s=in.p(k);float* q=o.p(k);const float bk=c.b[k];
            for(int y=0;y<H;++y){float* d=q+size_t(y)*W;
                const float* L[3]{};const float* F[3]{};int nl=0;
                for(int dy=-1;dy<=1;++dy){const int yy=y+dy;if(yy<0||yy>=H)continue;L[nl]=s+size_t(yy)*W;F[nl]=wk+(dy+1)*3;++nl;}
                {float e=bk;for(int j=0;j<nl;++j)e+=F[j][1]*L[j][0]+(W>1?F[j][2]*L[j][1]:0.f);d[0]=(!r||e>0.f)?e:0.f;}
                if(W>1){float e=bk;for(int j=0;j<nl;++j)e+=F[j][0]*L[j][W-2]+F[j][1]*L[j][W-1];d[W-1]=(!r||e>0.f)?e:0.f;}
                if(W>2){if(nl==3)dw_linha<V,3>(L,F,bk,d,W,r);else if(nl==2)dw_linha<V,2>(L,F,bk,d,W,r);else if(nl==1)dw_linha<V,1>(L,F,bk,d,W,r);}}}
        V::fim();
    }
    T& t(int i,int c,int h,int w){T& r=t(i);r.c=c;r.h=h;r.w=w;r.v.resize(size_t(c)*size_t(h)*size_t(w));return r;}
    static float sig(float x){return 1.f/(1.f+std::exp(-x));}
    static void relu(T& a){for(auto& x:a.v)x=x>0.f?x:0.f;}

    static void conv0(const T& in,T& o){
        const ConvPesos& c=convs[0];const int OW=o.w,OH=o.h;std::vector<float> g(static_cast<size_t>(OW));
        for(int k=0;k<o.c;++k){float* q=o.p(k);std::fill(q,q+size_t(OW)*OH,c.b[k]);}
        for(int oy=0;oy<OH;++oy)for(int ci=0;ci<3;++ci)for(int ky=0;ky<3;++ky){
            const int iy=2*oy+ky-1;if(iy<0||iy>=in.h)continue;const float* r=in.p(ci)+size_t(iy)*in.w;
            for(int kx=0;kx<3;++kx){
                for(int ox=0;ox<OW;++ox){const int ix=2*ox+kx-1;g[size_t(ox)]=(ix>=0&&ix<in.w)?r[ix]:0.f;}
                for(int k=0;k<o.c;++k){const float wv=c.w[((k*3+ci)*3+ky)*3+kx];float* q=o.p(k)+size_t(oy)*OW;for(int ox=0;ox<OW;++ox)q[ox]+=wv*g[size_t(ox)];}}
        }
    }

    static void pw(const T& in,T& o,const ConvPesos& c){
        const int P=in.h*in.w,CI=c.ci,CO=c.co;constexpr int B=256;
        for(int p0=0;p0<P;p0+=B){const int n=(std::min)(B,P-p0);int k=0;
            for(;k+4<=CO;k+=4){float* q0=o.v.data()+size_t(k)*P+p0;float* q1=q0+P;float* q2=q1+P;float* q3=q2+P;
                for(int i=0;i<n;++i){q0[i]=c.b[k];q1[i]=c.b[k+1];q2[i]=c.b[k+2];q3[i]=c.b[k+3];}
                for(int ci=0;ci<CI;++ci){const float* s=in.v.data()+size_t(ci)*P+p0;const float w0=c.w[k*CI+ci],w1=c.w[(k+1)*CI+ci],w2=c.w[(k+2)*CI+ci],w3=c.w[(k+3)*CI+ci];
                    for(int i=0;i<n;++i){const float v=s[i];q0[i]+=w0*v;q1[i]+=w1*v;q2[i]+=w2*v;q3[i]+=w3*v;}}}
            for(;k<CO;++k){float* q=o.v.data()+size_t(k)*P+p0;for(int i=0;i<n;++i)q[i]=c.b[k];
                for(int ci=0;ci<CI;++ci){const float* s=in.v.data()+size_t(ci)*P+p0;const float wv=c.w[k*CI+ci];for(int i=0;i<n;++i)q[i]+=wv*s[i];}}
        }
    }

    static void dw(const T& in,T& o,const ConvPesos& c,bool r){
        const int W=in.w,H=in.h;
        for(int k=0;k<in.c;++k){const float* wk=c.w+k*9;const float* s=in.p(k);float* q=o.p(k);
            for(int y=0;y<H;++y){float* d=q+size_t(y)*W;for(int x=0;x<W;++x)d[x]=c.b[k];
                for(int dy=-1;dy<=1;++dy){const int yy=y+dy;if(yy<0||yy>=H)continue;const float* l=s+size_t(yy)*W;const float* f=wk+(dy+1)*3;
                    d[0]+=f[1]*l[0]+(W>1?f[2]*l[1]:0.f);
                    for(int x=1;x<W-1;++x)d[x]+=f[0]*l[x-1]+f[1]*l[x]+f[2]*l[x+1];
                    if(W>1)d[W-1]+=f[0]*l[W-2]+f[1]*l[W-1];}
                if(r)for(int x=0;x<W;++x)d[x]=d[x]>0.f?d[x]:0.f;}}
    }

    static void maxpool16(const T& in,T& o){
        const __m512i ip=_mm512_setr_epi32(0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30),ii=_mm512_setr_epi32(1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31);
        for(int k=0;k<in.c;++k){const float* s=in.p(k);float* q=o.p(k);
            for(int y=0;y<o.h;++y){const float* a=s+size_t(2*y)*in.w;const float* b=a+in.w;float* d=q+size_t(y)*o.w;int x=0;
                for(;x+16<=o.w;x+=16){const __m512 a0=_mm512_loadu_ps(a+2*x),a1=_mm512_loadu_ps(a+2*x+16),b0=_mm512_loadu_ps(b+2*x),b1=_mm512_loadu_ps(b+2*x+16);
                    const __m512 ma=_mm512_max_ps(_mm512_permutex2var_ps(a0,ii,a1),_mm512_permutex2var_ps(a0,ip,a1));
                    const __m512 mb=_mm512_max_ps(_mm512_permutex2var_ps(b0,ii,b1),_mm512_permutex2var_ps(b0,ip,b1));
                    _mm512_storeu_ps(d+x,_mm512_max_ps(mb,ma));}
                for(;x<o.w;++x)d[x]=(std::max)((std::max)(a[2*x],a[2*x+1]),(std::max)(b[2*x],b[2*x+1]));}}
        _mm256_zeroupper();
    }
    static void maxpool(const T& in,T& o){
        for(int k=0;k<in.c;++k){const float* s=in.p(k);float* q=o.p(k);
            for(int y=0;y<o.h;++y){const float* a=s+size_t(2*y)*in.w;const float* b=a+in.w;float* d=q+size_t(y)*o.w;
                for(int x=0;x<o.w;++x)d[x]=(std::max)((std::max)(a[2*x],a[2*x+1]),(std::max)(b[2*x],b[2*x+1]));}}
    }
    static void sobe_soma(const T& pq,const T& gr,T& o){
        for(int k=0;k<gr.c;++k){const float* s=pq.p(k);const float* g=gr.p(k);float* q=o.p(k);
            for(int y=0;y<gr.h;++y){const float* l=s+size_t((std::min)(y/2,pq.h-1))*pq.w;for(int x=0;x<gr.w;++x)q[size_t(y)*gr.w+x]=g[size_t(y)*gr.w+x]+l[(std::min)(x/2,pq.w-1)];}}
    }
};
}
