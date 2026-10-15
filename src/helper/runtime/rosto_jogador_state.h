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
#include "orcamento.h"
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
#include <vector>

namespace nrrj {

inline constexpr int npc_slots=7;
inline constexpr int n_inst=1+npc_slots;
inline thread_local int inst_tl=0;
inline int inst_atual() noexcept {return inst_tl;}
struct Inst {int ant;explicit Inst(int i) noexcept:ant(inst_tl){inst_tl=i;}~Inst(){inst_tl=ant;}Inst(const Inst&)=delete;Inst& operator=(const Inst&)=delete;};
inline std::atomic<bool> npc2{false};
inline std::atomic<int> npc_max{npc_slots};
inline std::atomic<int> caminha_dela{0};
inline std::atomic<int> reancora{0};
inline std::atomic<int> adquire{0};
inline std::atomic<int> npc_degraus{0};
inline std::atomic<int> npc_recorte_min2{0};
inline std::atomic<float> npc_fracao{0.f};
inline std::atomic<int> npc_amplia{0};
inline std::atomic<float> npc_limiar{0.f};
inline constexpr unsigned npc_rede_min=384;
inline bool npc_amplia_inst() noexcept {return inst_atual()>=1&&npc_amplia.load(std::memory_order_relaxed)&&npc_fracao.load(std::memory_order_relaxed)>0.f;}
inline std::atomic<int> npc_recorte_forcado{0};
inline std::atomic<float> npc_segura_peq{0.f};
inline std::atomic<int> npc_frente{0};
inline std::atomic<float> npc_conf_s{0.f};
inline std::atomic<int> npc_vel_img{0};
inline std::atomic<int> npc_atras{0};
inline std::atomic<int> npc_camada{0};
inline std::atomic<int> npc_vida{0};

inline std::atomic<int> npc_leve{0};
inline std::atomic<float> npc_leve_fracao{.33f},npc_leve_salto{.6f};
inline std::atomic<unsigned> npc_leve_recorte{1024},npc_leve_jan{1024},npc_leve_rede{512};

inline std::atomic<int> npc_canon{0};
inline std::atomic<float> npc_canon_fracao{.36f},npc_canon_razao{2.35f},npc_canon_corte{.6f},npc_canon_beta{4.f};
inline std::atomic<unsigned> npc_canon_rede{384};
inline bool canon_inst() noexcept {return inst_atual()>=1&&npc_canon.load(std::memory_order_relaxed)!=0;}

inline std::atomic<int> npc_canon_perto{2};inline std::atomic<unsigned> npc_canon_jan_peq{768};
inline bool canon2_inst() noexcept {return inst_atual()>=1&&npc_canon.load(std::memory_order_relaxed)==2;}
inline unsigned canon2_rede(unsigned L,unsigned t,unsigned ult) noexcept {
    static constexpr unsigned st[4]={384u,512u,768u,1024u};unsigned alvo=1024u;
    for(unsigned s:st)if(1.3*s>=double(L)){alvo=s;break;}
    int iu=-1;for(int i=0;i<4;++i)if(st[i]==ult)iu=i;
    if(iu>=0&&ult<=t&&double(L)<=1.42*ult&&(iu==0||double(L)>1.18*st[iu-1]))alvo=ult;
    unsigned n=384u;for(unsigned s:st)if(s<=alvo&&s<=(std::max)(t,384u))n=s;return n;}

inline std::atomic<int> npc_todos{0},npc_todos_grandes{2};inline std::atomic<float> npc_todos_nota{.7f};
inline bool todos_inst() noexcept {return inst_atual()>=1&&(npc_todos.load(std::memory_order_relaxed)&16)!=0;}

inline unsigned todos_rede(unsigned L,unsigned teto,unsigned ult) noexcept {
    unsigned n;
    if(teto>=768u)n=(L>=384u||(ult==768u&&L>=340u))?768u:(L>=256u?512u:384u);
    else n=(L>511u||(ult==512u&&L>=450u))?512u:384u;
    return (std::min)(n,(std::max)(teto,384u));}
inline std::atomic<float> npc_vida_in{.35f};
inline std::atomic<int> npc_corpo{0};
inline std::atomic<float> npc_cab_min{96.f};
inline std::atomic<int> npc_roi_conf{0};
inline std::atomic<int> giro_inteira{0};
inline std::atomic<int> npc_peito{0};
inline std::atomic<int> yunet_rapido{0};
inline std::atomic<int> yunet_simd{0};
inline std::atomic<int> detector{0};
inline std::atomic<int> detector_threads{2};

inline std::atomic<float> scrfd_b_inteira{.6f},scrfd_b_faixa{.5f};
inline float scrfd_a(bool faixa) noexcept {return (std::min)(.3f,(faixa?scrfd_b_faixa:scrfd_b_inteira).load(std::memory_order_relaxed)-.1f);}
inline float nota_scrfd(float s,bool faixa) noexcept {
    const float a=scrfd_a(faixa),b=(faixa?scrfd_b_faixa:scrfd_b_inteira).load(std::memory_order_relaxed);
    if(s<=a)return s*(.4f/a);if(s<=b)return .4f+(s-a)*(.3f/(b-a));return (std::min)(1.f,.7f+(s-b)*(.3f/(1.f-b)));
}
inline std::atomic<unsigned> yunet_faixa{0};
inline std::atomic<unsigned> npc2_teto{1024};
inline std::atomic<float> npc2_segura_s{1.f};

inline std::atomic<bool> npc2_acumula{true};
inline std::atomic<float> npc2_vel_max{0.f};

inline bool temporal_inst() noexcept {return inst_atual()==0||npc2_acumula.load(std::memory_order_relaxed);}

inline std::atomic<bool> npc2_frescor{false};
inline bool frescor_inst() noexcept {return inst_atual()==0||npc2_frescor.load(std::memory_order_relaxed);}

inline std::atomic<bool> ligado{false};
inline std::atomic<bool> menu_escreveu{false};
inline std::atomic<bool> ini_lido{false};

inline bool valor_de(const wchar_t* t) noexcept {return t&&t[0]>=L'1'&&t[0]<=L'9';}
inline constexpr unsigned teto_padrao=1024;
inline unsigned teto_de(const wchar_t* t) noexcept {
    if(!t)return teto_padrao;const long v=std::wcstol(t,nullptr,10);
    return v>=256&&v<=2048&&v%128==0?unsigned(v):teto_padrao;
}
inline std::atomic<unsigned> teto{teto_padrao};

inline unsigned teto_inst() noexcept {
    if(inst_atual()>=1){const unsigned t=npc2_teto.load(std::memory_order_relaxed);
        return (npc_leve.load(std::memory_order_relaxed)&1)?(std::min)(t,npc_leve_rede.load(std::memory_order_relaxed)):t;}
    return teto.load(std::memory_order_relaxed);}

inline bool leve_npc() noexcept {return inst_atual()>=1&&npc_leve.load(std::memory_order_relaxed)!=0;}

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

inline std::atomic<bool> npc{false};
inline std::atomic<int> npc_modo{0};
inline std::atomic<int> npc_min{160},npc_recorte{512};
inline std::atomic<float> npc_forca{2.f},npc_expande{.8f};
inline std::atomic<float> npc_segura_s{1.5f};
inline std::atomic<bool> alvo_npc{false};

inline float forca_passo1() noexcept {
    if(alvo_npc.load(std::memory_order_relaxed)&&npc_modo.load(std::memory_order_relaxed)==2)return npc_forca.load(std::memory_order_relaxed);
    return passo1_forca.load(std::memory_order_relaxed);
}

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

inline unsigned rede_de(unsigned L,unsigned t) noexcept {if(canon_inst())return (std::min)(t,npc_canon_rede.load(std::memory_order_relaxed));
    if(L>=t)return t;if(npc_amplia_inst()&&L<npc_rede_min)return (std::min)(t,npc_rede_min);if(zoom.load(std::memory_order_relaxed)&8u)return (std::min)(t,2u*L)&~127u;return L;}

inline constexpr unsigned n_max=2048,n_min=256;
inline constexpr unsigned lado_sem_zoom=1024;
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

inline unsigned n_cap() noexcept {
    if(nrorc::aplicada()){const auto& d=nrorc::decisao();return inst_atual()>=1?(inst_atual()<=d.k_perto?d.cap_perto:d.cap_npc):d.cap_grace;}
    return inst_atual()>=1?npc2_teto.load(std::memory_order_relaxed):n_max;}
inline std::uint64_t bytes_meio_i() noexcept {return std::uint64_t(n_cap())*n_cap()*8u;}
inline std::uint64_t bytes_dados_i() noexcept {return std::uint64_t(slots)*2u*bytes_meio_i();}
inline std::uint64_t bytes_total_i() noexcept {return bytes_dados_i()+4096;}
inline std::uint64_t offset_ent(unsigned s) noexcept {return std::uint64_t(s)*2u*bytes_meio_i();}
inline std::uint64_t offset_sai(unsigned s) noexcept {return offset_ent(s)+bytes_meio_i();}
inline std::uint64_t offset_tag(unsigned s) noexcept {return bytes_dados_i()+std::uint64_t(s)*16u;}
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
    float zx{},zy{};
    float nivel{};
    std::uint64_t quadro{};
};
struct Compartilhado {
    Slot slot[slots];
    std::atomic<void*> handle{nullptr};std::atomic<std::uint64_t> bytes{0};
    std::atomic<std::uint64_t> offset{0};

