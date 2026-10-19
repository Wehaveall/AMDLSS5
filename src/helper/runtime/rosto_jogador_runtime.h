#pragma once

#include <array>
#include "rosto_jogador_state.h"
#include "rosto_jogador_detecta.h"
static_assert(nrrj::npc_slots==nrrj::npc_det_max,"npc-n: NPCs do estado = do detector");
#include "yunet.h"
#include "scrfd.h"
namespace nrrj_runtime {
struct ExternalDesc {int type{};union Handle {int fd;struct {void* handle;const void* name;} win32;const void* nvSci;Handle():win32{}{}} handle;
                     unsigned long long size{};unsigned flags{};unsigned reserved[16]{};};
struct BufferDesc {unsigned long long offset{},size{};unsigned flags{},reserved[16]{};};
static_assert(sizeof(ExternalDesc)==104&&sizeof(BufferDesc)==88);
struct ElipseK {float hx,hy,hl,ex,ey,r0,r1,m0,m1;};
static_assert(sizeof(ElipseK)==36);
struct DetSet {void* dq{};void* mvq{};void* d{};size_t cap{};std::atomic<bool> ocupado{false};};
struct DetTarefa {int set{-1};int gw{},gh{},dw{},dh{};int job{};std::uint64_t quadro{};int device{-1};int fonte{};};
#pragma warning(push)
#pragma warning(disable:4324)
struct Estado {
    std::mutex mutex;
    bool modulo_tentado{};void* module{};
    void* k_reduz{};void* k_cena{};void* k_entrada{};void* k_dif{};void* k_baixa{};void* k_blur4{};void* k_blur1{};void* k_delta{};void* k_guarda{};void* k_mede{};void* k_etiqueta{};
    void* k_meia4{};void* k_bandas{};
    void* k_mini{};void* k_sat{};
    void* k_lumin{};void* k_bandas2{};void* lin{};bool lin_tentado{};
    void* k_camada{};void* k_baixa_m{};void* k_blur4w{};void* k_delta_m{};int det_dinv{};unsigned long long camada_n{},camada_sem{};

    void* mini{};std::array<float,1024> mini_ref{};bool mini_tem{};float mini_hl{};std::uint64_t mini_ger{};int mini_job{};
    unsigned pulos_seguidos{};unsigned long long pulados{},portoes{};int pula_job{};

    void* in2{};void* out2{};bool buf2_tentado{};unsigned long long passos2{},passos2_pulados{};double p1_min{},p1_ema{};unsigned p1_n{};unsigned long long p2_k{};
    alignas(64) std::array<unsigned char,0x400> ctx{};bool construido{},carregado{};
    void* externo{};char* base_hip{};

    void* externo_grade{};char* grade_hip{};int grade_lido{-1};int fonte_ant{};unsigned proxy_ok_n{},falta_n{};unsigned long long sem_dados{};

    void* dc{};void* d0{};void* D{};void* lo{};void* lo2{};void* luma{};void* gl{};void* tmp1{};void* soma{};bool grades{};

    bool det_pend{};DetTarefa det_tarefa{};

    struct Fora {int job{};char* base{};void* primary{};void* dest{};int rw{},rh{},pitch{},formato{};};Fora fora_pend{};
    unsigned long long jobs{},avaliados{};double ms_soma{},ms_max{};unsigned ms_n{};double ultimo_ms{-1.0};
    std::filesystem::path pasta;
};
#pragma warning(pop)
inline Estado& estado(){static Estado* e=new Estado[nrrj::n_inst];return e[nrrj::inst_atual()];}
inline Estado& estado_de(int i){Estado* r=nullptr;
    {nrrj::Inst g(i);r=&estado();}
    return *r;}

inline bool ctx_unico() noexcept {return nrorc::aplicada()&&nrorc::estado().cfg.ctx_unico;}

inline void compartilhar(Estado& e,const Estado& e0) noexcept {
    e.module=e0.module;e.modulo_tentado=true;
    e.k_reduz=e0.k_reduz;e.k_cena=e0.k_cena;e.k_entrada=e0.k_entrada;e.k_dif=e0.k_dif;e.k_baixa=e0.k_baixa;e.k_blur4=e0.k_blur4;e.k_blur1=e0.k_blur1;
    e.k_delta=e0.k_delta;e.k_guarda=e0.k_guarda;e.k_mede=e0.k_mede;e.k_meia4=e0.k_meia4;e.k_bandas=e0.k_bandas;e.k_mini=e0.k_mini;e.k_sat=e0.k_sat;
    e.k_lumin=e0.k_lumin;e.k_bandas2=e0.k_bandas2;e.k_camada=e0.k_camada;e.k_baixa_m=e0.k_baixa_m;e.k_blur4w=e0.k_blur4w;e.k_delta_m=e0.k_delta_m;
    e.dc=e0.dc;e.d0=e0.d0;e.D=e0.D;e.lo=e0.lo;e.lo2=e0.lo2;e.luma=e0.luma;e.gl=e0.gl;e.tmp1=e0.tmp1;e.soma=e0.soma;e.grades=e0.grades;
    e.lin=e0.lin;e.lin_tentado=true;e.in2=e0.in2;e.out2=e0.out2;e.buf2_tentado=true;
    e.k_etiqueta=e0.k_etiqueta;}
inline std::atomic<bool> det_thread{false};

inline std::atomic<float> passo2_folga{0.f};
inline std::filesystem::path pasta_helper(){
    auto& e=estado();if(!e.pasta.empty())return e.pasta;
    HMODULE self{};wchar_t path[32768]{};
    if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&estado),&self)&&GetModuleFileNameW(self,path,32768))
        e.pasta=std::filesystem::path(path).parent_path();
    return e.pasta;
}

