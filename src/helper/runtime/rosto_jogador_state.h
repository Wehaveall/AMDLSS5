#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "pasta_helper.h"
#include "log_fila.h"
#include "nr_publico.h"
#include "rosto_pose_barata.h"
#include "rede_cada.h"
#include "orcamento.h"
#include "rosto_agenda.h"
#include "gpu_perdida.h"
#include "rosto_jogador/acum_modo.h"
#include "cabeca_segura.h"
#include "../source/implementation/async-reprojection/area_rosto.hlsli"
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
#include <condition_variable>
#include <stdexcept>
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
inline std::atomic<int> npc_recorte_min2{0};
inline std::atomic<float> npc_fracao{0.f};
inline std::atomic<int> npc_amplia{0};
inline std::atomic<float> npc_limiar{0.f};
inline constexpr unsigned npc_rede_min=384;
inline bool npc_amplia_inst() noexcept {return inst_atual()>=1&&npc_amplia.load(std::memory_order_relaxed)&&npc_fracao.load(std::memory_order_relaxed)>0.f;}
inline std::atomic<int> npc_recorte_forcado{0};
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

inline std::atomic<int> detector_threads{2};
inline bool detector_cpu_ok(int simd) noexcept {return simd>=1;}
inline std::atomic<bool> detector_avx2{false};

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

inline std::atomic<bool> teto_tela_lig{true};
inline unsigned teto_tela(unsigned ow,unsigned oh) noexcept {
    const unsigned m=(std::min)(ow,oh)&~127u;return teto_tela_lig.load(std::memory_order_relaxed)&&m>=256u?m:~0u;}

inline unsigned teto_inst() noexcept {
    if(inst_atual()>=1){const unsigned t=npc2_teto.load(std::memory_order_relaxed);
        return (npc_leve.load(std::memory_order_relaxed)&1)?(std::min)(t,npc_leve_rede.load(std::memory_order_relaxed)):t;}
    return teto.load(std::memory_order_relaxed);}

inline bool leve_npc() noexcept {return inst_atual()>=1&&npc_leve.load(std::memory_order_relaxed)!=0;}

inline std::atomic<unsigned> cada{1};
inline std::atomic<bool> fora{false};

inline std::atomic<int> cada_quadros{-1};

inline std::atomic<int> corte_modo{0};

inline std::atomic<float> corte_ok{.25f};
inline std::atomic<int> sinal_n{6};

inline std::atomic<int> rosto_calibra{0},rosto_manda{0};

inline std::atomic<float> alfa_pos{.6f},alfa_tam{.25f};

inline std::atomic<int> rosto_ancora{0};inline std::atomic<float> ancora_nota{.75f};
inline std::atomic<float> nota_grande{0.f},nota_grande_px{900.f};
inline std::atomic<bool> nota_rastreio{false};
inline std::atomic<float> lugar_rosto_s{0.f};inline std::atomic<bool> adq_elig_min{false};inline std::atomic<bool> salto_cadeia{false};
inline std::atomic<bool> npc_borda_camada{false};
inline std::atomic<bool> soma_cena{false};inline std::atomic<bool> camada_normaliza{false};inline std::atomic<bool> camada_fresco{false};

inline std::atomic<float> escala_salto{0.f};

inline std::atomic<unsigned> rede_fixa{0};

inline std::atomic<unsigned> zoom{0};
inline std::atomic<int> recorte_forcado{0};
inline std::atomic<int> recorte_fixo_ini{0};
inline constexpr unsigned zoom_lado=2048;
inline constexpr float zoom_cab_4k=1360.f;
inline unsigned cada_quadros_efetivo() noexcept {
    const int v=cada_quadros.load(std::memory_order_relaxed);
    return v>=0?unsigned(v):0u;
}

inline std::atomic<bool> acumula{false};

inline std::atomic<unsigned> espera_us{0};
inline constexpr unsigned espera_auto=0xffffffffu;

inline std::atomic<bool> espera_job{false};

inline std::atomic<bool> thread_rede{false};
inline std::atomic<float> acum_baixa{.15f},acum_alta{.4f},acum_gama{1.f},acum_sigma{6.f};

inline std::atomic<float> acum_gama_parado{0.f};

inline std::atomic<bool> acum_alta_mov{false};

inline std::atomic<bool> acum_alta_idade{false};

inline std::atomic<float> entrada_media{0.f},entrada_media_limiar{.02f};

struct EntradaMediaReg {int x{},y{};unsigned L{},n{};std::uint64_t ger{};bool valido{};};
inline bool entrada_media_continua(EntradaMediaReg& r,int x,int y,unsigned L,unsigned n,std::uint64_t ger) noexcept {
    const bool c=r.valido&&r.x==x&&r.y==y&&r.L==L&&r.n==n&&r.ger==ger;r=EntradaMediaReg{x,y,L,n,ger,true};return c;}