    std::atomic<const volatile std::uint32_t*> marcador{nullptr};
    std::atomic<bool> envenenado{false};
    std::atomic<unsigned long long> avaliacoes{0},prontos{0},pulados_marcador{0},composicoes{0},recortes{0};
    std::uint32_t marcado(unsigned s)const{auto* m=marcador.load(std::memory_order_acquire);return m?m[s]:0u;}
};
inline Compartilhado& global(){static auto* g=new Compartilhado[n_inst];return g[inst_atual()];}

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

inline std::atomic<unsigned> sem_job_q{0};
inline unsigned sj_q() noexcept {return sem_job_q.load(std::memory_order_relaxed);}
inline unsigned sj_q_de(int sem_job,int cada_q) noexcept {if(sem_job!=1)return 0u;return cada_q>=1?unsigned((std::min)(cada_q,16)):2u;}
inline constexpr int id_quadro_base=1<<30;
inline int id_de_quadro(std::uint64_t f) noexcept {return int(unsigned(id_quadro_base)|unsigned(f&0x3fffffffu));}
inline bool id_e_quadro(int id) noexcept {return id>=id_quadro_base;}

inline bool sj_devido(std::uint64_t quadro,std::uint64_t quadro_ult,bool ger_nova,unsigned Q) noexcept {
    return ger_nova||quadro_ult==0||quadro>=quadro_ult+(std::max)(Q,1u);}