inline void ler_ini(char* base) noexcept {
    if(nrrj::ini_lido.load(std::memory_order_acquire))return;
    auto& e=estado();std::lock_guard<std::mutex> l(e.mutex);if(nrrj::ini_lido.load())return;
    try{
        const std::wstring ini=nrinipath::resolve(base,reinterpret_cast<const void*>(&estado));
        if(ini.empty())return;
        nrrj::ini_lido.store(true,std::memory_order_release);
        wchar_t t[16]{};NRCFG_STR(L"DlssNrOnAmd",L"NeuralRostoJogador",L"0",t,16,ini.c_str());
        if(!nrrj::menu_escreveu.load())nrrj::ligado.store(nrrj::valor_de(t));
        nrrj::teto.store(nrrj::teto_de(t));

        nrrj::cada.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorCada",1,ini.c_str())),1,4)));
        nrrj::fora.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorFora",0,ini.c_str())==1);

        nrrj::cada_quadros.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorCadaQuadros",-1,ini.c_str())),-1,64));
        nrrj::corte_modo.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorCorte",0,ini.c_str())),0,31));
        {wchar_t b[32]{};NRCFG_STR(L"DlssNrOnAmd",L"NeuralRostoJogadorCorteOk",L"",b,32,ini.c_str());
         if(*b){const double v=std::wcstod(b,nullptr);if(std::isfinite(v)&&v>=0.&&v<=1.)nrrj::corte_ok.store(float(v));}
         nrrj::sinal_n.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorSinalN",6,ini.c_str())),0,64));
         nrrj::rosto_calibra.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRostoCalibra",0,ini.c_str())==1?1:0);
         nrrj::rosto_manda.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRostoManda",0,ini.c_str())==1?1:0);
         auto fl=[&](const wchar_t* k,float padrao){wchar_t b2[32]{};NRCFG_STR(L"DlssNrOnAmd",k,L"",b2,32,ini.c_str());if(!*b2)return padrao;
             const double v=std::wcstod(b2,nullptr);return std::isfinite(v)&&v>.02&&v<=1.?float(v):padrao;};
         nrrj::alfa_pos.store(fl(L"NeuralRostoJogadorAlfaPos",.6f));nrrj::alfa_tam.store(fl(L"NeuralRostoJogadorAlfaTam",.25f));
         nrrj::rosto_ancora.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRostoAncora",0,ini.c_str())),0,30));
         nrrj::ancora_nota.store(fl(L"NeuralRostoJogadorAncoraNota",.75f));
         nrrj::nota_grande.store(fl(L"NeuralRostoJogadorNotaGrande",0.f));
         nrrj::soma_cena.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorSomaCena",0,ini.c_str())==1);
         nrrj::npc_borda_camada.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcBordaCamada",0,ini.c_str())==1);
         nrrj::nota_grande_px.store(float((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNotaGrandePx",900,ini.c_str())),200,4000)));
         {wchar_t b3[32]{};NRCFG_STR(L"DlssNrOnAmd",L"NeuralRostoJogadorEscalaSalto",L"",b3,32,ini.c_str());
          const double v=*b3?std::wcstod(b3,nullptr):0.;nrrj::escala_salto.store(std::isfinite(v)&&v>1.01&&v<4.?float(v):0.f);}
         nrrj::rede_fixa.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRedeFixa",0,ini.c_str())),0,4)));}
        nrrj::recorte_fixo_ini.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRecorteFixo",0,ini.c_str())),0,2048)&~127);
        nrrj::recorte_forcado.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRecorteForcado",0,ini.c_str())),0,2048));
        nrrj::zoom.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorZoom",0,ini.c_str())),0,63)));
        det_thread.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorDetThread",0,ini.c_str())==1);

        {auto flt=[&](const wchar_t* k,float padrao,float lo,float hi){wchar_t b[32]{};NRCFG_STR(L"DlssNrOnAmd",k,L"",b,32,ini.c_str());
            if(!b[0])return padrao;const float v=float(std::wcstod(b,nullptr));return std::isfinite(v)?(std::clamp)(v,lo,hi):padrao;};
         nrrj::acumula.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorAcumula",0,ini.c_str())==1);
         nrrj::acum_baixa.store(flt(L"NeuralRostoJogadorAcumBaixa",.15f,.02f,1.f));nrrj::acum_alta.store(flt(L"NeuralRostoJogadorAcumAlta",.4f,.02f,1.f));
         nrrj::acum_gama.store(flt(L"NeuralRostoJogadorAcumGama",1.f,0.f,8.f));

         nrrj::pula_limiar.store(flt(L"NeuralRostoJogadorPula",0.f,-1.f,1.f));
         nrrj::pula_max.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorPulaMax",2,ini.c_str())),1,3)));
         nrrj::passos.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorPassos",1,ini.c_str())),1,2)));
         nrrj::passo2_forca.store(flt(L"NeuralRostoJogadorPasso2Forca",.5f,.05f,2.f));
         passo2_folga.store(flt(L"NeuralRostoJogadorPasso2Folga",0.f,0.f,4.f));
         nrrj::frescor.store(flt(L"NeuralRostoJogadorFrescor",0.f,0.f,1.f));

         nrrj::passo2_cada.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorPasso2Cada",1,ini.c_str())),1,4)));
         nrrj::rede_alvo.store(flt(L"NeuralRostoJogadorRedeAlvo",0.f,0.f,1024.f));
         nrrj::compor_elipse.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorComporElipse",0,ini.c_str())==1);
         nrrj::passo1_forca.store(flt(L"NeuralRostoJogadorPasso1Forca",1.f,.25f,4.f));

         nrrj::npc_modo.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpc",0,ini.c_str())),0,2));
         nrrj::npc.store(nrrj::npc_modo.load()!=0);
         nrrj::rosto_hz.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRostoHz",10,ini.c_str())),0,30)));
         nrrj::rosto_hz_busca.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRostoHzBusca",30,ini.c_str())),0,60)));
         nrrj::rosto_nota.store(flt(L"NeuralRostoJogadorRostoNota",.7f,.3f,.99f));
         nrrj::npc_min.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcMin",160,ini.c_str())),32,2048));
         nrrj::npc_recorte.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcRecorte",512,ini.c_str())),256,1024)&~127);
         nrrj::npc_forca.store(flt(L"NeuralRostoJogadorNpcForca",2.f,.25f,4.f));
         nrrj::npc_segura_s.store(flt(L"NeuralRostoJogadorNpcSegura",1.5f,0.f,10.f));

         nrrj::npc2.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpc2",0,ini.c_str())==1);
         nrrj::npc2_segura_s.store(flt(L"NeuralRostoJogadorNpc2Segura",1.f,.3f,30.f));
         nrrj::npc2_acumula.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpc2Acumula",1,ini.c_str())==1);
         nrrj::npc2_vel_max.store(flt(L"NeuralRostoJogadorNpc2VelMax",0.f,0.f,50.f));
         nrrj::npc2_frescor.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpc2Frescor",0,ini.c_str())==1);
         nrrj::npc2_teto.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpc2Teto",1024,ini.c_str())),256,int(nrrj::n_max)))&~127u);
         nrrj::npc_max.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcMax",nrrj::npc_slots,ini.c_str())),1,nrrj::npc_slots));
         nrrj::adquire.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorAdquire",0,ini.c_str())),0,63));
         nrrj::caminha_dela.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorCaminhaDela",0,ini.c_str())),0,3));
         nrrj::reancora.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorReancora",0,ini.c_str())),0,1));
         {const int f=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorYunetFaixa",0,ini.c_str()));
          nrrj::yunet_faixa.store(f>=1024&&f<=int(nrrj::faixa_w_max)&&f%64==0?unsigned(f):0u);}
         {const int f=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorYunetInteira",0,ini.c_str()));
          nrrj::yunet_inteira.store(f>640&&f<=int(nrrj::inteira_w_max)&&f%64==0?unsigned(f):0u);}
         nrrj::npc_degraus.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcDegraus",0,ini.c_str())==1?1:0);
         {const int r=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcRecorteMin",0,ini.c_str()));nrrj::npc_recorte_min2.store(r==384||r==512?r:0);}
         {const int r=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcRecorteForcado",0,ini.c_str()));nrrj::npc_recorte_forcado.store(r>=256&&r<=1024&&r%128==0?r:0);}
         nrrj::npc_segura_peq.store(flt(L"NeuralRostoJogadorNpcSeguraPeq",0.f,0.f,10.f));
         {const float f=flt(L"NeuralRostoJogadorNpcFracao",0.f,0.f,.6f);nrrj::npc_fracao.store(f>=.25f?f:0.f);}
         nrrj::npc_amplia.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcAmplia",0,ini.c_str())==1?1:0);
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcLeve",0,ini.c_str()));nrrj::npc_leve.store(v>=1&&v<=3?v:0);
          nrrj::npc_leve_fracao.store(flt(L"NeuralRostoJogadorNpcLeveFracao",.33f,.2f,.6f));nrrj::npc_leve_salto.store(flt(L"NeuralRostoJogadorNpcLeveSalto",.6f,.3f,2.f));
          {const int r=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcLeveRecorte",1024,ini.c_str()));nrrj::npc_leve_recorte.store(r>=384&&r<=1024&&r%128==0?unsigned(r):1024u);}
          {const int r=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcLeveJanela",1024,ini.c_str()));nrrj::npc_leve_jan.store(r>=768&&r<=1536&&r%128==0?unsigned(r):1024u);}
          {const int r=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcLeveRede",512,ini.c_str()));nrrj::npc_leve_rede.store(r==384||r==512||r==640?unsigned(r):512u);}
          if(v&1){nrrj::npc_fracao.store(nrrj::npc_leve_fracao.load());nrrj::npc_amplia.store(1);}}
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcCanonico",0,ini.c_str()));nrrj::npc_canon.store(v==1||v==2?v:0);
          {const int pp=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcCanonPerto",2,ini.c_str()));nrrj::npc_canon_perto.store(std::clamp(pp,0,2));}
          {const int jq=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcCanonJanelaPeq",768,ini.c_str()));nrrj::npc_canon_jan_peq.store(jq>=512&&jq<=1024&&jq%128==0?unsigned(jq):768u);}
          nrrj::npc_canon_fracao.store(flt(L"NeuralRostoJogadorNpcCanonFracao",.36f,.2f,.6f));nrrj::npc_canon_razao.store(flt(L"NeuralRostoJogadorNpcCanonRazao",2.35f,1.f,5.f));
          nrrj::npc_canon_corte.store(flt(L"NeuralRostoJogadorNpcCanonCorte",.6f,.05f,10.f));nrrj::npc_canon_beta.store(flt(L"NeuralRostoJogadorNpcCanonBeta",4.f,0.f,100.f));
          {const int r=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcCanonRede",384,ini.c_str()));nrrj::npc_canon_rede.store(r==512?512u:384u);}}
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcTodos",0,ini.c_str()));nrrj::npc_todos.store(v>0&&v<=127?v:0);
          nrrj::npc_todos_nota.store(flt(L"NeuralRostoJogadorNpcTodosNota",.7f,.5f,.95f));
          nrrj::npc_todos_grandes.store(std::clamp(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcTodosGrandes",2,ini.c_str())),1,7));
          if(v>0&&v<=127){char lt[160]{};NRLOGF(lt,sizeof lt,"ROSTO_NPC2_TODOS bits=%d nota=%.2f (npc-max)",v,double(nrrj::npc_todos_nota.load()));nrrj::log(lt);}}
         nrrj::npc_limiar.store(flt(L"NeuralRostoJogadorNpcLimiar",0.f,0.f,1.f));
         nrrj::npc_peito.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcPeito",0,ini.c_str())==1?1:0);
         nrrj::yunet_rapido.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorYunetRapido",0,ini.c_str())==1?1:0);
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorYunetAvx",0,ini.c_str()));nrrj::yunet_simd.store(v==1||v==2?v:0);}
         nrrj::detector.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorDetector",0,ini.c_str())==2?2:0);
         nrrj::detector_threads.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorDetectorThreads",2,ini.c_str())),1,8));
         nrrj::scrfd_b_inteira.store(flt(L"NeuralRostoJogadorDetectorInteira",.6f,.35f,.9f));nrrj::scrfd_b_faixa.store(flt(L"NeuralRostoJogadorDetectorFaixa",.5f,.35f,.9f));
         if(nrrj::detector.load()){char ld[200]{};NRLOGF(ld,sizeof ld,"ROSTO_NPC2_DETECTOR scrfd threads=%d nota_0.7=%.2f/%.2f (inteira/faixa; ROI do NPC segue no YuNet)",nrrj::detector_threads.load(),double(nrrj::scrfd_b_inteira.load()),double(nrrj::scrfd_b_faixa.load()));nrrj::log(ld);}
         if(nrrj::npc2.load()){char l2[260]{};NRLOGF(l2,sizeof l2,"ROSTO_NPC2_CONFIG ligado npcs=%d teto=%u acumula=%d vel_max=%.2f segura=%.1f faixa=%u degraus=%d recmin=%d segura_peq=%.1f peito=%d rapido=%d avx=%d (pipeline do rosto para cada NPC)",nrrj::npc_max.load(),nrrj::npc2_teto.load(),
             nrrj::npc2_acumula.load()?1:0,double(nrrj::npc2_vel_max.load()),double(nrrj::npc2_segura_s.load()),nrrj::yunet_faixa.load(),nrrj::npc_degraus.load(),nrrj::npc_recorte_min2.load(),double(nrrj::npc_segura_peq.load()),nrrj::npc_peito.load(),nrrj::yunet_rapido.load(),nrrj::yunet_simd.load());nrrj::log(l2);}
         nrrj::npc_frente.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcFrente",0,ini.c_str())==1?1:0);
         nrrj::npc_conf_s.store(flt(L"NeuralRostoJogadorNpcConfS",0.f,0.f,1.f));
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcRoiConf",0,ini.c_str()));nrrj::npc_roi_conf.store(v>0&&v<=16?v:0);}
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcVelImg",0,ini.c_str()));nrrj::npc_vel_img.store(v==1||v==2?v:0);}
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcAtras",0,ini.c_str()));nrrj::npc_atras.store(v>=1&&v<=3?v:0);}
         nrrj::npc_camada.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcCamada",0,ini.c_str())==1?1:0);
         nrrj::npc_corpo.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcCorpo",0,ini.c_str())==1?1:0);
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcVida",0,ini.c_str()));nrrj::npc_vida.store(v==1||v==2?v:0);}
         nrrj::npc_vida_in.store(flt(L"NeuralRostoJogadorNpcVidaEntrada",.35f,0.f,2.f));
         if(nrrj::npc2.load()&&nrrj::npc_vida.load()){char lv[160]{};NRLOGF(lv,sizeof lv,"ROSTO_NPC2_VIDA modo=%d entrada_s=%.2f (pendente ate confirmar; saida pela rampa; na frente dela fica%s)",nrrj::npc_vida.load(),double(nrrj::npc_vida_in.load()),nrrj::npc_vida.load()>=2?"; oculto atras/na frente dela com a camada no quadro atual":"");nrrj::log(lv);}
         {const int v=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcCabMin",96,ini.c_str()));nrrj::npc_cab_min.store(v>=32&&v<=512?float(v):96.f);}

         {const int om=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorOrcamento",0,ini.c_str()));
          nrorc_vram::configurar(om==1||om==2?om:0,unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorOrcamentoFolgaMb",256,ini.c_str())),0,8192)),
              unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorOrcamentoOutrosMb",0,ini.c_str())),0,262144)));
          if(nrorc::modo()){nrregswin::orc_contar(base);nrorc_vram::hip_inicio();}}
         if(nrrj::npc2.load()){char l3[240]{};NRLOGF(l3,sizeof l3,"ROSTO_NPC2_CONFIG2 rosto_hz=%u frente=%d conf_s=%.3f inteira=%u adquire=%d roi_conf=%d vel_img=%d atras=%d cab_min=%.0f corpo=%d camada=%d",nrrj::rosto_hz.load(),nrrj::npc_frente.load(),double(nrrj::npc_conf_s.load()),nrrj::yunet_inteira.load(),nrrj::adquire.load(),nrrj::npc_roi_conf.load(),nrrj::npc_vel_img.load(),nrrj::npc_atras.load(),double(nrrj::npc_cab_min.load()),nrrj::npc_corpo.load(),nrrj::npc_camada.load());nrrj::log(l3);}
         if(nrrj::caminha_dela.load()){char l5[96]{};NRLOGF(l5,sizeof l5,"ROSTO_JOGADOR_CAMINHA_DELA_CONFIG caminha_dela=%d",nrrj::caminha_dela.load());nrrj::log(l5);}
         if(nrrj::reancora.load()){char l4[96]{};NRLOGF(l4,sizeof l4,"ROSTO_JOGADOR_REANCORA_CONFIG reancora=%d",nrrj::reancora.load());nrrj::log(l4);}
         if(nrrj::npc2.load()&&(nrrj::npc_fracao.load()>0.f||nrrj::npc_recorte_forcado.load()||nrrj::npc_limiar.load()>0.f)){char l4[160]{};NRLOGF(l4,sizeof l4,"ROSTO_NPC2_FRACAO f0=%.2f faixa=%.2f-%.2f teto=%u amplia=%d limiar=%.3f forcado=%d (npc-luz)",double(nrrj::npc_fracao.load()),.8*nrrj::npc_fracao.load(),1.3*nrrj::npc_fracao.load(),nrrj::npc2_teto.load(),nrrj::npc_amplia.load(),double(nrrj::npc_limiar.load()),nrrj::npc_recorte_forcado.load());nrrj::log(l4);}
         if(nrrj::npc2.load()&&nrrj::npc_canon.load()){char l6[200]{};NRLOGF(l6,sizeof l6,"ROSTO_NPC2_CANON_CONFIG fracao=%.2f razao=%.2f rede=%u corte=%.2f beta=%.2f (npc-canon)",double(nrrj::npc_canon_fracao.load()),double(nrrj::npc_canon_razao.load()),nrrj::npc_canon_rede.load(),double(nrrj::npc_canon_corte.load()),double(nrrj::npc_canon_beta.load()));nrrj::log(l6);}
         if(nrrj::npc2.load()&&nrrj::npc_leve.load()){char l5[200]{};NRLOGF(l5,sizeof l5,"ROSTO_NPC2_LEVE bits=%d fracao=%.2f rede=%u recorte=%u janela=%u salto=%.2f (npc-leve)",nrrj::npc_leve.load(),double(nrrj::npc_leve_fracao.load()),nrrj::npc_leve_rede.load(),nrrj::npc_leve_recorte.load(),nrrj::npc_leve_jan.load(),double(nrrj::npc_leve_salto.load()));nrrj::log(l5);}
         nrrj::npc_expande.store(flt(L"NeuralRostoJogadorNpcExpande",.8f,.0001f,16.f));
         if(nrrj::npc.load()){char ln[240]{};NRLOGF(ln,sizeof ln,"ROSTO_JOGADOR_NPC_CONFIG modo=%d min=%d recorte=%d forca=%.2f expande=%.4f rosto_hz=%u rosto_nota=%.2f",nrrj::npc_modo.load(),nrrj::npc_min.load(),nrrj::npc_recorte.load(),double(nrrj::npc_forca.load()),double(nrrj::npc_expande.load()),nrrj::rosto_hz.load(),double(nrrj::rosto_nota.load()));nrrj::log(ln);}
         nrrj::segura.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorSegura",0,ini.c_str())),0,64)));
         nrrj::segura_pixel.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorSeguraPixel",0,ini.c_str())==1);
         nrrj::compor_rapido.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorComporRapido",0,ini.c_str())==1);

         nrrj::baixa_forca.store(flt(L"NeuralRostoJogadorBaixaForca",1.f,0.f,2.f));
         nrrj::banda_sigma.store(flt(L"NeuralRostoJogadorBandaSigma",16.f,4.f,2048.f));
         nrrj::corte_ok_ini.store(flt(L"NeuralRostoJogadorCorteOk",.07f,0.f,1.f));
         nrrj::desliga_jobs.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorDesligaJobs",12,ini.c_str())),1,6000));
         nrrj::npc_todos_rede.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcTodosRede",768,ini.c_str())),384,1536))&~127u);
         nrrj::npc_todos_jan.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorNpcTodosJanela",768,ini.c_str())),384,2304))&~127u);
         nrrj::npc_todos_cab_max.store(flt(L"NeuralRostoJogadorNpcTodosCabMax",700.f,100.f,4000.f));
         nrrj::busca_sempre.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorBuscaSempre",0,ini.c_str())==1);
         nrrj::ceu_fora.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorCeuFora",0,ini.c_str())==1);
         nrrj::npc_zona.store(flt(L"NeuralRostoJogadorNpcZona",1.15f,.3f,3.f));
         nrrj::diag_hist.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorDiagHist",0,ini.c_str())==1);
         nrrj::liga_forte.store(flt(L"NeuralRostoJogadorLigaForte",.75f,0.f,1.f));
         nrrj::adq_nota1.store(flt(L"NeuralRostoJogadorAdqNota1",.60f,0.f,1.f));
         nrrj::camada.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoCamada",0,ini.c_str())==1);
         nrrj::camada_segura.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoCamadaSegura",90,ini.c_str())),0,1200));
         nrrj::camada_meia.store(flt(L"NeuralRostoCamadaMeiaVida",20.f,1.f,600.f));
         nrrj::camada_margem.store(flt(L"NeuralRostoCamadaMargem",48.f,0.f,1024.f));
         nrrj::camada_forca.store(flt(L"NeuralRostoCamadaForca",1.f,0.f,2.f));nrrj::camada_normaliza.store(flt(L"NeuralRostoCamadaNormaliza",0.f,0.f,1.f)>.5f);nrrj::camada_fresco.store(flt(L"NeuralRostoCamadaFresco",0.f,0.f,1.f)>.5f);
         nrrj::teste_falha_ini.store(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoTesteFalhaIni",0,ini.c_str())));
         nrrj::teste_falha_fim.store(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoTesteFalhaFim",0,ini.c_str())));
         if(nrrj::camada.load()){char lc[200]{};NRLOGF(lc,sizeof lc,"ROSTO_CAMADA_CONFIG segura=%d meia_vida=%.1f margem=%.1f forca=%.2f",nrrj::camada_segura.load(),double(nrrj::camada_meia.load()),double(nrrj::camada_margem.load()),double(nrrj::camada_forca.load()));nrrj::log(lc);}
         nrrj::baixa_lenta.store(flt(L"NeuralRostoJogadorBaixaLenta",0.f,0.f,1.f));
         nrrj::baixa_rapida.store(flt(L"NeuralRostoJogadorBaixaRapida",0.f,0.f,1.f));
         nrrj::baixa_mov_px.store(flt(L"NeuralRostoJogadorBaixaMovPx",4.f,.25f,64.f));
         nrrj::acum_limiar.store(flt(L"NeuralRostoJogadorAcumLimiar",0.f,0.f,1.f));
         nrrj::borda_largura.store(flt(L"NeuralRostoJogadorBordaLargura",0.f,0.f,6.f));
         nrrj::borda_suave.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorBordaSuave",0,ini.c_str())),0,3)));

         nrrj::borda_camada.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorBordaCamada",0,ini.c_str())),0,2)));
         nrrj::borda_camada_reg.store(flt(L"NeuralRostoJogadorBordaCamadaReg",.05f,.001f,1.f));
         nrrj::borda_camada_z0.store(flt(L"NeuralRostoJogadorBordaCamadaZ0",1.15f,1.01f,8.f));
         nrrj::borda_camada_z1.store((std::max)(nrrj::borda_camada_z0.load()+.01f,flt(L"NeuralRostoJogadorBordaCamadaZ1",1.45f,1.02f,16.f)));
         nrrj::borda_teto.store(flt(L"NeuralRostoJogadorBordaTeto",0.f,0.f,8.f));
         if(nrrj::borda_teto.load()>0.f){char l6[100]{};NRLOGF(l6,sizeof l6,"ROSTO_JOGADOR_R5 borda_teto=%.2f",double(nrrj::borda_teto.load()));nrrj::log(l6);}
         nrrj::borda_camada_perto.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorBordaCamadaPerto",0,ini.c_str())==1);
         if(nrrj::borda_camada.load()){char l5[160]{};NRLOGF(l5,sizeof l5,"ROSTO_JOGADOR_R5 borda_camada=%u reg=%.3f perto=%d z=%.2f..%.2f",nrrj::borda_camada.load(),double(nrrj::borda_camada_reg.load()),nrrj::borda_camada_perto.load()?1:0,double(nrrj::borda_camada_z0.load()),double(nrrj::borda_camada_z1.load()));nrrj::log(l5);}
         if(nrrj::baixa_forca.load()!=1.f||nrrj::baixa_lenta.load()>0.f||nrrj::acum_limiar.load()>0.f||nrrj::borda_suave.load()!=0){char l4[220]{};NRLOGF(l4,sizeof l4,"ROSTO_JOGADOR_R4 baixa_forca=%.3f baixa_lenta=%.3f baixa_rapida=%.3f mov_px=%.2f limiar=%.4f borda_suave=%u largura=%.2f",double(nrrj::baixa_forca.load()),double(nrrj::baixa_lenta.load()),double(nrrj::baixa_rapida.load()),double(nrrj::baixa_mov_px.load()),double(nrrj::acum_limiar.load()),nrrj::borda_suave.load(),double(nrrj::borda_largura.load()));nrrj::log(l4);}
         if(nrrj::passo2_cada.load()>1||nrrj::rede_alvo.load()>0.f||nrrj::compor_elipse.load()||nrrj::compor_rapido.load()||nrrj::ablacao.load()||nrrj::passo1_forca.load()!=1.f||nrrj::segura.load()){char lr[220]{};NRLOGF(lr,sizeof lr,"ROSTO_JOGADOR_R3 passo2_cada=%u rede_alvo=%.0f compor_elipse=%d compor_rapido=%d ablacao=%u forca1=%.2f segura=%u segura_pixel=%d",nrrj::passo2_cada.load(),double(nrrj::rede_alvo.load()),nrrj::compor_elipse.load()?1:0,nrrj::compor_rapido.load()?1:0,nrrj::ablacao.load(),double(nrrj::passo1_forca.load()),nrrj::segura.load(),nrrj::segura_pixel.load()?1:0);nrrj::log(lr);}
         if(nrrj::frescor.load()>0.f){char lf[120]{};NRLOGF(lf,sizeof lf,"ROSTO_JOGADOR_FRESCOR limiar=%.3f",double(nrrj::frescor.load()));nrrj::log(lf);}
         if(nrrj::pula_limiar.load()!=0.f||nrrj::passos.load()>1){char lp[200]{};NRLOGF(lp,sizeof lp,"ROSTO_JOGADOR_R2 pula=%.4f pula_max=%u passos=%u forca2=%.2f folga2=%.2f",double(nrrj::pula_limiar.load()),nrrj::pula_max.load(),nrrj::passos.load(),double(nrrj::passo2_forca.load()),double(passo2_folga.load()));nrrj::log(lp);}

         nrrj::grade_rede.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorGrade",0,ini.c_str())),0,256)));
         nrrj::recorte_fator.store(flt(L"NeuralRostoJogadorRecorteFator",0.f,0.f,1.5f));
         {const float ev=flt(L"NeuralRostoJogadorEspera",0.f,-2.f,12.f);nrrj::espera_us.store(ev<=-1.5f?nrrj::espera_present:ev<0.f?nrrj::espera_auto:unsigned(std::lround(ev*1000.f)));}
         nrrj::espera_job.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorEsperaJob",0,ini.c_str())==1);
         nrrj::thread_rede.store(nrrj::fora.load()&&NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorThreadRede",0,ini.c_str())==1);
         nrregswin::rjsync::ligado.store(nrrj::thread_rede.load());
         if(nrrj::thread_rede.load())nrrj::log("ROSTO_JOGADOR_THREAD_REDE rede do recorte numa thread propria (depois do k_flag_set da cena)");

         {const int sj=int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorSemJob",0,ini.c_str()));
          nrrj::sem_job_q.store(nrrj::thread_rede.load()?nrrj::sj_q_de(sj,nrrj::cada_quadros.load()):0u);
          if(sj){char ls[160]{};NRLOGF(ls,sizeof ls,"ROSTO_JOGADOR_SEMJOB_CONFIG pedido=%d q=%u thread_rede=%d",sj,nrrj::sj_q(),nrrj::thread_rede.load()?1:0);nrrj::log(ls);}}
         if(nrrj::espera_us.load()){char lc[120]{};NRLOGF(lc,sizeof lc,"ROSTO_JOGADOR_ESPERA %u us depois do publish so_job_com_recorte=%d",nrrj::espera_us.load(),nrrj::espera_job.load()?1:0);nrrj::log(lc);}
         if(nrrj::grade_rede.load()||nrrj::recorte_fator.load()>0.f){char lb[160]{};NRLOGF(lb,sizeof lb,"ROSTO_JOGADOR_RECORTE grade=%u fator=%.3f",nrrj::grade_rede.load(),double(nrrj::recorte_fator.load()));nrrj::log(lb);}nrrj::acum_sigma.store(flt(L"NeuralRostoJogadorAcumSigma",6.f,1.f,32.f));
         if(nrrj::acumula.load()){char la[200]{};NRLOGF(la,sizeof la,"ROSTO_JOGADOR_ACUMULA baixa=%.3f alta=%.3f gama=%.2f sigma=%.1f",double(nrrj::acum_baixa.load()),double(nrrj::acum_alta.load()),double(nrrj::acum_gama.load()),double(nrrj::acum_sigma.load()));nrrj::log(la);}}
        char linha[240]{};NRLOGF(linha,sizeof linha,"ROSTO_JOGADOR_CONFIG ligado=%d teto=%u zoom=%u cada=%u fora=%d det_thread=%d menu=%d adquire=%d",nrrj::ligado.load()?1:0,nrrj::teto.load(),nrrj::zoom.load(),nrrj::cada.load(),nrrj::fora.load()?1:0,det_thread.load()?1:0,nrrj::menu_escreveu.load()?1:0,nrrj::adquire.load());
        nrrj::log(linha);
        nrorc::estado().ini_ok.store(true,std::memory_order_release);
    }catch(...){}
}
inline void lancar(Hip& hip,void* k,size_t n,void** args){check(hip.launch(k,unsigned((n+255)/256),1,1,256,1,1,0,nullptr,args,nullptr));}
inline bool modulo(Estado& e,Hip& hip){
    if(e.k_etiqueta)return true;
    if(nrorc::aplicada()&&nrrj::inst_atual()>=1){Estado& e0=estado_de(0);{nrrj::Inst i0(0);if(!modulo(e0,hip))return false;}compartilhar(e,e0);return true;}
    if(e.modulo_tentado)return false;
    e.modulo_tentado=true;
    const auto arquivo=pasta_helper()/L"nr_rosto.hsaco";std::error_code ec;
    if(!std::filesystem::is_regular_file(arquivo,ec)){nrrj::envenenar("nr_rosto.hsaco_ausente");return false;}
    if(hip.load_module(&e.module,arquivo.string().c_str())||!e.module){(void)hip.error();nrrj::envenenar("nr_rosto.hsaco_recusado");return false;}
    const char* nomes[]={"rj_reduz_v1","rj_cena_v1","rj_entrada_v1","rj_dif_v1","rj_baixa_v1","rj_blur4_v1","rj_blur1_v1","rj_delta_v1","rj_guarda_v1","rj_mede_v1","rj_etiqueta_v1"};
    void** alvos[]={&e.k_reduz,&e.k_cena,&e.k_entrada,&e.k_dif,&e.k_baixa,&e.k_blur4,&e.k_blur1,&e.k_delta,&e.k_guarda,&e.k_mede,&e.k_etiqueta};
    for(int i=0;i<11;++i)if(hip.get_kernel(alvos[i],e.module,nomes[i])){(void)hip.error();e.k_etiqueta=nullptr;nrrj::envenenar("simbolos_v1_ausentes");return false;}
    if(hip.get_kernel(&e.k_meia4,e.module,"rj_meia4_v1")||hip.get_kernel(&e.k_bandas,e.module,"rj_bandas_v1")){(void)hip.error();e.k_meia4=e.k_bandas=nullptr;}
    if(hip.get_kernel(&e.k_mini,e.module,"rj_mini_v1")||hip.get_kernel(&e.k_sat,e.module,"rj_sat_v1")){(void)hip.error();e.k_mini=e.k_sat=nullptr;}
    if(hip.get_kernel(&e.k_lumin,e.module,"rj_lumin_v1")||hip.get_kernel(&e.k_bandas2,e.module,"rj_bandas2_v1")){(void)hip.error();e.k_lumin=e.k_bandas2=nullptr;}
    if(hip.get_kernel(&e.k_camada,e.module,"rj_camada_v1")||hip.get_kernel(&e.k_baixa_m,e.module,"rj_baixa_m_v1")||hip.get_kernel(&e.k_blur4w,e.module,"rj_blur4w_v1")||hip.get_kernel(&e.k_delta_m,e.module,"rj_delta_m_v1")){(void)hip.error();e.k_camada=e.k_baixa_m=e.k_blur4w=e.k_delta_m=nullptr;}
    nrrj::log("ROSTO_JOGADOR_MODULO nr_rosto.hsaco carregado");
    return true;
}
inline bool grades(Estado& e,Hip& hip){
    if(e.grades)return true;
    if(nrorc::aplicada()&&nrrj::inst_atual()>=1){compartilhar(e,estado_de(0));return e.grades;}
    const size_t n2=size_t(nrrj::n_cap())*nrrj::n_cap(),nl2=n2/16;
    void** f4[]={&e.dc,&e.d0,&e.D};void** fl[]={&e.lo,&e.lo2};void** f1[]={&e.luma,&e.gl,&e.tmp1};
    bool ok=true;
    for(auto p:f4)ok=ok&&!hip.allocate(p,n2*16);
    for(auto p:fl)ok=ok&&!hip.allocate(p,nl2*16);
    for(auto p:f1)ok=ok&&!hip.allocate(p,n2*4);
    ok=ok&&!hip.allocate(&e.soma,16);
    if(!ok){(void)hip.error();nrrj::envenenar("hipMalloc grades");return false;}
    e.grades=true;
    char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_GRADES %.1f MB",double(n2*(48+12)+nl2*32+16)/1048576.0);nrrj::log(l);
    return true;
}
inline bool importar_buffer(void* h,std::uint64_t bytes,void*& externo,char*& base_hip,const char* nome){
    if(base_hip)return true;
    if(!h)return false;
    using Import=int(*)(void**,const ExternalDesc*);using Map=int(*)(void**,void*,const BufferDesc*);
    HMODULE m=GetModuleHandleW(L"amdhip64_7.dll");if(!m)throw std::runtime_error("HIP ausente");
    auto imp=reinterpret_cast<Import>(GetProcAddress(m,"hipImportExternalMemory"));auto map=reinterpret_cast<Map>(GetProcAddress(m,"hipExternalMemoryGetMappedBuffer"));
    if(!imp||!map)throw std::runtime_error("hipImportExternalMemory ausente");
    ExternalDesc d{};d.type=5;d.handle.win32.handle=h;d.size=bytes;d.flags=1;
    if(const int r=imp(&externo,&d)){char t[96]{};NRLOGF(t,sizeof t,"hipImportExternalMemory=%d",r);throw std::runtime_error(t);}
    BufferDesc b{};b.size=bytes;void* p{};
    if(const int r=map(&p,externo,&b)){char t[96]{};NRLOGF(t,sizeof t,"hipExternalMemoryGetMappedBuffer=%d",r);throw std::runtime_error(t);}
    base_hip=static_cast<char*>(p);
    char t[160]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_IMPORTADO %s bytes=%llu",nome,static_cast<unsigned long long>(bytes));nrrj::log(t);
    return true;
}
inline bool importar(Estado& e){auto& g=nrrj::global();

    if(nrorc::aplicada()&&nrrj::inst_atual()>=1){
        if(e.base_hip)return true;
        static void* ext{};static char* base{};
        if(!importar_buffer(g.handle.load(std::memory_order_acquire),g.bytes.load(),ext,base,"pool_npc"))return false;
        e.externo=ext;e.base_hip=base+g.offset.load(std::memory_order_relaxed);return true;}
    return importar_buffer(g.handle.load(std::memory_order_acquire),g.bytes.load(),e.externo,e.base_hip,"recorte");}

inline constexpr std::uint64_t marca_rosto_legado=0x4E52313652535430ull;
inline std::filesystem::path pasta_proxy(char* base){
    wchar_t path[32768];const auto length=GetModuleFileNameW(reinterpret_cast<HMODULE>(base),path,32768);
    if(!length||length>=32768)throw std::runtime_error("caminho do proxy");
    return std::filesystem::path(path).parent_path();}
inline bool hibrido_rosto(char* base){
    static const bool v=[&]{std::error_code ec;const bool h=std::filesystem::exists(pasta_proxy(base)/L"dlssnr_on_amd_weights_legado.bin",ec)&&!ec;
        nrrj::log(h?"ROSTO_HIBRIDO ligado: rostos na rede velha (dlssnr_on_amd_weights_legado.bin + marca +0x3F8), cena na rede do proxy"
                   :"ROSTO_HIBRIDO desligado (sem dlssnr_on_amd_weights_legado.bin)");return h;}();
    return v;}
inline std::string pesos_rosto(char* base){
    return (pasta_proxy(base)/(hibrido_rosto(base)?L"dlssnr_on_amd_weights_legado.bin":L"dlssnr_on_amd_weights.bin")).string();}
inline void marcar_rosto(unsigned char* p,char* base){if(hibrido_rosto(base))write<std::uint64_t>(p,0x3f8,marca_rosto_legado);}
inline void* contexto(Estado& e0,Hip& hip,char* base,const void* primary){

    Estado& e=ctx_unico()?estado_de(0):(nrrj::inst_atual()>=2?estado_de(1):e0);
    auto p=e.ctx.data();
    if(!e.construido){native<void*(*)(void*)>(base,0x1ce20)(p);e.construido=true;}
    if(!e.carregado){
        const auto t=std::chrono::steady_clock::now();
        std::string pesos=pesos_rosto(base);
        const bool ok=native<bool(*)(void*,const std::string*)>(base,0x1e120)(p,&pesos);
        hip.completed("rj_pesos");
        if(!ok||!read<void*>(p,0x10)||!read<void*>(p,0x168)||!read<size_t>(p,0x160))throw std::runtime_error("pesos do recorte indisponiveis");
        e.carregado=true;
        char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_CONTEXTO pesos carregados em %.1f ms",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count());nrrj::log(l);
    }
    std::memcpy(p+0x20,static_cast<const char*>(primary)+0x20,20);

    write<unsigned>(p,0xf0,0);write<unsigned>(p,0x120,0);write<unsigned>(p,0x38,0);write<void*>(p,0x118,nullptr);
    marcar_rosto(p,base);
    return p;
}

inline unsigned raio_cv(float s) noexcept {const long k=std::lround(double(s)*8.0+1.0)|1L;return unsigned(std::clamp<long>(k/2,1,256));}
inline void despejar(Hip& hip,const void* dev,size_t bytes,const std::filesystem::path& arquivo){
    std::vector<char> h(bytes);check(hip.copy(h.data(),dev,bytes,2));
    std::ofstream f(arquivo,std::ios::binary);f.write(h.data(),std::streamsize(bytes));
}

inline unsigned despejo_n(){return 0;}

inline unsigned r1_modo(){return 0;}

inline unsigned r3_modo(){return 0;}

inline bool tempos_modo(){return false;}
struct Tempos {std::chrono::steady_clock::time_point t;double ms[6]{};int i{};
    void marca(Hip& hip){if(!tempos_modo())return;hip.completed("rj_tempos");const auto a=std::chrono::steady_clock::now();if(i>0&&i<=6)ms[i-1]=std::chrono::duration<double,std::milli>(a-t).count();t=a;++i;}};
inline float meia(std::uint16_t h) noexcept {
    const unsigned s=(h>>15)&1u,e=(h>>10)&31u,m=h&1023u;float v;
    if(e==0)v=std::ldexp(float(m),-24);else if(e==31)v=m?NAN:INFINITY;else v=std::ldexp(float(m|1024u),int(e)-25);
    return s?-v:v;
}
struct R1Medida {double chamada{},total{};};
inline R1Medida r1_eval(Hip& hip,char* base,void* p,char* ent,char* sai,unsigned n){
    hip.completed("rj_r1");
    const auto a0=std::chrono::steady_clock::now();
    native<Eval>(base,0x2a080)(p,ent,int(n*8),0,int(n),int(n),sai,int(n*8),0,0,nullptr);
    const auto a1=std::chrono::steady_clock::now();
    hip.completed("rj_r1");
    const auto a2=std::chrono::steady_clock::now();
    return {std::chrono::duration<double,std::milli>(a1-a0).count(),std::chrono::duration<double,std::milli>(a2-a0).count()};
}

inline void r3_diag(Estado& e,Hip& hip,char* base,const void* primary,char* ent,char* sai,unsigned n){
    static void* din{};static void* dout{};
    if(!din){if(hip.allocate(&din,size_t(nrrj::n_max)*nrrj::n_max*8)||hip.allocate(&dout,size_t(nrrj::n_max)*nrrj::n_max*8)){(void)hip.error();din=dout=nullptr;return;}}
    const size_t N=size_t(n)*n;const int nn=int(n);
    std::vector<std::uint16_t> hin(N*4),hout(N*4),hb(N*4);
    check(hip.copy(hin.data(),ent,N*8,2));check(hip.copy(hb.data(),sai,N*8,2));
    auto residuo=[&](const std::vector<std::uint16_t>& in,const std::vector<std::uint16_t>& out,std::vector<float>& r){
        r.assign(N,0.f);
        for(size_t i=0;i<N;++i){float a[3];for(int c=0;c<3;++c){float s=meia(out[i*4+c]);s=s<0.f?0.f:(s>1.f?1.f:s);a[c]=s-meia(in[i*4+c]);}
            const float l=.2126f*a[0]+.7152f*a[1]+.0722f*a[2];r[i]=std::isfinite(l)?l:0.f;}};
    auto alta=[&](const std::vector<float>& r){
        std::vector<double> I;const int W=nn+1;I.assign(size_t(W)*(nn+1),0.0);
        for(int y=0;y<nn;++y){double row=0;for(int x=0;x<nn;++x){row+=r[size_t(y)*n+x];I[size_t(y+1)*W+x+1]=I[size_t(y)*W+x+1]+row;}}
        std::vector<float> o(N);
        for(int y=0;y<nn;++y)for(int x=0;x<nn;++x){const int x0=(std::max)(0,x-8),x1=(std::min)(nn,x+9),y0=(std::max)(0,y-8),y1=(std::min)(nn,y+9);
            const double s=I[size_t(y1)*W+x1]-I[size_t(y0)*W+x1]-I[size_t(y1)*W+x0]+I[size_t(y0)*W+x0];o[size_t(y)*n+x]=r[size_t(y)*n+x]-float(s/double((x1-x0)*(y1-y0)));}
        return o;};
    std::vector<float> rb;residuo(hin,hb,rb);const auto rbh=alta(rb);
    auto roda=[&](const std::vector<std::uint16_t>& entrada,std::vector<float>& r){
        check(hip.copy(din,entrada.data(),N*8,1));void* p=contexto(e,hip,base,primary);
        native<Eval>(base,0x2a080)(p,din,int(n*8),0,nn,nn,dout,int(n*8),0,0,nullptr);hip.completed("rj_r3");
        check(hip.copy(hout.data(),dout,N*8,2));residuo(entrada,hout,r);};
    auto compara=[&](const std::vector<float>& r,int sx,int sy,const char* nome){
        const auto rh=alta(r);double d=0,dh=0,ref=0,refh=0;size_t c=0;
        for(int y=64;y<nn-64-sy;++y)for(int x=64;x<nn-64-sx;++x){const size_t i=size_t(y)*n+x,j=size_t(y+sy)*n+(x+sx);
            d+=std::fabs(r[i]-rb[j]);dh+=std::fabs(rh[i]-rbh[j]);ref+=std::fabs(rb[j]);refh+=std::fabs(rbh[j]);++c;}
        if(!c)return;
        char t[260]{};NRLOGF(t,sizeof t,"ROSTO_R3 aval=%llu n=%u %s sx=%d sy=%d dif=%.6f dif_alta=%.6f ref=%.6f ref_alta=%.6f rel=%.4f rel_alta=%.4f",
            e.avaliados+1,n,nome,sx,sy,d/double(c),dh/double(c),ref/double(c),refh/double(c),ref>0?d/ref:0.,refh>0?dh/refh:0.);nrrj::log(t);};
    std::vector<float> r;
    roda(hin,r);compara(r,0,0,"mesma");
    std::vector<std::uint16_t> hs(N*4);
    for(int s:{1,2,4,8,16,32,64,128})for(int modo=0;modo<2;++modo){
        const int sx=s,sy=modo==0?s:0;
        for(int y=0;y<nn;++y)for(int x=0;x<nn;++x){const size_t i=size_t(y)*n+x,j=size_t((std::min)(y+sy,nn-1))*n+size_t((std::min)(x+sx,nn-1));
            for(int c=0;c<4;++c)hs[i*4+c]=hin[j*4+c];}
        roda(hs,r);compara(r,sx,sy,modo==0?"diag":"x");
    }
}

#pragma warning(push)
#pragma warning(disable:4324)
struct Det {
    DetSet sets[2];
    std::mutex m;std::condition_variable cv;DetTarefa fila{};bool tem{};bool thread_tentada{},thread_ok{};
    std::vector<float> h_dq,h_mvq,h_d;
    nrrj::Rastreio rastreio;std::uint64_t quadro_ant{};bool tem_ant{};float est_ant[3]{};std::uint64_t ger_vista{};
    struct N2Log {bool tem_ant{};float est_ant[3]{};std::uint64_t log_q{};double t0{-1.0};unsigned long long movs0{},saltos0{};};
    N2Log n2l[nrrj::npc_slots];
    std::vector<nrrj::RostoDet> rostos;std::uint64_t rostos_versao{},rostos_quadro{};double rostos_t{-1.0};
    std::vector<nrrj::RostoDet> roi[nrrj::npc_slots];std::uint64_t roi_versao[nrrj::npc_slots]{},roi_quadro[nrrj::npc_slots]{};double roi_t[nrrj::npc_slots]{-1.0,-1.0,-1.0,-1.0,-1.0,-1.0,-1.0};
    unsigned long long deteccoes{},on_jobs{},cortes{},cortes_fsr{};double det_ms{},det_max{};unsigned det_n{};
    std::atomic<unsigned long long> pulados{0},descartados{0};

    bool on_ant{};unsigned long long trocas{},off_seguidos{};unsigned motivos_off{};

    std::atomic<unsigned> sem_cabeca{0};std::atomic<unsigned long long> economizados{0};
};
inline constexpr unsigned economia_apos=240,economia_cada=4;
#pragma warning(pop)
inline Det& det(){static Det* d=new Det;return *d;}

inline void laco_rostos() noexcept {
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_LOWEST);
    nryn::Rede rede;std::vector<std::uint16_t> hb;std::vector<float> lin;std::vector<std::uint8_t> bgr;std::vector<nryn::Rosto> rs;std::uint32_t ult=0;
    nryn::Rede rede_roi;std::uint32_t ult_roi[nrrj::npc_slots]{};double roi_ms_soma=0;unsigned long long roi_n=0;
    int simd_ant=-1;
    nrsc::Rede* scrfd=nullptr;bool scrfd_falhou=false;int scrfd_thr=-1;
    for(;;){
        Sleep(nrrj::npc2.load(std::memory_order_relaxed)?3:10);
        {const int sm=nrrj::yunet_simd.load(std::memory_order_relaxed);if(sm!=simd_ant){simd_ant=sm;rede.usar_simd(sm);rede_roi.usar_simd(sm);
             char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ROSTOS yunet simd=%d (pedido %d; 0 escalar, 1 AVX, 2 AVX-512)",rede.simd(),sm);nrrj::log(l);}}
        const bool usa_scrfd=nrrj::detector.load(std::memory_order_relaxed)==2;
        if(usa_scrfd&&!scrfd&&!scrfd_falhou){
            wchar_t path[32768]{};HMODULE self{};char erro[160]{"caminho do helper"};bool ok=false;
            if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&laco_rostos),&self)){
                const DWORD n=GetModuleFileNameW(self,path,32768);
                if(n&&n<32768){std::wstring p(path,n);const size_t barra=p.find_last_of(L"\\/");p=(barra==std::wstring::npos?std::wstring():p.substr(0,barra+1))+L"nr_scrfd.bin";
                    try{scrfd=new nrsc::Rede;ok=scrfd->carregar(p.c_str(),erro,sizeof erro);}catch(...){std::snprintf(erro,sizeof erro,"excecao");}}}
            if(!ok){delete scrfd;scrfd=nullptr;scrfd_falhou=true;}
            char l[240]{};NRLOGF(l,sizeof l,ok?"ROSTO_JOGADOR_ROSTOS scrfd carregado (nr_scrfd.bin %s)":"ROSTO_JOGADOR_ROSTOS scrfd FALLBACK (%s): segue o YuNet",erro);nrrj::log(l);}
        if(scrfd){const int tn=nrrj::detector_threads.load(std::memory_order_relaxed);if(tn!=scrfd_thr){scrfd_thr=tn;scrfd->usar_threads(tn,THREAD_PRIORITY_LOWEST);
             char l[120]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ROSTOS scrfd threads=%d",scrfd->threads());nrrj::log(l);}}
        try{

            bool fez_roi=false;
            for(int k=0;k<nrrj::npc_slots;++k){auto& RR=nrrj::roi_img(k);const unsigned char* rb=RR.base.load(std::memory_order_acquire);
             int ir=-1;std::uint32_t sr=0;
             if(rb)for(unsigned i=0;i<nrrj::roi_aneis;++i){const auto s=RR.ent[i].seq.load(std::memory_order_acquire);
                 if(s>ult_roi[k]&&s>sr&&nrrj::global().marcado(nrrj::roi_marca0+unsigned(k)*nrrj::roi_aneis+i)==s){sr=s;ir=int(i);}}
             if(ir>=0){const auto& E=RR.ent[unsigned(ir)];const int x0=E.x0,y0=E.y0,L=E.l;const std::uint64_t quadro=E.quadro;const double t_cap=E.t;
                 const int n=int(nrrj::roi_lado);hb.resize(size_t(n)*size_t(n)*4);std::memcpy(hb.data(),rb+std::uint64_t(ir)*nrrj::roi_bytes,hb.size()*2);
                 if(E.seq.load(std::memory_order_acquire)==sr&&L>0){ult_roi[k]=sr;fez_roi=true;
                     const auto t0=std::chrono::steady_clock::now();
                     if(nrrj::yunet_rapido.load(std::memory_order_relaxed))nryn::tom_bgr_meia(hb.data(),n,n,bgr,meia);
                     else{lin.resize(size_t(n)*size_t(n)*3);for(size_t i=0;i<size_t(n)*size_t(n);++i)for(int c=0;c<3;++c)lin[i*3+size_t(c)]=meia(hb[i*4+size_t(c)]);
                     nryn::tom_bgr(lin.data(),n,n,bgr);}
                     if(scrfd&&(nrrj::npc_todos.load(std::memory_order_relaxed)&1)){scrfd->detectar(bgr.data(),n,n,n*3,rs,.3f,.4f);for(auto& r:rs)r.nota=nrrj::nota_scrfd(r.nota,false);}
                     else rede_roi.detectar(bgr.data(),n,n,n*3,rs,.3f,.3f);
                     const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();roi_ms_soma+=ms;++roi_n;
                     const float s=float(L)/float(n);auto& RS=nrrj::rostos_roi(k);
                     {std::lock_guard<std::mutex> l(RS.m);RS.lista.clear();
                         for(const auto& r:rs){RS.lista.push_back({float(x0)+r.x*s,float(y0)+r.y*s,r.w*s,r.h*s,r.nota});auto& b=RS.lista.back();b.px=r.h;for(int i=0;i<5;++i){b.lm[2*i]=float(x0)+r.lm[2*i]*s;b.lm[2*i+1]=float(y0)+r.lm[2*i+1]*s;}}
                         ++RS.versao;RS.quadro=quadro;RS.t=t_cap;RS.ms=ms;++RS.rodadas;}
                     if(roi_n<=4||roi_n%300u==0){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_NPC2_ROI rodadas=%llu npc=%d rostos=%zu ms=%.2f ms_medio=%.2f roi=%d,%d/%d",roi_n,k+2,rs.size(),ms,roi_ms_soma/double(roi_n),x0,y0,L);nrrj::log(l);}}}}
            if(fez_roi)continue;
            auto& RI=nrrj::rosto_img();const unsigned char* base=RI.base.load(std::memory_order_acquire);if(!base)continue;
            int ib=-1;std::uint32_t sb=0;
            for(unsigned i=0;i<nrrj::rosto_aneis;++i){const auto s=RI.ent[i].seq.load(std::memory_order_acquire);
                if(s>ult&&s>sb&&nrrj::global().marcado(nrrj::rosto_marca0+i)==s){sb=s;ib=int(i);}}
            if(ib<0)continue;
            const auto& E=RI.ent[unsigned(ib)];const int w=E.w,h=E.h,ow=E.ow,oh=E.oh;const std::uint64_t quadro=E.quadro;const double t_cap=E.t;
            const int rx0=E.x0,ry0=E.y0,rw=E.rw>0?E.rw:ow,rh=E.rh>0?E.rh:oh;
            if(w<32||h<32||std::uint64_t(w)*h*8u>nrrj::rosto_bytes){ult=sb;continue;}
            hb.resize(size_t(w)*size_t(h)*4);std::memcpy(hb.data(),base+std::uint64_t(ib)*nrrj::rosto_bytes,hb.size()*2);
            if(E.seq.load(std::memory_order_acquire)!=sb)continue;
            ult=sb;
            const auto t0=std::chrono::steady_clock::now();
            if(nrrj::yunet_rapido.load(std::memory_order_relaxed))nryn::tom_bgr_meia(hb.data(),w,h,bgr,meia);
            else{lin.resize(size_t(w)*size_t(h)*3);for(size_t i=0;i<size_t(w)*size_t(h);++i)for(int c=0;c<3;++c)lin[i*3+size_t(c)]=meia(hb[i*4+size_t(c)]);
            nryn::tom_bgr(lin.data(),w,h,bgr);}
            const bool faixa=ry0>0,sc=usa_scrfd&&scrfd;
            if(sc){scrfd->detectar(bgr.data(),w,h,w*3,rs,nrrj::scrfd_a(faixa),.4f);
                for(auto& r:rs)r.nota=nrrj::nota_scrfd(r.nota,faixa);}
            else rede.detectar(bgr.data(),w,h,w*3,rs,.4f,.3f);
            const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
            auto& RS=nrrj::rostos();const float sx=float(rw)/float(w),sy=float(rh)/float(h);
            {std::lock_guard<std::mutex> l(RS.m);RS.lista.clear();
                for(const auto& r:rs){RS.lista.push_back({float(rx0)+r.x*sx,float(ry0)+r.y*sy,r.w*sx,r.h*sy,r.nota});auto& b=RS.lista.back();b.px=r.h;for(int i=0;i<5;++i){b.lm[2*i]=float(rx0)+r.lm[2*i]*sx;b.lm[2*i+1]=float(ry0)+r.lm[2*i+1]*sy;}}
                ++RS.versao;RS.quadro=quadro;RS.t=t_cap;RS.ms=ms;++RS.rodadas;}
            if(RS.rodadas<=4||RS.rodadas%100u==0){float m=0,nm=0;for(const auto& r:rs)if(r.h*sy>m){m=r.h*sy;nm=r.nota;}
                char l[220]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ROSTOS rodadas=%llu rostos=%zu ms=%.1f img=%dx%d y0=%d maior_altura=%.0f nota=%.2f%s",RS.rodadas,rs.size(),ms,w,h,ry0,double(m),double(nm),sc?" det=scrfd":"");nrrj::log(l);}
        }catch(...){nrrj::log("ROSTO_JOGADOR_ROSTOS excecao na thread do detector de rosto");Sleep(1000);}
    }
}
inline void iniciar_rostos() noexcept {
    static std::atomic<bool> iniciado{false};if(iniciado.exchange(true))return;
    try{std::thread(laco_rostos).detach();nrrj::log("ROSTO_JOGADOR_ROSTOS thread do detector de rosto (YuNet, CPU, prioridade baixa)");}catch(...){}
}

inline void motivo_texto(unsigned m,char* b,size_t n){
    (void)m;if(b&&n)b[0]=0;
}

inline std::atomic<bool> prof_trocada{false};

template<class Copiar> inline void executar_deteccao(Det& D,const DetTarefa& t,Copiar&& copiar){
    const auto t0=std::chrono::steady_clock::now();
    auto& S=D.sets[t.set];
    const size_t ng=size_t(t.gw)*t.gh,nd=size_t(t.dw)*t.dh;
    D.h_dq.resize(ng);D.h_mvq.resize(2*ng);D.h_d.resize(nd);
    const bool ok=copiar(D.h_dq.data(),S.dq,ng*4)&&copiar(D.h_mvq.data(),S.mvq,ng*8)&&copiar(D.h_d.data(),S.d,nd*4);
    S.ocupado.store(false,std::memory_order_release);
    if(!ok){nrrj::envenenar("copia das grades do detector");return;}
    float k=1.f;
    if(t.quadro&&D.quadro_ant&&t.quadro>D.quadro_ant)k=float((std::min<std::uint64_t>)(t.quadro-D.quadro_ant,8u));
    const auto q=nrprescale::unpack_quadro(nrprescale::quadro.load(std::memory_order_acquire));
    const float esc=q.saida_w&&t.dw?float(q.saida_w)/float(t.dw):3.f,saida_h=q.saida_h?float(q.saida_h):2160.f;
    nrrj::Grades gr;gr.W=t.gw;gr.H=t.gh;gr.dq=D.h_dq.data();gr.mvq=D.h_mvq.data();gr.w=t.dw;gr.h=t.dh;gr.d=D.h_d.data();gr.k=k;gr.esc=esc;gr.saida_h=saida_h;
    gr.t=std::chrono::duration<double>(t0.time_since_epoch()).count();
    D.rastreio.P.q=(std::max)(1,t.dh/t.gh);
    D.rastreio.P.corte_modo=nrrj::corte_modo.load(std::memory_order_relaxed);
    D.rastreio.P.corte_ok=nrrj::corte_ok_ini.load(std::memory_order_relaxed);
    D.rastreio.P.liga_forte=nrrj::liga_forte.load(std::memory_order_relaxed);
    D.rastreio.P.adq_nota1=nrrj::adq_nota1.load(std::memory_order_relaxed);
    D.rastreio.P.sinal_n=nrrj::sinal_n.load(std::memory_order_relaxed);
    D.rastreio.P.rosto_calibra=nrrj::rosto_calibra.load(std::memory_order_relaxed);D.rastreio.P.rosto_manda=nrrj::rosto_manda.load(std::memory_order_relaxed);
    D.rastreio.P.alfa_pos=nrrj::alfa_pos.load(std::memory_order_relaxed);D.rastreio.P.alfa_tam=nrrj::alfa_tam.load(std::memory_order_relaxed);
    D.rastreio.P.rosto_ancora=nrrj::rosto_ancora.load(std::memory_order_relaxed);D.rastreio.P.ancora_nota=nrrj::ancora_nota.load(std::memory_order_relaxed);
    D.rastreio.P.nota_grande=nrrj::nota_grande.load(std::memory_order_relaxed);D.rastreio.P.nota_grande_px=nrrj::nota_grande_px.load(std::memory_order_relaxed);
    {const unsigned z=nrrj::zoom.load(std::memory_order_relaxed);
     if(z&1u){D.rastreio.P.cab_max_4k=nrrj::zoom_cab_4k;D.rastreio.P.recorte_max=int(nrrj::zoom_lado);}
     D.rastreio.P.persistente=(z&4u)!=0;D.rastreio.P.desliga_n_pers=nrrj::desliga_jobs.load(std::memory_order_relaxed);D.rastreio.P.npc2_zona=nrrj::npc_zona.load(std::memory_order_relaxed);D.rastreio.P.todos_cab_max=nrrj::npc_todos_cab_max.load(std::memory_order_relaxed);if(z&8u)D.rastreio.P.recorte_min=512;
     D.rastreio.P.recorte_forcado=nrrj::recorte_forcado.load(std::memory_order_relaxed);
     if(z&16u){const int tr=int(nrrj::teto.load(std::memory_order_relaxed)),fx=nrrj::recorte_fixo_ini.load(std::memory_order_relaxed);
               (void)tr;D.rastreio.P.recorte_fixo=(std::max)(256,fx>0?fx:1024);}
     D.rastreio.P.cinema_livre=(z&2u)!=0;}
    D.rastreio.P.npc=nrrj::npc.load(std::memory_order_relaxed);
    D.rastreio.P.npc_modo=nrrj::npc_modo.load(std::memory_order_relaxed);D.rastreio.P.rosto_nota=nrrj::rosto_nota.load(std::memory_order_relaxed);
    D.rastreio.P.npc_segura_s=nrrj::npc_segura_s.load(std::memory_order_relaxed);
    D.rastreio.P.npc2=nrrj::npc2_ativo();D.rastreio.P.npc2_segura_s=nrrj::npc2_segura_s.load(std::memory_order_relaxed);D.rastreio.P.npc2_vel_max=nrrj::npc2_vel_max.load(std::memory_order_relaxed);
    D.rastreio.P.npc_max=nrrj::npc_n()>0?nrrj::npc_n():1;
    D.rastreio.P.adquire=nrrj::adquire.load(std::memory_order_relaxed);D.rastreio.P.reancora=nrrj::reancora.load(std::memory_order_relaxed);D.rastreio.P.caminha_dela=nrrj::caminha_dela.load(std::memory_order_relaxed);
    D.rastreio.P.npc_degraus=nrrj::npc_degraus.load(std::memory_order_relaxed);D.rastreio.P.npc2_segura_peq=nrrj::npc_segura_peq.load(std::memory_order_relaxed);
    if(const int r=nrrj::npc_recorte_min2.load(std::memory_order_relaxed))D.rastreio.P.npc2_recorte_min=r;
    D.rastreio.P.npc2_recorte_forcado=nrrj::npc_recorte_forcado.load(std::memory_order_relaxed);
    D.rastreio.P.npc_fracao=nrrj::npc_fracao.load(std::memory_order_relaxed);D.rastreio.P.npc_frac_teto=int((nrrj::npc_leve.load(std::memory_order_relaxed)&1)?nrrj::npc_leve_recorte.load(std::memory_order_relaxed):nrrj::npc2_teto.load(std::memory_order_relaxed));
    D.rastreio.P.npc_canon=nrrj::npc_canon.load(std::memory_order_relaxed);D.rastreio.P.canon.fracao=nrrj::npc_canon_fracao.load(std::memory_order_relaxed);D.rastreio.P.canon.razao=nrrj::npc_canon_razao.load(std::memory_order_relaxed);
    D.rastreio.P.canon.min_corte=nrrj::npc_canon_corte.load(std::memory_order_relaxed);D.rastreio.P.canon.beta=nrrj::npc_canon_beta.load(std::memory_order_relaxed);D.rastreio.P.canon.rede=int(nrrj::npc_canon_rede.load(std::memory_order_relaxed));
    D.rastreio.P.npc_leve=nrrj::npc_leve.load(std::memory_order_relaxed);D.rastreio.P.npc_leve_salto=nrrj::npc_leve_salto.load(std::memory_order_relaxed);
    D.rastreio.P.npc_amplia=nrrj::npc_amplia.load(std::memory_order_relaxed)?int(nrrj::npc_rede_min):0;
    D.rastreio.P.npc_peito=nrrj::npc_peito.load(std::memory_order_relaxed);
    D.rastreio.P.npc_frente=nrrj::npc_frente.load(std::memory_order_relaxed);
    D.rastreio.P.npc2_conf_s=nrrj::npc_conf_s.load(std::memory_order_relaxed);
    D.rastreio.P.npc_roi_conf=nrrj::npc_roi_conf.load(std::memory_order_relaxed);D.rastreio.P.npc_vel_img=nrrj::npc_vel_img.load(std::memory_order_relaxed);
    D.rastreio.P.npc_atras=nrrj::npc_atras.load(std::memory_order_relaxed);
    D.rastreio.P.npc2_cab_min=nrrj::npc_cab_min.load(std::memory_order_relaxed);
    {const int tb=nrrj::npc_todos.load(std::memory_order_relaxed);D.rastreio.P.npc_todos=tb;D.rastreio.P.todos_nota=nrrj::npc_todos_nota.load(std::memory_order_relaxed);
     D.rastreio.P.todos_perto=((tb&16)&&nrorc::aplicada())?nrorc::decisao().k_perto:0;if(tb&2)D.rastreio.P.npc2_cab_min=(std::min)(D.rastreio.P.npc2_cab_min,64.f);}
    D.rastreio.P.npc_corpo=nrrj::npc_corpo.load(std::memory_order_relaxed);
    D.rastreio.P.npc_camada=nrrj::npc_camada.load(std::memory_order_relaxed);
    D.rastreio.P.npc_vida=nrrj::npc_vida.load(std::memory_order_relaxed);D.rastreio.P.vida_in_s=nrrj::npc_vida_in.load(std::memory_order_relaxed);
    if(D.rastreio.P.npc_modo==1||D.rastreio.P.npc2){
        iniciar_rostos();
        auto& RS=nrrj::rostos();std::lock_guard<std::mutex> l(RS.m);
        if(RS.versao!=D.rostos_versao){D.rostos_versao=RS.versao;D.rostos_quadro=RS.quadro;D.rostos_t=RS.t;D.rostos.clear();
            for(const auto& c:RS.lista){D.rostos.push_back({c.x,c.y,c.w,c.h,c.nota});auto& b=D.rostos.back();b.px=c.px;std::memcpy(b.lm,c.lm,sizeof b.lm);}}
        gr.rostos=D.rostos.data();gr.n_rostos=int(D.rostos.size());gr.rostos_versao=D.rostos_versao;gr.rostos_t=D.rostos_t;
        for(int kn=0;kn<nrrj::npc_slots;++kn){
            {auto& RR=nrrj::rostos_roi(kn);std::lock_guard<std::mutex> l2(RR.m);
             if(RR.versao!=D.roi_versao[kn]){D.roi_versao[kn]=RR.versao;D.roi_quadro[kn]=RR.quadro;D.roi_t[kn]=RR.t;D.roi[kn].clear();for(const auto& c:RR.lista){D.roi[kn].push_back({c.x,c.y,c.w,c.h,c.nota});auto& b=D.roi[kn].back();b.px=c.px;std::memcpy(b.lm,c.lm,sizeof b.lm);}}}
            gr.roi[kn]=D.roi[kn].data();gr.n_roi[kn]=int(D.roi[kn].size());gr.roi_versao[kn]=D.roi_versao[kn];gr.roi_t[kn]=D.roi_t[kn];
            gr.roi_idade[kn]=t.quadro>D.roi_quadro[kn]&&D.roi_quadro[kn]?float((std::min<std::uint64_t>)(t.quadro-D.roi_quadro[kn],30u)):0.f;}
        gr.rostos_idade=t.quadro>D.rostos_quadro&&D.rostos_quadro?float((std::min<std::uint64_t>)(t.quadro-D.rostos_quadro,30u)):0.f;
    }
    D.rastreio.P.perto=(nrrj::zoom.load(std::memory_order_relaxed)&32u)!=0;
    D.rastreio.P.npc_min=nrrj::npc_min.load(std::memory_order_relaxed);D.rastreio.P.npc_recorte=nrrj::npc_recorte.load(std::memory_order_relaxed);
    D.rastreio.P.npc_expande=nrrj::npc_expande.load(std::memory_order_relaxed);

    {const unsigned par=nrredecada::cam::par.load(std::memory_order_relaxed);D.rastreio.P.corte_multi=par>1&&k>=float((std::min)(par,8u));}

    {const auto ger=nrrj::ger().load(std::memory_order_acquire);if(D.ger_vista&&ger!=D.ger_vista){D.rastreio.reset_corte();D.tem_ant=false;++D.cortes_fsr;}D.ger_vista=ger;}
    const auto R=D.rastreio.passo(gr);
    if(D.rastreio.P.adquire&32)nrrj::giro_inteira.store(R.giro?1:0,std::memory_order_relaxed);
    if(R.giro_rosto&&(D.rastreio.giro_rosto_n<=32||(D.rastreio.giro_rosto_n&63u)==0)){char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_GIRO_ROSTO caixa no rosto do YuNet depois do giro job=%d n=%llu",t.job,D.rastreio.giro_rosto_n);nrrj::log(l);}
    ++D.deteccoes;if(R.on)++D.on_jobs;
    if(R.tem_est||R.busca)D.sem_cabeca.store(0,std::memory_order_relaxed);else D.sem_cabeca.fetch_add(1,std::memory_order_relaxed);
    if(R.conv_errada){const bool v=!prof_trocada.load();prof_trocada.store(v);
        char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_CONVENCAO profundidade ao contrario (mediana da proximidade %.3f > 0,5): conversao trocada (trocada=%d) a partir do proximo job",double(R.med_prox),v?1:0);nrrj::log(l);}
    if(R.adotou){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ESCALA nova kface=%.0f (hipotese %d; a do RE e 1550): nenhuma cabeca na escala anterior por 60 jobs",double(R.kface),R.hip);nrrj::log(l);}

    {static unsigned long long ad_visto=0;const auto& s=D.rastreio.ad_ult;if(D.rastreio.ad_n!=ad_visto&&D.rastreio.ad_n<=3000){ad_visto=D.rastreio.ad_n;char la[300]{};NRLOGF(la,sizeof la,"ROSTO_JOGADOR_DESLIGADO_LISTA rostos=%.0f maior_nota=%.2f maior_cab4k=%.0f em=%.0f,%.0f bloqueio=%.0f ancoraveis=%.0f tem_est=%.0f tem_meas=%.0f velha=%.0f job=%d (w3-adqdiag; bits 1 nota 2 pequeno 4 nao_unico 8 sem_prox 16 sem_corpo 32 fraco 64 rastreio/corte)",s[0],s[1],s[2],s[3],s[4],s[5],s[6],s[7],s[8],s[9],t.job);nrrj::log(la);}}
    if(R.reancorou){char l[220]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_REANCOROU pelo rosto da lista job=%d cab4k=%.0f caixa=%.0f,%.0f reancorou=%llu promovido=%llu",t.job,double(R.cab4k),double(R.est[0]),double(R.est[1]),D.rastreio.reancorou_n,D.rastreio.promovido_n);nrrj::log(l);}
    if(R.adquiriu){char l[220]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ADQUIRIU pelo rosto do YuNet job=%d cab4k=%.0f caixa=%.0f,%.0f kf_rosto/kf=%.2f adquiriu=%llu promovido=%llu",t.job,double(R.cab4k),double(R.est[0]),double(R.est[1]),double(R.kf_razao),D.rastreio.adquiriu_n,D.rastreio.promovido_n);nrrj::log(l);}
    if(R.escala_conf&&(D.rastreio.escala_conf_n<=32||(D.rastreio.escala_conf_n&63u)==0)){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ESCALA confirmada pelo rosto kface=%.0f kf_rosto/kf=%.2f escala_confirmada=%llu job=%d",double(R.kface),double(R.kf_razao),D.rastreio.escala_conf_n,t.job);nrrj::log(l);}
    if(R.adocao_vetada&&(D.rastreio.adocoes_vetadas<=32||(D.rastreio.adocoes_vetadas&63u)==0)){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ESCALA adocao vetada (hipotese %d: o objeto nao e um rosto do YuNet) vetadas=%llu job=%d",R.hip,D.rastreio.adocoes_vetadas,t.job);nrrj::log(l);}
    {static int sinal_ant=1;if(R.sinal!=sinal_ant){sinal_ant=R.sinal;char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_SINAL do vetor trocado para %d (o vetor trocado explicava o quadro melhor: ok=%.2f ok_trocado=%.2f)",R.sinal,double(R.ok),double(R.ok_neg));nrrj::log(l);}}
    {
        char mt[160];
        if(R.on!=D.on_ant){D.on_ant=R.on;++D.trocas;
            if(D.trocas<=32||(D.trocas&63u)==0){motivo_texto(R.motivo_off,mt,sizeof mt);char l[320]{};
                NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ESTADO %s job=%d trocas=%llu motivo=%s conf=%.2f rival=%.2f fonte=%s cab_frac=%.3f",R.on?"LIGOU":"DESLIGOU",t.job,D.trocas,R.on?"-":mt,
                    double(R.conf),double(R.rival),t.fonte==2?"render":"proxy",double(R.cab4k)/2160.);nrrj::log(l);}}
        if(R.on){D.off_seguidos=0;D.motivos_off=0;}
        else{D.motivos_off|=R.motivo_off;
            if(++D.off_seguidos%256u==0){motivo_texto(D.motivos_off,mt,sizeof mt);char l[320]{};
                NRLOGF(l,sizeof l,"ROSTO_JOGADOR_OFF desligado ha %llu jobs; motivos nesse tempo=%s (ultimo conf=%.2f ncand=%d kface=%.0f busca=%d fonte=%s economizados=%llu)",D.off_seguidos,mt,
                    double(R.conf),R.ncand,double(R.kface),R.busca?1:0,t.fonte==2?"render":"proxy",D.economizados.load(std::memory_order_relaxed));nrrj::log(l);D.motivos_off=0;}}
    }
    if(R.corte){++D.cortes;nrrj::cortar();D.ger_vista=nrrj::ger().load(std::memory_order_acquire);}
    {const auto& rr=D.rastreio;static unsigned long long cal_log=0;
     if((rr.P.rosto_calibra||rr.P.rosto_manda)&&(rr.cal_n+rr.manda_n)>0&&(rr.cal_n+rr.manda_n)/64!=cal_log){cal_log=(rr.cal_n+rr.manda_n)/64;char l[200]{};
        NRLOGF(l,sizeof l,"ROSTO_JOGADOR_CALIBRA medidas=%llu manda=%llu dx=%.3f dy=%.3f escala=%.3f (cabeca pelo rosto - profundidade, em lados) job=%d",
            rr.cal_n,rr.manda_n,rr.cal_x,rr.cal_y,rr.cal_s,t.job);nrrj::log(l);}}
    if(R.perto_inicio||R.perto_fim){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_PERTO %s segurou=%u jobs motivo=%u lado=%d on=%d cab4k=%.0f escala=%.3f job=%d",R.perto_inicio?"inicio":"fim",
        R.perto_jobs,R.perto_motivo,R.tem_recorte?R.recorte[2]:0,R.on?1:0,double(R.cab4k),double(R.perto_escala),t.job);nrrj::log(l);}

    {
        static double lado_ant=0.;static unsigned long long saltos=0;const float emax=nrrj::escala_salto.load(std::memory_order_relaxed);
        if(emax>1.f&&R.on&&R.est[2]>0.){
            bool novo=R.reancorou&&D.rastreio.P.rosto_ancora>0;
            if(lado_ant>0.&&std::fabs(std::log(double(R.est[2])/lado_ant))>std::log(double(emax)))novo=true;
            if(novo&&!R.corte&&!R.troca_alvo){nrrj::cortar_troca_alvo();D.ger_vista=nrrj::ger().load(std::memory_order_acquire);D.tem_ant=false;++saltos;
                if(saltos<=32||(saltos&63u)==0){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ESCALA_SALTO n=%llu lado=%.0f antes=%.0f reancorou=%d job=%d (geracao nova)",saltos,double(R.est[2]),lado_ant,R.reancorou?1:0,t.job);nrrj::log(l);}}
            lado_ant=double(R.est[2]);
        }else if(!R.on)lado_ant=0.;
    }
    if(D.rastreio.P.npc||R.troca_alvo){
        static unsigned trocas_alvo=0;static std::uint64_t npc_log_q=0;
        if(R.troca_alvo){++trocas_alvo;D.tem_ant=false;if(!R.corte){nrrj::cortar_troca_alvo();D.ger_vista=nrrj::ger().load(std::memory_order_acquire);}}
        nrrj::alvo_npc.store(R.alvo_npc,std::memory_order_relaxed);
        const std::uint64_t qq=t.quadro?t.quadro:D.deteccoes;
        static bool rf_g_ant=false;const bool rf_g_troca=R.rf_g!=rf_g_ant;rf_g_ant=R.rf_g;
        if(R.troca_alvo&&D.rastreio.P.npc_modo==1){
            static std::deque<double> tt;tt.push_back(gr.t);while(!tt.empty()&&gr.t-tt.front()>60.0)tt.pop_front();
            static const char* const motivos[]{"-","rosto_dela_sumiu","rosto_dela_voltou","npc_maior","npc_sumiu","corte","outro_npc_no_lugar_do_sumido"};
            char l[300]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_TROCA para=%s motivo=%s nota_dela=%.2f nota_npc=%.2f idade_lista_s=%.3f npc_cab4k=%.0f trocas_por_minuto=%zu job=%d",
                R.alvo_npc?"npc":"grace",motivos[(R.rf_motivo>=0&&R.rf_motivo<=6)?R.rf_motivo:0],double(R.rf_nota),double(R.rf_npc_nota),double(R.rf_idade_s),double(R.cab4k),tt.size(),t.job);nrrj::log(l);}
        if(R.troca_alvo||rf_g_troca||qq>=npc_log_q+300u||qq<npc_log_q){npc_log_q=qq;char l[400]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_NPC alvo=%s expande=%.4f npc_cab4k=%.0f trocas=%u bruto=%.4f ativo=%d celulas=%d job=%d corte=%d"
            " modo=%d rosto_dela=%d nota_dela=%.2f sem_rosto_s=%.2f rostos=%d recorte_pelo_rosto=%d cab4k=%.0f",
            R.alvo_npc?"npc":(R.rf_g?"grace_rosto":"grace"),double(R.npc_expande),double(R.npc_cab4k),trocas_alvo,double(R.npc_bruto),R.npc_ativo?1:0,R.npc_celulas,t.job,R.corte?1:0,
            D.rastreio.P.npc_modo,R.rf_vis?1:0,double(R.rf_nota),double(R.rf_sem),R.rf_n,R.rf_g?1:0,double(R.cab4k));nrrj::log(l);}
    }
    nrrj::grace_on.store(R.on?1:0,std::memory_order_relaxed);
    nrrj::Deteccao d{};d.on=R.on;d.tem=R.tem_est&&R.tem_recorte;d.quadro=t.quadro;d.conf=R.conf;d.cab4k=R.cab4k;
    if(D.rastreio.cd_tem&&!D.rastreio.trk_npc)d.zela=float(D.rastreio.cd_z);
    d.motivo=(R.corte?1u:0u)|(R.grande?2u:0u)|(R.cinema?4u:0u)|(R.tem_est?0u:8u);
    if(R.tem_est){d.cx=R.est[0]*esc;d.cy=R.est[1]*esc;d.lado=R.est[2]*esc;

        const bool pulo_rosto=D.rastreio.P.rosto_ancora>0&&std::hypot(R.est[0]-D.est_ant[0],R.est[1]-D.est_ant[1])>.25*(std::max)(double(R.est[2]),1.0);
        if(D.tem_ant&&k>0&&!pulo_rosto){d.vx=(R.est[0]-D.est_ant[0])*esc/k;d.vy=(R.est[1]-D.est_ant[1])*esc/k;}
        D.est_ant[0]=R.est[0];D.est_ant[1]=R.est[1];D.est_ant[2]=R.est[2];D.tem_ant=true;}
    else D.tem_ant=false;
    if(R.tem_recorte){d.rl=unsigned(R.recorte[2]);d.rx=int(std::lround(R.recorte[0]*esc-d.rl*.5f));d.ry=int(std::lround(R.recorte[1]*esc-d.rl*.5f));}
    nrrj::publicar(d);

    if(D.rastreio.P.npc2)for(int kn=0;kn<nrrj::npc_slots;++kn){
        const auto& NS=D.rastreio.n2s[kn];const auto& R2=NS.saida;auto& L2=D.n2l[kn];auto& RT=D.rastreio;
        if(kn>=nrrj::npc_n()&&!R2.troca_alvo&&!R2.on&&!R2.vida_sai)continue;
        nrrj::Inst i1(1+kn);
        if(R2.troca_alvo)nrrj::cortar();
        nrrj::Deteccao d2{};d2.on=R2.on&&!R2.vida_pend;d2.pend=R2.on&&R2.vida_pend;d2.camq=R2.vida_camq;d2.tem=R2.tem_est&&R2.tem_recorte;d2.quadro=t.quadro;d2.conf=R2.conf;d2.cab4k=R2.cab4k;d2.motivo=R2.tem_est?0u:8u;
        if(R2.tem_est){d2.cx=R2.est[0]*esc;d2.cy=R2.est[1]*esc;d2.lado=R2.est[2]*esc;

            const double pulo=std::hypot(R2.est[0]-L2.est_ant[0],R2.est[1]-L2.est_ant[1])/(std::max)(double(R2.est[2]),1.0);
            if(L2.tem_ant&&!R2.troca_alvo&&k>0&&pulo<=.25&&!(NS.ocl_t>0)){d2.vx=(R2.est[0]-L2.est_ant[0])*esc/k;d2.vy=(R2.est[1]-L2.est_ant[1])*esc/k;}
            L2.est_ant[0]=R2.est[0];L2.est_ant[1]=R2.est[1];L2.est_ant[2]=R2.est[2];L2.tem_ant=true;}
        else L2.tem_ant=false;
        if(R2.tem_camada&&R2.tem_est){d2.zdx=(R2.camada[0]-R2.est[0])*esc;d2.zdy=(R2.camada[1]-R2.est[1])*esc;}
        if(R2.tem_recorte){d2.rl=unsigned(R2.recorte[2]);const float c0=R2.rec_f_tem?R2.rec_f[0]:float(R2.recorte[0]),c1=R2.rec_f_tem?R2.rec_f[1]:float(R2.recorte[1]);
            d2.rx=int(std::lround(c0*esc-d2.rl*.5f));d2.ry=int(std::lround(c1*esc-d2.rl*.5f));}
        nrrj::publicar(d2);

        const std::uint64_t qq=t.quadro?t.quadro:D.deteccoes;const double agora=gr.t;
        if(R2.troca_alvo&&R2.on){L2.t0=agora;L2.movs0=NS.movs;}
        auto& e1=estado();const double ms1=e1.ms_n?e1.ms_soma/e1.ms_n:-1.0;
        static const bool depura=[]{char v[8]{};std::size_t n=0;return getenv_s(&n,v,sizeof v,"NR_NPC2_DEPURA")==0&&n>1&&v[0]=='1';}();
        if(depura&&(R2.on||R2.troca_alvo||R2.vida_sai)){char l[340]{};NRLOGF(l,sizeof l,"ROSTO_NPC2_JOB q=%llu on=%d pend=%d sai=%d id=%u caixa4k=%.0f,%.0f,%.0f coberto_s=%.2f prof=%.4f conf_t=%.2f cand=%d nota=%.2f roi_ok=%llu roi_falta=%llu motivo=%d cam=%.0f,%.0f grace=%.0f,%.0f,%.0f prof_ela=%.4f",
            static_cast<unsigned long long>(t.quadro),(R2.on&&!R2.vida_pend)?1:0,R2.vida_pend?1:0,R2.vida_sai?1:0,NS.id,R2.est[0]*esc,R2.est[1]*esc,R2.est[2]*esc,NS.ocl_t,NS.prof,NS.conf_t,RT.n2c_n,double(NS.nota),NS.roi_ok,NS.roi_falta,R2.rf_motivo,R2.tem_camada?double(R2.camada[0])*esc:0.0,R2.tem_camada?double(R2.camada[1])*esc:0.0,
            R.tem_est?double(R.est[0])*esc:0.0,R.tem_est?double(R.est[1])*esc:0.0,R.tem_est?double(R.est[2])*esc:0.0,RT.dbg_prof_ela);nrrj::log(l);}
        if(depura&&RT.P.npc_canon&&R2.on){char l[260]{};NRLOGF(l,sizeof l,"ROSTO_NPC2_CANON q=%llu id=%u trilha=%d cab4k=%.0f s4k=%.1f razao=%.3f lado=%d centro4k=%.1f,%.1f medidas=%llu rejeitadas=%llu reinicios=%llu sem=%llu",static_cast<unsigned long long>(t.quadro),NS.id,NS.cn.tem?1:0,double(R2.est[2])*esc,NS.cn.tem?NS.cn.s*esc:0.0,NS.cn.tem&&NS.cn.s>0?double(R2.est[2])/NS.cn.s:0.0,R2.recorte[2],double(R2.rec_f[0])*esc,double(R2.rec_f[1])*esc,NS.cn.medidas,NS.cn.rejeitadas,NS.cn.reinicios,NS.cn_sem);nrrj::log(l);}
        if(R2.troca_alvo&&R2.on&&R2.rf_motivo>0){char l[200]{};static const char* const mt[]{"-","sem_rosto","fora_da_tela","corte","rapido","sobre_grace","outro","promovido"};
            NRLOGF(l,sizeof l,"ROSTO_NPC2 soltou motivo=%s (e prendeu outro no mesmo job) job=%d",mt[(R2.rf_motivo>=1&&R2.rf_motivo<=7)?R2.rf_motivo:6],t.job);nrrj::log(l);}
        if(R2.vida_oculto_novo||R2.vida_volta){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_NPC2 %s id=%u cab4k=%.0f coberto_jobs=%llu ocultos=%llu job=%d",R2.vida_oculto_novo?"oculto":"reapareceu",NS.id,double(R2.cab4k),NS.ocl_jobs,NS.vida_ocultos,t.job);nrrj::log(l);}
        if((D.rastreio.P.npc_leve&2)&&NS.saltos_id!=L2.saltos0){L2.saltos0=NS.saltos_id;char l[200]{};NRLOGF(l,sizeof l,"ROSTO_NPC2 salto_pessoa id=%u cab4k=%.0f saltos=%llu roi_dono=%llu job=%d (npc-leve: geracao nova)",NS.id,double(R2.cab4k),NS.saltos_id,NS.roi_dono,t.job);nrrj::log(l);}
        if(R2.vida_pub_novo){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_NPC2 publicou id=%u cab4k=%.0f nota=%.2f roi_com_rosto=%llu roi_sem=%llu job=%d",NS.id,double(R2.cab4k),double(R2.conf),NS.roi_ok,NS.roi_falta,t.job);nrrj::log(l);}
        {static unsigned long long pd_visto=0;const auto& s=D.rastreio.pend_ult;if(D.rastreio.pend_n!=pd_visto&&(D.rastreio.pend_n<=400||(D.rastreio.pend_n&15u)==0)){pd_visto=D.rastreio.pend_n;char lp[300]{};NRLOGF(lp,sizeof lp,"ROSTO_NPC2 PENDENTE k=%.0f est=%.0f,%.0f,%.0f hits=%.0f desde=%.2f conf_t=%.2f dentro_logo=%.0f roi_ok=%.0f borda_ok=%.0f tela=%.0fx%.0f n=%llu (w3-pend diag)",s[0],s[1],s[2],s[3],s[4],s[5],s[6],s[7],s[8],s[9],s[10],s[11],D.rastreio.pend_n);nrrj::log(lp);}}
        if(R2.troca_alvo||R2.vida_sai||(R2.on&&(qq>=L2.log_q+120u||qq<L2.log_q))){L2.log_q=qq;
            const double minutos=L2.t0>=0?(std::max)(1e-3,(agora-L2.t0)/60.0):1e-3;
            static const char* const motivos[]{"-","sem_rosto","fora_da_tela","corte","rapido","sobre_grace","outro","promovido"};
            {static unsigned long long sz_visto=0;const auto& s=D.rastreio.sz_ult;if(D.rastreio.sz_n!=sz_visto){sz_visto=D.rastreio.sz_n;char lz[260]{};NRLOGF(lz,sizeof lz,"ROSTO_NPC2 SOBRE_GRACE ela=%.0f,%.0f,%.0f npc=%.0f,%.0f,%.0f zona=%.2f na_zona=%.0f corpo=%.0f n=%llu (w3-zona diag)",s[0],s[1],s[2],s[3],s[4],s[5],s[6],s[7],s[8],D.rastreio.sz_n);nrrj::log(lz);}}

            const char* vida=!D.rastreio.P.npc_vida?"":R2.vida_sai?" vida=suave":R2.on?(R2.vida_pend?(R2.vida_oculto?" vida=oculto":R2.vida_re?" vida=re":" vida=pend"):" vida=pub"):(R2.vida_era_pub?" vida=pub":" vida=pend");
            char l[440]{};NRLOGF(l,sizeof l,"ROSTO_NPC2 %s motivo=%s id=%u cab4k=%.0f recorte=%d,%d/%d nota=%.2f presos=%llu recortes_andaram_min=%.1f recortes_andaram=%llu vel=%.2f vel_cand=%.2f roi_com_rosto=%llu roi_sem=%llu reancoras=%llu coberto_jobs=%llu vetor_ruim=%llu rede_ms=%.2f job=%d%s",
                R2.mig_para>=0?"migrou":R2.mig_de>=0?"migrado":R2.vida_sai?"soltou":R2.troca_alvo?(R2.on?"prendeu":"soltou"):"estado",
(R2.vida_sai||(R2.troca_alvo&&!R2.on))?motivos[(R2.rf_motivo>=1&&R2.rf_motivo<=7)?R2.rf_motivo:6]:"-",NS.id,double(R2.cab4k),
                R2.recorte[0],R2.recorte[1],R2.recorte[2],double(R2.conf),NS.presos,double(NS.movs-L2.movs0)/minutos,NS.movs-L2.movs0,NS.vel,RT.n2c_vel,NS.roi_ok,NS.roi_falta,NS.reancoras,NS.ocl_jobs,NS.mv_ruim,ms1,t.job,vida);nrrj::log(l);}
    }
    D.quadro_ant=t.quadro;
    const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
    D.det_ms+=ms;++D.det_n;D.det_max=(std::max)(D.det_max,ms);
    if(D.deteccoes<=8||(D.deteccoes&63u)==0||R.corte){

        char l[900]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_DETECCAO n=%llu job=%d on=%d conf=%.2f ok=%.2f k=%.0f cab4k=%.0f motivo=%u caixa=%.0f,%.0f,%.0f recorte=%d,%d,%u on_jobs=%llu cortes=%llu det_ms=%.2f det_max=%.2f grade=%dx%d thread=%d pulados=%llu descartados=%llu"
            " fonte=%s cheia=%dx%d motivo_off=%u cab_frac=%.3f kface=%.0f hip=%d busca=%d sinal=%d votos=%d med_prox=%.4f prof_trocada=%d esc_med=%.0f ncand=%d cam=%.4f/%.0f/%.3f cmulti=%llu/%llu vetos=%llu cmodo=%d tend=%.2f nb=%.2f vetou=%d",
            D.deteccoes,t.job,R.on?1:0,double(R.conf),double(R.ok),double(k),double(R.cab4k),d.motivo,double(d.cx),double(d.cy),double(d.lado),d.rx,d.ry,d.rl,D.on_jobs,D.cortes,
            D.det_ms/(D.det_n?D.det_n:1),D.det_max,t.gw,t.gh,det_thread.load()?1:0,D.pulados.load(),D.descartados.load(),
            t.fonte==2?"render":"proxy",t.dw,t.dh,R.motivo_off,double(R.cab4k)/2160.,double(R.kface),R.hip,R.busca?1:0,R.sinal,R.votos,double(R.med_prox),prof_trocada.load()?1:0,double(R.escala),R.ncand,
            double(nrrj::cam_near.load()),double(nrrj::cam_far.load()),double(nrrj::cam_fov.load()),D.rastreio.cortes_multi_salvos,D.rastreio.cortes_multi_testes,D.rastreio.cortes_vetados,D.rastreio.P.corte_modo,double(R.tend),double(R.nb_melhor),R.recontou?1:0);nrrj::log(l);
        D.det_ms=0;D.det_n=0;D.det_max=0;
    }
}

inline void laco_detector() noexcept {
    auto& D=det();
    using SetDev=int(*)(int);using Cria=int(*)(void**,unsigned);using CopiaA=int(*)(void*,const void*,size_t,int,void*);using Sinc=int(*)(void*);
    HMODULE h=GetModuleHandleW(L"amdhip64_7.dll");
    auto setdev=h?reinterpret_cast<SetDev>(GetProcAddress(h,"hipSetDevice")):nullptr;auto cria=h?reinterpret_cast<Cria>(GetProcAddress(h,"hipStreamCreateWithFlags")):nullptr;
    auto copia=h?reinterpret_cast<CopiaA>(GetProcAddress(h,"hipMemcpyAsync")):nullptr;auto sinc=h?reinterpret_cast<Sinc>(GetProcAddress(h,"hipStreamSynchronize")):nullptr;
    void* stream{};bool dispositivo=false;
    for(;;){
        DetTarefa t{};
        {std::unique_lock<std::mutex> l(D.m);D.cv.wait(l,[&]{return D.tem;});t=D.fila;D.tem=false;}
        try{
            if(!copia||!sinc||!cria){D.sets[t.set].ocupado.store(false);nrrj::envenenar("hipMemcpyAsync ausente (detector em thread)");continue;}
            if(!dispositivo){if(setdev&&t.device>=0)(void)setdev(t.device);if(cria(&stream,1u)||!stream){D.sets[t.set].ocupado.store(false);nrrj::envenenar("stream do detector");continue;}dispositivo=true;}
            executar_deteccao(D,t,[&](void* dst,const void* src,size_t n){return copia(dst,src,n,2,stream)==0&&sinc(stream)==0;});
        }catch(...){D.sets[t.set].ocupado.store(false);nrrj::envenenar("excecao no detector (thread)");}
    }
}

inline bool grade_do_job(Estado& e,int job,CenaMv& m,const char*& motivo){
    auto& G=nrrj::grade();
    if(!G.quer.exchange(true)){nrrj::log("ROSTO_JOGADOR_FONTE render pedida: o proxy nao tem o ReprojParams da cena ha 3 jobs (Temporal=0: o proxy nao importa vetor/profundidade para o HIP); a render passa a gravar a grade do detector");}

    for(unsigned s=0;s<nrrj::grade_slots;++s){auto w=G.slot[s].palavra.load(std::memory_order_acquire);
        if((w&255u)==nrrj::GEscrito&&nrrj::job_de(w)<job)G.slot[s].palavra.compare_exchange_strong(w,nrrj::palavra_g(nrrj::GLivre,0),std::memory_order_acq_rel);}
    int s=-1;std::uint64_t w=0;
    for(unsigned k=0;k<nrrj::grade_slots;++k){w=G.slot[k].palavra.load(std::memory_order_acquire);if((w&255u)==nrrj::GEscrito&&nrrj::job_de(w)==job){s=int(k);break;}}
    if(s<0){motivo="grade_ausente_no_job";return false;}
    auto& S=G.slot[unsigned(s)];
    if(nrrj::global().marcado(nrrj::grade_marca0+unsigned(s))!=S.seq){motivo="grade_marcador";return false;}
    if(S.w<64||S.h<64||std::uint64_t(S.w)*S.h>nrrj::grade_cap){motivo="grade_tamanho";return false;}
    if(!importar_buffer(G.handle.load(std::memory_order_acquire),nrrj::grade_bytes,e.externo_grade,e.grade_hip,"grade")){motivo="grade_sem_handle";return false;}
    if(!S.palavra.compare_exchange_strong(w,nrrj::palavra_g(nrrj::GLendo,job),std::memory_order_acq_rel)){motivo="grade_corrida";return false;}
    e.grade_lido=s;G.usadas.fetch_add(1);
    m=CenaMv{};m.mv=e.grade_hip+nrrj::grade_off_mv(unsigned(s));m.pitch=S.w*4;m.fmt=0;m.mvw=S.w;m.mvh=S.h;m.sx=1.f;m.sy=1.f;m.neth=S.h;m.netw=S.w;
    m.dep=e.grade_hip+nrrj::grade_off_d(unsigned(s));m.dpitch=S.w*4;m.dfmt=0;m.dw=S.w;m.dh=S.h;m.dinv=1;
    return true;
}
inline void sem_dados_log(Estado& e,int job,const char* motivo){
    if(++e.sem_dados>8&&(e.sem_dados&255u)!=0)return;
    const unsigned char* p=nrcena::params;CenaMv m{};
    std::memcpy(&m.mv,p+0x10,8);std::memcpy(&m.pitch,p+0x18,4);std::memcpy(&m.fmt,p+0x1c,4);std::memcpy(&m.mvw,p+0x20,4);std::memcpy(&m.mvh,p+0x24,4);
    std::memcpy(&m.neth,p+0x68,4);std::memcpy(&m.netw,p+0x6c,4);std::memcpy(&m.dep,p+0x30,8);std::memcpy(&m.dpitch,p+0x38,4);std::memcpy(&m.dfmt,p+0x3c,4);
    std::memcpy(&m.dw,p+0x40,4);std::memcpy(&m.dh,p+0x44,4);std::memcpy(&m.dinv,p+0x48,4);
    auto& G=nrrj::grade();
    char l[600]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_SEM_DADOS n=%llu job=%d motivo=%s params_ok=%d mv=%p fmt=%d mvw=%d mvh=%d pitch=%d dep=%p dw=%d dh=%d dpitch=%d dfmt=%d dinv=%d netw=%d neth=%d grade_quer=%d gravadas=%llu usadas=%llu sem_slot=%llu",
        e.sem_dados,job,motivo,nrcena::params_ok?1:0,m.mv,m.fmt,m.mvw,m.mvh,m.pitch,m.dep,m.dw,m.dh,m.dpitch,m.dfmt,m.dinv,m.netw,m.neth,G.quer.load()?1:0,
        static_cast<unsigned long long>(G.gravadas.load()),static_cast<unsigned long long>(G.usadas.load()),static_cast<unsigned long long>(G.sem_slot.load()));
    nrrj::log(l);
}
inline void preparar_deteccao(Estado& e,Hip& hip,char* base,int job){
    auto& D=det();e.det_pend=false;

    nrrj::grade().invertida.store(((read<int>(base,0x94A80)!=0)!=prof_trocada.load(std::memory_order_relaxed))?1:0,std::memory_order_relaxed);
    CenaMv m{};int fonte=0;const char* motivo="?";
    if(CenaGpu::mv_do_job(m)){
        if(m.dep){fonte=1;if(prof_trocada.load(std::memory_order_relaxed))m.dinv=m.dinv?0:1;}
        else motivo="proxy_sem_profundidade";
    }else motivo=nrcena::params_ok?"proxy_vetor_invalido":"proxy_sem_reprojparams";

    if(fonte==1){e.falta_n=0;if(++e.proxy_ok_n>=8)nrrj::grade().quer.store(false,std::memory_order_relaxed);}else{e.proxy_ok_n=0;++e.falta_n;}
    if(!fonte){const char* mg="aguardando_3_jobs";if(e.falta_n>=3&&grade_do_job(e,job,m,mg))fonte=2;else{char t[96];NRLOGF(t,sizeof t,"%s+%s",motivo,mg);sem_dados_log(e,job,t);return;}}
    if(fonte!=e.fonte_ant){if(e.fonte_ant){char l[120];NRLOGF(l,sizeof l,"ROSTO_JOGADOR_FONTE %s (job %d)",fonte==2?"render":"proxy",job);nrrj::log(l);}e.fonte_ant=fonte;}

    bool rosto_grande=false;
    if((nrrj::adquire.load(std::memory_order_relaxed)&1)&&D.sem_cabeca.load(std::memory_order_relaxed)>economia_apos){
        const auto qd=nrprescale::unpack_quadro(nrprescale::quadro.load(std::memory_order_acquire));const float f4=qd.saida_h?2160.f/float(qd.saida_h):1.f;
        auto& RS=nrrj::rostos();std::lock_guard<std::mutex> l(RS.m);for(const auto& c:RS.lista)if(c.nota>=.7f&&1.6f*c.h*f4>=280.f)rosto_grande=true;}
    if(!rosto_grande&&D.sem_cabeca.load(std::memory_order_relaxed)>economia_apos&&(unsigned(job)%economia_cada)!=0u){D.economizados.fetch_add(1,std::memory_order_relaxed);return;}
    const int q=(std::max)(1,m.mvh/180),gw=m.mvw/q,gh=m.mvh/q;if(gw<16||gh<16){sem_dados_log(e,job,"grade_pequena");return;}
    const int set=det_thread.load(std::memory_order_relaxed)?int(unsigned(job)&1u):0;
    auto& S=D.sets[set];
    if(S.ocupado.load(std::memory_order_acquire)){D.pulados.fetch_add(1);return;}
    const size_t need=size_t(m.mvw)*m.mvh;
    if(need>S.cap){
        for(void** p:{&S.dq,&S.mvq,&S.d})if(*p){(void)hip.release(*p);*p=nullptr;}
        S.cap=0;
        if(hip.allocate(&S.dq,size_t(gw)*gh*4)||hip.allocate(&S.mvq,size_t(gw)*gh*8)||hip.allocate(&S.d,need*4)){(void)hip.error();nrrj::envenenar("hipMalloc detector");return;}
        S.cap=need;
    }
    int qq=q,ugw=gw,ugh=gh;void* a[]={&m,&qq,&ugw,&ugh,&S.dq,&S.mvq,&S.d};lancar(hip,e.k_reduz,size_t(gw)*gh,a);
    int dev=-1;if(hip.device(&dev))dev=-1;
    e.det_tarefa=DetTarefa{set,gw,gh,m.mvw,m.mvh,job,nrreusostate::quadro_do_job(job),dev,fonte};e.det_pend=true;e.det_dinv=m.dinv;
}

struct SjAval {int slot{-1};int id{};float dx{},dy{};};
inline void avaliar_recorte_i(Estado& e,Hip& hip,char* base,void* primary,int job,void* dest,int rw,int rh,int pitch,int formato,const SjAval* sa=nullptr);
inline void sj_intercalar() noexcept;

inline void avaliar_recorte(Estado& e,Hip& hip,char* base,void* primary,int job,void* dest,int rw,int rh,int pitch,int formato){
    avaliar_recorte_i(e,hip,base,primary,job,dest,rw,rh,pitch,formato);
    if(nrrj::inst_atual()==0&&nrrj::npc2_ativo()){std::lock_guard<std::mutex> l1(estado_de(1).mutex);
     for(int kn=1;kn<=nrrj::npc_n();++kn){nrrj::Inst i1(kn);
        if(nrrj::ativo()){auto& e1=estado();std::unique_lock<std::mutex> l(e1.mutex,std::defer_lock);if(kn>1)l.lock();
            try{if(modulo(e1,hip))avaliar_recorte_i(e1,hip,base,primary,job,dest,rw,rh,pitch,formato);}
            catch(const HipFailure& f){char t[160]{};NRLOGF(t,sizeof t,"hip fase=%s status=%d (recorte npc2)",f.phase,f.status);nrrj::envenenar(t);}
            catch(const std::exception& x){nrrj::envenenar(x.what());}
            catch(...){nrrj::envenenar("excecao recorte npc2");}}
        if(nrrj::sj_q())sj_intercalar();}}
}

inline void pontos_dela(nrrj::Slot& S,unsigned s) noexcept {
    try{
        auto& P=nrrj::pontos();auto& E=P.ent[s];const unsigned char* base=P.base.load(std::memory_order_acquire);
        if(!base||E.seq.load(std::memory_order_acquire)!=S.seq||E.l<=0||!(S.hl>0.f))return;
        thread_local nryn::Rede rede;thread_local int simd=-1;thread_local std::vector<std::uint8_t> bgr;thread_local std::vector<nryn::Rosto> rs;thread_local std::vector<std::uint16_t> hb;
        {const int sm=nrrj::yunet_simd.load(std::memory_order_relaxed);if(sm!=simd){simd=sm;rede.usar_simd(sm);}}
        const int n=int(nrrj::roi_lado);hb.resize(size_t(n)*size_t(n)*4);std::memcpy(hb.data(),base+std::uint64_t(s)*nrrj::roi_bytes,hb.size()*2);
        if(E.seq.load(std::memory_order_acquire)!=S.seq)return;
        const auto t0=std::chrono::steady_clock::now();
        nryn::tom_bgr_meia(hb.data(),n,n,bgr,meia);rede.detectar(bgr.data(),n,n,n*3,rs,.5f,.3f);
        const float k=float(E.l)/float(n),hl=S.hl;int ib=-1;float bd=1e9f,cx=0,cy=0;
        for(size_t i=0;i<rs.size();++i){const auto& r=rs[i];if(r.nota<.6f)continue;const float fh=r.h*k;if(fh<.4f*hl||fh>1.f*hl)continue;
            float mx=0,my=0;for(int p=0;p<5;++p){mx+=r.lm[2*p];my+=r.lm[2*p+1];}mx=float(E.x0)+mx/5.f*k;my=float(E.y0)+my/5.f*k;
            const float d=std::hypot(mx-S.hx,my-S.hy);if(d<=.6f*hl&&d<bd){bd=d;ib=int(i);cx=mx;cy=my;}}
        P.us.fetch_add(static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-t0).count()),std::memory_order_relaxed);
        const auto rod=P.rodadas.fetch_add(1,std::memory_order_relaxed)+1;
        const float ax=S.hx,ay=S.hy;
        if(ib>=0){const float nx=cx+.05f*hl,ny=cy-.08f*hl;S.zx+=nx-S.hx;S.zy+=ny-S.hy;S.hx=nx;S.hy=ny;P.achou.fetch_add(1,std::memory_order_relaxed);}
        (void)ax;(void)ay;(void)rod;
    }catch(...){}
}
inline void avaliar_recorte_i(Estado& e,Hip& hip,char* base,void* primary,int job,void* dest,int rw,int rh,int pitch,int formato,const SjAval* sa){
    nrorc::EscopoOrigem eo(nrrj::inst_atual()>=1?nrorc::Origem::Npc:nrorc::Origem::Rosto);
    nrorc::EscopoRedeRosto er;
    auto& g=nrrj::global();
    int s=-1;
    if(sa){
        const unsigned k=unsigned(sa->slot);
        if(k<nrrj::slots){auto w=g.slot[k].palavra.load(std::memory_order_acquire);
            if(nrrj::estado_de(w)==nrrj::Escrito&&nrrj::job_de(w)==sa->id&&g.marcado(k)==g.slot[k].seq&&
               g.slot[k].palavra.compare_exchange_strong(w,nrrj::palavra(nrrj::Avaliando,sa->id),std::memory_order_acq_rel))s=int(k);}}
    else for(unsigned k=0;k<nrrj::slots;++k){auto w=g.slot[k].palavra.load(std::memory_order_acquire);
        if(nrrj::estado_de(w)==nrrj::Escrito&&nrrj::job_de(w)==job){
            if(g.marcado(k)!=g.slot[k].seq){g.slot[k].palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);g.pulados_marcador.fetch_add(1);continue;}
            if(g.slot[k].palavra.compare_exchange_strong(w,nrrj::palavra(nrrj::Avaliando,job),std::memory_order_acq_rel))s=int(k);break;}}
    if(s<0)return;
    const int etiqueta=sa?sa->id:job;
    auto& S=g.slot[unsigned(s)];
    try{
        if(nrneuraltone::requested&&!nrneuraltone::multipass_ok)throw std::runtime_error("ToneCurve ACES ou ToneLift > 0: dominio da rede diferente do da composicao");
        if(formato!=0||!dest||rw<64||rh<64||pitch<rw*8)throw std::runtime_error("residuo da cena fora do contrato (half RGBA)");
        if(!importar(e)||!grades(e,hip)){S.palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);return;}
        const unsigned n=S.n,L=S.L,ow=S.ow,oh=S.oh;
        if(nrrj::inst_atual()==0&&nrrj::caminha_dela.load(std::memory_order_relaxed)==3)pontos_dela(S,unsigned(s));
        bool amplia=(nrrj::zoom.load(std::memory_order_relaxed)&8u)&&n<=2u*L;
        amplia=amplia||((nrrj::npc_amplia_inst()||nrrj::canon_inst())&&n<=2u*L);
        amplia=amplia||(nrrj::todos_inst()&&n<=2u*L);
        amplia=amplia||(nrrj::inst_atual()==0&&nrrj::rede_fixa.load(std::memory_order_relaxed)&&n<=nrrj::rede_fixa.load(std::memory_order_relaxed)*L);
        if(n<nrrj::n_min||n>nrrj::n_cap()||(n&127u)||(L<n&&!amplia)||!ow||!oh)throw std::runtime_error("recorte fora do contrato");

        {static std::atomic<unsigned> vistos{0};const unsigned b=1u<<((n/128u)&31u);
         if(!(vistos.fetch_or(b,std::memory_order_relaxed)&b)){char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_REDE_LADO n=%u recorte=%u teto=%u reduz=%d",n,L,nrrj::teto.load(),L>n?1:0);nrrj::log(l);}}
        Tempos tm3;tm3.marca(hip);
        const auto t0=std::chrono::steady_clock::now();
        const float expo=[&]{const float v=read<float>(base,0x90CC0+4*(unsigned(job)&3u));return std::isfinite(v)&&v>0.f?v:1.f;}();
        const unsigned tonemap=read<unsigned>(base,0x94698)!=0?1u:0u;
        float passo=float(L)/float(n),bx=float(S.x),by=float(S.y),fow=float(ow),foh=float(oh);

        float bxc=sa?bx+sa->dx:bx,byc=sa?by+sa->dy:by;
        char* ent=e.base_hip+nrrj::offset_ent(unsigned(s));char* sai=e.base_hip+nrrj::offset_sai(unsigned(s));
        unsigned urw=unsigned(rw),urh=unsigned(rh),upitch=unsigned(pitch/2),un=n;
        const bool despeja=despejo_n()&&e.avaliados+1==despejo_n();
        std::filesystem::path pasta;if(despeja){pasta=pasta_helper()/L"nr_rosto_jogador_despejo";std::error_code ec;std::filesystem::create_directories(pasta,ec);}
        if(despeja){hip.completed("rj_despejo");despejar(hip,ent,size_t(n)*n*8,pasta/L"recorte_linear.raw");}

        if(nrrj::soma_cena.load(std::memory_order_relaxed))check(hip.set(e.dc,0,size_t(n)*n*16));
        else{void* a[]={&dest,&urw,&urh,&upitch,&bxc,&byc,&passo,&fow,&foh,&un,&e.dc};lancar(hip,e.k_cena,size_t(n)*n,a);}
        {float ex=expo;unsigned tm=tonemap;void* a[]={&ent,&un,&ex,&tm};lancar(hip,e.k_entrada,size_t(n)*n,a);}

        if(!sa&&e.pula_job==job){e.pula_job=0;S.palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);return;}

        bool fres=nrrj::frescor.load(std::memory_order_relaxed)>0.f&&nrrj::acumula.load(std::memory_order_relaxed)&&nrrj::temporal_inst()&&nrrj::frescor_inst()&&e.k_lumin&&e.k_bandas2&&e.k_meia4;
        if(fres&&!e.lin&&!e.lin_tentado){e.lin_tentado=true;if(hip.allocate(&e.lin,size_t(nrrj::n_cap())*nrrj::n_cap()*4)){(void)hip.error();e.lin=nullptr;}}
        fres=fres&&e.lin;
        if(fres){void* a[]={&ent,&un,&e.lin};lancar(hip,e.k_lumin,size_t(n)*n,a);}
        tm3.marca(hip);
        void* p=contexto(e,hip,base,primary);
        {const float f1=nrrj::forca_passo1();if(f1!=1.f)write<float>(p,0x30,read<float>(p,0x30)*f1);}
        if(const unsigned r1=r1_modo()){

            static const unsigned r1_antes=[]{char b[16]{};size_t k=0;return getenv_s(&k,b,sizeof b,"NR_RJ_R1_ANTES")==0&&k?unsigned(std::strtoul(b,nullptr,10)):0u;}();
            static const unsigned r1_sono=[]{char b[16]{};size_t k=0;return getenv_s(&k,b,sizeof b,"NR_RJ_R1_SONO")==0&&k?unsigned(std::strtoul(b,nullptr,10)):0u;}();
            static const unsigned r1_flush=[]{char b[16]{};size_t k=0;return getenv_s(&k,b,sizeof b,"NR_RJ_R1_FLUSH")==0&&k?unsigned(std::strtoul(b,nullptr,10)):0u;}();
            static void* r1_lixo{};if(r1_flush&&!r1_lixo&&hip.allocate(&r1_lixo,size_t(r1_flush)<<20)){(void)hip.error();r1_lixo=nullptr;}
            if(r1_antes){hip.completed("rj_r1");Sleep(r1_antes);}
            const auto m1=r1_eval(hip,base,p,ent,sai,n);
            if(r1_sono)Sleep(r1_sono);
            if(r1_lixo){check(hip.set(r1_lixo,int(e.avaliados&255u),size_t(r1_flush)<<20));hip.completed("rj_r1");}
            p=contexto(e,hip,base,primary);const auto m2=r1_eval(hip,base,p,ent,sai,n);
            static unsigned long long r1_linhas=0;
            if(r1_linhas<400||(e.avaliados&15u)==0){++r1_linhas;
                char t[260]{};NRLOGF(t,sizeof t,"ROSTO_R1 aval=%llu n=%u e1_cpu=%.3f e1=%.3f e2_cpu=%.3f e2=%.3f",e.avaliados+1,n,m1.chamada,m1.total,m2.chamada,m2.total);nrrj::log(t);}
            if(r1>=2&&e.avaliados+1==40){
                const unsigned nn_max=(std::min)(nrrj::teto.load(std::memory_order_relaxed),nrrj::n_max);
                for(unsigned nn=256u;nn<=(std::max)(1024u,nn_max)&&nn<=nrrj::n_max;nn+=128u){
                    R1Medida a{},b{};p=contexto(e,hip,base,primary);a=r1_eval(hip,base,p,ent,sai,nn);
                    p=contexto(e,hip,base,primary);b=r1_eval(hip,base,p,ent,sai,nn);
                    p=contexto(e,hip,base,primary);const auto c=r1_eval(hip,base,p,ent,sai,nn);
                    char t[260]{};NRLOGF(t,sizeof t,"ROSTO_R1_LADO n=%u mpx=%.4f a=%.3f b_cpu=%.3f b=%.3f c_cpu=%.3f c=%.3f",nn,double(nn)*nn/1e6,a.total,b.chamada,b.total,c.chamada,c.total);nrrj::log(t);
                }
                p=contexto(e,hip,base,primary);(void)r1_eval(hip,base,p,ent,sai,n);
            }
        }
        else if(nrrj::ablacao.load(std::memory_order_relaxed)&2u)check(hip.copy(sai,ent,size_t(n)*n*8,3));
        else native<Eval>(base,0x2a080)(p,ent,int(n*8),0,int(n),int(n),sai,int(n*8),0,0,nullptr);
        hip.completed("rj_rede");
        tm3.marca(hip);

        char* sai_f=sai;
        if(nrrj::passos.load(std::memory_order_relaxed)>=2&&e.k_sat&&!(nrrj::ablacao.load(std::memory_order_relaxed)&2u)){
            const double p1=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
            if(e.p1_n==0||p1<e.p1_min)e.p1_min=p1;else e.p1_min+=.001*(p1-e.p1_min);
            e.p1_ema=e.p1_n?.85*e.p1_ema+.15*p1:p1;++e.p1_n;
            const float folga=passo2_folga.load(std::memory_order_relaxed);
            bool cabe=folga<=0.f||e.p1_n<4||e.p1_ema<=double(folga)*e.p1_min;

            {const unsigned c2=nrrj::passo2_cada.load(std::memory_order_relaxed);if(c2>1)cabe=cabe&&(e.p2_k++%c2)==0;}
            if(!e.in2&&!e.buf2_tentado){e.buf2_tentado=true;const size_t b=size_t(nrrj::n_cap())*nrrj::n_cap()*8;
                if(hip.allocate(&e.in2,b)||hip.allocate(&e.out2,b)){(void)hip.error();e.in2=e.out2=nullptr;nrrj::log("ROSTO_JOGADOR_PASSOS sem memoria: 1 passo");}}
            if(cabe&&e.in2&&e.out2){
                {void* a[]={&sai,&un,&e.in2};lancar(hip,e.k_sat,size_t(n)*n,a);}
                p=contexto(e,hip,base,primary);write<float>(p,0x30,.03125f*nrrj::passo2_forca.load(std::memory_order_relaxed));
                native<Eval>(base,0x2a080)(p,e.in2,int(n*8),0,int(n),int(n),e.out2,int(n*8),0,0,nullptr);
                hip.completed("rj_rede2");sai_f=static_cast<char*>(e.out2);++e.passos2;
            }else if(e.in2)++e.passos2_pulados;
            if(e.p1_n<=4||(e.p1_n&63u)==0){char t[200]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_PASSOS n=%u p1=%.2f p1_ema=%.2f p1_min=%.2f cabe=%d passos2=%llu pulados2=%llu",e.p1_n,p1,e.p1_ema,e.p1_min,cabe?1:0,e.passos2,e.passos2_pulados);nrrj::log(t);}
        }
        if(r3_modo()&&e.avaliados+1>=30&&(e.avaliados+1-30)%20==0&&e.avaliados+1<=130)r3_diag(e,hip,base,primary,ent,sai,n);
        tm3.marca(hip);
        const auto t_rede=std::chrono::steady_clock::now();
        {void* a[]={&ent,&sai_f,&e.dc,&un,&e.d0};lancar(hip,e.k_dif,size_t(n)*n,a);}

        const float cel=foh/2160.f/passo;
        unsigned f=4,nl=n/4;
        {void* a[]={&e.d0,&un,&f,&nl,&e.lo};lancar(hip,e.k_baixa,size_t(nl)*nl,a);}
        {float sl=nrrj::banda_sigma.load(std::memory_order_relaxed)*cel/float(f);unsigned rl=raio_cv(sl),h1=1,h0=0,ref=1;
         void* a1[]={&e.lo,&e.lo2,&nl,&nl,&sl,&rl,&h1,&ref};lancar(hip,e.k_blur4,size_t(nl)*nl,a1);
         void* a2[]={&e.lo2,&e.lo,&nl,&nl,&sl,&rl,&h0,&ref};lancar(hip,e.k_blur4,size_t(nl)*nl,a2);}
        ElipseK el{(S.hx-bx)/passo,(S.hy-by)/passo,S.hl/passo,.50f,.60f,.85f,1.15f,.55f,.85f};
        unsigned dep=0;

        bool camada=false;
        if(const unsigned cmodo=nrrj::borda_camada.load(std::memory_order_relaxed);cmodo&&e.k_camada&&e.k_baixa_m&&e.k_blur4w&&e.k_delta_m){

            const bool emprestada=nrrj::inst_atual()>=1&&nrrj::npc_borda_camada.load(std::memory_order_relaxed);
            const DetTarefa T=emprestada?estado_de(0).det_tarefa:e.det_tarefa;const int dinv_t=emprestada?estado_de(0).det_dinv:e.det_dinv;
            auto& DS=det().sets[unsigned(T.set>=0?T.set:0)];
            if(T.job==job&&T.set>=0&&DS.d&&T.dw>=16&&T.dh>=16&&size_t(T.dw)*T.dh<=DS.cap){
                float kk=float(T.dw)/fow,hxt=(sa?S.zx+sa->dx:S.zx)*kk,hyt=(sa?S.zy+sa->dy:S.zy)*kk,c0=nrrj::borda_camada_z0.load(std::memory_order_relaxed),c1=nrrj::borda_camada_z1.load(std::memory_order_relaxed);int idw=T.dw,idh=T.dh;unsigned dv=dinv_t?1u:0u,perto=nrrj::borda_camada_perto.load(std::memory_order_relaxed)?1u:0u;
                {void* a[]={&DS.d,&idw,&idh,&dv,&hxt,&hyt,&bxc,&byc,&passo,&kk,&un,&c0,&c1,&perto,&e.tmp1};lancar(hip,e.k_camada,size_t(n)*n,a);}
                {void* a[]={&e.d0,&e.tmp1,&un,&f,&nl,&e.lo,&e.gl};lancar(hip,e.k_baixa_m,size_t(nl)*nl,a);}
                {float sl=nrrj::banda_sigma.load(std::memory_order_relaxed)*cel/float(f);unsigned rl=raio_cv(sl),h1=1,h0=0,ref=1;
                 void* a1[]={&e.lo,&e.lo2,&nl,&nl,&sl,&rl,&h1,&ref};lancar(hip,e.k_blur4w,size_t(nl)*nl,a1);
                 void* a2[]={&e.lo2,&e.lo,&nl,&nl,&sl,&rl,&h0,&ref};lancar(hip,e.k_blur4w,size_t(nl)*nl,a2);
                 void* b1[]={&e.gl,&e.lo2,&nl,&nl,&sl,&rl,&h1,&ref};lancar(hip,e.k_blur4w,size_t(nl)*nl,b1);
                 void* b2[]={&e.lo2,&e.gl,&nl,&nl,&sl,&rl,&h0,&ref};lancar(hip,e.k_blur4w,size_t(nl)*nl,b2);}
                float rg=nrrj::borda_camada_reg.load(std::memory_order_relaxed);unsigned enc=cmodo>=2?1u:0u;
                {void* a[]={&e.d0,&e.lo,&e.gl,&e.tmp1,&un,&f,&nl,&el,&dep,&rg,&enc,&e.D,&e.luma};lancar(hip,e.k_delta_m,size_t(n)*n,a);}
                if(despeja){hip.completed("rj_despejo");despejar(hip,e.tmp1,size_t(n)*n*4,pasta/L"mascara_camada.raw");}
                camada=true;++e.camada_n;
            }else ++e.camada_sem;
            if(e.camada_n+e.camada_sem<=4||((e.camada_n+e.camada_sem)&63u)==0){char t[160]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_CAMADA job=%d usadas=%llu sem_grade=%llu modo=%u",job,e.camada_n,e.camada_sem,cmodo);nrrj::log(t);}
        }
        if(!camada){void* a[]={&e.d0,&e.lo,&un,&f,&nl,&el,&dep,&e.D,&e.luma};lancar(hip,e.k_delta,size_t(n)*n,a);}
        {float s4=4.f*cel;unsigned r4=raio_cv(s4),h1=1,h0=0,zero=2;
         void* b1[]={&e.luma,&e.tmp1,&un,&un,&s4,&r4,&h1,&zero};lancar(hip,e.k_blur1,size_t(n)*n,b1);
         void* b2[]={&e.tmp1,&e.gl,&un,&un,&s4,&r4,&h0,&zero};lancar(hip,e.k_blur1,size_t(n)*n,b2);}
        if(despeja){hip.completed("rj_despejo");
            despejar(hip,sai,size_t(n)*n*8,pasta/L"saida_rede.raw");despejar(hip,ent,size_t(n)*n*8,pasta/L"entrada_tom.raw");
            despejar(hip,e.dc,size_t(n)*n*16,pasta/L"residuo_cena.raw");despejar(hip,e.D,size_t(n)*n*16,pasta/L"delta_antes_guarda.raw");}
        {void* a[]={&e.D,&e.gl,&un,&el,&dep,&ent};lancar(hip,e.k_guarda,size_t(n)*n,a);}

        float soma_h[3]{};const bool mede=(e.avaliados%64u)==0u;
        if(mede){check(hip.set(e.soma,0,16));void* a[]={&e.D,&un,&e.soma};lancar(hip,e.k_mede,(size_t(n)*n+15)/16,a);}

        if(nrrj::acumula.load(std::memory_order_relaxed)&&nrrj::temporal_inst()&&e.k_meia4&&e.k_bandas){
            {void* a[]={&ent,&un,&e.dc};lancar(hip,e.k_meia4,size_t(n)*n,a);}
            float sb=nrrj::acum_sigma.load(std::memory_order_relaxed)*cel;unsigned rb=raio_cv(sb),h1=1,h0=0,ref=1;
            {void* a1[]={&e.dc,&e.d0,&un,&un,&sb,&rb,&h1,&ref};lancar(hip,e.k_blur4,size_t(n)*n,a1);
             void* a2[]={&e.d0,&e.dc,&un,&un,&sb,&rb,&h0,&ref};lancar(hip,e.k_blur4,size_t(n)*n,a2);}
            if(fres){void* a[]={&ent,&e.dc,&un,&sai,&e.lin};lancar(hip,e.k_bandas2,size_t(n)*n,a);nrrj::frescor_pronto.store(true,std::memory_order_release);}
            else{void* a[]={&ent,&e.dc,&un,&sai};lancar(hip,e.k_bandas,size_t(n)*n,a);}
        }
        {void* tag=e.base_hip+nrrj::offset_tag(unsigned(s));unsigned uj=unsigned(etiqueta);void* a[]={&tag,&uj};check(hip.launch(e.k_etiqueta,1,1,1,1,1,1,0,nullptr,a,nullptr));}
        hip.completed("rj_delta");
        tm3.marca(hip);
        if(tempos_modo()){static unsigned long long nt=0;if(nt++<400){char t[200]{};NRLOGF(t,sizeof t,"ROSTO_R3_TEMPOS aval=%llu n=%u pre=%.3f rede1=%.3f rede2=%.3f pos=%.3f",e.avaliados+1,n,tm3.ms[0],tm3.ms[1],tm3.ms[2],tm3.ms[3]);nrrj::log(t);}}
        if(mede)check(hip.copy(soma_h,e.soma,12,2));
        if(despeja){despejar(hip,ent,size_t(n)*n*8,pasta/L"delta_final.raw");
            std::ofstream fj(pasta/L"despejo.json");
            fj<<"{\"job\":"<<job<<",\"slot\":"<<s<<",\"x\":"<<S.x<<",\"y\":"<<S.y<<",\"L\":"<<L<<",\"n\":"<<n<<",\"saida\":["<<ow<<","<<oh<<"],\"cabeca\":["<<S.hx<<","<<S.hy<<","<<S.hl<<"],"
              <<"\"exposicao\":"<<expo<<",\"tonemap\":"<<tonemap<<",\"residuo_cena\":["<<rw<<","<<rh<<"],"
              <<"\"formatos\":{\"recorte_linear\":\"half4 n x n (cor linear da saida do FSR)\",\"entrada_tom\":\"half4 (entrada da rede)\",\"saida_rede\":\"half4\","
              <<"\"residuo_cena\":\"float4 n x n\",\"delta_antes_guarda\":\"float4\",\"delta_final\":\"half4 (dominio do tom)\"}}\n";
            char t[160];NRLOGF(t,sizeof t,"ROSTO_JOGADOR_DESPEJO job=%d pasta=nr_rosto_jogador_despejo",job);nrrj::log(t);}
        S.palavra.store(nrrj::palavra(nrrj::Pronto,etiqueta),std::memory_order_release);g.prontos.fetch_add(1);
        const auto t1=std::chrono::steady_clock::now();
        const double ms=std::chrono::duration<double,std::milli>(t1-t0).count(),ms_rede=std::chrono::duration<double,std::milli>(t_rede-t0).count();
        e.ms_soma+=ms;++e.ms_n;e.ms_max=(std::max)(e.ms_max,ms);e.ultimo_ms=ms;
        ++e.avaliados;g.avaliacoes.fetch_add(1);
        if(e.avaliados<=6||mede||(nrrj::leve_npc()&&(e.avaliados&15u)==0u)){
            char t[360]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_TEMPO avaliacoes=%llu job=%d n=%u L=%u ms=%.2f rede_ms=%.2f ms_media=%.2f ms_max=%.2f prontos=%llu pulados_marcador=%llu delta_abs=%.5f delta_escuro=%.6f",
                e.avaliados,job,n,L,ms,ms_rede,e.ms_soma/(e.ms_n?e.ms_n:1),e.ms_max,g.prontos.load(),g.pulados_marcador.load(),
                soma_h[2]>0?double(soma_h[0]/soma_h[2]):0.,soma_h[2]>0?double(soma_h[1]/soma_h[2]):0.);nrrj::log(t);
            if(mede){e.ms_soma=0;e.ms_n=0;e.ms_max=0;}
        }
    }catch(const HipFailure& f){
        S.palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);
        char t[160]{};NRLOGF(t,sizeof t,"hip fase=%s status=%d",f.phase,f.status);nrrj::envenenar(t);
    }catch(const std::exception& x){S.palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);nrrj::envenenar(x.what());}
    catch(...){S.palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);nrrj::envenenar("excecao");}
}

inline void portao_pulo(Estado& e,Hip& hip,char* base,int job){
    e.pula_job=0;
    const float lim=nrrj::pula_limiar.load(std::memory_order_relaxed);if(lim==0.f||!e.k_mini||!importar(e))return;
    auto& g=nrrj::global();int s=-1;
    for(unsigned k=0;k<nrrj::slots;++k){const auto w=g.slot[k].palavra.load(std::memory_order_acquire);
        if(nrrj::estado_de(w)==nrrj::Escrito&&nrrj::job_de(w)==job){if(g.marcado(k)==g.slot[k].seq)s=int(k);break;}}
    if(s<0)return;
    auto& S=g.slot[unsigned(s)];
    const unsigned n=S.n,L=S.L;if(n<nrrj::n_min||n>nrrj::n_max||L<n)return;
    if(!e.mini&&hip.allocate(&e.mini,4096)){(void)hip.error();e.mini=nullptr;}
    if(!e.mini)return;
    const float passo=float(L)/float(n);
    float cxc=(S.hx-float(S.x))/passo,cyc=(S.hy-float(S.y))/passo,meio=.6f*S.hl/passo;
    float expo=[&]{const float v=read<float>(base,0x90CC0+4*(unsigned(job)&3u));return std::isfinite(v)&&v>0.f?v:1.f;}();
    unsigned tm=read<unsigned>(base,0x94698)!=0?1u:0u,un=n;
    char* ent=e.base_hip+nrrj::offset_ent(unsigned(s));
    void* a[]={&ent,&un,&cxc,&cyc,&meio,&expo,&tm,&e.mini};lancar(hip,e.k_mini,1024,a);
    std::array<float,1024> m{};check(hip.copy(m.data(),e.mini,4096,2));
    double dif=0,ref=0,rel=-1,esc=-1;
    const bool comp=e.mini_tem&&e.mini_ger==S.geracao&&e.mini_hl>0.f;
    if(comp){for(size_t i=0;i<1024;++i){dif+=std::fabs(double(m[i])-double(e.mini_ref[i]));ref+=std::fabs(double(e.mini_ref[i]));}
        rel=dif/(std::max)(ref,1e-3);esc=std::fabs(double(S.hl)/double(e.mini_hl)-1.0);}
    const int sa=comp?nrrj::slot_do_job(e.mini_job):-1;
    const bool pronto=sa>=0&&nrrj::estado_de(g.slot[unsigned(sa)].palavra.load(std::memory_order_acquire))==nrrj::Pronto;
    const bool pula=lim>0.f&&comp&&pronto&&rel<double(lim)&&esc<.04&&e.pulos_seguidos<nrrj::pula_max.load(std::memory_order_relaxed);
    ++e.portoes;
    if(e.portoes<=300||(e.portoes&15u)==0){char t[220]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_PULA n=%llu job=%d rel=%.5f esc=%.4f pronto=%d pula=%d seguidos=%u pulados=%llu",e.portoes,job,rel,esc,pronto?1:0,pula?1:0,e.pulos_seguidos,e.pulados+(pula?1u:0u));nrrj::log(t);}
    if(pula){++e.pulos_seguidos;++e.pulados;e.pula_job=job;return;}
    e.pulos_seguidos=0;e.mini_ref=m;e.mini_hl=S.hl;e.mini_ger=S.geracao;e.mini_tem=true;e.mini_job=job;
}

struct Preparo {char* base{};void* primary{};bool feito{},falhou{};};
inline Preparo& preparo(){static auto* p=new Preparo;return *p;}
inline bool preparar_orcamento(Estado& e0,Hip& hip,char* base,void* primary){
    const auto t0=std::chrono::steady_clock::now();
    const auto& d=nrorc::decisao();
    if(!modulo(e0,hip))return false;
    (void)contexto(e0,hip,base,primary);
    if(!grades(e0,hip))return false;
    const size_t n2=size_t(d.cap_grace)*d.cap_grace;
    if(nrrj::frescor.load()>0.f&&nrrj::acumula.load()&&e0.k_lumin&&e0.k_bandas2&&e0.k_meia4&&!e0.lin){e0.lin_tentado=true;check(hip.allocate(&e0.lin,n2*4));}
    if(nrrj::passos.load()>=2&&e0.k_sat&&!e0.in2){e0.buf2_tentado=true;check(hip.allocate(&e0.in2,n2*8));check(hip.allocate(&e0.out2,n2*8));}
    if(nrrj::pula_limiar.load()!=0.f&&e0.k_mini&&!e0.mini)check(hip.allocate(&e0.mini,4096));
    if(!importar(e0))return false;
    for(int k=1;k<=d.k_npc;++k){nrrj::Inst ik(k);auto& e=estado();std::lock_guard<std::mutex> l(e.mutex);
        compartilhar(e,e0);if(!importar(e))return false;
        if(k==1&&!ctx_unico())(void)contexto(e,hip,base,primary);}
    const auto t1=std::chrono::steady_clock::now();
    const auto la=nrorc::lados_aquecer(d,ctx_unico());
    unsigned lados[32]{};int avaliacoes=0;
    auto aquece=[&](unsigned teto){const int q=nrorc::lados_ordem(teto,nrorc::lado_piso,lados,32);
        for(int i=0;i<q;++i){const unsigned n=lados[i];check(hip.set(e0.dc,0,size_t(n)*n*8));void* p=contexto(estado(),hip,base,primary);
            {nrorc::EscopoRedeRosto er;native<Eval>(base,0x2a080)(p,e0.dc,int(n*8),0,int(n),int(n),e0.d0,int(n*8),0,0,nullptr);}
            hip.completed("rj_aquece");++avaliacoes;
            if(nrrj::npc_leve.load(std::memory_order_relaxed)){
                const auto a0=std::chrono::steady_clock::now();{nrorc::EscopoRedeRosto er;native<Eval>(base,0x2a080)(p,e0.dc,int(n*8),0,int(n),int(n),e0.d0,int(n*8),0,0,nullptr);}
                hip.completed("rj_aquece2");char la[96]{};NRLOGF(la,sizeof la,"ROSTO_JOGADOR_LADO_MS n=%u ms=%.2f (npc-leve)",n,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-a0).count());nrrj::log(la);}}};
    aquece(la.n[0]);
    if(la.quantos>1){nrrj::Inst i1(1);aquece(la.n[1]);}
    nrorc::estado().preparado.store(true,std::memory_order_release);
    const auto t2=std::chrono::steady_clock::now();
    const double ms=std::chrono::duration<double,std::milli>(t2-t0).count(),ms_aq=std::chrono::duration<double,std::milli>(t2-t1).count();
    const std::uint64_t real=nrorc::estado().guarda.bytes_rosto();
    char l[360]{};NRLOGF(l,sizeof l,"NPC_ORCAMENTO_PRONTO ms=%.1f aquecimento_ms=%.1f grace=%u npc=%d recorte_npc=%u previsto_mb=%llu contado_mb=%llu aquecido=%u..%u npc_ctx=%u avaliacoes=%d ctx_unico=%d",
        ms,ms_aq,d.cap_grace,d.k_npc,d.cap_npc,static_cast<unsigned long long>(d.total/nrorc::MB),static_cast<unsigned long long>(real/nrorc::MB),
        la.n[0],nrorc::lado_piso,la.quantos>1?la.n[1]:0u,avaliacoes,ctx_unico()?1:0);
    nrrj::log(l);nrorc_vram::log("%s",l);nrorc_vram::log_pool(nrorc_vram::job_visto.load(),"pronto");
    if(real>d.total+64u*nrorc::MB)nrorc_vram::log("NPC_ORCAMENTO_AVISO contado %llu MB > previsto %llu MB + 64: conferir o D2 do FASE1-MEDIDAS.md",
        static_cast<unsigned long long>(real/nrorc::MB),static_cast<unsigned long long>(d.total/nrorc::MB));
    return true;}

struct UltimoPub {std::mutex m;Estado::Fora f{};};
inline UltimoPub& ultimo_pub(){static auto* u=new UltimoPub;return *u;}
inline void antes_do_publish(char* base,void* primary,int job,void* dest,int rw,int rh,int pitch,int formato) noexcept {
    if(job<=0)return;
    ler_ini(base);
    nrorc_vram::por_job(job);
    if(!nrrj::ativo())return;
    auto& e=estado();std::lock_guard<std::mutex> lock(e.mutex);
    e.fora_pend=Estado::Fora{};
    if(nrrj::sj_q()&&nrrj::inst_atual()==0){auto& U=ultimo_pub();std::lock_guard<std::mutex> lu(U.m);U.f=Estado::Fora{job,base,primary,dest,rw,rh,pitch,formato};
        if(job>nrrj::sj_job_publicado.load(std::memory_order_relaxed))nrrj::sj_job_publicado.store(job,std::memory_order_release);}
    try{
        nrorc::EscopoOrigem eo(nrorc::Origem::Rosto);
        Hip hip;
        if(!modulo(e,hip))return;
        ++e.jobs;
        const bool orc1=nrorc::modo()==1;
        if(!orc1){

            if(!e.carregado)(void)contexto(e,hip,base,primary);

            if(nrrj::npc2_ativo())for(int kn=1;kn<=nrrj::npc_n();++kn){nrrj::Inst i1(kn);auto& e1=estado();std::lock_guard<std::mutex> l1(e1.mutex);
                try{if(nrrj::ativo()&&modulo(e1,hip)&&kn==1&&!e1.carregado)(void)contexto(e1,hip,base,primary);}catch(...){nrrj::envenenar("pesos npc2");}}
        }

        else if(!preparo().feito){preparo().base=base;preparo().primary=primary;preparar_deteccao(e,hip,base,job);return;}
        preparar_deteccao(e,hip,base,job);

        if(nrrj::fora.load(std::memory_order_relaxed))e.fora_pend=Estado::Fora{job,base,primary,dest,rw,rh,pitch,formato};
        else avaliar_recorte(e,hip,base,primary,job,dest,rw,rh,pitch,formato);
    }catch(const HipFailure& f){char t[160]{};NRLOGF(t,sizeof t,"hip fase=%s status=%d (preparo)",f.phase,f.status);nrrj::envenenar(t);}
    catch(const std::exception& x){nrrj::envenenar(x.what());}
    catch(...){nrrj::envenenar("excecao preparo");}
}

struct FilaRede {std::mutex m;std::condition_variable cv;std::deque<std::pair<Estado::Fora,int>> fila;unsigned long long hs{};bool tentada{},ok{};
    unsigned long long feitos{},sem_recorte{},hs_espera_ok{},hs_espera_falhou{};
    bool sj_pend{};double total_ms{};
    std::uint32_t sj_seq{},sj_adiado{};std::chrono::steady_clock::time_point sj_desde{};double sj_ms_soma{},sj_espera_soma{};unsigned long long sj_n{};};
inline FilaRede& fila_rede(){static FilaRede* f=new FilaRede;return *f;}
inline thread_local bool na_thread_rede=false;
inline void sj_acordar() noexcept {auto& F=fila_rede();{std::lock_guard<std::mutex> l(F.m);F.sj_pend=true;}F.cv.notify_one();}

inline int sj_escolher_pendente() noexcept {
    nrrj::Inst i0(0);auto& g=nrrj::global();nrrj::SjVista v[nrrj::slots]{};
    for(unsigned k=0;k<nrrj::slots;++k){v[k].palavra=g.slot[k].palavra.load(std::memory_order_acquire);if(nrrj::estado_de(v[k].palavra)==nrrj::Escrito)v[k].quadro=g.slot[k].quadro;}
    int velhos[nrrj::slots]{};unsigned nv=0;const int s=nrrj::sj_pendente(v,nrrj::slots,velhos,nv);
    for(unsigned i=0;i<nv;++i){auto w=v[velhos[i]].palavra;
        if(g.slot[unsigned(velhos[i])].palavra.compare_exchange_strong(w,nrrj::palavra(nrrj::Livre,0),std::memory_order_acq_rel))nrrj::sj_conta().velhos.fetch_add(1,std::memory_order_relaxed);}
    return s;
}

inline void sj_avaliar(Estado& e0,int s) noexcept {
    nrrj::Inst i0(0);auto& g=nrrj::global();auto& C=nrrj::sj_conta();auto& S=g.slot[unsigned(s)];
    const auto w=S.palavra.load(std::memory_order_acquire);const int id=nrrj::job_de(w);
    auto livre=[&]{auto ww=w;S.palavra.compare_exchange_strong(ww,nrrj::palavra(nrrj::Livre,0),std::memory_order_acq_rel);};
    if(nrrj::estado_de(w)!=nrrj::Escrito||!nrrj::id_e_quadro(id))return;
    Estado::Fora f{};{auto& U=ultimo_pub();std::lock_guard<std::mutex> lu(U.m);f=U.f;}
    if(f.job<=0||!f.dest){C.sem_pub.fetch_add(1,std::memory_order_relaxed);livre();return;}
    const std::uint64_t Fj=nrrj::sj_quadro_do_job(f.job);const int k=nrrj::sj_base_k(S.quadro,Fj);
    if(k==nrrj::sj_fora){C.sem_pub.fetch_add(1,std::memory_order_relaxed);livre();return;}
    if(k<0)C.futuro.fetch_add(1,std::memory_order_relaxed);
    SjAval a{s,id,0.f,0.f};
    {const int r=nrrj::sj_desloc_par(nrrj::caminho_rb.load(std::memory_order_acquire),S.quadro,Fj,a.dx,a.dy);
     if(r==1)C.estendido.fetch_add(1,std::memory_order_relaxed);else if(r==2)C.zero.fetch_add(1,std::memory_order_relaxed);else if(r==3)C.tabela_tarde.fetch_add(1,std::memory_order_relaxed);}
    const auto t0=std::chrono::steady_clock::now();
    try{Hip hip;if(modulo(e0,hip))avaliar_recorte_i(e0,hip,f.base,f.primary,f.job,f.dest,f.rw,f.rh,f.pitch,f.formato,&a);}
    catch(const HipFailure& x){char tt[160]{};NRLOGF(tt,sizeof tt,"hip fase=%s status=%d (recorte de quadro)",x.phase,x.status);nrrj::envenenar(tt);}
    catch(...){nrrj::envenenar("excecao recorte de quadro");}
    if(nrrj::estado_de(S.palavra.load(std::memory_order_acquire))==nrrj::Escrito)livre();
    const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
    auto& F=fila_rede();F.sj_ms_soma+=ms;++F.sj_n;
    const auto n=C.avaliados.fetch_add(1,std::memory_order_relaxed)+1;
    if(n<=6||(n&127u)==0){char l[300]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_SEMJOB_AVAL n=%llu quadro=%llu job=%d k=%d dx=%.1f dy=%.1f ms=%.2f ms_media=%.2f espera_marcador_media=%.2f",
        n,static_cast<unsigned long long>(S.quadro),f.job,k,double(a.dx),double(a.dy),ms,F.sj_ms_soma/double(F.sj_n),F.sj_espera_soma/double(F.sj_n));nrrj::log(l);}
}

inline void sj_intercalar() noexcept {
    if(!na_thread_rede)return;
    if(nrrj::sj_cena_pendente(nrrj::sj_job_aceito.load(std::memory_order_acquire),nrrj::sj_job_publicado.load(std::memory_order_acquire)))return;
    const int s=sj_escolher_pendente();if(s<0)return;
    {nrrj::Inst i0(0);auto& g=nrrj::global();if(g.marcado(unsigned(s))!=g.slot[unsigned(s)].seq)return;}
    nrrj::sj_conta().intercalados.fetch_add(1,std::memory_order_relaxed);sj_avaliar(estado_de(0),s);
}
inline void laco_rede() noexcept {
    auto& F=fila_rede();auto& e=estado();
    using SetDev=int(*)(int);HMODULE h=GetModuleHandleW(L"amdhip64_7.dll");auto setdev=h?reinterpret_cast<SetDev>(GetProcAddress(h,"hipSetDevice")):nullptr;
    static HANDLE timer=CreateWaitableTimerExW(nullptr,nullptr,0x00000002 ,TIMER_ALL_ACCESS);
    const auto dorme_us=[](unsigned us){if(!us)return;LARGE_INTEGER due{};due.QuadPart=-LONGLONG(us)*10;
        if(timer&&SetWaitableTimer(timer,&due,0,nullptr,nullptr,FALSE))WaitForSingleObject(timer,us/1000u+50u);else Sleep((us+999u)/1000u);};
    static constexpr unsigned auto_us[5]{0,1500,3000,4500,6000};
    double auto_ms[5]{};unsigned auto_n[5]{};unsigned long long auto_k=0;int dev_atual=-1;
    na_thread_rede=true;
    if(nrrj::sj_q())nrrj::acordar_rede.store(&sj_acordar,std::memory_order_release);
    for(;;){
        nrorc::EscopoOrigem eo(nrorc::Origem::Rosto);
        std::pair<Estado::Fora,int> item{};unsigned long long hs=0;bool tem_item=false;

        int sj_s=-1;bool sj_marcado=false;
        if(nrrj::sj_q()){sj_s=sj_escolher_pendente();
            if(sj_s>=0){nrrj::Inst i0(0);auto& g=nrrj::global();const auto& S=g.slot[unsigned(sj_s)];sj_marcado=g.marcado(unsigned(sj_s))==S.seq;
                if(S.seq!=F.sj_seq){F.sj_seq=S.seq;F.sj_desde=std::chrono::steady_clock::now();}

                if(sj_marcado&&nrrj::sj_cena_pendente(nrrj::sj_job_aceito.load(std::memory_order_acquire),nrrj::sj_job_publicado.load(std::memory_order_acquire))&&
                   std::chrono::steady_clock::now()-F.sj_desde<std::chrono::milliseconds(40)){sj_marcado=false;if(F.sj_adiado!=S.seq){F.sj_adiado=S.seq;nrrj::sj_conta().adiados.fetch_add(1,std::memory_order_relaxed);}}
                if(!sj_marcado&&std::chrono::steady_clock::now()-F.sj_desde>std::chrono::milliseconds(250)){
                    auto w=S.palavra.load(std::memory_order_acquire);if(nrrj::estado_de(w)==nrrj::Escrito)
                        g.slot[unsigned(sj_s)].palavra.compare_exchange_strong(w,nrrj::palavra(nrrj::Livre,0),std::memory_order_acq_rel);
                    nrrj::sj_conta().marcador_tarde.fetch_add(1,std::memory_order_relaxed);sj_s=-1;}}}
        {std::unique_lock<std::mutex> l(F.m);
         if(sj_s>=0&&!sj_marcado){if(F.fila.empty()){l.unlock();dorme_us(200);l.lock();}}
         else if(sj_s<0)F.cv.wait(l,[&]{return !F.fila.empty()||F.sj_pend;});
         F.sj_pend=false;
         if(!F.fila.empty()){item=F.fila.front();F.fila.pop_front();hs=F.hs;tem_item=true;}}
        if(sj_s>=0&&sj_marcado){
            F.sj_espera_soma+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-F.sj_desde).count();
            nrregswin::rjsync::Rosto ev;std::lock_guard<std::mutex> lock(e.mutex);sj_avaliar(e,sj_s);}
        if(!tem_item)continue;
        const auto t_item=std::chrono::steady_clock::now();
        const auto f=item.first;
        try{
            if(setdev&&item.second>=0&&item.second!=dev_atual){(void)setdev(item.second);dev_atual=item.second;}

            if(nrregswin::rjsync::flag_set_n.load(std::memory_order_acquire)>hs)++F.hs_espera_ok;else ++F.hs_espera_falhou;
            bool tem=false,pula=false;int auto_i=-1;unsigned esp=0;

            {nrregswin::rjsync::Rosto ev;std::lock_guard<std::mutex> lock(e.mutex);
                try{Hip hip;portao_pulo(e,hip,f.base,f.job);}catch(...){e.pula_job=0;}
                pula=e.pula_job==f.job;
                if(pula){try{Hip hip;avaliar_recorte(e,hip,f.base,f.primary,f.job,f.dest,f.rw,f.rh,f.pitch,f.formato);}catch(...){}}
                auto& g=nrrj::global();
                for(unsigned k=0;k<nrrj::slots&&!tem;++k){const auto w=g.slot[k].palavra.load(std::memory_order_acquire);tem=nrrj::estado_de(w)==nrrj::Escrito&&nrrj::job_de(w)==f.job;}
            }
            if(pula)continue;
            if(!tem){++F.sem_recorte;
                nrregswin::rjsync::Rosto ev;std::lock_guard<std::mutex> lock(e.mutex);
                try{Hip hip;avaliar_recorte(e,hip,f.base,f.primary,f.job,f.dest,f.rw,f.rh,f.pitch,f.formato);}catch(...){}
                continue;}
            esp=nrrj::espera_us.load(std::memory_order_relaxed);
            if(esp==nrrj::espera_present)esp=0;
            if(esp==nrrj::espera_auto){
                const unsigned long long k=auto_k++;
                if(k<25)auto_i=int(k%5);else if(k%16==0)auto_i=int((k/16)%5);
                else{auto_i=0;for(int i=1;i<5;++i)if(auto_n[i]&&(auto_n[auto_i]==0||auto_ms[i]<auto_ms[auto_i]))auto_i=i;}
                esp=auto_us[auto_i];if(k<5)auto_i=-1;
            }
            dorme_us(esp);
            double ms=-1.0;
            {nrregswin::rjsync::Rosto ev;std::lock_guard<std::mutex> lock(e.mutex);
                e.ultimo_ms=-1.0;
                try{Hip hip;avaliar_recorte(e,hip,f.base,f.primary,f.job,f.dest,f.rw,f.rh,f.pitch,f.formato);}
                catch(const HipFailure& x){char t[160]{};NRLOGF(t,sizeof t,"hip fase=%s status=%d (recorte thread)",x.phase,x.status);nrrj::envenenar(t);}
                catch(...){nrrj::envenenar("excecao recorte thread");}
                ms=e.ultimo_ms;}
            ++F.feitos;
            if(auto_i>=0&&ms>0){auto_ms[auto_i]=auto_n[auto_i]?.75*auto_ms[auto_i]+.25*ms:ms;++auto_n[auto_i];}
            F.total_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t_item).count();
            if(F.feitos<=8||(F.feitos&31u)==0){char t[300]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_THREAD_REDE feitos=%llu sem_recorte=%llu hs_ok=%llu hs_falhou=%llu espera_us=%u ms=%.2f total_ms=%.2f auto[0/1.5/3/4.5/6]=%.2f/%.2f/%.2f/%.2f/%.2f",
                F.feitos,F.sem_recorte,F.hs_espera_ok,F.hs_espera_falhou,esp,ms,F.total_ms,auto_ms[0],auto_ms[1],auto_ms[2],auto_ms[3],auto_ms[4]);nrrj::log(t);}
        }catch(...){nrrj::envenenar("excecao na thread do recorte");}
    }
}
inline void depois_do_publish() noexcept {
    if(!nrrj::ativo())return;
    auto& e=estado();std::lock_guard<std::mutex> lock(e.mutex);
    nrorc::EscopoOrigem eo(nrorc::Origem::Rosto);
    if(nrorc::modo()==1){auto& P=preparo();
        if(!P.feito&&!P.falhou&&P.base&&nrorc::aplicada()&&nrorc::estado().d3d_pronto.load(std::memory_order_acquire)){
            try{Hip hip;P.feito=preparar_orcamento(e,hip,P.base,P.primary);}
            catch(const HipFailure& f){nrorc_vram::log("NPC_ORCAMENTO_FALHOU hip fase=%s status=%d (preparo)",f.phase,f.status);}
            catch(const std::exception& x){nrorc_vram::log("NPC_ORCAMENTO_FALHOU %s (preparo)",x.what());}
            catch(...){nrorc_vram::log("NPC_ORCAMENTO_FALHOU excecao no preparo");}
            if(!P.feito){P.falhou=true;nrrj::envenenar("orcamento: preparo do lado HIP");}}}
    if(e.grade_lido>=0){auto& S=nrrj::grade().slot[unsigned(e.grade_lido)];e.grade_lido=-1;S.palavra.store(nrrj::palavra_g(nrrj::GLivre,0),std::memory_order_release);}

    if(e.fora_pend.job>0&&nrrj::thread_rede.load(std::memory_order_relaxed)){
        auto& F=fila_rede();
        if(!F.tentada){F.tentada=true;try{std::thread(laco_rede).detach();F.ok=true;}catch(...){F.ok=false;}
            if(!F.ok&&nrrj::sj_q()){nrrj::sem_job_q.store(0);nrrj::log("ROSTO_JOGADOR_SEMJOB_FALHOU thread da rede nao subiu: recorte so no job");}}
        if(F.ok){
            int dev=-1;{using GetDev=int(*)(int*);HMODULE h=GetModuleHandleW(L"amdhip64_7.dll");auto gd=h?reinterpret_cast<GetDev>(GetProcAddress(h,"hipGetDevice")):nullptr;if(!gd||gd(&dev))dev=-1;}
            {std::lock_guard<std::mutex> l(F.m);F.fila.emplace_back(e.fora_pend,dev);F.hs=nrregswin::rjsync::flag_set_n.load(std::memory_order_acquire);}
            e.fora_pend=Estado::Fora{};F.cv.notify_one();
        }
    }

    if(e.fora_pend.job>0){
        const auto f=e.fora_pend;e.fora_pend=Estado::Fora{};

        try{Hip hip;portao_pulo(e,hip,f.base,f.job);}catch(...){e.pula_job=0;}
        if(e.pula_job==f.job){try{Hip hip;avaliar_recorte(e,hip,f.base,f.primary,f.job,f.dest,f.rw,f.rh,f.pitch,f.formato);}catch(...){}
            goto detector;}

        static constexpr unsigned auto_us[5]{0,1500,3000,4500,6000};
        static double auto_ms[5]{};static unsigned auto_n[5]{};static unsigned long long auto_k=0;int auto_i=-1;
        unsigned esp=nrrj::espera_us.load(std::memory_order_relaxed);

        static constexpr unsigned pres_us[5]{0,1000,2000,3000,4000};
        static double pres_ms[5]{};static unsigned pres_n[5]{};static unsigned long long pres_k=0;static double pres_espera=0;int pres_i=-1;
        if(esp==nrrj::espera_present){
            static HANDLE tp=CreateWaitableTimerExW(nullptr,nullptr,0x00000002,TIMER_ALL_ACCESS);
            const auto p0=nrrj::presents_fim.load(std::memory_order_acquire);const auto tw=std::chrono::steady_clock::now();
            for(int it=0;it<160&&nrrj::presents_fim.load(std::memory_order_acquire)==p0;++it){
                LARGE_INTEGER due{};due.QuadPart=-2500;
                if(tp&&SetWaitableTimer(tp,&due,0,nullptr,nullptr,FALSE))WaitForSingleObject(tp,20);else Sleep(1);}
            pres_espera=.9*pres_espera+.1*std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tw).count();
            const unsigned long long k=pres_k++;
            if(k<20)pres_i=int(k%5);else if(k%16==0)pres_i=int((k/16)%5);
            else{pres_i=0;for(int i=1;i<5;++i)if(pres_n[i]&&(pres_n[pres_i]==0||pres_ms[i]<pres_ms[pres_i]))pres_i=i;}
            esp=pres_us[pres_i];
        }

        static unsigned long long r6_sem=0,r6_com=0;bool r6_descarta=false;
        if(nrrj::espera_job.load(std::memory_order_relaxed)&&esp){
            auto& g=nrrj::global();bool tem=false;
            for(unsigned k=0;k<nrrj::slots&&!tem;++k){const auto w=g.slot[k].palavra.load(std::memory_order_acquire);tem=nrrj::estado_de(w)==nrrj::Escrito&&nrrj::job_de(w)==f.job;}
            if(!tem){esp=0;++r6_sem;}
            else{++r6_com;
                if(esp==nrrj::espera_auto){

                    const unsigned long long k=auto_k++;
                    if(k<25)auto_i=int(k%5);else if(k%16==0)auto_i=int((k/16)%5);
                    else{auto_i=0;for(int i=1;i<5;++i)if(auto_n[i]&&(auto_n[auto_i]==0||auto_ms[i]<auto_ms[auto_i]))auto_i=i;}
                    r6_descarta=k<5;esp=auto_us[auto_i];
                }}
            if((r6_sem+r6_com)<=8||((r6_sem+r6_com)&255u)==0){char t[160]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_ESPERA_JOB job=%d com_recorte=%llu sem_recorte=%llu espera_us=%u",f.job,r6_com,r6_sem,esp);nrrj::log(t);}
        }
        if(esp==nrrj::espera_auto){
            const unsigned long long k=auto_k++;
            if(k<20)auto_i=int(k%5);else if(k%16==0)auto_i=int((k/16)%5);
            else{auto_i=0;for(int i=1;i<5;++i)if(auto_n[i]&&(auto_n[auto_i]==0||auto_ms[i]<auto_ms[auto_i]))auto_i=i;}
            esp=auto_us[auto_i];
        }
        if(esp){
            static HANDLE timer=CreateWaitableTimerExW(nullptr,nullptr,0x00000002 ,TIMER_ALL_ACCESS);
            LARGE_INTEGER due{};due.QuadPart=-LONGLONG(esp)*10;
            if(timer&&SetWaitableTimer(timer,&due,0,nullptr,nullptr,FALSE))WaitForSingleObject(timer,esp/1000u+50u);else Sleep((esp+999u)/1000u);
        }
        e.ultimo_ms=-1.0;
        try{Hip hip;avaliar_recorte(e,hip,f.base,f.primary,f.job,f.dest,f.rw,f.rh,f.pitch,f.formato);}
        catch(const HipFailure& x){char t[160]{};NRLOGF(t,sizeof t,"hip fase=%s status=%d (recorte fora)",x.phase,x.status);nrrj::envenenar(t);}
        catch(...){nrrj::envenenar("excecao recorte fora");}
        if(pres_i>=0&&e.ultimo_ms>0){
            pres_ms[pres_i]=pres_n[pres_i]?.75*pres_ms[pres_i]+.25*e.ultimo_ms:e.ultimo_ms;++pres_n[pres_i];
            if(pres_k<=24||(pres_k&63u)==0){char t[240]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_ESPERA_PRESENT k=%llu ate_present=%.2f ms[0/1/2/3/4]=%.2f/%.2f/%.2f/%.2f/%.2f",pres_k,pres_espera,pres_ms[0],pres_ms[1],pres_ms[2],pres_ms[3],pres_ms[4]);nrrj::log(t);}
        }
        if(auto_i>=0&&e.ultimo_ms>0&&!r6_descarta){
            auto_ms[auto_i]=auto_n[auto_i]?.75*auto_ms[auto_i]+.25*e.ultimo_ms:e.ultimo_ms;++auto_n[auto_i];
            if(auto_k<=24||(auto_k&63u)==0){char t[220]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_ESPERA_AUTO k=%llu ms[0/1.5/3/4.5/6]=%.2f/%.2f/%.2f/%.2f/%.2f",auto_k,auto_ms[0],auto_ms[1],auto_ms[2],auto_ms[3],auto_ms[4]);nrrj::log(t);}
        }
    }
detector:
    if(!e.det_pend)return;e.det_pend=false;
    auto& D=det();const auto t=e.det_tarefa;
    if(det_thread.load(std::memory_order_relaxed)){
        if(!D.thread_tentada){D.thread_tentada=true;try{std::thread(laco_detector).detach();D.thread_ok=true;nrrj::log("ROSTO_JOGADOR_DETECTOR thread propria");}catch(...){D.thread_ok=false;}}
        if(D.thread_ok){
            D.sets[t.set].ocupado.store(true,std::memory_order_release);
            {std::lock_guard<std::mutex> l(D.m);
             if(D.tem&&D.fila.set!=t.set){D.sets[D.fila.set].ocupado.store(false);D.descartados.fetch_add(1);}
             D.fila=t;D.tem=true;}
            D.cv.notify_one();return;
        }
    }
    try{Hip hip;executar_deteccao(D,t,[&](void* dst,const void* src,size_t n){return hip.copy(dst,src,n,2)==0;});}
    catch(const HipFailure& f){D.sets[t.set].ocupado.store(false);char l[160]{};NRLOGF(l,sizeof l,"hip fase=%s status=%d (detector)",f.phase,f.status);nrrj::envenenar(l);}
    catch(...){D.sets[t.set].ocupado.store(false);nrrj::envenenar("excecao detector");}
}
}