inline void entrada_media_px(float e[3],float m[3],float a,float lim,bool continua) noexcept {
    float d=0.f;for(int c=0;c<3;++c){if(!std::isfinite(m[c]))d=1e30f;else{const float x=std::fabs(e[c]-m[c]);d=x>d?x:d;}}
    const bool novo=!continua||!(d<=lim);
    for(int c=0;c<3;++c){const float r=novo?e[c]:m[c]+a*(e[c]-m[c]);m[c]=r;e[c]=r;}}

inline constexpr const wchar_t* entrada_sigma_chave=L"NeuralRostoJogadorEntradaSigma";
inline constexpr float entrada_sigma_max=3.f;
inline std::atomic<float> entrada_sigma{0.f};

inline float entrada_sigma_de(bool lido,double v) noexcept {
    if(!lido||!std::isfinite(v)||!(v>0.))return 0.f;return v<double(entrada_sigma_max)?float(v):entrada_sigma_max;}

inline float entrada_sigma_celulas(float s4k,float foh,float passo) noexcept {
    return s4k>0.f&&foh>0.f&&passo>0.f?s4k*foh/2160.f/passo:0.f;}

inline constexpr const wchar_t* delta_inteiro_chave=L"NeuralRostoJogadorDeltaInteiro";
inline std::atomic<bool> delta_inteiro_lig{false};
inline bool delta_inteiro() noexcept {return delta_inteiro_lig.load(std::memory_order_relaxed);}
inline std::atomic<float> elipse_r0{.75f},elipse_r1{1.f};
inline constexpr float delta_inteiro_borda_px4k=64.f;
struct ElipseRaios {float r0,r1;};

inline ElipseRaios elipse_raios_de(bool l0,double v0,bool l1,double v1) noexcept {
    float r0=l0&&std::isfinite(v0)?float(v0<0.?0.:v0>2.?2.:v0):.75f;
    float r1=l1&&std::isfinite(v1)?float(v1<0.?0.:v1>3.?3.:v1):1.f;
    if(!(r1>=r0+.05f))r1=r0+.05f;
    return {r0,r1};}

inline float delta_inteiro_borda_celulas(float foh,float passo) noexcept {
    return foh>0.f&&passo>0.f?delta_inteiro_borda_px4k*foh/2160.f/passo:0.f;}

inline std::atomic<float> pula_limiar{0.f};
inline std::atomic<unsigned> pula_max{2};

inline std::atomic<bool> pula_congela{false};

inline std::atomic<float> pula_congela_vel{.3f};
inline bool congela_parado(float vx,float vy,float lim) noexcept {return std::sqrt(vx*vx+vy*vy)<=lim;}

inline std::atomic<float> pula_congela_px{1.f};
inline bool congela_no_lugar(float hx,float hy,float hl,float hx0,float hy0,float hl0,float lim) noexcept {
    return hl0>0.f&&std::fabs(hl/hl0-1.f)<=.01f&&std::sqrt((hx-hx0)*(hx-hx0)+(hy-hy0)*(hy-hy0))<=lim;}

inline std::atomic<bool> pose_barata{false},pose_longe{false};
inline std::atomic<float> pose_yaw{50.f};
inline std::atomic<unsigned> pose_cadencia{2},pose_seguidas{3};

struct PortaoReuso {
    float ref[1024]{};float ref_hl{};std::uint64_t ref_ger{};bool ref_tem{};unsigned seguidos{};unsigned long long pulados{};
    bool pend{};float pend_hl{};std::uint64_t pend_ger{};bool pend_rodou{};
    double rel{-1.};
    static double relativo(const float* m,const float* r) noexcept {double d=0,s=0;for(int i=0;i<1024;++i){d+=std::fabs(double(m[i])-double(r[i]));s+=std::fabs(double(r[i]));}return d/(std::max)(s,1e-3);}
    bool decidir(const float* m,float hl,std::uint64_t ger,int idade,unsigned q,float lim,unsigned pmax) noexcept {
        rel=-1.;bool pula=false;
        if(m&&pend_ger==ger&&ref_tem&&ref_ger==ger&&ref_hl>0.f){rel=relativo(m,ref);
            pula=idade>=0&&unsigned(idade)+q+4u<16u&&rel<double(lim)&&std::fabs(double(hl)/double(ref_hl)-1.0)<.04&&seguidos<pmax;}
        if(m){if(pend_rodou){std::memcpy(ref,m,sizeof ref);ref_hl=pend_hl;ref_ger=pend_ger;ref_tem=true;}pend=false;}
        if(pula){++seguidos;++pulados;}else seguidos=0;
        return pula;}

    void lancou(float hl,std::uint64_t ger,bool rodou) noexcept {pend=true;pend_hl=hl;pend_ger=ger;pend_rodou=rodou;}
};

inline std::atomic<unsigned> passos{1};
inline std::atomic<float> passo2_forca{.5f};

inline std::atomic<float> passo1_forca{1.f};

inline constexpr unsigned espera_present=0xfffffffeu;