inline constexpr unsigned caminho_passos=16,caminho_bytes=caminho_passos*16u,caminho_anel=32;
inline constexpr std::uint64_t caminho_total=std::uint64_t(caminho_bytes)*caminho_anel;
struct CaminhoEnt {float dx,dy;std::uint32_t ok,quadro;};
static_assert(sizeof(CaminhoEnt)==16);

inline int sj_desloc(const CaminhoEnt* t,int k,float& dx,float& dy) noexcept {
    dx=dy=0.f;if(k<0||k>=int(caminho_passos))return -1;if(k==0)return 0;
    if(t[k].ok>=1u&&t[k].ok<=5u&&std::isfinite(t[k].dx)&&std::isfinite(t[k].dy)){dx=t[k].dx;dy=t[k].dy;return 0;}
    for(int j=k-1;j>=1;--j)if(t[j].ok>=1u&&t[j].ok<=5u&&std::isfinite(t[j].dx)&&std::isfinite(t[j].dy)){const float f=float(k)/float(j);dx=t[j].dx*f;dy=t[j].dy*f;return 1;}
    return 2;
}

struct SjVista {std::uint64_t palavra{},quadro{},geracao{};};
inline int sj_escolher(const SjVista* v,unsigned n,std::uint64_t ger,std::uint64_t agora,unsigned hist,int abaixo) noexcept {
    int melhor=-1;std::uint64_t qm=0;
    for(unsigned s=0;s<n;++s){const int id=job_de(v[s].palavra);
        if(estado_de(v[s].palavra)!=Pronto||!id_e_quadro(id)||v[s].geracao!=ger||!v[s].quadro||v[s].quadro>agora||agora-v[s].quadro>=hist)continue;
        if(abaixo>0&&id>=abaixo)continue;
        if(melhor<0||v[s].quadro>qm){melhor=int(s);qm=v[s].quadro;}}
    return melhor;
}

