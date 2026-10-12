#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "nr_publico.h"
#include "rede_cada.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <mutex>

namespace nrrj {

inline std::atomic<bool> ligado{false};
inline std::atomic<bool> menu_escreveu{false};
inline std::atomic<bool> ini_lido{false};

inline bool valor_de(const wchar_t* t) noexcept {return t&&t[0]>=L'1'&&t[0]<=L'9';}
inline constexpr unsigned teto_padrao=1024;
inline unsigned teto_de(const wchar_t* t) noexcept {
    if(!t)return teto_padrao;const long v=std::wcstol(t,nullptr,10);
    return v>=256&&v<=1024&&v%128==0?unsigned(v):teto_padrao;
}
inline std::atomic<unsigned> teto{teto_padrao};

inline std::atomic<unsigned> cada{1};
inline std::atomic<bool> fora{false};

inline std::atomic<int> cada_quadros{-1};

inline std::atomic<int> corte_modo{0};

inline std::atomic<unsigned> zoom{0};
inline std::atomic<int> recorte_forcado{0};
inline std::atomic<int> recorte_fixo_ini{0};
inline constexpr unsigned zoom_lado=2048;
inline constexpr float zoom_cab_4k=1360.f;
inline constexpr unsigned cada_quadros_auto=16;
inline unsigned cada_quadros_efetivo() noexcept {
    const int v=cada_quadros.load(std::memory_order_relaxed);
    if(v>=0)return unsigned(v);
    return nrredecada::cam::par.load(std::memory_order_relaxed)>1?cada_quadros_auto:0u;
}

inline std::atomic<bool> acumula{false};

inline std::atomic<unsigned> espera_us{0};
inline constexpr unsigned espera_auto=0xffffffffu;

inline std::atomic<bool> espera_job{false};

inline std::atomic<bool> thread_rede{false};
inline std::atomic<float> acum_baixa{.15f},acum_alta{.4f},acum_gama{1.f},acum_sigma{6.f};

inline std::atomic<float> pula_limiar{0.f};
inline std::atomic<unsigned> pula_max{2};

inline std::atomic<unsigned> passos{1};
inline std::atomic<float> passo2_forca{.5f};

inline std::atomic<float> passo1_forca{1.f};

inline constexpr unsigned espera_present=0xfffffffeu;

inline std::atomic<float> frescor{0.f};
inline std::atomic<bool> frescor_pronto{false};
inline std::atomic<std::uint64_t> presents_fim{0};

inline std::atomic<unsigned> passo2_cada{1};
inline std::atomic<float> rede_alvo{0.f};
inline std::atomic<bool> compor_elipse{false};

inline std::atomic<bool> compor_rapido{false};

inline std::atomic<unsigned> segura{0};

inline std::atomic<bool> segura_pixel{false};
inline std::atomic<unsigned> ablacao{0};

inline std::atomic<float> baixa_forca{1.f},baixa_lenta{0.f},baixa_rapida{0.f},baixa_mov_px{4.f},acum_limiar{0.f};

inline std::atomic<unsigned> borda_suave{0};

inline std::atomic<float> borda_largura{0.f};

inline std::atomic<unsigned> borda_camada{0};
inline std::atomic<float> borda_camada_reg{.05f};
inline std::atomic<bool> borda_camada_perto{false};

inline std::atomic<float> borda_camada_z0{1.15f},borda_camada_z1{1.45f};

inline std::atomic<float> borda_teto{0.f};

inline unsigned teto_jobs() noexcept {
    unsigned c=cada.load(std::memory_order_relaxed);
    if(pula_limiar.load(std::memory_order_relaxed)>0.f)c=(std::max)(c,1u+pula_max.load(std::memory_order_relaxed));
    return c+(fora.load(std::memory_order_relaxed)?1u:0u);
}

inline unsigned rede_de(unsigned L,unsigned t) noexcept {if(L>=t)return t;if(zoom.load(std::memory_order_relaxed)&8u)return (std::min)(t,2u*L)&~127u;return L;}

inline constexpr unsigned n_max=1024,n_min=256;
inline constexpr unsigned slots=6;

inline unsigned rede_por_cabeca(unsigned L,unsigned t,float cabeca,float alvo,unsigned ant) noexcept {
    const unsigned teto_n=(std::min)(L,t)&~127u;
    if(!(alvo>0.f)||!(cabeca>0.f)||teto_n<n_min)return L<=t?L:t;
    const float alvo_n=float(L)*alvo/cabeca;
    unsigned n=unsigned(std::lround(alvo_n/128.f))*128u;
    n=std::clamp(n,n_min,teto_n);
    if(ant>=n_min&&ant<=teto_n&&std::fabs(alvo_n-float(ant))<96.f)n=ant;
    return n;
}
inline constexpr std::uint64_t bytes_meio=std::uint64_t(n_max)*n_max*8u;
inline constexpr std::uint64_t bytes_slot=2*bytes_meio;
inline constexpr std::uint64_t bytes_dados=std::uint64_t(slots)*bytes_slot;
inline constexpr std::uint64_t bytes_total=bytes_dados+4096;
inline std::uint64_t offset_ent(unsigned s) noexcept {return std::uint64_t(s)*bytes_slot;}
inline std::uint64_t offset_sai(unsigned s) noexcept {return offset_ent(s)+bytes_meio;}
inline std::uint64_t offset_tag(unsigned s) noexcept {return bytes_dados+std::uint64_t(s)*16u;}
enum Estado:unsigned{Livre=0,Gravado=1,Escrito=2,Avaliando=3,Pronto=4};
inline std::uint64_t palavra(Estado e,int job) noexcept {return (std::uint64_t(unsigned(job))<<8)|unsigned(e);}
inline Estado estado_de(std::uint64_t p) noexcept {return Estado(p&255u);}
inline int job_de(std::uint64_t p) noexcept {return int(unsigned(p>>8));}
inline std::uint64_t palavra_g(unsigned e,int job) noexcept {return (std::uint64_t(unsigned(job))<<8)|e;}
struct Slot {
    std::atomic<std::uint64_t> palavra{0};