struct EsperaAuto {
    const unsigned* us;double ms[5]{};unsigned n[5]{};unsigned long long k{};
    explicit EsperaAuto(const unsigned* t) noexcept:us(t){}
    int escolher(unsigned long long volta) noexcept {const unsigned long long j=k++;int i;
        if(j<volta)i=int(j%5);else if(j%16==0)i=int((j/16)%5);
        else{i=0;for(int a=1;a<5;++a)if(n[a]&&(n[i]==0||ms[a]<ms[i]))i=a;}
        return i;}
    void medir(int i,double v) noexcept {if(i<0||!(v>0))return;ms[i]=n[i]?.75*ms[i]+.25*v:v;++n[i];}
};
inline constexpr unsigned espera_auto_us[5]{0,1500,3000,4500,6000},espera_present_us[5]{0,1000,2000,3000,4000};

inline std::atomic<float> frescor{0.f};

inline std::atomic<bool> frescor_borrado{true};
inline std::atomic<bool> log_jobs{false};
inline std::atomic<int> externo{1};inline std::atomic<float> externo_dy{-.1f},externo_esc{1.05f};

inline std::atomic<unsigned> area_segura{0u};inline std::atomic<bool> area_externa{false};
inline std::atomic<float> identidade_s{0.f};inline std::atomic<unsigned> identidade_bits{1u};

inline std::atomic<int> cabeca_prof{0},cabeca_prof_liga_n{3};inline std::atomic<float> cabeca_prof_nota{.35f},cabeca_prof_liga{.5f};
inline std::atomic<float> predicao_s{0.f};inline std::atomic<unsigned> predicao_vetos{127u};
inline std::atomic<bool> frescor_pronto{false};
inline std::atomic<std::uint64_t> presents_fim{0};

inline std::atomic<unsigned> passo2_cada{1};
inline std::atomic<bool> compor_elipse{false};

inline std::atomic<bool> compor_rapido{false};

inline std::atomic<unsigned> segura{0};

inline std::atomic<bool> segura_pixel{false};

inline std::atomic<bool> camada{false};
inline std::atomic<int> camada_segura{90};
inline std::atomic<float> camada_meia{20.f},camada_margem{48.f},camada_forca{1.f};

inline std::atomic<bool> camada_regiao_limitada{false};

inline std::atomic<int> teste_falha_ini{0},teste_falha_fim{0};
inline std::atomic<unsigned> ablacao{0};

inline std::atomic<float> baixa_forca{1.f},baixa_lenta{0.f},baixa_rapida{0.f},baixa_mov_px{4.f},acum_limiar{0.f};inline std::atomic<float> baixa_pos_mov{0.f};

inline std::atomic<float> banda_sigma{16.f};

inline std::atomic<float> corte_ok_ini{.07f};

inline std::atomic<int> desliga_jobs{12};
inline std::atomic<bool> diag_hist{false};
inline std::atomic<float> npc_zona{1.15f};
inline std::atomic<bool> busca_sempre{false};

inline std::atomic<bool> ceu_fora{false};

inline std::atomic<unsigned> ruido_rt{0};
inline std::atomic<float> ruido_t0{.03f},ruido_t1{.06f},ruido_alfa{.15f},ruido_alfa_var{.10f},ruido_gama{2.5f},ruido_pixel{.5f},ruido_passo{2.f};
inline std::atomic<float> ruido_banda{.75f},ruido_fino{2.5f},ruido_rosto0{.25f},ruido_rosto1{.5f},ruido_peso_sigma{4.f};
inline std::atomic<unsigned> ruido_margem{16},ruido_escala{2};
inline std::atomic<bool> ruido_mistura{false};
inline bool ruido_inst() noexcept {return (ruido_rt.load(std::memory_order_relaxed)&(inst_atual()==0?1u:2u))!=0u;}

inline std::atomic<bool> ruido_peso{false};

inline std::atomic<float> ruido_limpa{0.f},ruido_limpa_borda{.03f},ruido_limpa_fina{2.f},ruido_limpa_grossa{16.f},ruido_limpa_contraste{.3f},ruido_limpa_razao{1.4f};
inline std::atomic<unsigned> ruido_fonte{0};
inline std::atomic<unsigned> npc_todos_rede{768},npc_todos_jan{768};

inline unsigned todos_rede_fino(unsigned L,unsigned tv,unsigned ult) noexcept {
    const unsigned g=npc_todos_rede.load(std::memory_order_relaxed);
    if(g>768u&&tv>768u&&L>=768u){const unsigned n=(std::min)((std::min)((L+64u)&~127u,g),tv&~127u);return (std::max)(n,768u);}
    return todos_rede(L,tv,ult);}
inline std::atomic<float> npc_todos_cab_max{700.f};
inline std::atomic<float> adq_nota1{.60f};
inline std::atomic<float> liga_forte{.75f};

inline std::atomic<unsigned> borda_suave{0};

inline std::atomic<float> borda_largura{0.f};

inline std::atomic<unsigned> borda_camada{0};
inline std::atomic<float> borda_camada_reg{.05f};
inline std::atomic<bool> borda_camada_perto{false};

inline std::atomic<float> borda_camada_z0{1.15f},borda_camada_z1{1.45f};

inline std::atomic<float> borda_teto{0.f};

