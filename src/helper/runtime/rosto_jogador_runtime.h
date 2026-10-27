#pragma once

#include <array>
#include "rosto_jogador_state.h"
#include "rosto_jogador_detecta.h"
#include "rosto_jogador_ini.h"
#include "jogador_externo.h"
#include "rosto_cor_luz.h"
static_assert(nrrj::npc_slots==nrrj::npc_det_max,"npc-n: NPCs do estado = do detector");
#include "scrfd.h"
namespace nrrj_runtime {
struct ExternalDesc {int type{};union Handle {int fd;struct {void* handle;const void* name;} win32;const void* nvSci;Handle():win32{}{}} handle;
                     unsigned long long size{};unsigned flags{};unsigned reserved[16]{};};
struct BufferDesc {unsigned long long offset{},size{};unsigned flags{},reserved[16]{};};
static_assert(sizeof(ExternalDesc)==104&&sizeof(BufferDesc)==88);
struct ElipseK {float hx,hy,hl,ex,ey,r0,r1,m0,m1;};
static_assert(sizeof(ElipseK)==36);
struct DetSet {void* dq{};void* mvq{};void* d{};size_t cap{};std::atomic<bool> ocupado{false};size_t gcap{};void* amostra{};bool amostra_tentada{};nrrj::DetAlocPausa pausa{};};
inline constexpr size_t det_amostra_max=8192;
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
    void* k_ema{};void* ema{};bool ema_tentado{};nrrj::EntradaMediaReg ema_reg{};unsigned long long ema_usos{},ema_novos{};
    void* k_f4meia{};void* esg{};size_t esg_px{};bool esg_falhou{};unsigned long long esg_n{};
    void* k_delta_el{};void* k_guarda_el{};unsigned long long di_n{};
    void* k_camada{};void* k_baixa_m{};void* k_blur4w{};void* k_delta_m{};int det_dinv{};unsigned long long camada_n{},camada_sem{};
    void* k_rpeso{};void* k_rmedia{};void* k_rbanda{};void* rw{};void* rsoma{};bool r_tentado{};unsigned long long r_cortes{},r_sem_peso{};
    void* k_lgrad{};void* k_lpeso{};void* k_limpa{};void* lsoma{};bool l_tentado{};unsigned long long l_n{},l_rt_desligado{};
    void* k_cl_cena{};void* k_cl_baixa{};void* k_cl_aplica{};unsigned long long cl_n{};

    void* mini{};

    void* in2{};void* out2{};bool buf2_tentado{};unsigned long long passos2{},passos2_pulados{};double p1_min{},p1_ema{};unsigned p1_n{};unsigned long long p2_k{};
    alignas(64) std::array<unsigned char,0x400> ctx{};bool construido{},carregado{};
    void* externo{};char* base_hip{};

    void* externo_grade{};char* grade_hip{};int grade_lido{-1};int fonte_ant{};unsigned proxy_ok_n{},falta_n{};unsigned long long sem_dados{};

    void* dc{};void* d0{};void* D{};void* lo{};void* lo2{};void* luma{};void* gl{};void* tmp1{};void* soma{};bool grades{};

    bool det_pend{};DetTarefa det_tarefa{};

    struct Fora {int job{};char* base{};void* primary{};void* dest{};int rw{},rh{},pitch{},formato{};};Fora fora_pend{};
    unsigned long long jobs{},avaliados{};double ms_soma{},ms_max{};unsigned ms_n{};double ultimo_ms{-1.0};
    std::vector<unsigned char> modulo_bytes;
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
    e.k_rpeso=e0.k_rpeso;e.k_rmedia=e0.k_rmedia;e.k_rbanda=e0.k_rbanda;
    e.k_lgrad=e0.k_lgrad;e.k_lpeso=e0.k_lpeso;e.k_limpa=e0.k_limpa;
    e.k_cl_cena=e0.k_cl_cena;e.k_cl_baixa=e0.k_cl_baixa;e.k_cl_aplica=e0.k_cl_aplica;
    e.k_f4meia=e0.k_f4meia;
    e.k_delta_el=e0.k_delta_el;e.k_guarda_el=e0.k_guarda_el;
    e.k_etiqueta=e0.k_etiqueta;}
using nrrj::det_thread;

using nrrj::passo2_folga;
inline std::filesystem::path pasta_helper(){return nrpasta::helper();}

inline void ler_ini(char* base) noexcept {
    if(nrrj::ini_lido.load(std::memory_order_acquire))return;
    auto& e=estado();std::lock_guard<std::mutex> l(e.mutex);if(nrrj::ini_lido.load())return;
    try{
        const std::wstring ini=nrinipath::resolve(base,reinterpret_cast<const void*>(&estado));
        if(ini.empty())return;
        nrrj::ini_lido.store(true,std::memory_order_release);
        nrrj::IniGanchos g{};g.ctx=base;
        g.orcamento=[](void* b,int om,unsigned folga,unsigned outros){nrorc_vram::configurar(om,folga,outros);if(nrorc::modo()){nrregswin::orc_contar(static_cast<char*>(b));nrorc_vram::hip_inicio();}};
        g.rede_sync=[](void*,bool v){nrregswin::rjsync::ligado.store(v);};
        nrrj::ler_ini_rosto(ini,g);
        nrrjcl::ler(ini.c_str());

        if(!nrrj::detector_cpu_ok(nrdet::cpu_simd()))nrrj::desligar_rostos("CPU sem AVX2+FMA: o detector de rosto (SCRFD) exige AVX2 e FMA (Intel 4a geracao / AMD Ryzen em diante)");
    }catch(...){}
}
inline void lancar(Hip& hip,void* k,size_t n,void** args){check(hip.launch(k,unsigned((n+255)/256),1,1,256,1,1,0,nullptr,args,nullptr));}

inline int aloca(Hip& hip,void** p,size_t n,const char* onde) noexcept {const int r=hip.allocate(p,n);if(r){(void)nrgpu::hip(r,onde);(void)hip.error();*p=nullptr;}return r;}

inline int aloca_par(Hip& hip,void** a,size_t na,void** b,size_t nb,const char* onde) noexcept {
    if(const int r=aloca(hip,a,na,onde))return r;
    if(const int r=aloca(hip,b,nb,onde)){(void)hip.release(*a);*a=nullptr;return r;}
    return 0;}

inline bool hip_ok(int r,const char* onde) noexcept {if(!r)return true;(void)nrgpu::hip(r,onde);return false;}

inline float expo_do_job(char* base,int job) noexcept {const float v=read<float>(base,nrproxy::rva_expo_job+4*(unsigned(job)&3u));return std::isfinite(v)&&v>0.f?v:1.f;}
inline unsigned tonemap_do_proxy(char* base) noexcept {return read<unsigned>(base,nrproxy::rva_tom_ef)!=0?1u:0u;}
inline bool modulo(Estado& e,Hip& hip){
    if(e.k_etiqueta)return true;
    if(nrorc::aplicada()&&nrrj::inst_atual()>=1){Estado& e0=estado_de(0);{nrrj::Inst i0(0);if(!modulo(e0,hip))return false;}compartilhar(e,e0);return true;}
    if(e.modulo_tentado)return false;
    e.modulo_tentado=true;

    const auto leitura=nrmodpin::ler(pasta_helper()/L"nr_rosto.hsaco",nullptr,nullptr,e.modulo_bytes);
    if(leitura==nrmodpin::Leitura::ausente){nrrj::envenenar("nr_rosto.hsaco_ausente");return false;}
    if(leitura!=nrmodpin::Leitura::ok||!hip.load_data||hip.load_data(&e.module,e.modulo_bytes.data())||!e.module){(void)hip.error();e.module=nullptr;nrrj::envenenar("nr_rosto.hsaco_recusado");return false;}
    const char* nomes[]={"rj_reduz_v1","rj_cena_v1","rj_entrada_v1","rj_dif_v1","rj_baixa_v1","rj_blur4_v1","rj_blur1_v1","rj_delta_v1","rj_guarda_v1","rj_mede_v1","rj_etiqueta_v1"};
    void** alvos[]={&e.k_reduz,&e.k_cena,&e.k_entrada,&e.k_dif,&e.k_baixa,&e.k_blur4,&e.k_blur1,&e.k_delta,&e.k_guarda,&e.k_mede,&e.k_etiqueta};
    for(int i=0;i<11;++i)if(hip.get_kernel(alvos[i],e.module,nomes[i])){(void)hip.error();e.k_etiqueta=nullptr;nrrj::envenenar("simbolos_v1_ausentes");return false;}
    if(hip.get_kernel(&e.k_meia4,e.module,"rj_meia4_v1")||hip.get_kernel(&e.k_bandas,e.module,"rj_bandas_v1")){(void)hip.error();e.k_meia4=e.k_bandas=nullptr;}
    if(hip.get_kernel(&e.k_mini,e.module,"rj_mini_v1")||hip.get_kernel(&e.k_sat,e.module,"rj_sat_v1")){(void)hip.error();e.k_mini=e.k_sat=nullptr;}
    if(hip.get_kernel(&e.k_lumin,e.module,"rj_lumin_v1")||hip.get_kernel(&e.k_bandas2,e.module,"rj_bandas2_v1")){(void)hip.error();e.k_lumin=e.k_bandas2=nullptr;}
    if(hip.get_kernel(&e.k_ema,e.module,"rj_entrada_media_v1")){(void)hip.error();e.k_ema=nullptr;if(nrrj::entrada_media.load()>0.f)nrrj::log("ROSTO_JOGADOR_ENTRADA_MEDIA simbolo ausente no nr_rosto.hsaco: desligada");}
    if(hip.get_kernel(&e.k_f4meia,e.module,"rj_f4_meia_v1")){(void)hip.error();e.k_f4meia=nullptr;if(nrrj::entrada_sigma.load()>0.f)nrrj::log("ROSTO_JOGADOR_ENTRADA_SIGMA simbolo ausente no nr_rosto.hsaco: desligada");}
    if(hip.get_kernel(&e.k_delta_el,e.module,"rj_delta_el_v1")||hip.get_kernel(&e.k_guarda_el,e.module,"rj_guarda_el_v1")){(void)hip.error();e.k_delta_el=e.k_guarda_el=nullptr;if(nrrj::delta_inteiro())nrrj::log("ROSTO_JOGADOR_DELTA_INTEIRO simbolos rj_delta_el_v1/rj_guarda_el_v1 ausentes no nr_rosto.hsaco: o rosto de hoje (banda fina)");}
    if(hip.get_kernel(&e.k_camada,e.module,"rj_camada_v1")||hip.get_kernel(&e.k_baixa_m,e.module,"rj_baixa_m_v1")||hip.get_kernel(&e.k_blur4w,e.module,"rj_blur4w_v1")||hip.get_kernel(&e.k_delta_m,e.module,"rj_delta_m_v1")){(void)hip.error();e.k_camada=e.k_baixa_m=e.k_blur4w=e.k_delta_m=nullptr;}

    if(nrrj::blur_rapido.load(std::memory_order_relaxed)){void* r4{};void* r1{};void* r4w{};
        if(!hip.get_kernel(&r4,e.module,"rj_blur4r_v1")&&!hip.get_kernel(&r1,e.module,"rj_blur1r_v1")&&!hip.get_kernel(&r4w,e.module,"rj_blur4wr_v1")){
            e.k_blur4=r4;e.k_blur1=r1;if(e.k_blur4w)e.k_blur4w=r4w;nrrj::log("ROSTO_PERF_BLUR rapido ligado (rj_blur4r/blur1r/blur4wr)");}
        else{(void)hip.error();nrrj::blur_rapido.store(false);nrrj::log("ROSTO_PERF_BLUR simbolos ausentes no nr_rosto.hsaco: blurs de hoje");}}

    if(hip.get_kernel(&e.k_rpeso,e.module,"rj_ruido_peso_v1")||hip.get_kernel(&e.k_rmedia,e.module,"rj_ruido_media_v1")||hip.get_kernel(&e.k_rbanda,e.module,"rj_ruido_banda_v1")){
        (void)hip.error();e.k_rpeso=e.k_rmedia=e.k_rbanda=nullptr;if(nrrj::ruido_rt.load())nrrj::log("ROSTO_RUIDO simbolos rj_ruido_*_v1 ausentes no nr_rosto.hsaco: corte da banda desligado");}

    if(hip.get_kernel(&e.k_lgrad,e.module,"rj_limpa_grad_v1")||hip.get_kernel(&e.k_lpeso,e.module,"rj_limpa_peso_v1")||hip.get_kernel(&e.k_limpa,e.module,"rj_limpa_v1")){
        (void)hip.error();e.k_lgrad=e.k_lpeso=e.k_limpa=nullptr;if(nrrj::ruido_limpa.load()>0.f)nrrj::log("ROSTO_RUIDO_LIMPA simbolos rj_limpa_*_v1 ausentes no nr_rosto.hsaco: limpeza desligada");}

    if(hip.get_kernel(&e.k_cl_cena,e.module,"rj_cl_cena_v1")||hip.get_kernel(&e.k_cl_baixa,e.module,"rj_cl_baixa_v1")||hip.get_kernel(&e.k_cl_aplica,e.module,"rj_cl_aplica_v1")){
        (void)hip.error();e.k_cl_cena=e.k_cl_baixa=e.k_cl_aplica=nullptr;if(nrrjcl::ativo())nrrj::log("ROSTO_COR simbolos rj_cl_*_v1 ausentes no nr_rosto.hsaco: NeuralRostoCor/Luz sem efeito");}
    nrrj::log("ROSTO_JOGADOR_MODULO nr_rosto.hsaco carregado");
    return true;
}
inline bool grades(Estado& e,Hip& hip){
    if(e.grades)return true;
    if(nrorc::aplicada()&&nrrj::inst_atual()>=1){compartilhar(e,estado_de(0));return e.grades;}
    const size_t n2=size_t(nrrj::n_cap())*nrrj::n_cap(),nl2=n2/16;
    void** f4[]={&e.dc,&e.d0,&e.D};void** fl[]={&e.lo,&e.lo2};void** f1[]={&e.luma,&e.gl,&e.tmp1};
    bool ok=true;
    for(auto p:f4)ok=ok&&!aloca(hip,p,n2*16,"grades do rosto");
    for(auto p:fl)ok=ok&&!aloca(hip,p,nl2*16,"grades do rosto");
    for(auto p:f1)ok=ok&&!aloca(hip,p,n2*4,"grades do rosto");
    ok=ok&&!aloca(hip,&e.soma,16,"grades do rosto");

    if(!ok){(void)hip.error();for(void** p:{&e.dc,&e.d0,&e.D,&e.lo,&e.lo2,&e.luma,&e.gl,&e.tmp1,&e.soma})if(*p){(void)hip.release(*p);*p=nullptr;}
        nrrj::passageira(nrrj::Falha::Recorte,"hipMalloc grades");return false;}
    e.grades=true;
    char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_GRADES %.1f MB",double(n2*(48+12)+nl2*32+16)/1048576.0);nrrj::log(l);
    return true;
}
inline bool importar_buffer(void* h,std::uint64_t bytes,void*& externo,char*& base_hip,const char* nome){
    if(base_hip)return true;
    if(!h)return false;
    const auto a=nrhip::api_rosto();if(!a.modulo)throw std::runtime_error("HIP ausente");
    if(!a.import_external_memory||!a.external_memory_mapped_buffer)throw std::runtime_error("hipImportExternalMemory ausente");
    ExternalDesc d{};d.type=5;d.handle.win32.handle=h;d.size=bytes;d.flags=1;
    if(const int r=a.import_external_memory(&externo,&d)){char t[96]{};NRLOGF(t,sizeof t,"hipImportExternalMemory=%d",r);throw std::runtime_error(t);}
    BufferDesc b{};b.size=bytes;void* p{};
    if(const int r=a.external_memory_mapped_buffer(&p,externo,&b)){char t[96]{};NRLOGF(t,sizeof t,"hipExternalMemoryGetMappedBuffer=%d",r);throw std::runtime_error(t);}
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

inline std::string caminho_estreito(const std::filesystem::path& p){
    std::string s=nrcaminho::estreito(p);
    if(!s.empty())return s;
    throw nrrj::FalhaPermanente("pesos do recorte: caminho fora da pagina de codigo");
}

inline std::string pesos_rosto(char* base){
    return caminho_estreito(pasta_proxy(base)/(hibrido_rosto(base)?L"dlssnr_on_amd_weights_legado.bin":L"dlssnr_on_amd_weights.bin"));}
inline void marcar_rosto(unsigned char* p,char* base){if(hibrido_rosto(base))write<std::uint64_t>(p,0x3f8,marca_rosto_legado);}
inline bool carregar_pesos(Hip& hip,char* base,unsigned char* p,bool& construido){
    if(!construido){native<void*(*)(void*)>(base,0x1ce20)(p);construido=true;}
    const auto t=std::chrono::steady_clock::now();
    std::string pesos=pesos_rosto(base);
    const bool ok=native<bool(*)(void*,const std::string*)>(base,0x1e120)(p,&pesos);
    hip.completed("rj_pesos");
    const bool bom=ok&&read<void*>(p,0x10)&&read<void*>(p,0x168)&&read<size_t>(p,0x160);
    char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_CONTEXTO pesos %s em %.1f ms",bom?"carregados":"FALHARAM",
        std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count());nrrj::log(l);
    return bom;}