inline int sj_pendente(const SjVista* v,unsigned n,int* velhos,unsigned& nv) noexcept {
    int melhor=-1;std::uint64_t qm=0;nv=0;
    for(unsigned s=0;s<n;++s){if(estado_de(v[s].palavra)!=Escrito||!id_e_quadro(job_de(v[s].palavra)))continue;
        if(melhor<0||v[s].quadro>qm){if(melhor>=0)velhos[nv++]=melhor;melhor=int(s);qm=v[s].quadro;}else velhos[nv++]=int(s);}
    return melhor;
}

inline constexpr int sj_fora=0x7fffffff;
inline int sj_base_k(std::uint64_t F,std::uint64_t Fj) noexcept {
    if(!F||!Fj)return sj_fora;if(Fj<=F)return F-Fj<caminho_passos?int(F-Fj):sj_fora;return Fj-F<caminho_passos?-int(Fj-F):sj_fora;}

inline int sj_desloc_par(const CaminhoEnt* anel,std::uint64_t F,std::uint64_t Fj,float& dx,float& dy) noexcept {
    dx=dy=0.f;const int k=sj_base_k(F,Fj);if(k==sj_fora)return -1;if(k==0)return 0;
    const std::uint64_t f=k>0?F:Fj;const CaminhoEnt* t=anel?anel+unsigned(f%caminho_anel)*caminho_passos:nullptr;
    if(!t||t[0].quadro!=std::uint32_t(f))return 3;
    const int r=sj_desloc(t,k>0?k:-k,dx,dy);if(k<0){dx=-dx;dy=-dy;}return r;}

struct JobQuadro {std::atomic<int> job{0};std::atomic<std::uint64_t> quadro{0};};
inline JobQuadro job_quadro_v[64];
inline void sj_marcar_job(int job,std::uint64_t q) noexcept {
    if(job<=0)return;auto& e=job_quadro_v[unsigned(job)%64u];e.job.store(0,std::memory_order_relaxed);e.quadro.store(q,std::memory_order_release);e.job.store(job,std::memory_order_release);}
inline std::uint64_t sj_quadro_do_job(int job) noexcept {
    if(job<=0)return 0;const auto& e=job_quadro_v[unsigned(job)%64u];if(e.job.load(std::memory_order_acquire)!=job)return 0;
    const auto q=e.quadro.load(std::memory_order_acquire);return e.job.load(std::memory_order_acquire)==job?q:0;}

inline std::atomic<int> sj_job_aceito{0},sj_job_publicado{0};
inline bool sj_cena_pendente(int aceito,int publicado) noexcept {return aceito>0&&aceito>publicado;}

inline std::atomic<void(*)()> acordar_rede{nullptr};
struct SjConta {std::atomic<unsigned long long> recortes{0},avaliados{0},velhos{0},futuro{0},sem_pub{0},estendido{0},zero{0},marcador_tarde{0},intercalados{0},sem_tabela{0},tabela_tarde{0},adiados{0};};
inline SjConta& sj_conta(){static auto* c=new SjConta;return *c;}

inline std::atomic<const CaminhoEnt*> caminho_rb{nullptr};

inline constexpr unsigned grade_slots=3,grade_h_max=720,grade_w_max=2560;
inline constexpr std::uint64_t grade_cap=std::uint64_t(grade_w_max)*grade_h_max;
inline constexpr std::uint64_t grade_bytes_slot=grade_cap*8u;
inline constexpr std::uint64_t grade_bytes=grade_bytes_slot*grade_slots;
inline std::uint64_t grade_off_d(unsigned s) noexcept {return std::uint64_t(s)*grade_bytes_slot;}
inline std::uint64_t grade_off_mv(unsigned s) noexcept {return grade_off_d(s)+grade_cap*4u;}
inline constexpr unsigned cab_n=48,cab_aneis=8,cab_marca0=56;
inline constexpr std::uint64_t cab_bytes=std::uint64_t(cab_n)*cab_n*4u;
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
    float zela{};
    float zdx{},zdy{};
    bool pend{};
    bool camq{};
};
struct Publicada {std::mutex m;Deteccao d;};
inline Publicada& publicada(){static auto* p=new Publicada[n_inst];return p[inst_atual()];}
inline void publicar(const Deteccao& d){auto& p=publicada();std::lock_guard<std::mutex> l(p.m);const auto v=p.d.versao+1;p.d=d;p.d.versao=v;}
inline Deteccao ler_deteccao(){auto& p=publicada();std::lock_guard<std::mutex> l(p.m);return p.d;}