inline std::atomic<float> npc_orc_ms{0.f},npc_orc_grande{480.f};
inline std::atomic<unsigned> npc_orc_kmax{2};
inline bool agenda_ligada() noexcept {return npc_orc_ms.load(std::memory_order_relaxed)>0.f;}
inline std::atomic<float> custo_ms_v[n_inst]{};

inline float custo_previsto(int i,unsigned n) noexcept {const float c=custo_ms_v[i].load(std::memory_order_relaxed);
    return c>0.f?c*nrag::custo_tabela(n)/nrag::custo_tabela(1024u):nrag::custo_tabela(n);}
inline std::atomic<std::uint32_t> agenda_bits{0};
inline std::atomic<std::uint64_t> agenda_quadro{0};
struct AgendaConta {std::atomic<unsigned long long> planos{0},querem{0},escolhidos{0},forcados{0},grandes{0},adiados{0};std::atomic<double> usado{0.};};
inline AgendaConta& agenda_conta(){static auto* c=new AgendaConta;return *c;}

inline std::atomic<bool> blur_rapido{false};

inline unsigned teto_jobs() noexcept {
    unsigned c=cada.load(std::memory_order_relaxed);
    if(inst_atual()>=1&&agenda_ligada())c=(std::max)(c,npc_orc_kmax.load(std::memory_order_relaxed));
    if(pula_limiar.load(std::memory_order_relaxed)>0.f)c=(std::max)(c,1u+pula_max.load(std::memory_order_relaxed));
    return c+(fora.load(std::memory_order_relaxed)?1u:0u);
}

inline unsigned rede_de(unsigned L,unsigned t) noexcept {if(canon_inst())return (std::min)(t,npc_canon_rede.load(std::memory_order_relaxed));
    if(L>=t)return t;if(inst_atual()==0&&rede_fixa.load(std::memory_order_relaxed))return (std::min)(t,rede_fixa.load(std::memory_order_relaxed)*L)&~127u;
    if(npc_amplia_inst()&&L<npc_rede_min)return (std::min)(t,npc_rede_min);return L;}

inline constexpr unsigned n_max=2048,n_min=256;
inline constexpr unsigned lado_sem_zoom=1024;
inline constexpr unsigned slots=6;

inline std::atomic<bool> rede_adapt{false};
inline std::atomic<float> rede_adapt_alvo{420.f};
inline std::atomic<unsigned> rede_adapt_segura{30};
struct RedeAdapt {unsigned n{};std::uint64_t desce_desde{};bool desce{};};
inline unsigned rede_adapt_degrau(unsigned L,unsigned teto_n,float cabeca,float alvo) noexcept {
    static constexpr unsigned degraus[4]{512u,768u,1024u,1536u};
    for(const unsigned s:degraus)if(s<=teto_n&&cabeca*float(s)>=alvo*float(L))return s;
    return teto_n;
}

inline unsigned rede_adaptativa(RedeAdapt& e,unsigned L,unsigned t,float cabeca,float alvo,unsigned prazo,std::uint64_t q_agora) noexcept {
    const unsigned teto_n=(std::min)(L,t)&~127u;
    if(!(alvo>0.f)||!(cabeca>0.f)||teto_n<512u){e=RedeAdapt{};return 0;}
    const unsigned sobe=rede_adapt_degrau(L,teto_n,cabeca,alvo),desce=rede_adapt_degrau(L,teto_n,cabeca,alvo*1.15f);
    if(e.n==0||e.n>teto_n||sobe>e.n){e.n=sobe;e.desce=false;return e.n;}
    if(desce<e.n){
        if(!e.desce){e.desce=true;e.desce_desde=q_agora;}
        if(q_agora-e.desce_desde>=prazo){e.n=desce;e.desce=false;}
    }else e.desce=false;
    return e.n;
}
inline constexpr std::uint64_t bytes_meio=std::uint64_t(n_max)*n_max*8u;
inline constexpr std::uint64_t bytes_slot=2*bytes_meio;
inline constexpr std::uint64_t bytes_dados=std::uint64_t(slots)*bytes_slot;
inline constexpr std::uint64_t bytes_total=bytes_dados+4096;

inline std::atomic<unsigned> cap0_fixo{0};
inline unsigned cap0_de(unsigned t) noexcept {return (std::min)(n_max,(std::max)(n_min,t&~127u));}
inline unsigned n_cap() noexcept {
    if(nrorc::aplicada()){const auto& d=nrorc::decisao();return inst_atual()>=1?(inst_atual()<=d.k_perto?d.cap_perto:d.cap_npc):d.cap_grace;}
    if(inst_atual()>=1)return npc2_teto.load(std::memory_order_relaxed);
    if(const unsigned c=cap0_fixo.load(std::memory_order_relaxed))return c;
    if(!nrorc::estado().ini_ok.load(std::memory_order_acquire))return n_max;
    unsigned z=0;cap0_fixo.compare_exchange_strong(z,cap0_de(teto.load(std::memory_order_relaxed)),std::memory_order_acq_rel);
    return cap0_fixo.load(std::memory_order_relaxed);}
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
    float hx0{},hy0{};
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

    std::atomic<unsigned> seguidas[3]{};std::atomic<std::uint64_t> pausa_ate{0};std::atomic<bool> retomar{false};
    std::atomic<unsigned long long> passageiras{0};
    std::atomic<unsigned long long> avaliacoes{0},prontos{0},pulados_marcador{0},composicoes{0},recortes{0};
    std::uint32_t marcado(unsigned s)const{auto* m=marcador.load(std::memory_order_acquire);return m?m[s]:0u;}
};
inline Compartilhado& global(){static auto* g=new Compartilhado[n_inst];return g[inst_atual()];}

