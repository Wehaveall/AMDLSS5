#pragma once

#include <array>
#include "rosto_jogador_state.h"
#include "rosto_jogador_detecta.h"
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
inline Estado& estado(){static Estado* e=new Estado;return *e;}
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
        nrrj::recorte_fixo_ini.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRecorteFixo",0,ini.c_str())),0,2048)&~127);
        nrrj::recorte_forcado.store((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorRecorteForcado",0,ini.c_str())),0,2048));
        nrrj::zoom.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorZoom",0,ini.c_str())),0,31)));
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
         nrrj::segura.store(unsigned((std::clamp)(int(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorSegura",0,ini.c_str())),0,64)));
         nrrj::segura_pixel.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorSeguraPixel",0,ini.c_str())==1);
         nrrj::compor_rapido.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralRostoJogadorComporRapido",0,ini.c_str())==1);

         nrrj::baixa_forca.store(flt(L"NeuralRostoJogadorBaixaForca",1.f,0.f,2.f));
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
         if(nrrj::espera_us.load()){char lc[120]{};NRLOGF(lc,sizeof lc,"ROSTO_JOGADOR_ESPERA %u us depois do publish so_job_com_recorte=%d",nrrj::espera_us.load(),nrrj::espera_job.load()?1:0);nrrj::log(lc);}
         if(nrrj::grade_rede.load()||nrrj::recorte_fator.load()>0.f){char lb[160]{};NRLOGF(lb,sizeof lb,"ROSTO_JOGADOR_RECORTE grade=%u fator=%.3f",nrrj::grade_rede.load(),double(nrrj::recorte_fator.load()));nrrj::log(lb);}nrrj::acum_sigma.store(flt(L"NeuralRostoJogadorAcumSigma",6.f,1.f,32.f));
         if(nrrj::acumula.load()){char la[200]{};NRLOGF(la,sizeof la,"ROSTO_JOGADOR_ACUMULA baixa=%.3f alta=%.3f gama=%.2f sigma=%.1f",double(nrrj::acum_baixa.load()),double(nrrj::acum_alta.load()),double(nrrj::acum_gama.load()),double(nrrj::acum_sigma.load()));nrrj::log(la);}}
        char linha[220]{};NRLOGF(linha,sizeof linha,"ROSTO_JOGADOR_CONFIG ligado=%d teto=%u zoom=%u cada=%u fora=%d det_thread=%d menu=%d",nrrj::ligado.load()?1:0,nrrj::teto.load(),nrrj::zoom.load(),nrrj::cada.load(),nrrj::fora.load()?1:0,det_thread.load()?1:0,nrrj::menu_escreveu.load()?1:0);
        nrrj::log(linha);
    }catch(...){}
}
inline void lancar(Hip& hip,void* k,size_t n,void** args){check(hip.launch(k,unsigned((n+255)/256),1,1,256,1,1,0,nullptr,args,nullptr));}
inline bool modulo(Estado& e,Hip& hip){
    if(e.k_etiqueta)return true;
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
    const size_t n2=size_t(nrrj::n_max)*nrrj::n_max,nl2=n2/16;
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
inline bool importar(Estado& e){auto& g=nrrj::global();return importar_buffer(g.handle.load(std::memory_order_acquire),g.bytes.load(),e.externo,e.base_hip,"recorte");}
inline void* contexto(Estado& e,Hip& hip,char* base,const void* primary){
    auto p=e.ctx.data();
    if(!e.construido){native<void*(*)(void*)>(base,0x1ce20)(p);e.construido=true;}
    if(!e.carregado){
        const auto t=std::chrono::steady_clock::now();
        wchar_t path[32768];const auto length=GetModuleFileNameW(reinterpret_cast<HMODULE>(base),path,32768);
        if(!length||length>=32768)throw std::runtime_error("caminho do proxy");
        std::string pesos=(std::filesystem::path(path).parent_path()/L"dlssnr_on_amd_weights.bin").string();
        const bool ok=native<bool(*)(void*,const std::string*)>(base,0x1e120)(p,&pesos);
        hip.completed("rj_pesos");
        if(!ok||!read<void*>(p,0x10)||!read<void*>(p,0x168)||!read<size_t>(p,0x160))throw std::runtime_error("pesos do recorte indisponiveis");
        e.carregado=true;
        char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_CONTEXTO pesos carregados em %.1f ms",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count());nrrj::log(l);
    }
    std::memcpy(p+0x20,static_cast<const char*>(primary)+0x20,20);

    write<unsigned>(p,0xf0,0);write<unsigned>(p,0x120,0);write<unsigned>(p,0x38,0);write<void*>(p,0x118,nullptr);
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
    unsigned long long deteccoes{},on_jobs{},cortes{},cortes_fsr{};double det_ms{},det_max{};unsigned det_n{};
    std::atomic<unsigned long long> pulados{0},descartados{0};

    bool on_ant{};unsigned long long trocas{},off_seguidos{};unsigned motivos_off{};

    std::atomic<unsigned> sem_cabeca{0};std::atomic<unsigned long long> economizados{0};
};
inline constexpr unsigned economia_apos=240,economia_cada=4;
#pragma warning(pop)
inline Det& det(){static Det* d=new Det;return *d;}

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
    D.rastreio.P.q=(std::max)(1,t.dh/t.gh);
    D.rastreio.P.corte_modo=nrrj::corte_modo.load(std::memory_order_relaxed);
    {const unsigned z=nrrj::zoom.load(std::memory_order_relaxed);
     if(z&1u){D.rastreio.P.cab_max_4k=nrrj::zoom_cab_4k;D.rastreio.P.recorte_max=int(nrrj::zoom_lado);}
     D.rastreio.P.persistente=(z&4u)!=0;if(z&8u)D.rastreio.P.recorte_min=512;
     D.rastreio.P.recorte_forcado=nrrj::recorte_forcado.load(std::memory_order_relaxed);
     if(z&16u){const int tr=int(nrrj::teto.load(std::memory_order_relaxed)),fx=nrrj::recorte_fixo_ini.load(std::memory_order_relaxed);
               (void)tr;D.rastreio.P.recorte_fixo=(std::max)(256,fx>0?fx:1024);}
     D.rastreio.P.cinema_livre=(z&2u)!=0;}

    {const unsigned par=nrredecada::cam::par.load(std::memory_order_relaxed);D.rastreio.P.corte_multi=par>1&&k>=float((std::min)(par,8u));}

    {const auto ger=nrrj::geracao.load(std::memory_order_acquire);if(D.ger_vista&&ger!=D.ger_vista){D.rastreio.reset_corte();D.tem_ant=false;++D.cortes_fsr;}D.ger_vista=ger;}
    const auto R=D.rastreio.passo(gr);
    ++D.deteccoes;if(R.on)++D.on_jobs;
    if(R.tem_est||R.busca)D.sem_cabeca.store(0,std::memory_order_relaxed);else D.sem_cabeca.fetch_add(1,std::memory_order_relaxed);
    if(R.conv_errada){const bool v=!prof_trocada.load();prof_trocada.store(v);
        char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_CONVENCAO profundidade ao contrario (mediana da proximidade %.3f > 0,5): conversao trocada (trocada=%d) a partir do proximo job",double(R.med_prox),v?1:0);nrrj::log(l);}
    if(R.adotou){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ESCALA nova kface=%.0f (hipotese %d; a do RE e 1550): nenhuma cabeca na escala anterior por 60 jobs",double(R.kface),R.hip);nrrj::log(l);}
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
    if(R.corte){++D.cortes;nrrj::cortar();D.ger_vista=nrrj::geracao.load(std::memory_order_acquire);}
    nrrj::Deteccao d{};d.on=R.on;d.tem=R.tem_est&&R.tem_recorte;d.quadro=t.quadro;d.conf=R.conf;d.cab4k=R.cab4k;
    d.motivo=(R.corte?1u:0u)|(R.grande?2u:0u)|(R.cinema?4u:0u)|(R.tem_est?0u:8u);
    if(R.tem_est){d.cx=R.est[0]*esc;d.cy=R.est[1]*esc;d.lado=R.est[2]*esc;
        if(D.tem_ant&&k>0){d.vx=(R.est[0]-D.est_ant[0])*esc/k;d.vy=(R.est[1]-D.est_ant[1])*esc/k;}
        D.est_ant[0]=R.est[0];D.est_ant[1]=R.est[1];D.est_ant[2]=R.est[2];D.tem_ant=true;}
    else D.tem_ant=false;
    if(R.tem_recorte){d.rl=unsigned(R.recorte[2]);d.rx=int(std::lround(R.recorte[0]*esc-d.rl*.5f));d.ry=int(std::lround(R.recorte[1]*esc-d.rl*.5f));}
    nrrj::publicar(d);
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
    if(D.sem_cabeca.load(std::memory_order_relaxed)>economia_apos&&(unsigned(job)%economia_cada)!=0u){D.economizados.fetch_add(1,std::memory_order_relaxed);return;}
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

inline void avaliar_recorte(Estado& e,Hip& hip,char* base,void* primary,int job,void* dest,int rw,int rh,int pitch,int formato){
    auto& g=nrrj::global();
    int s=-1;
    for(unsigned k=0;k<nrrj::slots;++k){auto w=g.slot[k].palavra.load(std::memory_order_acquire);
        if(nrrj::estado_de(w)==nrrj::Escrito&&nrrj::job_de(w)==job){
            if(g.marcado(k)!=g.slot[k].seq){g.slot[k].palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);g.pulados_marcador.fetch_add(1);continue;}
            if(g.slot[k].palavra.compare_exchange_strong(w,nrrj::palavra(nrrj::Avaliando,job),std::memory_order_acq_rel))s=int(k);break;}}
    if(s<0)return;
    auto& S=g.slot[unsigned(s)];
    try{
        if(nrneuraltone::requested&&!nrneuraltone::multipass_ok)throw std::runtime_error("ToneCurve ACES ou ToneLift > 0: dominio da rede diferente do da composicao");
        if(formato!=0||!dest||rw<64||rh<64||pitch<rw*8)throw std::runtime_error("residuo da cena fora do contrato (half RGBA)");
        if(!importar(e)||!grades(e,hip)){S.palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);return;}
        const unsigned n=S.n,L=S.L,ow=S.ow,oh=S.oh;
        const bool amplia=(nrrj::zoom.load(std::memory_order_relaxed)&8u)&&n<=2u*L;
        if(n<nrrj::n_min||n>nrrj::n_max||(n&127u)||(L<n&&!amplia)||!ow||!oh)throw std::runtime_error("recorte fora do contrato");
        Tempos tm3;tm3.marca(hip);
        const auto t0=std::chrono::steady_clock::now();
        const float expo=[&]{const float v=read<float>(base,0x90CC0+4*(unsigned(job)&3u));return std::isfinite(v)&&v>0.f?v:1.f;}();
        const unsigned tonemap=read<unsigned>(base,0x94698)!=0?1u:0u;
        float passo=float(L)/float(n),bx=float(S.x),by=float(S.y),fow=float(ow),foh=float(oh);
        char* ent=e.base_hip+nrrj::offset_ent(unsigned(s));char* sai=e.base_hip+nrrj::offset_sai(unsigned(s));
        unsigned urw=unsigned(rw),urh=unsigned(rh),upitch=unsigned(pitch/2),un=n;
        const bool despeja=despejo_n()&&e.avaliados+1==despejo_n();
        std::filesystem::path pasta;if(despeja){pasta=pasta_helper()/L"nr_rosto_jogador_despejo";std::error_code ec;std::filesystem::create_directories(pasta,ec);}
        if(despeja){hip.completed("rj_despejo");despejar(hip,ent,size_t(n)*n*8,pasta/L"recorte_linear.raw");}
        {void* a[]={&dest,&urw,&urh,&upitch,&bx,&by,&passo,&fow,&foh,&un,&e.dc};lancar(hip,e.k_cena,size_t(n)*n,a);}
        {float ex=expo;unsigned tm=tonemap;void* a[]={&ent,&un,&ex,&tm};lancar(hip,e.k_entrada,size_t(n)*n,a);}

        if(e.pula_job==job){e.pula_job=0;S.palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);return;}

        bool fres=nrrj::frescor.load(std::memory_order_relaxed)>0.f&&nrrj::acumula.load(std::memory_order_relaxed)&&e.k_lumin&&e.k_bandas2&&e.k_meia4;
        if(fres&&!e.lin&&!e.lin_tentado){e.lin_tentado=true;if(hip.allocate(&e.lin,size_t(nrrj::n_max)*nrrj::n_max*4)){(void)hip.error();e.lin=nullptr;}}
        fres=fres&&e.lin;
        if(fres){void* a[]={&ent,&un,&e.lin};lancar(hip,e.k_lumin,size_t(n)*n,a);}
        tm3.marca(hip);
        void* p=contexto(e,hip,base,primary);
        {const float f1=nrrj::passo1_forca.load(std::memory_order_relaxed);if(f1!=1.f)write<float>(p,0x30,read<float>(p,0x30)*f1);}
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
                for(unsigned nn:{256u,384u,512u,640u,768u,896u,1024u}){
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
            if(!e.in2&&!e.buf2_tentado){e.buf2_tentado=true;const size_t b=size_t(nrrj::n_max)*nrrj::n_max*8;
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
        {float sl=16.f*cel/float(f);unsigned rl=raio_cv(sl),h1=1,h0=0,ref=1;
         void* a1[]={&e.lo,&e.lo2,&nl,&nl,&sl,&rl,&h1,&ref};lancar(hip,e.k_blur4,size_t(nl)*nl,a1);
         void* a2[]={&e.lo2,&e.lo,&nl,&nl,&sl,&rl,&h0,&ref};lancar(hip,e.k_blur4,size_t(nl)*nl,a2);}
        ElipseK el{(S.hx-bx)/passo,(S.hy-by)/passo,S.hl/passo,.50f,.60f,.85f,1.15f,.55f,.85f};
        unsigned dep=0;

        bool camada=false;
        if(const unsigned cmodo=nrrj::borda_camada.load(std::memory_order_relaxed);cmodo&&e.k_camada&&e.k_baixa_m&&e.k_blur4w&&e.k_delta_m){
            const auto& T=e.det_tarefa;auto& DS=det().sets[unsigned(T.set>=0?T.set:0)];
            if(T.job==job&&T.set>=0&&DS.d&&T.dw>=16&&T.dh>=16&&size_t(T.dw)*T.dh<=DS.cap){
                float kk=float(T.dw)/fow,hxt=S.hx*kk,hyt=S.hy*kk,c0=nrrj::borda_camada_z0.load(std::memory_order_relaxed),c1=nrrj::borda_camada_z1.load(std::memory_order_relaxed);int idw=T.dw,idh=T.dh;unsigned dv=e.det_dinv?1u:0u,perto=nrrj::borda_camada_perto.load(std::memory_order_relaxed)?1u:0u;
                {void* a[]={&DS.d,&idw,&idh,&dv,&hxt,&hyt,&bx,&by,&passo,&kk,&un,&c0,&c1,&perto,&e.tmp1};lancar(hip,e.k_camada,size_t(n)*n,a);}
                {void* a[]={&e.d0,&e.tmp1,&un,&f,&nl,&e.lo,&e.gl};lancar(hip,e.k_baixa_m,size_t(nl)*nl,a);}
                {float sl=16.f*cel/float(f);unsigned rl=raio_cv(sl),h1=1,h0=0,ref=1;
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

        if(nrrj::acumula.load(std::memory_order_relaxed)&&e.k_meia4&&e.k_bandas){
            {void* a[]={&ent,&un,&e.dc};lancar(hip,e.k_meia4,size_t(n)*n,a);}
            float sb=nrrj::acum_sigma.load(std::memory_order_relaxed)*cel;unsigned rb=raio_cv(sb),h1=1,h0=0,ref=1;
            {void* a1[]={&e.dc,&e.d0,&un,&un,&sb,&rb,&h1,&ref};lancar(hip,e.k_blur4,size_t(n)*n,a1);
             void* a2[]={&e.d0,&e.dc,&un,&un,&sb,&rb,&h0,&ref};lancar(hip,e.k_blur4,size_t(n)*n,a2);}
            if(fres){void* a[]={&ent,&e.dc,&un,&sai,&e.lin};lancar(hip,e.k_bandas2,size_t(n)*n,a);nrrj::frescor_pronto.store(true,std::memory_order_release);}
            else{void* a[]={&ent,&e.dc,&un,&sai};lancar(hip,e.k_bandas,size_t(n)*n,a);}
        }
        {void* tag=e.base_hip+nrrj::offset_tag(unsigned(s));unsigned uj=unsigned(job);void* a[]={&tag,&uj};check(hip.launch(e.k_etiqueta,1,1,1,1,1,1,0,nullptr,a,nullptr));}
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
        S.palavra.store(nrrj::palavra(nrrj::Pronto,job),std::memory_order_release);g.prontos.fetch_add(1);
        const auto t1=std::chrono::steady_clock::now();
        const double ms=std::chrono::duration<double,std::milli>(t1-t0).count(),ms_rede=std::chrono::duration<double,std::milli>(t_rede-t0).count();
        e.ms_soma+=ms;++e.ms_n;e.ms_max=(std::max)(e.ms_max,ms);e.ultimo_ms=ms;
        ++e.avaliados;g.avaliacoes.fetch_add(1);
        if(e.avaliados<=6||mede){
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
inline void antes_do_publish(char* base,void* primary,int job,void* dest,int rw,int rh,int pitch,int formato) noexcept {
    if(job<=0)return;
    ler_ini(base);
    if(!nrrj::ativo())return;
    auto& e=estado();std::lock_guard<std::mutex> lock(e.mutex);
    e.fora_pend=Estado::Fora{};
    try{
        Hip hip;
        if(!modulo(e,hip))return;
        ++e.jobs;

        if(!e.carregado)(void)contexto(e,hip,base,primary);
        preparar_deteccao(e,hip,base,job);

        if(nrrj::fora.load(std::memory_order_relaxed))e.fora_pend=Estado::Fora{job,base,primary,dest,rw,rh,pitch,formato};
        else avaliar_recorte(e,hip,base,primary,job,dest,rw,rh,pitch,formato);
    }catch(const HipFailure& f){char t[160]{};NRLOGF(t,sizeof t,"hip fase=%s status=%d (preparo)",f.phase,f.status);nrrj::envenenar(t);}
    catch(const std::exception& x){nrrj::envenenar(x.what());}
    catch(...){nrrj::envenenar("excecao preparo");}
}

struct FilaRede {std::mutex m;std::condition_variable cv;std::deque<std::pair<Estado::Fora,int>> fila;unsigned long long hs{};bool tentada{},ok{};
    unsigned long long feitos{},sem_recorte{},hs_espera_ok{},hs_espera_falhou{};};
inline FilaRede& fila_rede(){static FilaRede* f=new FilaRede;return *f;}
inline void laco_rede() noexcept {
    auto& F=fila_rede();auto& e=estado();
    using SetDev=int(*)(int);HMODULE h=GetModuleHandleW(L"amdhip64_7.dll");auto setdev=h?reinterpret_cast<SetDev>(GetProcAddress(h,"hipSetDevice")):nullptr;
    static HANDLE timer=CreateWaitableTimerExW(nullptr,nullptr,0x00000002 ,TIMER_ALL_ACCESS);
    const auto dorme_us=[](unsigned us){if(!us)return;LARGE_INTEGER due{};due.QuadPart=-LONGLONG(us)*10;
        if(timer&&SetWaitableTimer(timer,&due,0,nullptr,nullptr,FALSE))WaitForSingleObject(timer,us/1000u+50u);else Sleep((us+999u)/1000u);};
    static constexpr unsigned auto_us[5]{0,1500,3000,4500,6000};
    double auto_ms[5]{};unsigned auto_n[5]{};unsigned long long auto_k=0;int dev_atual=-1;
    for(;;){
        std::pair<Estado::Fora,int> item{};unsigned long long hs=0;
        {std::unique_lock<std::mutex> l(F.m);F.cv.wait(l,[&]{return !F.fila.empty();});item=F.fila.front();F.fila.pop_front();hs=F.hs;}
        const auto f=item.first;
        try{
            if(setdev&&item.second>=0&&item.second!=dev_atual){(void)setdev(item.second);dev_atual=item.second;}

            if(nrregswin::rjsync::flag_set_n.load(std::memory_order_acquire)>hs)++F.hs_espera_ok;else ++F.hs_espera_falhou;
            bool tem=false,pula=false;int auto_i=-1;unsigned esp=0;

            {std::lock_guard<std::recursive_mutex> ev(nrregswin::rjsync::trava());std::lock_guard<std::mutex> lock(e.mutex);
                try{Hip hip;portao_pulo(e,hip,f.base,f.job);}catch(...){e.pula_job=0;}
                pula=e.pula_job==f.job;
                if(pula){try{Hip hip;avaliar_recorte(e,hip,f.base,f.primary,f.job,f.dest,f.rw,f.rh,f.pitch,f.formato);}catch(...){}}
                auto& g=nrrj::global();
                for(unsigned k=0;k<nrrj::slots&&!tem;++k){const auto w=g.slot[k].palavra.load(std::memory_order_acquire);tem=nrrj::estado_de(w)==nrrj::Escrito&&nrrj::job_de(w)==f.job;}
            }
            if(pula)continue;
            if(!tem){++F.sem_recorte;
                std::lock_guard<std::recursive_mutex> ev(nrregswin::rjsync::trava());std::lock_guard<std::mutex> lock(e.mutex);
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
            {std::lock_guard<std::recursive_mutex> ev(nrregswin::rjsync::trava());std::lock_guard<std::mutex> lock(e.mutex);
                e.ultimo_ms=-1.0;
                try{Hip hip;avaliar_recorte(e,hip,f.base,f.primary,f.job,f.dest,f.rw,f.rh,f.pitch,f.formato);}
                catch(const HipFailure& x){char t[160]{};NRLOGF(t,sizeof t,"hip fase=%s status=%d (recorte thread)",x.phase,x.status);nrrj::envenenar(t);}
                catch(...){nrrj::envenenar("excecao recorte thread");}
                ms=e.ultimo_ms;}
            ++F.feitos;
            if(auto_i>=0&&ms>0){auto_ms[auto_i]=auto_n[auto_i]?.75*auto_ms[auto_i]+.25*ms:ms;++auto_n[auto_i];}
            if(F.feitos<=8||(F.feitos&31u)==0){char t[260]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_THREAD_REDE feitos=%llu sem_recorte=%llu hs_ok=%llu hs_falhou=%llu espera_us=%u ms=%.2f auto[0/1.5/3/4.5/6]=%.2f/%.2f/%.2f/%.2f/%.2f",
                F.feitos,F.sem_recorte,F.hs_espera_ok,F.hs_espera_falhou,esp,ms,auto_ms[0],auto_ms[1],auto_ms[2],auto_ms[3],auto_ms[4]);nrrj::log(t);}
        }catch(...){nrrj::envenenar("excecao na thread do recorte");}
    }
}
inline void depois_do_publish() noexcept {
    if(!nrrj::ativo())return;
    auto& e=estado();std::lock_guard<std::mutex> lock(e.mutex);
    if(e.grade_lido>=0){auto& S=nrrj::grade().slot[unsigned(e.grade_lido)];e.grade_lido=-1;S.palavra.store(nrrj::palavra_g(nrrj::GLivre,0),std::memory_order_release);}

    if(e.fora_pend.job>0&&nrrj::thread_rede.load(std::memory_order_relaxed)){
        auto& F=fila_rede();
        if(!F.tentada){F.tentada=true;try{std::thread(laco_rede).detach();F.ok=true;}catch(...){F.ok=false;}}
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