inline std::atomic<std::uint64_t> geracao_v[n_inst]{1,1,1,1,1,1,1,1};
inline std::atomic<std::uint64_t>& ger() noexcept {return geracao_v[inst_atual()];}
inline void cortar() noexcept {ger().fetch_add(1,std::memory_order_acq_rel);}

inline std::atomic<std::uint64_t> geracao_sem_rampa_v[n_inst]{};
inline std::atomic<std::uint64_t>& geracao_sem_rampa() noexcept {return geracao_sem_rampa_v[inst_atual()];}
inline void cortar_troca_alvo() noexcept {
    auto& G=ger();auto g=G.load(std::memory_order_acquire);
    do{geracao_sem_rampa().store(g+1,std::memory_order_release);}while(!G.compare_exchange_weak(g,g+1,std::memory_order_acq_rel));
}

struct Recorte {int x{},y{};unsigned L{};bool vazio()const{return L<n_min;}};

inline std::atomic<float> recorte_fator{0.f};
inline std::atomic<unsigned> grade_rede{0};
inline Recorte recorte_saida(const Deteccao& d,unsigned ow,unsigned oh,float fator,unsigned grade,unsigned teto_rede) noexcept {
    Recorte r{};if(!d.tem||!ow||!oh)return r;
    const unsigned lmax=(zoom.load(std::memory_order_relaxed)&1u)?zoom_lado:lado_sem_zoom;
    unsigned L=(std::min<unsigned>)({d.rl,lmax,ow,oh})&(canon_inst()?~3u:npc_amplia_inst()?~31u:~127u);if(L<n_min)return r;
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

inline std::atomic<unsigned> rosto_hz{10};
inline std::atomic<float> rosto_nota{.7f};
inline constexpr unsigned rosto_w_max=640,rosto_h_max=360,rosto_aneis=3,rosto_marca0=16;

inline constexpr unsigned faixa_w_max=1920,faixa_h_max=480,inteira_w_max=960,inteira_h_max=540;
inline constexpr std::uint64_t rosto_bytes=(std::max)((std::max)(std::uint64_t(rosto_w_max)*rosto_h_max,std::uint64_t(faixa_w_max)*faixa_h_max),std::uint64_t(inteira_w_max)*inteira_h_max)*8u;
inline std::atomic<unsigned> yunet_inteira{0};
inline constexpr unsigned faixa_w=960,faixa_h=240;
static_assert(std::uint64_t(faixa_w)*faixa_h*8u<=rosto_bytes,"faixa cabe no anel");
static_assert(rosto_marca0+rosto_aneis<=64,"marcadores no readback de 256 B");
struct RostoImgEnt {std::atomic<std::uint32_t> seq{0};int w{},h{},ow{},oh{};int x0{},y0{},rw{},rh{};std::uint64_t quadro{};double t{};};
struct RostoImg {RostoImgEnt ent[rosto_aneis];std::atomic<const unsigned char*> base{nullptr};std::atomic<unsigned long long> gravadas{0};};
inline RostoImg& rosto_img(){static auto* r=new RostoImg;return *r;}
struct RostoCaixa {float x,y,w,h,nota;float lm[10]{};float px=0.f;};
struct Rostos {std::mutex m;std::uint64_t versao{},quadro{};double t{-1.0};std::vector<RostoCaixa> lista;double ms{};unsigned long long rodadas{};};
inline Rostos& rostos(){static auto* r=new Rostos;return *r;}

inline constexpr unsigned roi_lado=192,roi_aneis=3,roi_marca0=rosto_marca0+rosto_aneis,roi_hz=30;
inline constexpr float roi_fator=2.2f;
inline constexpr std::uint64_t roi_bytes=std::uint64_t(roi_lado)*roi_lado*8u;

struct PontosEnt {std::atomic<std::uint32_t> seq{0};int x0{},y0{},l{};};
struct Pontos {PontosEnt ent[slots];std::atomic<const unsigned char*> base{nullptr};std::atomic<unsigned long long> rodadas{0},achou{0},us{0};};
inline Pontos& pontos(){static auto* p=new Pontos;return *p;}
static_assert(roi_marca0+npc_slots*roi_aneis<=64,"marcadores no readback de 256 B");
struct RoiEnt {std::atomic<std::uint32_t> seq{0};int x0{},y0{},l{};std::uint64_t quadro{};double t{};};
struct RoiImg {RoiEnt ent[roi_aneis];std::atomic<const unsigned char*> base{nullptr};};
inline RoiImg& roi_img(int k){static auto* r=new RoiImg[npc_slots];return r[k];}
inline Rostos& rostos_roi(int k){static auto* r=new Rostos[npc_slots];return r[k];}

inline constexpr float rampa_quadros=6.f;
struct Rampa {
    float nivel{};std::uint64_t geracao{};
    void passo(bool alvo,bool composto,std::uint64_t ger) noexcept {
        if(ger!=geracao){geracao=ger;nivel=0.f;
            if(ger==geracao_sem_rampa().load(std::memory_order_acquire))nivel=1.f;
        }

        if(alvo){if(composto)nivel=1.f;}
        else nivel=(std::max)(0.f,nivel-1.f/rampa_quadros);
    }
};

struct QuadroRender {int on{-1};float nivel{},ganho{};float cx{},cy{},lado{};int recorte_x{},recorte_y{},recorte_l{};int composto{};int job{};
    int idade{-1};};
inline QuadroRender& quadro_render(){static QuadroRender q[n_inst];return q[inst_atual()];}

inline void log(const char* linha) noexcept {
    (void)linha;
}
inline void envenenar(const char* motivo) noexcept {
    if(!global().envenenado.exchange(true)){char t[256]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_DESLIGADO motivo=%s (sessao inteira; a cena segue igual)",motivo?motivo:"?");log(t);}
}
inline int npc_n() noexcept {if(!npc2.load(std::memory_order_relaxed))return 0;const int n=std::clamp(npc_max.load(std::memory_order_relaxed),1,npc_slots);
    return nrorc::aplicada()?(std::min)(n,nrorc::decisao().k_npc):n;}
inline bool ativo() noexcept {return ligado.load(std::memory_order_relaxed)&&!global().envenenado.load(std::memory_order_relaxed)&&(inst_atual()==0||inst_atual()<=npc_n());}
inline bool npc2_ativo() noexcept {return npc2.load(std::memory_order_relaxed)&&ligado.load(std::memory_order_relaxed);}

inline nrorc::Config config_orcamento() noexcept {
    nrorc::Config c{};
    c.teto_grace=teto.load(std::memory_order_relaxed);c.janela_grace=nrorc::janela_grace_padrao;c.janela_npc=nrorc::janela_npc_padrao;
    if(npc_leve.load(std::memory_order_relaxed)&1){c.leve=true;c.janela_npc=npc_leve_jan.load(std::memory_order_relaxed);c.leve_rede=npc_leve_rede.load(std::memory_order_relaxed);}
    if(npc_canon.load(std::memory_order_relaxed)==2){c.perto=npc_canon_perto.load(std::memory_order_relaxed);c.jan_peq=npc_canon_jan_peq.load(std::memory_order_relaxed);}
    if(npc_todos.load(std::memory_order_relaxed)&16){c.todos=true;c.perto=0;c.todos_max=npc_todos_grandes.load(std::memory_order_relaxed);}
    c.npc=npc2.load(std::memory_order_relaxed);c.npc_max=c.npc?std::clamp(npc_max.load(std::memory_order_relaxed),1,npc_slots):0;
    c.teto_npc=npc2_teto.load(std::memory_order_relaxed);
    c.acumula=acumula.load(std::memory_order_relaxed);c.npc_acumula=npc2_acumula.load(std::memory_order_relaxed);
    c.frescor=frescor.load(std::memory_order_relaxed)>0.f;c.passos=passos.load(std::memory_order_relaxed);
    c.ctx=nrorc::contexto_medido;c.ctx_unico=true;
    c.fixo=(npc_modo.load(std::memory_order_relaxed)==1||c.npc)?rosto_bytes:std::uint64_t{0};
    return c;}

inline void aplicar_decisao(const nrorc::Config& c,const nrorc::Medida& m,const nrorc::Decisao& d) noexcept {
    if(nrorc::modo()==1){teto.store(d.cap_grace,std::memory_order_relaxed);if(d.k_npc>0)npc2_teto.store(d.cap_npc,std::memory_order_relaxed);}
    nrorc::publicar(c,m,d);}
}