inline std::atomic<std::uint64_t> geracao_v[n_inst]{1,1,1,1,1,1,1,1};
inline std::atomic<std::uint64_t>& ger() noexcept {return geracao_v[inst_atual()];}
inline void cortar() noexcept {ger().fetch_add(1,std::memory_order_acq_rel);}

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
    auto& g=global();const auto g0=ger().load(std::memory_order_acquire);int livre=-1,velho=-1;int jobs[slots]{};int js_n=0;
    for(unsigned s=0;s<slots;++s){const auto p=g.slot[s].palavra.load(std::memory_order_acquire);
        if(estado_de(p)==Livre||estado_de(p)==Gravado||(estado_de(p)==Pronto&&g.slot[s].geracao!=g0)){if(livre<0)livre=int(s);}
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

inline std::atomic<std::uint64_t> epoca_backend{1};
inline bool epoca_nova(std::uint64_t epoca,std::uint64_t& vista) noexcept {const bool v=vista&&epoca!=vista;vista=epoca;return v;}

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
struct SjConta {std::atomic<unsigned long long> recortes{0},avaliados{0},velhos{0},futuro{0},sem_pub{0},estendido{0},zero{0},marcador_tarde{0},intercalados{0},sem_tabela{0},tabela_tarde{0},adiados{0},pulados_reuso{0},congelados{0},pose_pulados{0};};
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

struct GradeDet {int q{},gw{},gh{};};
inline GradeDet grade_det(int mvw,int mvh) noexcept {GradeDet g;g.q=(std::max)(1,mvh/180);g.gw=mvw/g.q;g.gh=mvh/g.q;return g;}
struct DetCap {std::size_t cheia{},grade{};};

inline bool det_nova_cap(DetCap& c,std::size_t cheia,std::size_t grade) noexcept {
    if(cheia<=c.cheia&&grade<=c.grade)return false;c.cheia=(std::max)(c.cheia,cheia);c.grade=(std::max)(c.grade,grade);return true;}

struct DetAlocPausa {int desde{0};unsigned espera{0};};
inline bool det_aloc_pode(const DetAlocPausa& p,int job) noexcept {return !p.espera||job<p.desde||unsigned(job-p.desde)>=p.espera;}
inline void det_aloc_falhou(DetAlocPausa& p,int job) noexcept {p.espera=p.espera?(std::min)(p.espera*2u,1024u):64u;p.desde=job;}
inline void det_aloc_ok(DetAlocPausa& p) noexcept {p.espera=0;p.desde=0;}

inline float meia_para_float(std::uint16_t h) noexcept {
    const unsigned s=(h>>15)&1u,e=(h>>10)&31u,m=h&1023u;float v;
    if(e==0)v=std::ldexp(float(m),-24);else if(e==31)v=m?NAN:INFINITY;else v=std::ldexp(float(m|1024u),int(e)-25);
    return s?-v:v;
}

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

inline std::atomic<unsigned> rosto_hz_busca{30};inline std::atomic<int> grace_on{0};
inline std::atomic<float> rosto_nota{.7f};
inline constexpr unsigned rosto_w_max=640,rosto_h_max=360,rosto_aneis=3,rosto_marca0=16;

inline constexpr unsigned faixa_w_max=1920,faixa_h_max=480,inteira_w_max=960,inteira_h_max=540;
inline constexpr std::uint64_t rosto_bytes=(std::max)((std::max)(std::uint64_t(rosto_w_max)*rosto_h_max,std::uint64_t(faixa_w_max)*faixa_h_max),std::uint64_t(inteira_w_max)*inteira_h_max)*8u;
inline std::atomic<unsigned> yunet_inteira{0};
inline constexpr unsigned faixa_w=960,faixa_h=240;
static_assert(std::uint64_t(faixa_w)*faixa_h*8u<=rosto_bytes,"faixa cabe no anel");

inline std::atomic<unsigned> faixa_varre{0};

inline bool faixa_da_vez(unsigned n,bool giro,bool busca,double& y0f){
    const unsigned v=faixa_varre.load(std::memory_order_relaxed);
    if(!(n&1u)||giro||(busca&&!(v&1u)))return false;
    static constexpr double pos[4]{.25,0.,.25,1.-960.0/2160.0};
    y0f=(v&2u)?pos[(n>>1)&3u]:.25;return true;}
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

struct SinalRostos {std::mutex m;std::condition_variable cv;std::uint64_t escritas{};};
inline SinalRostos& sinal_rostos(){static auto* s=new SinalRostos;return *s;}
inline void acordar_rostos() noexcept {try{auto& s=sinal_rostos();{std::lock_guard<std::mutex> l(s.m);++s.escritas;}s.cv.notify_one();}catch(...){}}

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
    int idade{-1};

    float disco_x{},disco_y{};

    std::uint64_t quadro{};unsigned ow{},oh{};bool ext_tem{},ext_fora{};float ext_x{},ext_y{},ext_l{};};