    std::uint32_t seq{};
    std::uint64_t geracao{};
    int x{},y{};unsigned L{},n{},ow{},oh{};
    float hx{},hy{},hl{};
    float nivel{};
};
struct Compartilhado {
    Slot slot[slots];
    std::atomic<void*> handle{nullptr};std::atomic<std::uint64_t> bytes{0};

    std::atomic<const volatile std::uint32_t*> marcador{nullptr};
    std::atomic<bool> envenenado{false};
    std::atomic<unsigned long long> avaliacoes{0},prontos{0},pulados_marcador{0},composicoes{0},recortes{0};
    std::uint32_t marcado(unsigned s)const{auto* m=marcador.load(std::memory_order_acquire);return m?m[s]:0u;}
};
inline Compartilhado& global(){static auto* g=new Compartilhado;return *g;}

inline int slot_do_job(int job) noexcept {
    if(job<=0)return -1;auto& g=global();
    for(unsigned s=0;s<slots;++s){const auto p=g.slot[s].palavra.load(std::memory_order_acquire);
        if(estado_de(p)>=Escrito&&job_de(p)==job)return int(s);}
    return -1;
}

inline int slot_anterior(int job) noexcept {
    auto& g=global();int melhor=-1,jm=0;
    for(unsigned s=0;s<slots;++s){const auto p=g.slot[s].palavra.load(std::memory_order_acquire);
        if(estado_de(p)==Pronto&&job_de(p)<job&&job_de(p)>jm){jm=job_de(p);melhor=int(s);}}
    return melhor;
}

inline int slot_para_recorte() noexcept {
    auto& g=global();int livre=-1,velho=-1;int jobs[slots]{};int js_n=0;
    for(unsigned s=0;s<slots;++s){const auto p=g.slot[s].palavra.load(std::memory_order_acquire);
        if(estado_de(p)==Livre||estado_de(p)==Gravado){if(livre<0)livre=int(s);}
        else if(estado_de(p)==Pronto)jobs[js_n++]=job_de(p);}
    if(livre>=0)return livre;
    std::sort(jobs,jobs+js_n);
    const int protegido=js_n>=3?jobs[js_n-3]:(js_n?jobs[0]:0x7fffffff);
    for(unsigned s=0;s<slots;++s){const auto p=g.slot[s].palavra.load(std::memory_order_acquire);
        if(estado_de(p)==Pronto&&job_de(p)<protegido&&(velho<0||job_de(p)<job_de(g.slot[velho].palavra.load())))velho=int(s);}
    return velho;
}

inline constexpr unsigned grade_slots=3,grade_h_max=720,grade_w_max=2560;
inline constexpr std::uint64_t grade_cap=std::uint64_t(grade_w_max)*grade_h_max;
inline constexpr std::uint64_t grade_bytes_slot=grade_cap*8u;
inline constexpr std::uint64_t grade_bytes=grade_bytes_slot*grade_slots;
inline std::uint64_t grade_off_d(unsigned s) noexcept {return std::uint64_t(s)*grade_bytes_slot;}
inline std::uint64_t grade_off_mv(unsigned s) noexcept {return grade_off_d(s)+grade_cap*4u;}
inline constexpr unsigned grade_marca0=8;
enum GradeEstado:unsigned{GLivre=0,GGravado=1,GEscrito=2,GLendo=3};
struct GradeSlot {std::atomic<std::uint64_t> palavra{0};std::uint32_t seq{};int w{},h{};};
struct Grade {
    GradeSlot slot[grade_slots];
    std::atomic<void*> handle{nullptr};std::atomic<bool> quer{false};