inline void preparar_contexto(unsigned char* p,const void* primary) noexcept {
    std::memcpy(p+0x20,static_cast<const char*>(primary)+0x20,20);
    write<unsigned>(p,0xf0,0);write<unsigned>(p,0x120,0);write<unsigned>(p,0x38,0);write<void*>(p,0x118,nullptr);}
inline void* contexto(Estado& e0,Hip& hip,char* base,const void* primary){

    Estado& e=ctx_unico()?estado_de(0):(nrrj::inst_atual()>=2?estado_de(1):e0);
    auto p=e.ctx.data();
    if(!e.carregado){
        if(!carregar_pesos(hip,base,p,e.construido))throw nrrj::FalhaPermanente("pesos do recorte indisponiveis");
        e.carregado=true;
    }
    preparar_contexto(p,primary);
    write<std::uint64_t>(p,0x3f8,0);marcar_rosto(p,base);
    return p;
}

inline unsigned raio_cv(float s) noexcept {const long k=std::lround(double(s)*8.0+1.0)|1L;return unsigned(std::clamp<long>(k/2,1,256));}

inline float meia(std::uint16_t h) noexcept {return nrrj::meia_para_float(h);}

#pragma warning(push)
#pragma warning(disable:4324)
struct Det {
    DetSet sets[2];
    std::mutex m;std::condition_variable cv;DetTarefa fila{};bool tem{};bool thread_tentada{},thread_ok{};
    std::vector<float> h_dq,h_mvq,h_d;
    std::vector<float> h_amostra;std::vector<unsigned char> linhas_ok;unsigned long long sob_demanda{},bytes_d{},bytes_d_cheia{};
    std::atomic<unsigned long long> fora_cap{0};
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

inline std::wstring caminho_scrfd() noexcept {
    try{
        std::wstring p(nrpasta::modulo().wstring());if(p.empty())return {};const size_t barra=p.find_last_of(L"\\/");
        return (barra==std::wstring::npos?std::wstring():p.substr(0,barra+1))+L"nr_scrfd.bin";
    }catch(...){return {};}
}

inline nrsc::Rede* scrfd_carregar(bool reservar_grandes,char* erro,size_t ne) noexcept {
    std::snprintf(erro,ne,"caminho do helper");
    const auto p=caminho_scrfd();if(p.empty())return nullptr;
    nrsc::Rede* r=new(std::nothrow) nrsc::Rede;if(!r){std::snprintf(erro,ne,"sem memoria");return nullptr;}
    bool ok=false;try{ok=r->carregar(p.c_str(),erro,ne,reservar_grandes);}catch(...){std::snprintf(erro,ne,"excecao");}
    if(!ok){delete r;return nullptr;}
    const bool forcado=nrrj::detector_avx2.load(std::memory_order_relaxed)&&r->simd()>1&&r->usar_simd(1);
    std::snprintf(erro,ne,"ok, %s%s",nrdet::nome_simd(r->simd()),forcado?" forcado (NeuralRostoJogadorDetectorAvx2)":"");
    return r;}

inline double detectar_imagem(nrsc::Rede& rede,const std::vector<std::uint16_t>& hb,int w,int h,std::vector<std::uint8_t>& bgr,std::vector<nrdet::Rosto>& rs,float limiar,bool faixa){
    const auto t0=std::chrono::steady_clock::now();
    nrdet::tom_bgr_meia(hb.data(),w,h,bgr,meia);
    rede.detectar(bgr.data(),w,h,w*3,rs,limiar,.4f);
    for(auto& r:rs)r.nota=nrrj::nota_scrfd(r.nota,faixa);
    return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();}

inline nrrj::RostoCaixa caixa_saida(const nrdet::Rosto& r,float x0,float y0,float sx,float sy) noexcept {
    nrrj::RostoCaixa b{x0+r.x*sx,y0+r.y*sy,r.w*sx,r.h*sy,r.nota};b.px=r.h;
    for(int i=0;i<5;++i){b.lm[2*i]=x0+r.lm[2*i]*sx;b.lm[2*i+1]=y0+r.lm[2*i+1]*sy;}
    return b;}

inline void laco_rostos() noexcept {
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_LOWEST);
    std::vector<std::uint16_t> hb;std::vector<std::uint8_t> bgr;std::vector<nrdet::Rosto> rs;std::uint32_t ult=0;
    std::uint32_t ult_roi[nrrj::npc_slots]{};double roi_ms_soma=0;unsigned long long roi_n=0;
    nrrj::CrusT<nrrj::RostoCaixa> crus;
    nrsc::Rede* scrfd=nullptr;int scrfd_thr=-1;
    {char erro[160]{};scrfd=scrfd_carregar(true,erro,sizeof erro);
     if(!scrfd){char t[240]{};NRLOGF(t,sizeof t,"detector de rosto SCRFD indisponivel (%s)",erro);nrrj::desligar_rostos(t);return;}
     char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ROSTOS scrfd carregado (nr_scrfd.bin %s)",erro);nrrj::log(l);}

    std::uint64_t vistas=0;
    auto pendente=[&]{
        if(nrrj::rosto_img().base.load(std::memory_order_acquire))
            for(unsigned i=0;i<nrrj::rosto_aneis;++i)if(nrrj::rosto_img().ent[i].seq.load(std::memory_order_acquire)>ult)return true;
        for(int k=0;k<nrrj::npc_slots;++k){auto& RR=nrrj::roi_img(k);if(!RR.base.load(std::memory_order_acquire))continue;
            for(unsigned i=0;i<nrrj::roi_aneis;++i)if(RR.ent[i].seq.load(std::memory_order_acquire)>ult_roi[k])return true;}
        return false;};
    for(;;){
        try{const bool curta=pendente();auto& A=nrrj::sinal_rostos();std::unique_lock<std::mutex> l(A.m);
            if(curta)A.cv.wait_for(l,std::chrono::milliseconds(nrrj::npc2.load(std::memory_order_relaxed)?3:10));
            else A.cv.wait_for(l,std::chrono::milliseconds(250),[&]{return A.escritas!=vistas;});
            vistas=A.escritas;}catch(...){Sleep(10);}
        {const int tn=nrrj::detector_threads.load(std::memory_order_relaxed);if(tn!=scrfd_thr){scrfd_thr=tn;scrfd->usar_threads(tn,THREAD_PRIORITY_LOWEST);
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
                     const double ms=detectar_imagem(*scrfd,hb,n,n,bgr,rs,.3f,false);roi_ms_soma+=ms;++roi_n;
                     const float s=float(L)/float(n);auto& RS=nrrj::rostos_roi(k);
                     {std::lock_guard<std::mutex> l(RS.m);RS.lista.clear();
                         for(const auto& r:rs)RS.lista.push_back(caixa_saida(r,float(x0),float(y0),s,s));
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
            const bool faixa=ry0>0||rh<oh;
            const double ms=detectar_imagem(*scrfd,hb,w,h,bgr,rs,nrrj::scrfd_a(faixa),faixa);
            auto& RS=nrrj::rostos();const float sx=float(rw)/float(w),sy=float(rh)/float(h);
            {std::lock_guard<std::mutex> l(RS.m);RS.lista.clear();
                for(const auto& r:rs)RS.lista.push_back(caixa_saida(r,float(rx0),float(ry0),sx,sy));

                const float reg[4]{float(rx0),float(ry0),float(rx0+rw),float(ry0+rh)},res=float(w)/float((std::max)(rw,1));
                std::uint64_t q_l=quadro;double t_l=t_cap;
                crus.junta_guarda(RS.lista,reg,res,quadro,t_cap,.15,(nrrj::faixa_varre.load(std::memory_order_relaxed)&4u)!=0,.12f*float(oh),q_l,t_l);
                ++RS.versao;RS.quadro=q_l;RS.t=t_l;RS.ms=ms;++RS.rodadas;}
            if(RS.rodadas<=4||RS.rodadas%100u==0){float m=0,nm=0;for(const auto& r:rs)if(r.h*sy>m){m=r.h*sy;nm=r.nota;}
                char l[220]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ROSTOS rodadas=%llu rostos=%zu ms=%.1f img=%dx%d y0=%d maior_altura=%.0f nota=%.2f det=scrfd",RS.rodadas,rs.size(),ms,w,h,ry0,double(m),double(nm));nrrj::log(l);}
        }catch(...){nrrj::log("ROSTO_JOGADOR_ROSTOS excecao na thread do detector de rosto");Sleep(1000);}
    }
}
inline void iniciar_rostos() noexcept {
    static std::atomic<bool> iniciado{false};if(iniciado.exchange(true))return;
    try{std::thread(laco_rostos).detach();nrrj::log("ROSTO_JOGADOR_ROSTOS thread do detector de rosto (SCRFD, CPU, prioridade baixa)");}catch(...){}
}

inline void motivo_texto(unsigned m,char* b,size_t n){
    (void)m;if(b&&n)b[0]=0;
}

inline std::atomic<bool> prof_trocada{false};

template<class Copiar> struct LinhasD {
    Copiar* copiar{};float* h{};const float* src{};int w{},hh{};std::vector<unsigned char>* ok{};bool falhou{};unsigned long long bytes{};
    static bool buscar(void* p,int y0,int y1){
        auto& L=*static_cast<LinhasD*>(p);y0=(std::max)(y0,0);y1=(std::min)(y1,L.hh);
        for(int y=y0;y<y1;){if((*L.ok)[size_t(y)]){++y;continue;}int b=y;while(b<y1&&!(*L.ok)[size_t(b)])++b;
            const size_t off=size_t(y)*size_t(L.w),n=size_t(b-y)*size_t(L.w)*4u;
            if(!(*L.copiar)(L.h+off,L.src+off,n)){L.falhou=true;return false;}
            L.bytes+=n;for(int k=y;k<b;++k)(*L.ok)[size_t(k)]=1;y=b;}
        return true;}
};

template<class Copiar,class Copiar2d> inline void executar_deteccao(Det& D,const DetTarefa& t,Copiar&& copiar,Copiar2d&& copiar2d){
    const auto t0=std::chrono::steady_clock::now();
    auto& S=D.sets[t.set];
    const size_t ng=size_t(t.gw)*t.gh,nd=size_t(t.dw)*t.dh;

    if(ng>S.gcap||nd>S.cap){S.ocupado.store(false,std::memory_order_release);const auto n=D.fora_cap.fetch_add(1)+1;
        if(n<=8||(n&255u)==0){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_DETECTOR_FORA_DA_CAPACIDADE n=%llu grade=%zu/%zu cheia=%zu/%zu job=%d (pulado)",n,ng,S.gcap,nd,S.cap,t.job);nrrj::log(l);}
        return;}
    D.h_dq.resize(ng);D.h_mvq.resize(2*ng);D.h_d.resize(nd);
    bool ok=copiar(D.h_dq.data(),S.dq,ng*4)&&copiar(D.h_mvq.data(),S.mvq,ng*8);
    const size_t passo=nrrj::Rastreio::passo_amostra(nd),na=nd?(nd+passo-1)/passo:0;
    bool sob=ok&&S.amostra&&nd&&na<=det_amostra_max&&copiar2d(S.amostra,4,S.d,passo*4,4,na);
    if(sob){D.h_amostra.resize(na);ok=copiar(D.h_amostra.data(),S.amostra,na*4);if(ok)for(size_t i=0;i<na;++i)D.h_d[i*passo]=D.h_amostra[i];}
    else if(ok)ok=copiar(D.h_d.data(),S.d,nd*4);
    if(!sob)S.ocupado.store(false,std::memory_order_release);
    if(!ok){S.ocupado.store(false,std::memory_order_release);nrrj::passageira(nrrj::Falha::Detector,"copia das grades do detector");return;}
    using CopiarT=std::remove_reference_t<Copiar>;
    LinhasD<CopiarT> L{&copiar,D.h_d.data(),static_cast<const float*>(S.d),t.dw,t.dh,&D.linhas_ok};
    if(sob)D.linhas_ok.assign(size_t(t.dh),0);
    float k=1.f;
    if(t.quadro&&D.quadro_ant&&t.quadro>D.quadro_ant)k=float((std::min<std::uint64_t>)(t.quadro-D.quadro_ant,8u));
    const auto q=nrprescale::unpack_quadro(nrprescale::quadro.load(std::memory_order_acquire));
    const float esc=q.saida_w&&t.dw?float(q.saida_w)/float(t.dw):3.f,saida_h=q.saida_h?float(q.saida_h):2160.f;
    nrrj::Grades gr;gr.W=t.gw;gr.H=t.gh;gr.dq=D.h_dq.data();gr.mvq=D.h_mvq.data();gr.w=t.dw;gr.h=t.dh;gr.d=D.h_d.data();gr.k=k;gr.esc=esc;gr.saida_h=saida_h;
    if(sob){gr.linhas_ctx=&L;gr.linhas=&LinhasD<CopiarT>::buscar;}
    gr.t=std::chrono::duration<double>(t0.time_since_epoch()).count();
    nrrj::params_rastreio(D.rastreio.P,(std::max)(1,t.dh/t.gh),nrorc::aplicada(),nrorc::aplicada()?nrorc::decisao().k_perto:0,nrorc::aplicada()?int(nrorc::decisao().cap_perto):0);
    if(D.rastreio.P.npc2){
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

    {const auto ger=nrrj::ger().load(std::memory_order_acquire);if(D.ger_vista&&ger!=D.ger_vista){D.rastreio.reset_corte();D.tem_ant=false;++D.cortes_fsr;}D.ger_vista=ger;}
    if(nrrj::externo.load(std::memory_order_relaxed)){float eu=0,ev=0,ea=0,ed=0;bool efora=false;
        const bool eok=nrjext::ler(eu,ev,ea,ed,&efora);gr.ext_fora=efora;
        if(eok&&ea>.01f&&ea<.9f&&ed>.3f&&eu>-.1f&&eu<1.1f&&ev>-.1f&&ev<1.1f){const float dy=nrrj::externo_dy.load(std::memory_order_relaxed),es=nrrj::externo_esc.load(std::memory_order_relaxed);
            gr.ext_tem=true;gr.ext[0]=eu*float(gr.w);gr.ext[1]=(ev+dy*ea)*float(gr.h);gr.ext[2]=ea*float(gr.h)*es;}}
    const auto R=D.rastreio.passo(gr);

    if(nrrj::pose_barata.load(std::memory_order_relaxed)){static nrpose::Classificador PC;static std::uint64_t pose_versao=0;
        const bool nova=gr.rostos&&gr.rostos_versao!=pose_versao;if(nova)pose_versao=gr.rostos_versao;
        const double est[3]{double(R.est[0]),double(R.est[1]),double(R.est[2])};float yaw=-1.f;
        const auto a=nrpose::amostrar(nova,R.on&&R.tem_est,gr.ext_tem,gr.rostos,gr.n_rostos,double(gr.esc),est,D.rastreio.P.rosto_baixa,D.rastreio.P.rosto_cab,D.rastreio.P.rosto_dy,yaw);
        if(PC.amostra(a,yaw,nrrj::pose_yaw.load(std::memory_order_relaxed),nrrj::pose_seguidas.load(std::memory_order_relaxed))){
            nrrj::pose_longe.store(PC.longe,std::memory_order_release);
            if(PC.trocas<=64||(PC.trocas&31u)==0){char l[220]{};NRLOGF(l,sizeof l,"ROSTO_POSE_BARATA estado=%s yaw=%.0f motivo=%s seguidas=%u trocas=%llu pulados=%llu job=%d",
                PC.longe?"virada":"frente",double(PC.yaw),nrpose::motivo_texto(PC.motivo),PC.seguidas,PC.trocas,nrrj::sj_conta().pose_pulados.load(std::memory_order_relaxed),t.job);nrrj::log(l);}}}
    if(sob){S.ocupado.store(false,std::memory_order_release);++D.sob_demanda;D.bytes_d+=na*4+L.bytes;D.bytes_d_cheia+=nd*4;
        if(L.falhou)nrrj::passageira(nrrj::Falha::Detector,"copia das linhas da janela do detector");
        if(D.sob_demanda<=4||(D.sob_demanda&1023u)==0){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_DETECTOR_SOB_DEMANDA jobs=%llu copiado_mb=%.1f cheia_mb=%.1f (proximidade: amostra + linhas das janelas)",
            D.sob_demanda,double(D.bytes_d)/1048576.0,double(D.bytes_d_cheia)/1048576.0);nrrj::log(l);}}
    if(!L.falhou)nrrj::sucesso(nrrj::Falha::Detector);
    if(D.rastreio.P.adquire&32)nrrj::giro_inteira.store(R.giro?1:0,std::memory_order_relaxed);
    if(R.giro_rosto&&(D.rastreio.giro_rosto_n<=32||(D.rastreio.giro_rosto_n&63u)==0)){char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_GIRO_ROSTO caixa no rosto do detector depois do giro job=%d n=%llu",t.job,D.rastreio.giro_rosto_n);nrrj::log(l);}
    ++D.deteccoes;if(R.on)++D.on_jobs;
    if(R.tem_est||R.busca)D.sem_cabeca.store(0,std::memory_order_relaxed);else D.sem_cabeca.fetch_add(1,std::memory_order_relaxed);
    if(R.conv_errada){const bool v=!prof_trocada.load();prof_trocada.store(v);
        char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_CONVENCAO profundidade ao contrario (mediana da proximidade %.3f > 0,5): conversao trocada (trocada=%d) a partir do proximo job",double(R.med_prox),v?1:0);nrrj::log(l);}
    if(R.adotou){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ESCALA nova kface=%.0f (hipotese %d; a do RE e 1550): nenhuma cabeca na escala anterior por 60 jobs",double(R.kface),R.hip);nrrj::log(l);}

    {static unsigned long long ad_visto=0;const auto& s=D.rastreio.ad_ult;const auto an=D.rastreio.ad_n;if(an!=ad_visto&&(an<=64||(an&31u)==0)){ad_visto=an;char la[360]{};NRLOGF(la,sizeof la,"ROSTO_JOGADOR_DESLIGADO_LISTA rostos=%.0f maior_nota=%.2f maior_cab4k=%.0f em=%.0f,%.0f bloqueio=%.0f ancoraveis=%.0f tem_est=%.0f tem_meas=%.0f velha=%.0f job=%d n=%llu (w3-adqdiag; bits 1 nota 2 pequeno 4 nao_unico 8 sem_prox 16 sem_corpo 32 fraco 64 rastreio/corte)",s[0],s[1],s[2],s[3],s[4],s[5],s[6],s[7],s[8],s[9],t.job,an);nrrj::log(la);}}

    if(D.rastreio.P.cabeca_prof){const auto& rr=D.rastreio;static unsigned long long pl_visto=0;const double e4=double(esc)*(2160.0/(std::max)(1.0,double(saida_h)));
        if((rr.prof_liga_n!=pl_visto&&pl_visto==0)||(rr.cprof.jobs&255u)==0){pl_visto=rr.prof_liga_n;char lp[300]{};NRLOGF(lp,sizeof lp,"ROSTO_JOGADOR_CABECA_PROF jobs=%llu com_cabeca=%llu medidas=%llu ligou=%llu cortes_vetados=%llu id_vetos=%llu escala=%.1f escala_n=%d ultima=%.0f,%.0f,%.0f nota=%.2f seg=%d job=%d",
            rr.cprof.jobs,rr.cprof.com_cabeca,rr.prof_med_n,rr.prof_liga_n,rr.prof_corte_vet,rr.prof_id_veto,rr.cprof.esc,rr.cprof.esc_cont,rr.cps.x*e4,rr.cps.y*e4,rr.cps.lado*e4,rr.cps.nota,rr.cps.n_seg,t.job);nrrj::log(lp);}}
    if(R.reancorou){char l[220]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_REANCOROU pelo rosto da lista job=%d cab4k=%.0f caixa=%.0f,%.0f reancorou=%llu promovido=%llu",t.job,double(R.cab4k),double(R.est[0]),double(R.est[1]),D.rastreio.reancorou_n,D.rastreio.promovido_n);nrrj::log(l);}
    if(R.adquiriu){char l[220]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ADQUIRIU pelo rosto do detector job=%d cab4k=%.0f caixa=%.0f,%.0f kf_rosto/kf=%.2f adquiriu=%llu promovido=%llu",t.job,double(R.cab4k),double(R.est[0]),double(R.est[1]),double(R.kf_razao),D.rastreio.adquiriu_n,D.rastreio.promovido_n);nrrj::log(l);}
    if(R.escala_conf&&(D.rastreio.escala_conf_n<=32||(D.rastreio.escala_conf_n&63u)==0)){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ESCALA confirmada pelo rosto kface=%.0f kf_rosto/kf=%.2f escala_confirmada=%llu job=%d",double(R.kface),double(R.kf_razao),D.rastreio.escala_conf_n,t.job);nrrj::log(l);}
    if(R.adocao_vetada&&(D.rastreio.adocoes_vetadas<=32||(D.rastreio.adocoes_vetadas&63u)==0)){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ESCALA adocao vetada (hipotese %d: o objeto nao e um rosto do detector) vetadas=%llu job=%d",R.hip,D.rastreio.adocoes_vetadas,t.job);nrrj::log(l);}
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
            if(novo&&!R.corte){nrrj::cortar_troca_alvo();D.ger_vista=nrrj::ger().load(std::memory_order_acquire);D.tem_ant=false;++saltos;
                if(saltos<=32||(saltos&63u)==0){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ESCALA_SALTO n=%llu lado=%.0f antes=%.0f reancorou=%d job=%d (geracao nova)",saltos,double(R.est[2]),lado_ant,R.reancorou?1:0,t.job);nrrj::log(l);}}
            lado_ant=double(R.est[2]);
        }else if(!R.on)lado_ant=0.;
    }
    nrrj::grace_on.store(R.on?1:0,std::memory_order_relaxed);
    nrrj::Deteccao d{};d.on=R.on;d.tem=R.tem_est&&R.tem_recorte;d.quadro=t.quadro;d.conf=R.conf;d.cab4k=R.cab4k;
    if(D.rastreio.cd_tem)d.zela=float(D.rastreio.cd_z);
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
    const auto a=nrhip::api_rosto();
    const auto setdev=a.set_device;const auto cria=a.stream_create_with_flags;const auto copia=a.memcpy_async;const auto sinc=a.stream_synchronize;
    void* stream{};bool dispositivo=false;
    for(;;){
        DetTarefa t{};
        {std::unique_lock<std::mutex> l(D.m);D.cv.wait(l,[&]{return D.tem;});t=D.fila;D.tem=false;}
        if(nrgpu::perdida()){D.sets[t.set].ocupado.store(false);return;}
        try{
            if(!copia||!sinc||!cria){D.sets[t.set].ocupado.store(false);nrrj::envenenar("hipMemcpyAsync ausente (detector em thread)");continue;}
            if(!dispositivo){if(setdev&&t.device>=0)(void)setdev(t.device);if(cria(&stream,1u)||!stream){D.sets[t.set].ocupado.store(false);nrrj::envenenar("stream do detector");continue;}dispositivo=true;}
            const auto c2=a.memcpy_2d_async;
            executar_deteccao(D,t,[&](void* dst,const void* src,size_t n){return hip_ok(copia(dst,src,n,2,stream),"detector (copia)")&&hip_ok(sinc(stream),"detector (stream)");},
                [&](void* dst,size_t dp,const void* src,size_t sp,size_t w,size_t hh){return c2&&hip_ok(c2(dst,dp,src,sp,w,hh,3,stream),"detector (copia 2D)")&&hip_ok(sinc(stream),"detector (stream)");});
        }catch(...){D.sets[t.set].ocupado.store(false);nrrj::passageira(nrrj::Falha::Detector,"excecao no detector (thread)");}
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

    nrrj::grade().invertida.store(((read<int>(base,nrproxy::rva_flags_ffx)!=0)!=prof_trocada.load(std::memory_order_relaxed))?1:0,std::memory_order_relaxed);
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
    const auto gd=nrrj::grade_det(m.mvw,m.mvh);const int q=gd.q,gw=gd.gw,gh=gd.gh;if(gw<16||gh<16){sem_dados_log(e,job,"grade_pequena");return;}
    const int set=det_thread.load(std::memory_order_relaxed)?int(unsigned(job)&1u):0;
    auto& S=D.sets[set];
    if(S.ocupado.load(std::memory_order_acquire)){D.pulados.fetch_add(1);return;}
    const size_t need=size_t(m.mvw)*m.mvh;

    if(nrrj::DetCap c{S.cap,S.gcap};nrrj::det_nova_cap(c,need,size_t(gw)*gh)){
        if(!nrrj::det_aloc_pode(S.pausa,job))return;
        for(void** p:{&S.dq,&S.mvq,&S.d})if(*p){(void)hip.release(*p);*p=nullptr;}
        S.cap=0;S.gcap=0;
        if(aloca(hip,&S.dq,c.grade*4,"detector")||aloca(hip,&S.mvq,c.grade*8,"detector")||aloca(hip,&S.d,c.cheia*4,"detector")){(void)hip.error();
            for(void** p:{&S.dq,&S.mvq,&S.d})if(*p){(void)hip.release(*p);*p=nullptr;}
            nrrj::det_aloc_falhou(S.pausa,job);char t[96]{};NRLOGF(t,sizeof t,"hipMalloc detector (proxima tentativa em %u jobs)",S.pausa.espera);
            nrrj::passageira(nrrj::Falha::Detector,t);return;}
        nrrj::det_aloc_ok(S.pausa);
        S.cap=c.cheia;S.gcap=c.grade;
        if(c.cheia>need||c.grade>size_t(gw)*gh){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_DETECTOR_REALOCOU conjunto=%d cheia=%zu grade=%zu (pedido %zu/%d)",set,c.cheia,c.grade,need,gw*gh);nrrj::log(l);}
    }
    if(!S.amostra&&!S.amostra_tentada){S.amostra_tentada=true;if(aloca(hip,&S.amostra,det_amostra_max*4,"detector (amostra)"))S.amostra=nullptr;}
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
            catch(const HipFailure& f){char t[160]{};NRLOGF(t,sizeof t,"hip fase=%s status=%d (recorte npc2)",f.phase,f.status);nrrj::passageira(nrrj::Falha::Recorte,t);}
            catch(const nrrj::FalhaPermanente& x){nrrj::envenenar(x.what());}
            catch(const std::exception& x){nrrj::passageira(nrrj::Falha::Recorte,x.what());}
            catch(...){nrrj::passageira(nrrj::Falha::Recorte,"excecao recorte npc2");}}
        if(nrrj::sj_q())sj_intercalar();}}
}

inline void pontos_dela(nrrj::Slot& S,unsigned s) noexcept {
    try{
        auto& P=nrrj::pontos();auto& E=P.ent[s];const unsigned char* base=P.base.load(std::memory_order_acquire);
        if(!base||E.seq.load(std::memory_order_acquire)!=S.seq||E.l<=0||!(S.hl>0.f))return;

        thread_local nrsc::Rede* scrfd=nullptr;thread_local bool scrfd_tentou=false;
        thread_local std::vector<std::uint8_t> bgr;thread_local std::vector<nrdet::Rosto> rs;thread_local std::vector<std::uint16_t> hb;
        if(!scrfd_tentou){scrfd_tentou=true;char erro[160]{};scrfd=scrfd_carregar(false,erro,sizeof erro);
            char l[220]{};NRLOGF(l,sizeof l,scrfd?"ROSTO_JOGADOR_PONTOS detector scrfd (nr_scrfd.bin %s)":"ROSTO_JOGADOR_PONTOS sem o SCRFD (%s): caixa pela profundidade",erro);nrrj::log(l);}
        if(!scrfd)return;
        const int n=int(nrrj::roi_lado);hb.resize(size_t(n)*size_t(n)*4);std::memcpy(hb.data(),base+std::uint64_t(s)*nrrj::roi_bytes,hb.size()*2);
        if(E.seq.load(std::memory_order_acquire)!=S.seq)return;
        const auto t0=std::chrono::steady_clock::now();
        (void)detectar_imagem(*scrfd,hb,n,n,bgr,rs,nrrj::scrfd_a(false),false);
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

inline void aplicar_estilo(char* base) noexcept {
    if(!nrrjcl::ini_lido.load(std::memory_order_acquire)||!nrrjcl::tem_estilo())return;
    static unsigned ultimo=~0u;static float forca_base=0.f;
    static nrrjcl::CampoEstilo<float> cc{},cf{};static nrrjcl::CampoEstilo<bool> cl{};
    unsigned s=read<unsigned>(base,0x71d084);if(s>2)s=0;
    if(s==ultimo)return;
    if(forca_base<=0.f)forca_base=nrrj::passo1_forca.load(std::memory_order_relaxed);
    ultimo=s;
    const float c=nrrjcl::cor_estilo[s].load();
    nrrjcl::cor_publicar(nrrjcl::campo_estilo(cc,nrrjcl::cor_f.load(),c>=0.f,c,nrrjcl::cor_base.load()));
    const int l=nrrjcl::luz_estilo[s].load();
    nrrjcl::luz.store(nrrjcl::campo_estilo(cl,nrrjcl::luz.load(),l>=0,l!=0,nrrjcl::luz_base.load()));
    const float f=nrrjcl::forca_estilo[s].load();
    nrrj::passo1_forca.store(nrrjcl::campo_estilo(cf,nrrj::passo1_forca.load(std::memory_order_relaxed),f>0.f,f,forca_base));
    nrrjcl::menu_escreveu.store(true);
    char t[160]{};NRLOGF(t,sizeof t,"ROSTO_ESTILO estilo=%u cor=%.2f luz=%d forca=%.2f",s,double(nrrjcl::cor_f.load()),nrrjcl::luz.load()?1:0,double(nrrj::passo1_forca.load()));
    nrrj::log(t);
}
inline void avaliar_recorte_i(Estado& e,Hip& hip,char* base,void* primary,int job,void* dest,int rw,int rh,int pitch,int formato,const SjAval* sa){
    aplicar_estilo(base);
    nrorc::EscopoOrigem eo(nrrj::inst_atual()>=1?nrorc::Origem::Npc:nrorc::Origem::Rosto);
    nrorc::EscopoRedeRosto er;

    if(nrrj::retomou()){const unsigned v=nrrj::varrer_slots_velhos(job);nrrj::cortar();
        char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_RETOMOU depois da pausa job=%d slots_livres=%u (geracao nova)",job,v);nrrj::log(l);}
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
        if(nrneuraltone::requested&&!nrneuraltone::multipass_ok)throw nrrj::FalhaPermanente("ToneCurve ACES ou ToneLift > 0: dominio da rede diferente do da composicao");
        if(formato!=0||!dest||rw<64||rh<64||pitch<rw*8)throw std::runtime_error("residuo da cena fora do contrato (half RGBA)");
        if(!importar(e)||!grades(e,hip)){S.palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);return;}
        const unsigned n=S.n,L=S.L,ow=S.ow,oh=S.oh;
        if(nrrj::inst_atual()==0&&nrrj::caminha_dela.load(std::memory_order_relaxed)==3)pontos_dela(S,unsigned(s));
        bool amplia=false;
        amplia=amplia||((nrrj::npc_amplia_inst()||nrrj::canon_inst())&&n<=2u*L);
        amplia=amplia||(nrrj::todos_inst()&&n<=2u*L);
        amplia=amplia||(nrrj::inst_atual()==0&&nrrj::rede_fixa.load(std::memory_order_relaxed)&&n<=nrrj::rede_fixa.load(std::memory_order_relaxed)*L);

        amplia=amplia||(::nrgeo::ligada.load(std::memory_order_relaxed)&&n<=2u*L);
        if(n<nrrj::n_min||n>nrrj::n_cap()||(n&127u)||(L<n&&!amplia)||!ow||!oh)throw std::runtime_error("recorte fora do contrato");

        {static std::atomic<unsigned> vistos{0};const unsigned b=1u<<((n/128u)&31u);
         if(!(vistos.fetch_or(b,std::memory_order_relaxed)&b)){char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_REDE_LADO n=%u recorte=%u teto=%u reduz=%d",n,L,nrrj::teto.load(),L>n?1:0);nrrj::log(l);}}
        const auto t0=std::chrono::steady_clock::now();
        const float expo=expo_do_job(base,job);
        const unsigned tonemap=tonemap_do_proxy(base);
        float passo=float(L)/float(n),bx=float(S.x),by=float(S.y),fow=float(ow),foh=float(oh);

        float bxc=sa?bx+sa->dx:bx,byc=sa?by+sa->dy:by;
        char* ent=e.base_hip+nrrj::offset_ent(unsigned(s));char* sai=e.base_hip+nrrj::offset_sai(unsigned(s));
        unsigned urw=unsigned(rw),urh=unsigned(rh),upitch=unsigned(pitch/2),un=n;

        const bool di=nrrj::delta_inteiro()&&e.k_delta_el&&e.k_guarda_el;

        const bool scena=nrrj::soma_cena.load(std::memory_order_relaxed)&&!di;
        if(di&&nrrj::soma_cena.load(std::memory_order_relaxed)){static std::atomic<bool> avisou{false};if(!avisou.exchange(true))nrrj::log("ROSTO_JOGADOR_DELTA_INTEIRO aviso: NeuralRostoJogadorSomaCena=1 ignorado no modo (o Delta inteiro ja traz a banda baixa; a cena entraria 2 vezes)");}
        if(scena)check(hip.set(e.dc,0,size_t(n)*n*16));
        else{void* a[]={&dest,&urw,&urh,&upitch,&bxc,&byc,&passo,&fow,&foh,&un,&e.dc};lancar(hip,e.k_cena,size_t(n)*n,a);}

        const unsigned clm=nrrjcl::modo();const bool cl=!di&&clm&&e.k_cl_cena&&e.k_cl_baixa&&e.k_cl_aplica;
        if(cl&&nrrj::soma_cena.load(std::memory_order_relaxed)){void* a[]={&dest,&urw,&urh,&upitch,&bxc,&byc,&passo,&fow,&foh,&un,&e.D};lancar(hip,e.k_cena,size_t(n)*n,a);}

        bool rcorte=nrrj::ruido_inst()&&nrrj::ruido_banda.load(std::memory_order_relaxed)>0.f&&e.k_rpeso&&e.k_rmedia&&e.k_rbanda;

        const float lt=nrrj::ruido_limpa.load(std::memory_order_relaxed);
        bool limpa=nrrj::ruido_inst()&&lt>0.f&&e.k_lgrad&&e.k_lpeso&&e.k_limpa&&e.k_lumin&&e.k_blur1;
        const unsigned lfonte=nrrj::ruido_fonte.load(std::memory_order_relaxed);int lrt=-1;
        if(limpa&&lfonte==2u){bool mudou=false;std::string val;lrt=nrrtjogo::estado(&mudou,&val);
            if(mudou||e.l_n+e.l_rt_desligado==0){char t[200]{};NRLOGF(t,sizeof t,"ROSTO_RUIDO_FONTE inst=%d config.ini RayTracingSetting=%s rt=%d (%s)",nrrj::inst_atual(),val.empty()?"?":val.c_str(),lrt,lrt<0?"sem a chave: o detector":lrt?"limpeza ligada":"limpeza desligada");nrrj::log(t);}
            if(lrt==0){limpa=false;++e.l_rt_desligado;}}
        const bool lquer_det=limpa&&(lfonte==0u||(lfonte==2u&&lrt<0));
        bool ldet=lquer_det&&e.k_rpeso&&e.k_rmedia;
        if(lquer_det&&!ldet)limpa=false;
        if((rcorte||ldet)&&!e.r_tentado){e.r_tentado=true;if(aloca_par(hip,&e.rw,size_t(nrrj::n_cap())*nrrj::n_cap()*4,&e.rsoma,16,"RuidoRT")){nrrj::log("ROSTO_RUIDO sem memoria para o peso: corte desligado nesta instancia");}}
        rcorte=rcorte&&e.rw&&e.rsoma;
        if(ldet&&!(e.rw&&e.rsoma)){ldet=false;limpa=false;}
        if(rcorte||ldet){void* a[]={&ent,&un,&e.rw};lancar(hip,e.k_rpeso,size_t(n)*n,a);}
        {float ex=expo;unsigned tm=tonemap;void* a[]={&ent,&un,&ex,&tm};lancar(hip,e.k_entrada,size_t(n)*n,a);}

        {const float am=nrrj::entrada_media.load(std::memory_order_relaxed);
         if(am>0.f&&nrrj::inst_atual()==0&&e.k_ema){
            if(!e.ema&&!e.ema_tentado){e.ema_tentado=true;if(aloca(hip,&e.ema,size_t(nrrj::n_cap())*nrrj::n_cap()*8,"EntradaMedia"))e.ema=nullptr;}
            if(e.ema){const bool cont=nrrj::entrada_media_continua(e.ema_reg,S.x,S.y,L,n,std::uint64_t(S.geracao));
                float a=am,lim=nrrj::entrada_media_limiar.load(std::memory_order_relaxed);unsigned uc=cont?1u:0u;
                void* ar[]={&ent,&e.ema,&un,&a,&lim,&uc};lancar(hip,e.k_ema,size_t(n)*n,ar);
                if(cont)++e.ema_usos;else ++e.ema_novos;
                if(e.ema_usos+e.ema_novos<=3||((e.ema_usos+e.ema_novos)&255u)==0){char t[160]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_ENTRADA_MEDIA usos=%llu recortes_novos=%llu n=%u L=%u",e.ema_usos,e.ema_novos,n,L);nrrj::log(t);}}}}

        bool fres=nrrj::frescor.load(std::memory_order_relaxed)>0.f&&nrrj::acumula.load(std::memory_order_relaxed)&&nrrj::temporal_inst()&&nrrj::frescor_inst()&&e.k_lumin&&e.k_bandas2&&e.k_meia4;
        if(fres&&!e.lin&&!e.lin_tentado){e.lin_tentado=true;if(aloca(hip,&e.lin,size_t(nrrj::n_cap())*nrrj::n_cap()*4,"frescor"))e.lin=nullptr;}
        fres=fres&&e.lin;
        if(fres){void* a[]={&ent,&un,&e.lin};lancar(hip,e.k_lumin,size_t(n)*n,a);}

        if(fres&&nrrj::frescor_borrado.load(std::memory_order_relaxed)&&e.k_blur1){
            float sg=1.6f/passo;unsigned r=raio_cv(sg),h1=1,h0=0,ref=1;
            void* a1[]={&e.lin,&e.tmp1,&un,&un,&sg,&r,&h1,&ref};lancar(hip,e.k_blur1,size_t(n)*n,a1);
            void* a2[]={&e.tmp1,&e.lin,&un,&un,&sg,&r,&h0,&ref};lancar(hip,e.k_blur1,size_t(n)*n,a2);}

        void* ent_rede=ent;
        if(limpa){
            const float lcel=foh/2160.f/passo;const size_t n2=size_t(n)*n;
            float* pl=static_cast<float*>(e.d0);void* b2=pl;void* b16=pl+n2;void* grs=pl+2*n2;void* wp=pl+3*n2;
            unsigned h1=1,h0=0,ref=1;
            auto borra=[&](void* src,void* dst,float sg){unsigned r=raio_cv(sg);void* a1[]={&src,&e.tmp1,&un,&un,&sg,&r,&h1,&ref};lancar(hip,e.k_blur1,n2,a1);
                void* a2[]={&e.tmp1,&dst,&un,&un,&sg,&r,&h0,&ref};lancar(hip,e.k_blur1,n2,a2);};
            {void* a[]={&ent,&un,&e.luma};lancar(hip,e.k_lumin,n2,a);}
            borra(e.luma,b2,nrrj::ruido_limpa_fina.load(std::memory_order_relaxed)*lcel);
            borra(e.luma,b16,nrrj::ruido_limpa_grossa.load(std::memory_order_relaxed)*lcel);
            {float c=lcel;void* a[]={&b2,&b16,&un,&c,&e.gl};lancar(hip,e.k_lgrad,n2,a);}
            borra(e.gl,grs,3.f*lcel);
            {float lim=nrrj::ruido_limpa_borda.load(std::memory_order_relaxed),con=nrrj::ruido_limpa_contraste.load(std::memory_order_relaxed);
             void* a[]={&grs,&b2,&b16,&un,&lim,&con,&e.gl};lancar(hip,e.k_lpeso,n2,a);}
            borra(e.gl,wp,4.f*lcel);
            ElipseK el0{(S.hx-bx)/passo,(S.hy-by)/passo,S.hl/passo,float(NR_AREA_SEMI_X),float(NR_AREA_SEMI_Y),float(NR_AREA_BORDA0),float(NR_AREA_BORDA1),float(NR_AREA_MIOLO0),float(NR_AREA_MIOLO1)};
            if(ldet){check(hip.set(e.rsoma,0,16));float r6=.6f;void* a[]={&e.rw,&un,&el0,&r6,&e.rsoma};lancar(hip,e.k_rmedia,size_t(n/8)*(n/8),a);}
            {void* so=ldet?e.rsoma:nullptr;float f0=nrrj::ruido_rosto0.load(std::memory_order_relaxed),f1=nrrj::ruido_rosto1.load(std::memory_order_relaxed),t=lt,rmax=nrrj::ruido_limpa_razao.load(std::memory_order_relaxed);
             void* a[]={&ent,&e.luma,&b2,&b16,&wp,&so,&un,&t,&f0,&f1,&rmax,&e.D};lancar(hip,e.k_limpa,n2,a);}
            ent_rede=e.D;++e.l_n;
            if(e.l_n<=3||(e.l_n&63u)==0){
                if(!e.lsoma&&!e.l_tentado){e.l_tentado=true;if(aloca(hip,&e.lsoma,16,"RuidoRTLimpa"))e.lsoma=nullptr;}
                float ls[2]{},rs[2]{};
                if(e.lsoma){check(hip.set(e.lsoma,0,16));float r6=.6f;void* a[]={&wp,&un,&el0,&r6,&e.lsoma};lancar(hip,e.k_rmedia,size_t(n/8)*(n/8),a);check(hip.copy(ls,e.lsoma,8,2));}
                if(ldet)check(hip.copy(rs,e.rsoma,8,2));
                float wf=1.f;if(ldet){const float wm=rs[1]>0.f?rs[0]/rs[1]:0.f,a0=nrrj::ruido_rosto0.load(),a1=nrrj::ruido_rosto1.load();float q=(wm-a0)/(a1-a0);q=q<0.f?0.f:q>1.f?1.f:q;wf=q*q*(3.f-2.f*q);}
                char t[220]{};NRLOGF(t,sizeof t,"ROSTO_RUIDO_LIMPA_STAT inst=%d limpezas=%llu n=%u fonte=%u rt_jogo=%d Wf=%.3f forca=%.2f peso_pele=%.3f rt_desligado=%llu",
                    nrrj::inst_atual(),e.l_n,n,lfonte,lrt,double(wf),double(lt),ls[1]>0.f?double(ls[0]/ls[1]):-1.,e.l_rt_desligado);nrrj::log(t);}
        }

        {const float esl=nrrj::entrada_sigma_celulas(nrrj::entrada_sigma.load(std::memory_order_relaxed),foh,passo);
         if(esl>0.f&&e.k_meia4&&e.k_blur4&&e.k_f4meia&&!e.esg_falhou){
            const size_t n2=size_t(n)*n;
            if(e.esg&&e.esg_px<n2){(void)hip.release(e.esg);e.esg=nullptr;e.esg_px=0;}
            if(!e.esg){const size_t cap=(std::max)(size_t(nrrj::n_cap())*nrrj::n_cap(),n2);
                if(aloca(hip,&e.esg,cap*16,"EntradaSigma")){e.esg=nullptr;e.esg_falhou=true;nrrj::log("ROSTO_JOGADOR_ENTRADA_SIGMA sem memoria: desligada nesta instancia");}
                else e.esg_px=cap;}
            if(e.esg){float sg=esl;unsigned r=raio_cv(sg),h1=1,h0=0,ref=1;
                {void* a[]={&ent_rede,&un,&e.d0};lancar(hip,e.k_meia4,n2,a);}
                {void* a[]={&e.d0,&e.esg,&un,&un,&sg,&r,&h1,&ref};lancar(hip,e.k_blur4,n2,a);}
                {void* a[]={&e.esg,&e.d0,&un,&un,&sg,&r,&h0,&ref};lancar(hip,e.k_blur4,n2,a);}
                {void* a[]={&e.d0,&un,&e.esg};lancar(hip,e.k_f4meia,n2,a);}
                ent_rede=e.esg;++e.esg_n;
                if(e.esg_n<=3||(e.esg_n&255u)==0){char t[180]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_ENTRADA_SIGMA inst=%d usos=%llu n=%u L=%u sigma_cel=%.3f raio=%u limpa=%d",nrrj::inst_atual(),e.esg_n,n,L,double(sg),r,limpa?1:0);nrrj::log(t);}}}}
        void* p=contexto(e,hip,base,primary);
        {const float f1=nrrj::passo1_forca.load(std::memory_order_relaxed);if(f1!=1.f)write<float>(p,0x30,read<float>(p,0x30)*f1);}
        if(nrrj::ablacao.load(std::memory_order_relaxed)&2u)check(hip.copy(sai,ent,size_t(n)*n*8,3));
        else native<Eval>(base,0x2a080)(p,ent_rede,int(n*8),0,int(n),int(n),sai,int(n*8),0,0,nullptr);
        hip.completed("rj_rede");

        char* sai_f=sai;
        if(nrrj::passos.load(std::memory_order_relaxed)>=2&&e.k_sat&&!(nrrj::ablacao.load(std::memory_order_relaxed)&2u)){
            const double p1=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
            if(e.p1_n==0||p1<e.p1_min)e.p1_min=p1;else e.p1_min+=.001*(p1-e.p1_min);
            e.p1_ema=e.p1_n?.85*e.p1_ema+.15*p1:p1;++e.p1_n;
            const float folga=passo2_folga.load(std::memory_order_relaxed);
            bool cabe=folga<=0.f||e.p1_n<4||e.p1_ema<=double(folga)*e.p1_min;

            {const unsigned c2=nrrj::passo2_cada.load(std::memory_order_relaxed);if(c2>1)cabe=cabe&&(e.p2_k++%c2)==0;}
            if(!e.in2&&!e.buf2_tentado){e.buf2_tentado=true;const size_t b=size_t(nrrj::n_cap())*nrrj::n_cap()*8;
                if(aloca_par(hip,&e.in2,b,&e.out2,b,"2o passo")){nrrj::log("ROSTO_JOGADOR_PASSOS sem memoria: 1 passo");}}
            if(cabe&&e.in2&&e.out2){
                {void* a[]={&sai,&un,&e.in2};lancar(hip,e.k_sat,size_t(n)*n,a);}
                p=contexto(e,hip,base,primary);write<float>(p,0x30,.03125f*nrrj::passo2_forca.load(std::memory_order_relaxed));
                native<Eval>(base,0x2a080)(p,e.in2,int(n*8),0,int(n),int(n),e.out2,int(n*8),0,0,nullptr);
                hip.completed("rj_rede2");sai_f=static_cast<char*>(e.out2);++e.passos2;
            }else if(e.in2)++e.passos2_pulados;
            if(e.p1_n<=4||(e.p1_n&63u)==0){char t[200]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_PASSOS n=%u p1=%.2f p1_ema=%.2f p1_min=%.2f cabe=%d passos2=%llu pulados2=%llu",e.p1_n,p1,e.p1_ema,e.p1_min,cabe?1:0,e.passos2,e.passos2_pulados);nrrj::log(t);}
        }
        const auto t_rede=std::chrono::steady_clock::now();
        {void* a[]={&ent,&sai_f,&e.dc,&un,&e.d0};lancar(hip,e.k_dif,size_t(n)*n,a);}

        if(cl){void* sc=nrrj::soma_cena.load(std::memory_order_relaxed)?e.D:e.dc;void* a[]={&sc,&un,&sai};lancar(hip,e.k_cl_cena,size_t(n)*n,a);}

        const float cel=foh/2160.f/passo;
        unsigned f=4,nl=n/4;

        const bool camada_vai=!di&&nrrj::blur_rapido.load(std::memory_order_relaxed)&&[&]{
            const unsigned cm=nrrj::borda_camada.load(std::memory_order_relaxed);if(!(cm&&e.k_camada&&e.k_baixa_m&&e.k_blur4w&&e.k_delta_m))return false;
            const bool emp=nrrj::inst_atual()>=1&&nrrj::npc_borda_camada.load(std::memory_order_relaxed);
            const DetTarefa T=emp?estado_de(0).det_tarefa:e.det_tarefa;const auto& DS=det().sets[unsigned(T.set>=0?T.set:0)];
            return T.job==job&&T.set>=0&&DS.d&&T.dw>=16&&T.dh>=16&&size_t(T.dw)*T.dh<=DS.cap;}();
        if(!camada_vai&&!di){
        {void* a[]={&e.d0,&un,&f,&nl,&e.lo};lancar(hip,e.k_baixa,size_t(nl)*nl,a);}
        {float sl=nrrj::banda_sigma.load(std::memory_order_relaxed)*cel/float(f);unsigned rl=raio_cv(sl),h1=1,h0=0,ref=1;
         void* a1[]={&e.lo,&e.lo2,&nl,&nl,&sl,&rl,&h1,&ref};lancar(hip,e.k_blur4,size_t(nl)*nl,a1);
         void* a2[]={&e.lo2,&e.lo,&nl,&nl,&sl,&rl,&h0,&ref};lancar(hip,e.k_blur4,size_t(nl)*nl,a2);}}
        ElipseK el{(S.hx-bx)/passo,(S.hy-by)/passo,S.hl/passo,float(NR_AREA_SEMI_X),float(NR_AREA_SEMI_Y),float(NR_AREA_BORDA0),float(NR_AREA_BORDA1),float(NR_AREA_MIOLO0),float(NR_AREA_MIOLO1)};
        unsigned dep=0;

        bool camada=false;
        if(const unsigned cmodo=nrrj::borda_camada.load(std::memory_order_relaxed);!di&&cmodo&&e.k_camada&&e.k_baixa_m&&e.k_blur4w&&e.k_delta_m){

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
                camada=true;++e.camada_n;
            }else ++e.camada_sem;
            if(e.camada_n+e.camada_sem<=4||((e.camada_n+e.camada_sem)&63u)==0){char t[160]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_CAMADA job=%d usadas=%llu sem_grade=%llu modo=%u",job,e.camada_n,e.camada_sem,cmodo);nrrj::log(t);}
        }
        if(!camada&&camada_vai){
            {void* a[]={&e.d0,&un,&f,&nl,&e.lo};lancar(hip,e.k_baixa,size_t(nl)*nl,a);}
            {float sl=nrrj::banda_sigma.load(std::memory_order_relaxed)*cel/float(f);unsigned rl=raio_cv(sl),h1=1,h0=0,ref=1;
             void* a1[]={&e.lo,&e.lo2,&nl,&nl,&sl,&rl,&h1,&ref};lancar(hip,e.k_blur4,size_t(nl)*nl,a1);
             void* a2[]={&e.lo2,&e.lo,&nl,&nl,&sl,&rl,&h0,&ref};lancar(hip,e.k_blur4,size_t(nl)*nl,a2);}}
        if(di){
            float er0=nrrj::elipse_r0.load(std::memory_order_relaxed),er1=nrrj::elipse_r1.load(std::memory_order_relaxed),eb=nrrj::delta_inteiro_borda_celulas(foh,passo);
            void* a[]={&e.d0,&un,&el,&er0,&er1,&eb,&e.D,&e.luma};lancar(hip,e.k_delta_el,size_t(n)*n,a);
            if(++e.di_n<=3||(e.di_n&255u)==0){char t[180]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_DELTA_INTEIRO inst=%d usos=%llu n=%u L=%u r0=%.2f r1=%.2f borda_cel=%.1f hl_cel=%.1f",nrrj::inst_atual(),e.di_n,n,L,double(er0),double(er1),double(eb),double(el.hl));nrrj::log(t);}}
        else if(!camada){void* a[]={&e.d0,&e.lo,&un,&f,&nl,&el,&dep,&e.D,&e.luma};lancar(hip,e.k_delta,size_t(n)*n,a);}

        if(rcorte){check(hip.set(e.rsoma,0,16));
         {float r6=.6f;void* a[]={&e.rw,&un,&el,&r6,&e.rsoma};lancar(hip,e.k_rmedia,size_t(n/8)*(n/8),a);}
         {float sw=nrrj::ruido_peso_sigma.load(std::memory_order_relaxed)*cel;unsigned rw_=raio_cv(sw),h1=1,h0=0,ref=1;
          void* a1[]={&e.rw,&e.tmp1,&un,&un,&sw,&rw_,&h1,&ref};lancar(hip,e.k_blur1,size_t(n)*n,a1);
          void* a2[]={&e.tmp1,&e.gl,&un,&un,&sw,&rw_,&h0,&ref};lancar(hip,e.k_blur1,size_t(n)*n,a2);}
         {float sf=nrrj::ruido_fino.load(std::memory_order_relaxed)*cel;unsigned rf=raio_cv(sf),h1=1,h0=0,ref=1;
          void* a1[]={&e.D,&e.dc,&un,&un,&sf,&rf,&h1,&ref};lancar(hip,e.k_blur4,size_t(n)*n,a1);
          void* a2[]={&e.dc,&e.d0,&un,&un,&sf,&rf,&h0,&ref};lancar(hip,e.k_blur4,size_t(n)*n,a2);}
         {float sb=nrrj::ruido_banda.load(std::memory_order_relaxed),f0=nrrj::ruido_rosto0.load(std::memory_order_relaxed),f1=nrrj::ruido_rosto1.load(std::memory_order_relaxed);
          void* a[]={&e.D,&e.luma,&e.d0,&e.gl,&e.rsoma,&un,&sb,&f0,&f1};lancar(hip,e.k_rbanda,size_t(n)*n,a);}
         ++e.r_cortes;}
        {float s4=4.f*cel;unsigned r4=raio_cv(s4),h1=1,h0=0,zero=2;
         void* b1[]={&e.luma,&e.tmp1,&un,&un,&s4,&r4,&h1,&zero};lancar(hip,e.k_blur1,size_t(n)*n,b1);
         void* b2[]={&e.tmp1,&e.gl,&un,&un,&s4,&r4,&h0,&zero};lancar(hip,e.k_blur1,size_t(n)*n,b2);}

        if(cl){
            const float sg=nrrjcl::sigma.load(std::memory_order_relaxed),sp=sg>0.f?sg:nrrj::banda_sigma.load(std::memory_order_relaxed);
            if(clm&2u){{void* a[]={&sai,&e.D,&un,&f,&nl,&e.lo};lancar(hip,e.k_cl_baixa,size_t(nl)*nl,a);}
                float sl=sp*cel/float(f);unsigned rl=raio_cv(sl),h1=1,h0=0,ref=1;
                void* a1[]={&e.lo,&e.lo2,&nl,&nl,&sl,&rl,&h1,&ref};lancar(hip,e.k_blur4,size_t(nl)*nl,a1);
                void* a2[]={&e.lo2,&e.lo,&nl,&nl,&sl,&rl,&h0,&ref};lancar(hip,e.k_blur4,size_t(nl)*nl,a2);}
            {float km=nrrjcl::razao_max;unsigned um=clm;void* a[]={&e.D,&sai,&ent,&e.lo,&un,&f,&nl,&el,&dep,&um,&km};lancar(hip,e.k_cl_aplica,size_t(n)*n,a);}
            if(++e.cl_n<=3||(e.cl_n&255u)==0){char t[160]{};NRLOGF(t,sizeof t,"ROSTO_COR n=%llu inst=%d cor=%d luz=%d sigma=%.1f soma_cena=%d",e.cl_n,nrrj::inst_atual(),(clm&1u)?0:1,(clm&2u)?0:1,double(sp),nrrj::soma_cena.load()?1:0);nrrj::log(t);}
        }
        if(di){

            float sL=16.f*cel;unsigned rL=raio_cv(sL),h1=1,h0=0,ref=1;
            void* b1[]={&e.luma,&e.tmp1,&un,&un,&sL,&rL,&h1,&ref};lancar(hip,e.k_blur1,size_t(n)*n,b1);
            void* b2[]={&e.tmp1,&e.d0,&un,&un,&sL,&rL,&h0,&ref};lancar(hip,e.k_blur1,size_t(n)*n,b2);
            void* a[]={&e.D,&e.gl,&e.d0,&un,&el,&ent};lancar(hip,e.k_guarda_el,size_t(n)*n,a);}
        else{void* a[]={&e.D,&e.gl,&un,&el,&dep,&ent};lancar(hip,e.k_guarda,size_t(n)*n,a);}

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
        if(mede)check(hip.copy(soma_h,e.soma,12,2));
        if(rcorte&&(e.r_cortes<=3||(e.r_cortes&63u)==0)){float rs[2]{};check(hip.copy(rs,e.rsoma,8,2));
         const float wm=rs[1]>0.f?rs[0]/rs[1]:0.f,a0=nrrj::ruido_rosto0.load(),a1=nrrj::ruido_rosto1.load();float t=(wm-a0)/(a1-a0);t=t<0.f?0.f:t>1.f?1.f:t;
         char lr[200]{};NRLOGF(lr,sizeof lr,"ROSTO_RUIDO_STAT inst=%d cortes=%llu n=%u peso_miolo=%.3f Wf=%.3f s=%.2f amostras=%.0f",nrrj::inst_atual(),e.r_cortes,n,double(wm),double(t*t*(3.f-2.f*t)),double(nrrj::ruido_banda.load()),double(rs[1]));nrrj::log(lr);}
        S.palavra.store(nrrj::palavra(nrrj::Pronto,etiqueta),std::memory_order_release);g.prontos.fetch_add(1);
        nrrj::sucesso(nrrj::Falha::Recorte);
        const auto t1=std::chrono::steady_clock::now();
        const double ms=std::chrono::duration<double,std::milli>(t1-t0).count(),ms_rede=std::chrono::duration<double,std::milli>(t_rede-t0).count();
        e.ms_soma+=ms;++e.ms_n;e.ms_max=(std::max)(e.ms_max,ms);e.ultimo_ms=ms;
        if(nrrj::agenda_ligada()){const int ii=nrrj::inst_atual();auto& cm=nrrj::custo_ms_v[ii];
            const float norm=float(ms)*nrag::custo_tabela(1024u)/nrag::custo_tabela(n);cm.store(nrag::custo_ema(cm.load(std::memory_order_relaxed),norm),std::memory_order_relaxed);}
        ++e.avaliados;g.avaliacoes.fetch_add(1);
        if(e.avaliados<=6||mede||(nrrj::leve_npc()&&(e.avaliados&15u)==0u)){
            char t[360]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_TEMPO avaliacoes=%llu job=%d n=%u L=%u ms=%.2f rede_ms=%.2f ms_media=%.2f ms_max=%.2f prontos=%llu pulados_marcador=%llu delta_abs=%.5f delta_escuro=%.6f",
                e.avaliados,job,n,L,ms,ms_rede,e.ms_soma/(e.ms_n?e.ms_n:1),e.ms_max,g.prontos.load(),g.pulados_marcador.load(),
                soma_h[2]>0?double(soma_h[0]/soma_h[2]):0.,soma_h[2]>0?double(soma_h[1]/soma_h[2]):0.);nrrj::log(t);
            if(mede){e.ms_soma=0;e.ms_n=0;e.ms_max=0;}
        }
    }catch(const HipFailure& f){
        S.palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);
        char t[160]{};NRLOGF(t,sizeof t,"hip fase=%s status=%d",f.phase,f.status);nrrj::passageira(nrrj::Falha::Recorte,t);
    }catch(const nrrj::FalhaPermanente& x){S.palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);nrrj::envenenar(x.what());}
    catch(const std::exception& x){S.palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);nrrj::passageira(nrrj::Falha::Recorte,x.what());}
    catch(...){S.palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);nrrj::passageira(nrrj::Falha::Recorte,"excecao");}
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
    if(nrrj::pula_limiar.load()>0.f&&e0.k_mini&&!e0.mini)check(hip.allocate(&e0.mini,4096));

    auto ruido=[&](Estado& e,unsigned n){if(!nrrj::ruido_peso.load()||!nrrj::ruido_inst()||!e.k_rpeso||!e.k_rmedia||e.rw)return;
        e.r_tentado=true;check(hip.allocate(&e.rw,size_t(n)*n*4));check(hip.allocate(&e.rsoma,16));
        if(nrrj::ruido_limpa.load()>0.f&&!e.lsoma){e.l_tentado=true;check(hip.allocate(&e.lsoma,16));}};
    ruido(e0,d.cap_grace);
    if(!importar(e0))return false;
    for(int k=1;k<=d.k_npc;++k){nrrj::Inst ik(k);auto& e=estado();std::lock_guard<std::mutex> l(e.mutex);
        compartilhar(e,e0);if(!importar(e))return false;ruido(e,nrrj::n_cap());
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

    nrrj::tique.fetch_add(1,std::memory_order_relaxed);
    {static std::uint64_t epoca_vista=0;if(nrrj::falhas_epoca(nrrj::epoca_backend.load(std::memory_order_acquire),epoca_vista)){

        {auto& U=ultimo_pub();std::lock_guard<std::mutex> lu(U.m);U.f=Estado::Fora{};}
        nrrj::log("ROSTO_JOGADOR_FALHAS_ZERADAS backend novo (contadores de falhas passageiras e pausas do zero; ultimo job publicado e tabela job -> quadro limpos)");}}
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
                try{if(nrrj::ativo()&&modulo(e1,hip)&&kn==1&&!e1.carregado)(void)contexto(e1,hip,base,primary);}
                catch(const nrrj::FalhaPermanente&){nrrj::envenenar("pesos npc2");}catch(...){nrrj::passageira(nrrj::Falha::Recorte,"pesos npc2");}}
        }

        else if(!preparo().feito){preparo().base=base;preparo().primary=primary;preparar_deteccao(e,hip,base,job);return;}
        preparar_deteccao(e,hip,base,job);

        if(nrrj::fora.load(std::memory_order_relaxed))e.fora_pend=Estado::Fora{job,base,primary,dest,rw,rh,pitch,formato};
        else avaliar_recorte(e,hip,base,primary,job,dest,rw,rh,pitch,formato);
    }catch(const HipFailure& f){char t[160]{};NRLOGF(t,sizeof t,"hip fase=%s status=%d (preparo)",f.phase,f.status);nrrj::passageira(nrrj::Falha::Recorte,t);}
    catch(const nrrj::FalhaPermanente& x){nrrj::envenenar(x.what());}
    catch(const std::exception& x){nrrj::passageira(nrrj::Falha::Recorte,x.what());}
    catch(...){nrrj::passageira(nrrj::Falha::Recorte,"excecao preparo");}
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

inline int sj_idade_pronto(unsigned s) noexcept {
    auto& g=nrrj::global();const auto& S=g.slot[s];
    int idade=-1;
    {std::uint64_t qp=0;

     for(unsigned k2=0;k2<nrrj::slots;++k2){const auto& P=g.slot[k2];const auto w2=P.palavra.load(std::memory_order_acquire);
         if(nrrj::estado_de(w2)!=nrrj::Pronto||!nrrj::id_e_quadro(nrrj::job_de(w2)))continue;
         const std::uint64_t pg=P.geracao,pq=P.quadro;std::atomic_thread_fence(std::memory_order_acquire);
         if(P.palavra.load(std::memory_order_relaxed)!=w2)continue;
         if(pg==S.geracao&&pq<=S.quadro&&pq>qp)qp=pq;}
     if(qp)idade=int(S.quadro-qp);}
    return idade;
}
struct ReusoMede {nrrj::PortaoReuso p;float* host{};void* ev{};bool tentou{},sem_api{};unsigned long long n{},atrasadas{};};
inline ReusoMede& reuso_mede(){static auto* r=new ReusoMede;return *r;}
inline bool sj_medir_reuso(Estado& e,Hip& hip,char* base,int job,int s,bool* congela_ok=nullptr){
    const float lim=nrrj::pula_limiar.load(std::memory_order_relaxed);
    if(!(lim>0.f)||!e.k_mini||!importar(e))return false;
    auto& g=nrrj::global();auto& S=g.slot[unsigned(s)];

    const unsigned n=S.n,L=S.L;if(n<nrrj::n_min||n>nrrj::n_cap()||!L)return false;
    if(!e.mini&&aloca(hip,&e.mini,4096,"portao do rosto"))e.mini=nullptr;
    if(!e.mini)return false;
    auto& R=reuso_mede();
    const auto a=nrhip::api_rosto();
    if(!R.tentou){R.tentou=true;
        void* h{};if(a.host_malloc&&a.event_create_with_flags&&a.event_record&&a.event_query&&hip.copy_async&&!a.host_malloc(&h,4096,0u)&&h){
            R.host=static_cast<float*>(h);if(a.event_create_with_flags(&R.ev,0x2u )||!R.ev)R.ev=nullptr;}
        if(!R.host||!R.ev){R.sem_api=true;(void)hip.error();nrrj::log("ROSTO_REUSO sem a copia assincrona (hipHostMalloc/hipEvent): o pulo do recorte fica desligado");}}
    if(R.sem_api)return false;

    bool pronta=false;
    if(R.p.pend){const int q=a.event_query(R.ev);
        if(q==0)pronta=true;else if(q==600)++R.atrasadas;else{(void)nrgpu::hip(q,"portao do rosto (hipEventQuery)");return false;}}

    const int idade=sj_idade_pronto(unsigned(s));

    bool cg=false;if(nrrj::pula_congela.load(std::memory_order_relaxed)){const auto dt=nrrj::ler_deteccao();cg=dt.tem&&nrrj::congela_parado(dt.vx,dt.vy,nrrj::pula_congela_vel.load(std::memory_order_relaxed));}
    if(congela_ok)*congela_ok=cg;
    const bool real=R.p.decidir(pronta?R.host:nullptr,S.hl,S.geracao,idade,nrrj::sj_q(),lim,cg?0xffffffffu:nrrj::pula_max.load(std::memory_order_relaxed));

    if(!R.p.pend||pronta){
        const float passo=float(L)/float(n);
        float cxc=(S.hx-float(S.x))/passo,cyc=(S.hy-float(S.y))/passo,meio=.6f*S.hl/passo;
        float expo=expo_do_job(base,job);unsigned tm=tonemap_do_proxy(base),un=n;
        char* ent=e.base_hip+nrrj::offset_ent(unsigned(s));
        void* ar[]={&ent,&un,&cxc,&cyc,&meio,&expo,&tm,&e.mini};lancar(hip,e.k_mini,1024,ar);
        check(hip.copy_async(R.host,e.mini,4096,2,nullptr));
        if(const int r=a.event_record(R.ev,nullptr)){(void)nrgpu::hip(r,"portao do rosto (hipEventRecord)");return real;}
        R.p.lancou(S.hl,S.geracao,!real);}
    ++R.n;
    if(R.n<=64||(R.n&31u)==0){char t[300]{};NRLOGF(t,sizeof t,"ROSTO_REUSO n=%llu quadro=%llu ger=%llu hl=%.0f real=%d rel=%.4f idade=%d pulados=%llu lim=%.3f atrasadas=%llu (miniatura do recorte anterior, copia assincrona)",
        R.n,static_cast<unsigned long long>(S.quadro),static_cast<unsigned long long>(S.geracao),double(S.hl),real?1:0,R.p.rel,idade,R.p.pulados,double(lim),R.atrasadas);nrrj::log(t);}
    return real;
}

inline bool sj_congelar(Estado& e,Hip& hip,int s,int id) noexcept {
    try{
        if(!e.base_hip||!e.k_etiqueta)return false;
        auto& g=nrrj::global();auto& S=g.slot[unsigned(s)];
        int p=-1;std::uint64_t qp=0;std::uint64_t wp=0;
        for(unsigned k=0;k<nrrj::slots;++k){if(int(k)==s)continue;const auto& P=g.slot[k];const auto w2=P.palavra.load(std::memory_order_acquire);
            if(nrrj::estado_de(w2)!=nrrj::Pronto||!nrrj::id_e_quadro(nrrj::job_de(w2)))continue;
            if(P.geracao!=S.geracao||P.quadro>S.quadro||P.quadro<=qp)continue;
            if(P.x!=S.x||P.y!=S.y||P.L!=S.L||P.n!=S.n||P.ow!=S.ow||P.oh!=S.oh)continue;
            if(!nrrj::congela_no_lugar(S.hx,S.hy,S.hl,P.hx0,P.hy0,P.hl,nrrj::pula_congela_px.load(std::memory_order_relaxed)))continue;
            std::atomic_thread_fence(std::memory_order_acquire);if(P.palavra.load(std::memory_order_relaxed)!=w2)continue;
            p=int(k);qp=P.quadro;wp=w2;}
        if(p<0)return false;
        auto w=S.palavra.load(std::memory_order_acquire);
        if(nrrj::estado_de(w)!=nrrj::Escrito||nrrj::job_de(w)!=id||!S.palavra.compare_exchange_strong(w,nrrj::palavra(nrrj::Avaliando,id),std::memory_order_acq_rel))return false;
        const size_t b=size_t(S.n)*S.n*8u;
        check(hip.copy(e.base_hip+nrrj::offset_ent(unsigned(s)),e.base_hip+nrrj::offset_ent(unsigned(p)),b,3));
        check(hip.copy(e.base_hip+nrrj::offset_sai(unsigned(s)),e.base_hip+nrrj::offset_sai(unsigned(p)),b,3));
        S.hx0=g.slot[unsigned(p)].hx0;S.hy0=g.slot[unsigned(p)].hy0;
        if(g.slot[unsigned(p)].palavra.load(std::memory_order_acquire)!=wp){S.palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);return false;}
        {void* tag=e.base_hip+nrrj::offset_tag(unsigned(s));unsigned uj=unsigned(id);void* a[]={&tag,&uj};check(hip.launch(e.k_etiqueta,1,1,1,1,1,1,0,nullptr,a,nullptr));}
        hip.completed("rj_congela");
        S.palavra.store(nrrj::palavra(nrrj::Pronto,id),std::memory_order_release);g.prontos.fetch_add(1);
        return true;
    }catch(...){auto& S=nrrj::global().slot[unsigned(s)];auto w=S.palavra.load(std::memory_order_acquire);
        if(nrrj::estado_de(w)==nrrj::Avaliando)S.palavra.store(nrrj::palavra(nrrj::Livre,0),std::memory_order_release);return false;}
}

inline void sj_avaliar(Estado& e0,int s) noexcept {
    if(nrgpu::perdida())return;
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

    if(nrrj::pose_barata.load(std::memory_order_relaxed)){static unsigned pose_seg=0;
        if(nrpose::pula(nrrj::pose_longe.load(std::memory_order_acquire),pose_seg,nrrj::pose_cadencia.load(std::memory_order_relaxed),sj_idade_pronto(unsigned(s)),nrrj::sj_q())){
            ++pose_seg;livre();const auto np=C.pose_pulados.fetch_add(1,std::memory_order_relaxed)+1;
            if(np<=6||(np&127u)==0){char l[160]{};NRLOGF(l,sizeof l,"ROSTO_POSE_BARATA_PULO pulados=%llu quadro=%llu",np,static_cast<unsigned long long>(S.quadro));nrrj::log(l);}
            return;}
        pose_seg=0;}
    const auto t0=std::chrono::steady_clock::now();
    bool pulou=false,cg=false;
    try{Hip hip;if(modulo(e0,hip)){try{pulou=sj_medir_reuso(e0,hip,f.base,f.job,s,&cg);}catch(...){pulou=false;}}
        if(pulou&&cg&&sj_congelar(e0,hip,s,id)){
            const auto nc=C.congelados.fetch_add(1,std::memory_order_relaxed)+1;C.pulados_reuso.fetch_add(1,std::memory_order_relaxed);
            if(nc<=3||(nc&127u)==0){char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_PULA_CONGELA congelados=%llu quadro=%llu",nc,static_cast<unsigned long long>(S.quadro));nrrj::log(l);}
            return;}

        if(pulou&&!cg){livre();C.pulados_reuso.fetch_add(1,std::memory_order_relaxed);return;}
        if(modulo(e0,hip))avaliar_recorte_i(e0,hip,f.base,f.primary,f.job,f.dest,f.rw,f.rh,f.pitch,f.formato,&a);}
    catch(const HipFailure& x){char tt[160]{};NRLOGF(tt,sizeof tt,"hip fase=%s status=%d (recorte de quadro)",x.phase,x.status);nrrj::passageira(nrrj::Falha::Recorte,tt);}
    catch(const nrrj::FalhaPermanente& x){nrrj::envenenar(x.what());}
    catch(...){nrrj::passageira(nrrj::Falha::Recorte,"excecao recorte de quadro");}
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
    const auto setdev=nrhip::api_rosto().set_device;
    static HANDLE timer=CreateWaitableTimerExW(nullptr,nullptr,0x00000002 ,TIMER_ALL_ACCESS);
    const auto dorme_us=[](unsigned us){if(!us)return;LARGE_INTEGER due{};due.QuadPart=-LONGLONG(us)*10;
        if(timer&&SetWaitableTimer(timer,&due,0,nullptr,nullptr,FALSE))WaitForSingleObject(timer,us/1000u+50u);else Sleep((us+999u)/1000u);};
    nrrj::EsperaAuto ea(nrrj::espera_auto_us);int dev_atual=-1;
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
        if(nrgpu::perdida())return;
        if(sj_s>=0&&sj_marcado){
            F.sj_espera_soma+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-F.sj_desde).count();
            nrregswin::rjsync::Rosto ev;std::lock_guard<std::mutex> lock(e.mutex);sj_avaliar(e,sj_s);}
        if(!tem_item)continue;
        const auto t_item=std::chrono::steady_clock::now();
        const auto f=item.first;
        try{
            if(setdev&&item.second>=0&&item.second!=dev_atual){(void)setdev(item.second);dev_atual=item.second;}

            if(nrregswin::rjsync::flag_set_n.load(std::memory_order_acquire)>hs)++F.hs_espera_ok;else ++F.hs_espera_falhou;
            bool tem=false;int auto_i=-1;unsigned esp=0;

            {nrregswin::rjsync::Rosto ev;std::lock_guard<std::mutex> lock(e.mutex);
                auto& g=nrrj::global();
                for(unsigned k=0;k<nrrj::slots&&!tem;++k){const auto w=g.slot[k].palavra.load(std::memory_order_acquire);tem=nrrj::estado_de(w)==nrrj::Escrito&&nrrj::job_de(w)==f.job;}
            }
            if(!tem){++F.sem_recorte;
                nrregswin::rjsync::Rosto ev;std::lock_guard<std::mutex> lock(e.mutex);
                try{Hip hip;avaliar_recorte(e,hip,f.base,f.primary,f.job,f.dest,f.rw,f.rh,f.pitch,f.formato);}catch(...){}
                continue;}
            esp=nrrj::espera_us.load(std::memory_order_relaxed);
            if(esp==nrrj::espera_present)esp=0;
            if(esp==nrrj::espera_auto){const unsigned long long k=ea.k;auto_i=ea.escolher(25);esp=ea.us[auto_i];if(k<5)auto_i=-1;}
            dorme_us(esp);
            double ms=-1.0;
            {nrregswin::rjsync::Rosto ev;std::lock_guard<std::mutex> lock(e.mutex);
                e.ultimo_ms=-1.0;
                try{Hip hip;avaliar_recorte(e,hip,f.base,f.primary,f.job,f.dest,f.rw,f.rh,f.pitch,f.formato);}
                catch(const HipFailure& x){char t[160]{};NRLOGF(t,sizeof t,"hip fase=%s status=%d (recorte thread)",x.phase,x.status);nrrj::passageira(nrrj::Falha::Recorte,t);}
                catch(const nrrj::FalhaPermanente& x){nrrj::envenenar(x.what());}
                catch(...){nrrj::passageira(nrrj::Falha::Recorte,"excecao recorte thread");}
                ms=e.ultimo_ms;}
            ++F.feitos;
            ea.medir(auto_i,ms);
            F.total_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t_item).count();
            if(F.feitos<=8||(F.feitos&31u)==0){char t[300]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_THREAD_REDE feitos=%llu sem_recorte=%llu hs_ok=%llu hs_falhou=%llu espera_us=%u ms=%.2f total_ms=%.2f auto[0/1.5/3/4.5/6]=%.2f/%.2f/%.2f/%.2f/%.2f",
                F.feitos,F.sem_recorte,F.hs_espera_ok,F.hs_espera_falhou,esp,ms,F.total_ms,ea.ms[0],ea.ms[1],ea.ms[2],ea.ms[3],ea.ms[4]);nrrj::log(t);}
        }catch(...){nrrj::passageira(nrrj::Falha::Recorte,"excecao na thread do recorte");}
    }
}
inline void depois_do_publish() noexcept {
    if(!nrrj::ativo()){

        if(!nrrj::ligado.load(std::memory_order_relaxed)||nrrj::global().envenenado.load(std::memory_order_relaxed)||!nrrj::pausado())return;
        auto& e=estado();std::lock_guard<std::mutex> lock(e.mutex);
        if(e.grade_lido>=0){auto& S=nrrj::grade().slot[unsigned(e.grade_lido)];e.grade_lido=-1;S.palavra.store(nrrj::palavra_g(nrrj::GLivre,0),std::memory_order_release);}
        e.det_pend=false;e.fora_pend=Estado::Fora{};
        return;}
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
            int dev=-1;{const auto gd=nrhip::api_rosto().get_device;if(!gd||gd(&dev))dev=-1;}
            {std::lock_guard<std::mutex> l(F.m);F.fila.emplace_back(e.fora_pend,dev);F.hs=nrregswin::rjsync::flag_set_n.load(std::memory_order_acquire);}
            e.fora_pend=Estado::Fora{};F.cv.notify_one();
        }
    }

    if(e.fora_pend.job>0){
        const auto f=e.fora_pend;e.fora_pend=Estado::Fora{};

        static nrrj::EsperaAuto ea(nrrj::espera_auto_us);int auto_i=-1;
        unsigned esp=nrrj::espera_us.load(std::memory_order_relaxed);

        static nrrj::EsperaAuto ep(nrrj::espera_present_us);static double pres_espera=0;int pres_i=-1;
        if(esp==nrrj::espera_present){
            static HANDLE tp=CreateWaitableTimerExW(nullptr,nullptr,0x00000002,TIMER_ALL_ACCESS);
            const auto p0=nrrj::presents_fim.load(std::memory_order_acquire);const auto tw=std::chrono::steady_clock::now();
            for(int it=0;it<160&&nrrj::presents_fim.load(std::memory_order_acquire)==p0;++it){
                LARGE_INTEGER due{};due.QuadPart=-2500;
                if(tp&&SetWaitableTimer(tp,&due,0,nullptr,nullptr,FALSE))WaitForSingleObject(tp,20);else Sleep(1);}
            pres_espera=.9*pres_espera+.1*std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-tw).count();
            pres_i=ep.escolher(20);esp=ep.us[pres_i];
        }

        static unsigned long long r6_sem=0,r6_com=0;bool r6_descarta=false;
        if(nrrj::espera_job.load(std::memory_order_relaxed)&&esp){
            auto& g=nrrj::global();bool tem=false;
            for(unsigned k=0;k<nrrj::slots&&!tem;++k){const auto w=g.slot[k].palavra.load(std::memory_order_acquire);tem=nrrj::estado_de(w)==nrrj::Escrito&&nrrj::job_de(w)==f.job;}
            if(!tem){esp=0;++r6_sem;}
            else{++r6_com;
                if(esp==nrrj::espera_auto){

                    const unsigned long long k=ea.k;auto_i=ea.escolher(25);r6_descarta=k<5;esp=ea.us[auto_i];
                }}
            if((r6_sem+r6_com)<=8||((r6_sem+r6_com)&255u)==0){char t[160]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_ESPERA_JOB job=%d com_recorte=%llu sem_recorte=%llu espera_us=%u",f.job,r6_com,r6_sem,esp);nrrj::log(t);}
        }
        if(esp==nrrj::espera_auto){auto_i=ea.escolher(20);esp=ea.us[auto_i];}
        if(esp){
            static HANDLE timer=CreateWaitableTimerExW(nullptr,nullptr,0x00000002 ,TIMER_ALL_ACCESS);
            LARGE_INTEGER due{};due.QuadPart=-LONGLONG(esp)*10;
            if(timer&&SetWaitableTimer(timer,&due,0,nullptr,nullptr,FALSE))WaitForSingleObject(timer,esp/1000u+50u);else Sleep((esp+999u)/1000u);
        }
        e.ultimo_ms=-1.0;
        try{Hip hip;avaliar_recorte(e,hip,f.base,f.primary,f.job,f.dest,f.rw,f.rh,f.pitch,f.formato);}
        catch(const HipFailure& x){char t[160]{};NRLOGF(t,sizeof t,"hip fase=%s status=%d (recorte fora)",x.phase,x.status);nrrj::passageira(nrrj::Falha::Recorte,t);}
        catch(const nrrj::FalhaPermanente& x){nrrj::envenenar(x.what());}
        catch(...){nrrj::passageira(nrrj::Falha::Recorte,"excecao recorte fora");}
        if(pres_i>=0&&e.ultimo_ms>0){
            ep.medir(pres_i,e.ultimo_ms);
            if(ep.k<=24||(ep.k&63u)==0){char t[240]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_ESPERA_PRESENT k=%llu ate_present=%.2f ms[0/1/2/3/4]=%.2f/%.2f/%.2f/%.2f/%.2f",ep.k,pres_espera,ep.ms[0],ep.ms[1],ep.ms[2],ep.ms[3],ep.ms[4]);nrrj::log(t);}
        }
        if(auto_i>=0&&e.ultimo_ms>0&&!r6_descarta){
            ea.medir(auto_i,e.ultimo_ms);
            if(ea.k<=24||(ea.k&63u)==0){char t[220]{};NRLOGF(t,sizeof t,"ROSTO_JOGADOR_ESPERA_AUTO k=%llu ms[0/1.5/3/4.5/6]=%.2f/%.2f/%.2f/%.2f/%.2f",ea.k,ea.ms[0],ea.ms[1],ea.ms[2],ea.ms[3],ea.ms[4]);nrrj::log(t);}
        }
    }
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
    try{Hip hip;const auto c2=nrhip::api_rosto().memcpy_2d;
        executar_deteccao(D,t,[&](void* dst,const void* src,size_t n){return hip_ok(hip.copy(dst,src,n,2),"detector (copia)");},
            [&](void* dst,size_t dp,const void* src,size_t sp,size_t w,size_t hh){return c2&&hip_ok(c2(dst,dp,src,sp,w,hh,3),"detector (copia 2D)");});}
    catch(const HipFailure& f){D.sets[t.set].ocupado.store(false);char l[160]{};NRLOGF(l,sizeof l,"hip fase=%s status=%d (detector)",f.phase,f.status);nrrj::passageira(nrrj::Falha::Detector,l);}
    catch(...){D.sets[t.set].ocupado.store(false);nrrj::passageira(nrrj::Falha::Detector,"excecao detector");}
}
}