inline std::atomic<bool> captura_job{false};
inline void cabeca_da_captura(const QuadroRender& q,bool job,float& x,float& y) noexcept {x=job?q.disco_x:q.cx;y=job?q.disco_y:q.cy;}

inline void cabeca_no_slot(Slot& S,const QuadroRender& q,bool job,float zdx,float zdy,float lado,float nivel) noexcept {
    float x,y;cabeca_da_captura(q,job,x,y);S.hx=x;S.hy=y;S.zx=x+zdx;S.zy=y+zdy;S.hl=lado;S.nivel=nivel;S.hx0=x;S.hy0=y;}
inline QuadroRender& quadro_render(){static QuadroRender q[n_inst];return q[inst_atual()];}

inline bool disco_vale(const QuadroRender& q){return q.lado>1.f&&(q.on>0||(q.composto>0&&q.ganho>0.f));}

inline std::atomic<bool> area_unica{false};
enum class UsoArea {Filtros,Estabilizador};
struct AreaRosto {float x{},y{},rx{},ry{};bool elipse{};};
inline AreaRosto area_rosto(const QuadroRender& q,UsoArea u) noexcept {
    (void)u;AreaRosto a{q.disco_x,q.disco_y,.5f*q.lado,.5f*q.lado,false};
    if(area_unica.load(std::memory_order_relaxed)){a.rx=float(NR_AREA_SEMI_X)*q.lado;a.ry=float(NR_AREA_SEMI_Y)*q.lado;a.elipse=true;}
    return a;}

inline bool captura_pela_cabeca_do_job(bool inst0) noexcept {return (inst0&&captura_job.load(std::memory_order_relaxed))||area_unica.load(std::memory_order_relaxed);}

struct CabecaMedida {bool t1{},t0{};std::uint64_t f1{},f0{};double x1{},y1{},x0{},y0{};std::uint64_t rapido_ate{};};
inline bool cabeca_pela_medida(CabecaMedida& m,std::uint64_t quadro,float lado,bool ancora,QuadroRender& q){
    if(!m.t1||quadro<m.f1||quadro-m.f1>4u||ancora)return false;
    const float jy=q.cy;
    double vx=0,vy=0;if(m.t0&&m.f1>m.f0&&m.f1-m.f0<=3u){const double dt=double(m.f1-m.f0);vx=(m.x1-m.x0)/dt;vy=(m.y1-m.y0)/dt;}
    const double a=double(quadro-m.f1),mx=m.x1+vx*a,my=m.y1+vy*a;
    if(std::hypot(mx-q.cx,my-q.cy)>=.15*lado||std::hypot(vx,vy)>=.08*lado)m.rapido_ate=quadro+6u;
    if(quadro>m.rapido_ate)return false;
    q.cx=float(mx);q.cy=float(std::clamp(my,double(jy)-.3*lado,double(jy)+.3*lado));return true;}

