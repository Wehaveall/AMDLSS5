#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>
#include <immintrin.h>
#include <malloc.h>
#include "yunet.h"
#include "scrfd_grafo.h"
#include "scrfd_nucleo.h"

namespace nrsc {
using nryn::Rosto;

inline void sha256(const unsigned char* d,std::size_t n,unsigned char out[32]) noexcept {
    static const std::uint32_t K[64]={0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
        0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,
        0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,
        0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,
        0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    std::uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    auto rr=[](std::uint32_t x,int r){return (x>>r)|(x<<(32-r));};
    const std::uint64_t bits=std::uint64_t(n)*8u;const std::size_t tot=((n+9+63)/64)*64;
    for(std::size_t o=0;o<tot;o+=64){
        unsigned char b[64];for(int i=0;i<64;++i){const std::size_t p=o+std::size_t(i);
            b[i]=p<n?d[p]:p==n?0x80:(p>=tot-8?static_cast<unsigned char>(bits>>(8*(tot-1-p))):0);}
        std::uint32_t w[64];for(int i=0;i<16;++i)w[i]=std::uint32_t(b[4*i])<<24|std::uint32_t(b[4*i+1])<<16|std::uint32_t(b[4*i+2])<<8|b[4*i+3];
        for(int i=16;i<64;++i){const std::uint32_t s0=rr(w[i-15],7)^rr(w[i-15],18)^(w[i-15]>>3),s1=rr(w[i-2],17)^rr(w[i-2],19)^(w[i-2]>>10);w[i]=w[i-16]+s0+w[i-7]+s1;}
        std::uint32_t a=h[0],bb=h[1],c=h[2],dd=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
        for(int i=0;i<64;++i){const std::uint32_t t1=hh+(rr(e,6)^rr(e,11)^rr(e,25))+((e&f)^(~e&g))+K[i]+w[i],t2=(rr(a,2)^rr(a,13)^rr(a,22))+((a&bb)^(a&c)^(bb&c));
            hh=g;g=f;f=e;e=dd+t1;dd=c;c=bb;bb=a;a=t1+t2;}
        h[0]+=a;h[1]+=bb;h[2]+=c;h[3]+=dd;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=hh;}
    for(int i=0;i<8;++i){out[4*i]=static_cast<unsigned char>(h[i]>>24);out[4*i+1]=static_cast<unsigned char>(h[i]>>16);out[4*i+2]=static_cast<unsigned char>(h[i]>>8);out[4*i+3]=static_cast<unsigned char>(h[i]);}
}

class Pool {
public:
    Pool()=default;Pool(const Pool&)=delete;Pool& operator=(const Pool&)=delete;
    ~Pool(){parar();}
    void iniciar(int n,int prioridade){
        n=(std::clamp)(n,1,16);if(n==n_&&prio_==prioridade)return;
        parar();n_=n;prio_=prioridade;sair_=false;
        for(int i=1;i<n_;++i)th_.emplace_back([this]{laco();});
    }
    int n() const noexcept {return n_;}
    template<class F> void para(int nt,F& f){
        if(n_<=1||nt<=1||nt>65535){for(int i=0;i<nt;++i)f(i);return;}

        ctx_=&f;call_=[](void* c,int i){(*static_cast<F*>(c))(i);};
        feitas_.store(0,std::memory_order_relaxed);
        const std::uint32_t g=++geracao_;estado_.store(std::uint64_t(g)<<32|std::uint64_t(unsigned(nt))<<16,std::memory_order_release);
        gen_.store(g,std::memory_order_release);
        if(dormindo_.load(std::memory_order_acquire)>0){std::lock_guard<std::mutex> l(m_);cv_.notify_all();}
        trabalha(g);
        while(feitas_.load(std::memory_order_acquire)<nt)_mm_pause();
    }
private:
    void trabalha(std::uint32_t g){
        for(;;){std::uint64_t s=estado_.load(std::memory_order_acquire);
            if(std::uint32_t(s>>32)!=g)break;const unsigned i=unsigned(s&0xFFFFu),tot=unsigned((s>>16)&0xFFFFu);
            if(i>=tot)break;
            if(!estado_.compare_exchange_weak(s,s+1u,std::memory_order_acq_rel,std::memory_order_acquire))continue;
            call_(ctx_,int(i));feitas_.fetch_add(1,std::memory_order_acq_rel);}
    }
    void laco(){
#ifdef _WIN32
        if(prio_)SetThreadPriority(GetCurrentThread(),prio_);
#endif
        unsigned vista=gen_.load(std::memory_order_acquire);
        for(;;){
            unsigned g;int giros=0;
            while((g=gen_.load(std::memory_order_acquire))==vista&&!sair_.load(std::memory_order_acquire)){
                if(++giros<4000){_mm_pause();continue;}
                std::unique_lock<std::mutex> l(m_);dormindo_.fetch_add(1,std::memory_order_acq_rel);
                cv_.wait(l,[&]{return gen_.load(std::memory_order_acquire)!=vista||sair_.load(std::memory_order_acquire);});
                dormindo_.fetch_sub(1,std::memory_order_acq_rel);giros=0;}
            if(sair_.load(std::memory_order_acquire))return;
            vista=g;trabalha(g);}
    }
    void parar(){
        {std::lock_guard<std::mutex> l(m_);sair_.store(true,std::memory_order_release);cv_.notify_all();}
        for(auto& t:th_)if(t.joinable())t.join();
        th_.clear();n_=1;
    }
    int n_=1,prio_=0;std::vector<std::thread> th_;
    std::atomic<unsigned> gen_{0};std::atomic<std::uint64_t> estado_{0};std::uint32_t geracao_=0;std::atomic<int> feitas_{0},dormindo_{0};std::atomic<bool> sair_{false};
    void* ctx_=nullptr;void (*call_)(void*,int)=nullptr;std::mutex m_;std::condition_variable cv_;
};

class Rede {
public:
    Rede()=default;Rede(const Rede&)=delete;Rede& operator=(const Rede&)=delete;
    ~Rede(){if(mem_)_aligned_free(mem_);}

    bool carregar(const wchar_t* caminho,char* erro,std::size_t ne){
        pesos_.clear();
        if(nryn::cpu_simd()<2){std::snprintf(erro,ne,"CPU sem AVX-512F");return false;}
        FILE* f=nullptr;if(_wfopen_s(&f,caminho,L"rb")||!f){std::snprintf(erro,ne,"arquivo ausente");return false;}
        std::vector<unsigned char> b(static_cast<std::size_t>(pesos_bytes)+1u);const std::size_t lido=std::fread(b.data(),1,b.size(),f);std::fclose(f);
        if(lido!=pesos_bytes){std::snprintf(erro,ne,"tamanho %zu (esperado %llu)",lido,static_cast<unsigned long long>(pesos_bytes));return false;}
        unsigned char h[32];sha256(b.data(),lido,h);
        if(std::memcmp(h,pesos_sha256,32)!=0){std::snprintf(erro,ne,"SHA-256 diferente (%02X%02X%02X%02X)",h[0],h[1],h[2],h[3]);return false;}
        if(std::memcmp(b.data(),"NRSCRFD1",8)!=0){std::snprintf(erro,ne,"cabecalho");return false;}
        pesos_.resize(static_cast<std::size_t>(pesos_floats));std::memcpy(pesos_.data(),b.data()+16,pesos_.size()*4);
        reservar(640,384);reservar(1280,320);cand_.reserve(4096);ord_.reserve(4096);
        std::snprintf(erro,ne,"ok");return true;
    }
    bool pronto() const noexcept {return !pesos_.empty();}

    void usar_threads(int n,int prioridade=0){pool_.iniciar(n,prioridade);}
    int threads() const noexcept {return pool_.n();}

    void reservar(int w,int h){const int W=((w+31)/32)*32,H=((h+31)/32)*32;planejar(W,H);}

    void detectar(const std::uint8_t* bgr,int w,int h,int passo,std::vector<Rosto>& out,float limiar=.5f,float nms=.4f){
        out.clear();cand_.clear();if(!bgr||w<32||h<32||!pronto())return;
        const int W=((w+31)/32)*32,H=((h+31)/32)*32;if(W!=W32_||H!=H32_)planejar(W,H);
        for(int j=0;j<n_ops;++j){const Op& o=ops[j];
            switch(o.tipo){
            case 3:if(o.in==0)fases_entrada(bgr,w,h,passo);else fases(o.in);break;
            case 0:conv(o);break;
            case 1:maxpool(o);break;
            case 2:sobe(o);break;
            default:break;}}
        decodificar(limiar);

        ord_.clear();for(int i=0;i<int(cand_.size());++i)if(cand_[size_t(i)].nota>limiar)ord_.push_back(i);
        std::stable_sort(ord_.begin(),ord_.end(),[&](int u,int v){return cand_[size_t(u)].nota>cand_[size_t(v)].nota;});
        for(const int i:ord_){const Rosto& a=cand_[size_t(i)];bool ok=true;
            for(const auto& k:out){
                const double Aa=double(a.w)*a.h,Ab=double(k.w)*k.h;
                if(Aa+Ab<=2.220446049250313e-16){if(1.f>nms){ok=false;break;}continue;}
                const double x1=(std::max)(double(a.x),double(k.x)),y1=(std::max)(double(a.y),double(k.y));
                const double iw=(std::min)(double(a.x)+a.w,double(k.x)+k.w)-x1,ih=(std::min)(double(a.y)+a.h,double(k.y)+k.h)-y1;
                const double Aab=(iw<=0||ih<=0)?0.0:iw*ih;
                const float ov=1.f-static_cast<float>(1.0-Aab/(Aa+Ab-Aab));
                if(ov>nms){ok=false;break;}}
            if(ok)out.push_back(a);}
    }

    void brutas(int i,std::vector<float>& v) const {
        const int si=i%3,tipo=i/3;const Geo g=geo(cabeca[si]);const float* P=t_[cabeca[si]];const int nc=tipo==0?1:tipo==1?4:10;
        v.assign(size_t(g.H)*size_t(g.W)*2u*size_t(nc),0.f);
        for(int y=0;y<g.H;++y)for(int x=0;x<g.W;++x)for(int a=0;a<2;++a){const size_t base=size_t(y+1)*g.Wp+size_t(x+1),id=(size_t(y)*g.W+size_t(x))*2u+size_t(a);
            for(int j=0;j<nc;++j){const int ch=tipo==0?a:tipo==1?2+4*a+j:10+10*a+j;float val=P[size_t(ch)*g.ps+base];
                if(tipo==0)val=1.f/(1.f+std::exp(-val));else if(tipo==1)val*=escala_caixa[si];
                v[id*size_t(nc)+size_t(j)]=val;}}
    }
    const std::vector<Rosto>& candidatos() const noexcept {return cand_;}
private:
    struct Geo {int H,W,Wp;size_t ps;int Ho,Wo,Wpo;size_t qs;};
    static size_t r16(size_t x){return (x+15u)&~size_t(15);}
    Geo geo(int t) const {Geo g{};const int d=tensores[t].div;g.H=H32_/d;g.W=W32_/d;g.Wp=g.W+2;g.ps=r16(size_t(g.H+2)*size_t(g.Wp));
        g.Ho=g.H/2;g.Wo=g.W/2;g.Wpo=g.Wo+2;g.qs=r16(size_t(g.Ho+1)*size_t(g.Wpo));return g;}

    void planejar(int W,int H){
        W32_=W;H32_=H;
        int ult_t[n_tensores],ult_f[n_tensores];for(int t=0;t<n_tensores;++t){ult_t[t]=-1;ult_f[t]=-1;}
        for(int j=0;j<n_ops;++j){const Op& o=ops[j];
            if(o.tipo==0){if(o.fase)ult_f[o.in]=j;else ult_t[o.in]=j;if(o.res>=0)ult_t[o.res]=j;}
            else if(o.tipo==3){if(o.in!=0)ult_t[o.in]=j;}
            else{ult_t[o.in]=j;if(o.res>=0)ult_t[o.res]=j;}}
        for(int s=0;s<3;++s)ult_t[cabeca[s]]=n_ops;
        struct B {size_t off,len;};B livre[256];int nl=0;size_t topo=0;
        auto pega=[&](size_t len)->size_t{len=r16(len);
            for(int i=0;i<nl;++i)if(livre[i].len>=len){const size_t o=livre[i].off;livre[i].off+=len;livre[i].len-=len;if(!livre[i].len){for(int k=i;k+1<nl;++k)livre[k]=livre[k+1];--nl;}return o;}
            const size_t o=topo;topo+=len;return o;};
        auto solta=[&](size_t off,size_t len){len=r16(len);int i=0;while(i<nl&&livre[i].off<off)++i;
            if(nl<256){for(int k=nl;k>i;--k)livre[k]=livre[k-1];livre[i]={off,len};++nl;}
            for(int k=0;k+1<nl;){if(livre[k].off+livre[k].len==livre[k+1].off){livre[k].len+=livre[k+1].len;for(int q=k+1;q+1<nl;++q)livre[q]=livre[q+1];--nl;}else ++k;}};
        size_t off_t[n_tensores],off_f[n_tensores];
        auto tam_t=[&](int t){return size_t(tensores[t].c)*geo(t).ps;};
        auto tam_f=[&](int t){return size_t(tensores[t].c)*4u*geo(t).qs;};
        for(int j=0;j<n_ops;++j){const Op& o=ops[j];
            if(o.tipo==3)off_f[o.in]=pega(tam_f(o.in));else off_t[o.out]=pega(tam_t(o.out));
            for(int t=0;t<n_tensores;++t){if(ult_t[t]==j&&t!=0)solta(off_t[t],tam_t(t));if(ult_f[t]==j)solta(off_f[t],tam_f(t));}}
        const size_t need=topo+1024u;
        if(need>cap_){if(mem_)_aligned_free(mem_);mem_=static_cast<float*>(_aligned_malloc(need*4u,64));if(!mem_){cap_=0;pesos_.clear();return;}
            std::memset(mem_,0,need*4u);cap_=need;}
        for(int t=0;t<n_tensores;++t){t_[t]=nullptr;f_[t]=nullptr;}
        for(int j=0;j<n_ops;++j){const Op& o=ops[j];if(o.tipo==3)f_[o.in]=mem_+off_f[o.in];else t_[o.out]=mem_+off_t[o.out];}
    }
    void zera_anel(float* p,const Geo& g){
        std::memset(p,0,size_t(g.Wp)*4u);std::memset(p+size_t(g.H+1)*g.Wp,0,size_t(g.Wp)*4u);
        for(int r=1;r<=g.H;++r){p[size_t(r)*g.Wp]=0.f;p[size_t(r)*g.Wp+size_t(g.W+1)]=0.f;}
    }

    void fases_entrada(const std::uint8_t* bgr,int w,int h,int passo){
        const Geo g=geo(0);float lut[256];for(int v=0;v<256;++v)lut[v]=(float(v)-127.5f)*(1.f/128.f);const float pad=(0.f-127.5f)*(1.f/128.f);
        float* Q=f_[0];
        auto tarefa=[&](int pl){const int ch=pl/4,a=(pl%4)/2,b=pl%2;float* q=Q+size_t(pl)*g.qs;const int cb=2-ch;
            for(int i=0;i<=g.Ho;++i){float* row=q+size_t(i)*g.Wpo;const int r=2*i+a;
                if(r<1||r>g.H){std::memset(row,0,size_t(g.Wpo)*4u);continue;}
                const int y=r-1;const std::uint8_t* src=y<h?bgr+size_t(y)*size_t(passo):nullptr;
                for(int j=0;j<g.Wpo;++j){const int c=2*j+b;float v=0.f;
                    if(c>=1&&c<=g.W&&j<=g.Wo){const int x=c-1;v=(src&&x<w)?lut[src[size_t(x)*3+size_t(cb)]]:pad;}
                    row[j]=v;}}};
        pool_.para(12,tarefa);
    }
    void fases(int t){
        const Geo g=geo(t);const float* P=t_[t];float* Q=f_[t];
        auto tarefa=[&](int c){const float* p=P+size_t(c)*g.ps;
            for(int ph=0;ph<4;++ph){const int a=ph/2,b=ph%2;float* q=Q+(size_t(c)*4u+size_t(ph))*g.qs;
                for(int i=0;i<=g.Ho;++i){const float* src=p+size_t(2*i+a)*g.Wp+size_t(b);float* row=q+size_t(i)*g.Wpo;
                    for(int j=0;j<=g.Wo;++j)row[j]=src[2*j];row[g.Wo+1]=0.f;}}};
        pool_.para(tensores[t].c,tarefa);
    }
    void conv(const Op& o){
        const Geo go=geo(o.out),gi=geo(o.in);const int N=go.H*go.Wp-2,NV=(N+15)/16,M=o.K==8?3:4,NT=(NV+M-1)/M,ncob=o.cop/o.K;
        const float* inb;size_t cs;int toff[9];
        for(int t=0;t<o.nt;++t){const signed char* tp=taps[o.tap0+t];
            toff[t]=tp[0]<0?tp[1]*gi.Wp+tp[2]:int(tp[0]*int(gi.qs))+tp[1]*gi.Wpo+tp[2];}
        if(o.fase){inb=f_[o.in];cs=4u*gi.qs;}else{inb=t_[o.in];cs=gi.ps;}
        float* ob=t_[o.out];const float* rb=o.res>=0?t_[o.res]:nullptr;const size_t sh=size_t(go.Wp)+1u;
        const int n=pool_.n();const int chunk=n<=1?4:(std::clamp)(NT*ncob/(6*n),1,4);const int nch=(NT+chunk-1)/chunk;
        const float* W=pesos_.data()+o.woff;const float* Bv=pesos_.data()+o.boff;const size_t wbloco=size_t(o.ci)*size_t(o.nt)*size_t(o.K);
        auto tarefa=[&](int id){const int ch=id/ncob,cb=id%ncob;const int t0=ch*chunk,t1=(std::min)(NT,t0+chunk);
            const float* w=W+size_t(cb)*wbloco;const float* b=Bv+size_t(cb)*size_t(o.K);
            float* oc=ob+size_t(cb)*size_t(o.K)*go.ps;const float* rc=rb?rb+size_t(cb)*size_t(o.K)*go.ps:nullptr;
            for(int tl=t0;tl<t1;++tl){const int i=tl*M*16;__mmask16 mk[4];const __mmask16* pm=nullptr;
                if(i+M*16>N){for(int m=0;m<M;++m){const int r=N-(i+16*m);mk[m]=r>=16?__mmask16(0xFFFF):r<=0?__mmask16(0):__mmask16((1u<<unsigned(r))-1u);}pm=mk;}
                if(o.K==8)nucleo_8x3(inb+i,cs,toff,o.nt,o.ci,w,b,oc+sh+size_t(i),go.ps,rc?rc+sh+size_t(i):nullptr,o.relu!=0,pm);
                else nucleo_6x4(inb+i,cs,toff,o.nt,o.ci,w,b,oc+sh+size_t(i),go.ps,rc?rc+sh+size_t(i):nullptr,o.relu!=0,pm);}

            const size_t lo=sh+size_t(t0)*size_t(M)*16u,hi=sh+(std::min)(size_t(N),size_t(t1)*size_t(M)*16u);
            for(int k=0;k<o.K;++k){float* p=oc+size_t(k)*go.ps;
                for(size_t r=lo/size_t(go.Wp);r<=(hi-1)/size_t(go.Wp);++r){const size_t a0=r*size_t(go.Wp),a1=a0+size_t(go.W+1);
                    if(a0>=lo&&a0<hi)p[a0]=0.f;if(a1>=lo&&a1<hi)p[a1]=0.f;}
                if(t0==0)std::memset(p,0,sh*4u);
                if(t1==NT){const size_t z0=size_t(go.H+1)*size_t(go.Wp)-1u;std::memset(p+z0,0,(size_t(go.H+2)*size_t(go.Wp)-z0)*4u);}}
        };
        if(n<=1){for(int id=0;id<nch*ncob;++id)tarefa(id);}else pool_.para(nch*ncob,tarefa);
        _mm256_zeroupper();
    }
    void maxpool(const Op& o){
        const Geo gi=geo(o.in),go=geo(o.out);const float* I=t_[o.in];float* O=t_[o.out];
        auto tarefa=[&](int c){const float* p=I+size_t(c)*gi.ps;float* q=O+size_t(c)*go.ps;
            for(int y=0;y<go.H;++y){const float* a=p+size_t(2*y+1)*gi.Wp+1;const float* b=a+gi.Wp;float* d=q+size_t(y+1)*go.Wp+1;
                for(int x=0;x<go.W;++x)d[x]=(std::max)((std::max)(a[2*x],a[2*x+1]),(std::max)(b[2*x],b[2*x+1]));}
            zera_anel(q,go);};
        pool_.para(tensores[o.out].c,tarefa);
    }
    void sobe(const Op& o){
        const Geo gp=geo(o.in),go=geo(o.out);const float* I=t_[o.in];const float* R=t_[o.res];float* O=t_[o.out];
        auto tarefa=[&](int c){const float* p=I+size_t(c)*gp.ps;const float* r=R+size_t(c)*go.ps;float* q=O+size_t(c)*go.ps;
            for(int y=0;y<go.H;++y){const float* l=p+size_t(y/2+1)*gp.Wp+1;const float* rr=r+size_t(y+1)*go.Wp+1;float* d=q+size_t(y+1)*go.Wp+1;
                for(int x=0;x<go.W;++x)d[x]=rr[x]+l[x/2];}
            zera_anel(q,go);};
        pool_.para(tensores[o.out].c,tarefa);
    }
    void decodificar(float limiar){
        const float lg=limiar>0.f&&limiar<1.f?std::log(limiar/(1.f-limiar))-1e-3f:-1e30f;
        for(int si=0;si<3;++si){const Geo g=geo(cabeca[si]);const float* P=t_[cabeca[si]];const int st=8<<si;const float sc=escala_caixa[si];
            for(int y=0;y<g.H;++y)for(int x=0;x<g.W;++x){const size_t base=size_t(y+1)*g.Wp+size_t(x+1);
                for(int a=0;a<2;++a){const float lo=P[size_t(a)*g.ps+base];if(!(lo>=lg))continue;
                    const float nota=1.f/(1.f+std::exp(-lo));if(!(nota>=limiar))continue;
                    const float cx=float(x*st),cy=float(y*st);float d[4];
                    for(int j=0;j<4;++j)d[j]=(P[size_t(2+4*a+j)*g.ps+base]*sc)*float(st);
                    Rosto f{};const float x1=cx-d[0],y1=cy-d[1],x2=cx+d[2],y2=cy+d[3];f.x=x1;f.y=y1;f.w=x2-x1;f.h=y2-y1;f.nota=nota;
                    for(int k=0;k<5;++k){f.lm[2*k]=cx+P[size_t(10+10*a+2*k)*g.ps+base]*float(st);f.lm[2*k+1]=cy+P[size_t(11+10*a+2*k)*g.ps+base]*float(st);}
                    cand_.push_back(f);}}}
    }
    std::vector<float> pesos_;float* mem_=nullptr;size_t cap_=0;int W32_=0,H32_=0;
    float* t_[n_tensores]{};float* f_[n_tensores]{};
    std::vector<Rosto> cand_;std::vector<int> ord_;Pool pool_;
};
}