    std::atomic<int> invertida{1};
    std::atomic<unsigned long long> gravadas{0},usadas{0},sem_slot{0};
};
inline Grade& grade(){static auto* g=new Grade;return *g;}

inline int grade_slot_livre() noexcept {
    auto& g=grade();int melhor=-1;std::uint32_t seq=0;
    for(unsigned s=0;s<grade_slots;++s){const auto p=g.slot[s].palavra.load(std::memory_order_acquire);const auto e=GradeEstado(p&255u);
        if(e==GLivre)return int(s);
        if(e==GGravado&&(melhor<0||g.slot[s].seq<seq)){melhor=int(s);seq=g.slot[s].seq;}}
    return melhor;
}

inline std::atomic<float> cam_near{0.f},cam_far{0.f},cam_fov{0.f};

struct Deteccao {
    std::uint64_t versao{};bool on{};bool tem{};
    float cx{},cy{},lado{};
    float vx{},vy{};
    int rx{},ry{};unsigned rl{};
    std::uint64_t quadro{};
    float conf{},cab4k{};unsigned motivo{};
};
struct Publicada {std::mutex m;Deteccao d;};
inline Publicada& publicada(){static auto* p=new Publicada;return *p;}
inline void publicar(const Deteccao& d){auto& p=publicada();std::lock_guard<std::mutex> l(p.m);const auto v=p.d.versao+1;p.d=d;p.d.versao=v;}
inline Deteccao ler_deteccao(){auto& p=publicada();std::lock_guard<std::mutex> l(p.m);return p.d;}

inline std::atomic<std::uint64_t> geracao{1};
inline void cortar() noexcept {geracao.fetch_add(1,std::memory_order_acq_rel);}

struct Recorte {int x{},y{};unsigned L{};bool vazio()const{return L<n_min;}};

inline std::atomic<float> recorte_fator{0.f};
inline std::atomic<unsigned> grade_rede{0};
inline Recorte recorte_saida(const Deteccao& d,unsigned ow,unsigned oh,float fator,unsigned grade,unsigned teto_rede) noexcept {
    Recorte r{};if(!d.tem||!ow||!oh)return r;
    const unsigned lmax=(zoom.load(std::memory_order_relaxed)&1u)?zoom_lado:n_max;
    unsigned L=(std::min<unsigned>)({d.rl,lmax,ow,oh})&~127u;if(L<n_min)return r;
    long x=d.rx,y=d.ry;
    if(fator>0.f&&d.lado>0.f){
        const unsigned alvo=unsigned(std::ceil(double(fator)*double(d.lado)/128.0))*128u;
        const double cx=double(x)+L*.5,cy=double(y)+L*.5,folga=alvo/16.0;
        if(alvo>=n_min&&alvo<L&&std::fabs(double(d.cx)-cx)+d.lado*.5<=alvo*.5-folga&&std::fabs(double(d.cy)-cy)+d.lado*.5<=alvo*.5-folga){
            x=std::lround(cx-alvo*.5);y=std::lround(cy-alvo*.5);L=alvo;}
    }
    if(grade){
        const unsigned n=rede_de(L,teto_rede);
        const long G=(std::max)(1L,long(std::lround(double(grade)*double(L)/double(n?n:L))));
        x=std::lround(double(x)/double(G))*G;y=std::lround(double(y)/double(G))*G;
    }
    x=std::clamp<long>(x,0,long(ow)-long(L));y=std::clamp<long>(y,0,long(oh)-long(L));
    r.x=int(x);r.y=int(y);r.L=L;return r;
}
inline Recorte recorte_saida(const Deteccao& d,unsigned ow,unsigned oh) noexcept {return recorte_saida(d,ow,oh,0.f,0u,0u);}

inline constexpr float rampa_quadros=6.f;
struct Rampa {
    float nivel{};std::uint64_t geracao{};
    void passo(bool alvo,bool composto,std::uint64_t ger) noexcept {
        if(ger!=geracao){geracao=ger;nivel=0.f;}
        if(alvo){if(composto)nivel=(std::min)(1.f,nivel+1.f/rampa_quadros);}
        else nivel=(std::max)(0.f,nivel-1.f/rampa_quadros);
    }
};

struct QuadroRender {int on{-1};float nivel{},ganho{};float cx{},cy{},lado{};int recorte_x{},recorte_y{},recorte_l{};int composto{};int job{};};
inline QuadroRender& quadro_render(){static QuadroRender q;return q;}

inline void log(const char* linha) noexcept {
    (void)linha;
}
inline void envenenar(const char* motivo) noexcept {
    if(!global().envenenado.exchange(true)){char t[256]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_DESLIGADO motivo=%s (sessao inteira; a cena segue igual)",motivo?motivo:"?");log(t);}
}
inline bool ativo() noexcept {return ligado.load(std::memory_order_relaxed)&&!global().envenenado.load(std::memory_order_relaxed);}
}