inline void log(const char* linha) noexcept {
    (void)linha;
}
inline void envenenar(const char* motivo) noexcept {
    if(!global().envenenado.exchange(true)){char t[256]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_DESLIGADO motivo=%s (sessao inteira; a cena segue igual)",motivo?motivo:"?");log(t);}
}

inline bool desligar_rostos(const char* motivo) noexcept {
    bool algum=false;
    for(int i=0;i<n_inst;++i){Inst ii(i);if(!global().envenenado.exchange(true))algum=true;}
    if(algum){char t[256]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_DESLIGADO motivo=%s (protagonista e NPCs, sessao inteira; a cena segue igual)",motivo?motivo:"?");Inst i0(0);log(t);}
    return algum;
}

enum class Falha:unsigned{Recorte=0,Detector=1,Render=2};
struct FalhaPermanente:std::runtime_error {using std::runtime_error::runtime_error;};
inline std::atomic<unsigned> falha_max{4},falha_pausa{16};
inline std::atomic<std::uint64_t> tique{0};

inline bool falha_envenena(unsigned n,unsigned max) noexcept {return max<=1u||n>=max;}
inline std::uint64_t falha_pausa_jobs(unsigned n,unsigned base) noexcept {return std::uint64_t((std::max)(base,1u))<<(std::min)(n>0?n-1u:0u,6u);}
inline bool pausado() noexcept {return tique.load(std::memory_order_relaxed)<global().pausa_ate.load(std::memory_order_relaxed);}
inline void passageira(Falha f,const char* motivo) noexcept {
    auto& g=global();if(g.envenenado.load(std::memory_order_relaxed))return;
    const unsigned n=g.seguidas[unsigned(f)].fetch_add(1,std::memory_order_relaxed)+1u;g.passageiras.fetch_add(1,std::memory_order_relaxed);
    const unsigned mx=falha_max.load(std::memory_order_relaxed);
    if(falha_envenena(n,mx)){
        if(mx<=1u){envenenar(motivo);return;}
        char t[256]{};NRLOGF(t,sizeof t,"%s (%u falhas passageiras seguidas)",motivo?motivo:"?",n);envenenar(t);return;}
    const std::uint64_t p=falha_pausa_jobs(n,falha_pausa.load(std::memory_order_relaxed));
    g.pausa_ate.store(tique.load(std::memory_order_relaxed)+p,std::memory_order_relaxed);g.retomar.store(true,std::memory_order_release);
    char t[288]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_FALHA_PASSAGEIRA motivo=%s tipo=%u seguidas=%u/%u pausa_jobs=%llu total=%llu (a instancia tenta de novo depois da pausa)",
        motivo?motivo:"?",unsigned(f),n,mx,static_cast<unsigned long long>(p),g.passageiras.load(std::memory_order_relaxed));log(t);
}
inline void sucesso(Falha f) noexcept {auto& s=global().seguidas[unsigned(f)];if(s.load(std::memory_order_relaxed))s.store(0,std::memory_order_relaxed);}

inline bool retomou() noexcept {auto& g=global();return g.retomar.load(std::memory_order_acquire)&&!pausado()&&g.retomar.exchange(false,std::memory_order_acq_rel);}

inline unsigned varrer_slots_velhos(int job) noexcept {
    auto& g=global();unsigned n=0;
    for(unsigned s=0;s<slots;++s){auto w=g.slot[s].palavra.load(std::memory_order_acquire);
        if(estado_de(w)==Escrito&&!id_e_quadro(job_de(w))&&job_de(w)<job&&g.slot[s].palavra.compare_exchange_strong(w,palavra(Livre,0),std::memory_order_acq_rel))++n;}
    return n;}
inline int npc_n() noexcept {if(!npc2.load(std::memory_order_relaxed))return 0;const int n=std::clamp(npc_max.load(std::memory_order_relaxed),1,npc_slots);
    return nrorc::aplicada()?(std::min)(n,nrorc::decisao().k_npc):n;}
inline bool ativo() noexcept {return ligado.load(std::memory_order_relaxed)&&!global().envenenado.load(std::memory_order_relaxed)&&(inst_atual()==0||inst_atual()<=npc_n())&&!pausado()&&!nrgpu::perdida();}
inline bool npc2_ativo() noexcept {return npc2.load(std::memory_order_relaxed)&&ligado.load(std::memory_order_relaxed);}

inline void cabeca_externa_no_quadro(QuadroRender& q,unsigned ow,unsigned oh,bool eok,float eu,float ev,float ea,float ed,bool efora) noexcept {
    q.ext_fora=efora;q.ext_tem=false;
    if(!eok||!ow||!oh||!(ea>.01f&&ea<.9f&&ed>.3f&&eu>-.1f&&eu<1.1f&&ev>-.1f&&ev<1.1f))return;
    const float dy=externo_dy.load(std::memory_order_relaxed),es=externo_esc.load(std::memory_order_relaxed);
    q.ext_tem=true;q.ext_x=eu*float(ow);q.ext_y=(ev+dy*ea)*float(oh);q.ext_l=ea*float(oh)*es;}

inline nrcabseg::Estado& area_segura_estado() noexcept {static nrcabseg::Estado e[n_inst];return e[inst_atual()];}
inline unsigned areas_ligadas(UsoArea u,AreaRosto (&out)[4]) noexcept {
    unsigned n=0;
    const unsigned N=u==UsoArea::Filtros?area_segura.load(std::memory_order_relaxed):0u;const bool ex=u==UsoArea::Filtros&&area_externa.load(std::memory_order_relaxed);
    auto poe=[&]{const auto& q=quadro_render();
        if((!N&&!ex)||!q.quadro){if(n<4u&&disco_vale(q))out[n++]=area_rosto(q,u);return;}

        bool atual=disco_vale(q);AreaRosto a{};
        if(ex&&inst_atual()==0&&q.ext_tem&&q.ext_l>1.f){QuadroRender qe=q;qe.disco_x=q.ext_x;qe.disco_y=q.ext_y;qe.lado=q.ext_l;a=area_rosto(qe,u);atual=true;}
        else if(atual)a=area_rosto(q,u);
        nrcabseg::Area o{};
        if(nrcabseg::passo(area_segura_estado(),atual,nrcabseg::Area{a.x,a.y,a.rx,a.ry,a.elipse},q.quadro,ger().load(std::memory_order_relaxed),N,ex&&inst_atual()==0&&q.ext_fora,float(q.ow),float(q.oh),o)&&n<4u)
            out[n++]=AreaRosto{o.x,o.y,o.rx,o.ry,o.elipse};};
    if(ativo())poe();
    if(npc2.load(std::memory_order_relaxed))for(int kn=1;kn<=npc_n();++kn){Inst i1(kn);if(ativo())poe();}
    return n;}

inline std::atomic<bool> orc_completo{false};
inline std::atomic<int> orc_npc_mb{0};
inline std::atomic<bool> orc_grace_fixa{false};
inline std::atomic<unsigned> orc_saida_w{0},orc_saida_h{0},orc_render_w{0},orc_render_h{0},orc_saida_bpp{8};
inline std::atomic<int> det_conjuntos{1};
inline void orc_extras(nrorc::Config& c) noexcept {
    if(!orc_completo.load(std::memory_order_relaxed))return;
    const unsigned ow=orc_saida_w.load(std::memory_order_relaxed),oh=orc_saida_h.load(std::memory_order_relaxed);
    unsigned rw=orc_render_w.load(std::memory_order_relaxed),rh=orc_render_h.load(std::memory_order_relaxed);if(!rw||!rh){rw=ow;rh=oh;}
    if(camada.load(std::memory_order_relaxed))c.camada=nrorc::bytes_camada(ow,oh);
    const auto gd=grade_det(int(rw),int(rh));
    c.detector=nrorc::bytes_detector(std::uint64_t(rw)*rh,std::uint64_t((std::max)(gd.gw,0))*std::uint64_t((std::max)(gd.gh,0)),unsigned((std::max)(det_conjuntos.load(std::memory_order_relaxed),1)));
    if(borda_teto.load(std::memory_order_relaxed)>0.f&&compor_rapido.load(std::memory_order_relaxed)&&acumula.load(std::memory_order_relaxed))
        c.copia=nrorc::bytes_copia(c.janela_grace,orc_saida_bpp.load(std::memory_order_relaxed));}

inline nrorc::Config config_orcamento() noexcept {
    nrorc::Config c{};
    c.teto_grace=teto.load(std::memory_order_relaxed);c.janela_grace=nrorc::janela_grace_padrao;c.janela_npc=nrorc::janela_npc_padrao;
    if(npc_leve.load(std::memory_order_relaxed)&1){c.leve=true;c.janela_npc=npc_leve_jan.load(std::memory_order_relaxed);c.leve_rede=npc_leve_rede.load(std::memory_order_relaxed);}
    if(npc_canon.load(std::memory_order_relaxed)==2){c.perto=npc_canon_perto.load(std::memory_order_relaxed);c.jan_peq=npc_canon_jan_peq.load(std::memory_order_relaxed);}
    if(npc_todos.load(std::memory_order_relaxed)&16){c.todos=true;c.perto=0;c.todos_max=npc_todos_grandes.load(std::memory_order_relaxed);c.todos_rede=npc_todos_rede.load(std::memory_order_relaxed);c.todos_jan=npc_todos_jan.load(std::memory_order_relaxed);}
    c.npc=npc2.load(std::memory_order_relaxed);c.npc_max=c.npc?std::clamp(npc_max.load(std::memory_order_relaxed),1,npc_slots):0;
    c.teto_npc=npc2_teto.load(std::memory_order_relaxed);
    c.acumula=acumula.load(std::memory_order_relaxed);c.npc_acumula=npc2_acumula.load(std::memory_order_relaxed);
    c.frescor=frescor.load(std::memory_order_relaxed)>0.f;c.passos=passos.load(std::memory_order_relaxed);
    {const unsigned rb=ruido_peso.load(std::memory_order_relaxed)?ruido_rt.load(std::memory_order_relaxed):0u;c.ruido_grace=(rb&1u)!=0u;c.ruido_npc=(rb&2u)!=0u;}
    c.ctx=nrorc::contexto_medido;c.ctx_unico=true;
    c.fixo=c.npc?rosto_bytes:std::uint64_t{0};
    orc_extras(c);
    c.grace_fixa=orc_grace_fixa.load(std::memory_order_relaxed);
    c.npc_reserva=std::uint64_t((std::max)(0,orc_npc_mb.load(std::memory_order_relaxed)))<<20;
    return c;}

inline void aplicar_decisao(const nrorc::Config& c,const nrorc::Medida& m,const nrorc::Decisao& d) noexcept {
    if(nrorc::modo()==1){teto.store(d.cap_grace,std::memory_order_relaxed);if(d.k_npc>0)npc2_teto.store(d.cap_npc,std::memory_order_relaxed);}
    nrorc::publicar(c,m,d);}

inline bool falhas_epoca(std::uint64_t epoca,std::uint64_t& vista) noexcept {
    if(vista==epoca)return false;const bool nova=vista!=0;vista=epoca;if(!nova)return false;
    for(int i=0;i<n_inst;++i){
        Inst ii(i);
        auto& g=global();for(auto& s:g.seguidas)s.store(0,std::memory_order_relaxed);g.pausa_ate.store(0,std::memory_order_relaxed);}
    for(auto& e:job_quadro_v){e.job.store(0,std::memory_order_relaxed);e.quadro.store(0,std::memory_order_relaxed);}
    sj_job_aceito.store(0,std::memory_order_relaxed);sj_job_publicado.store(0,std::memory_order_relaxed);
    return true;}
}
