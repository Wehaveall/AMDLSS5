#include "../../../runtime/nr_publico.h"
#include "frame_limiter.h"
#include "native_ini_path.h"
#include "../async-reprojection/reproject_controls.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif
namespace nrlimit {
namespace {

std::atomic_flag ocupado=ATOMIC_FLAG_INIT;
std::atomic<bool> ativo{false};
std::atomic<bool> carregando{false};
Agenda agenda;
HANDLE timer=nullptr;bool timer_tentado=false,alta_resolucao=false;
std::atomic<unsigned long long> quadros{0},atrasados{0},esperados{0},pulados{0},espera_ns{0},alinhados{0},espera_depois_ns{0};
std::atomic<long long> folga_atual{0};

std::atomic<bool> pendente{false};
unsigned modo_quadro=0;long long prazo_quadro=0,fim_antes=0;
Folga folga;
unsigned long long quadros_no_log=0;

std::atomic<unsigned> fg_ritmo{0};
long long ritmo_saida=0;long long ritmo_nat[64]{};unsigned ritmo_n=0,ritmo_i=0;long long ritmo_T=0;
unsigned long long ritmo_quadros=0,ritmo_esperas=0;long long ritmo_espera_ns=0;

std::filesystem::path log_limitador() {
    HMODULE self{};wchar_t path[32768]{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&ritmar),&self))return {};
    const auto length=GetModuleFileNameW(self,path,32768);if(!length||length>=32768)return {};
    return std::filesystem::path(path).parent_path()/L"nr_limitador.log";
}
void ritmo_log(long long T) noexcept {
    try{
        const auto arquivo=log_limitador();if(arquivo.empty())return;
        long long c[64];const unsigned n=ritmo_n;for(unsigned i=0;i<n;++i)c[i]=ritmo_nat[i];std::sort(c,c+n);
        auto pc=[&](unsigned q){return n?double(c[(n-1)*q/100])/1e6:0.;};
        std::ofstream log(arquivo,std::ios::app);
        log<<"FG_RITMO P="<<fg_ritmo.load()<<" T_ms="<<double(T)/1e6<<" natural_p10/50/90_ms="<<pc(10)<<'/'<<pc(50)<<'/'<<pc(90)
           <<" quadros="<<ritmo_quadros<<" esperas="<<ritmo_esperas<<" espera_media_ms="<<(ritmo_quadros?double(ritmo_espera_ns)/1e6/double(ritmo_quadros):0.)
           <<" pid="<<GetCurrentProcessId()<<" tick="<<GetTickCount64()<<'\n';
    }catch(...){}
}

std::atomic<unsigned> fg_umjob{0};
long long umjob_saida=0,umjob_T=0;long long umjob_nat[16]{};unsigned umjob_n=0;unsigned long long umjob_quadros=0,umjob_faltas=0,umjob_esperas=0;long long umjob_espera_ns=0;
void umjob_log() noexcept {
    try{
        const auto arquivo=log_limitador();if(arquivo.empty())return;
        std::ofstream log(arquivo,std::ios::app);
        log<<"FG_UMJOB T_ms="<<double(umjob_T)/1e6<<" fps="<<(umjob_T?1e9/double(umjob_T):0.)<<" quadros="<<umjob_quadros<<" faltas="<<umjob_faltas
           <<" esperas="<<umjob_esperas<<" espera_media_ms="<<(umjob_quadros?double(umjob_espera_ns)/1e6/double(umjob_quadros):0.)
           <<" pid="<<GetCurrentProcessId()<<" tick="<<GetTickCount64()<<"\n";
    }catch(...){}
}
bool umjob_fg(long long chegada) noexcept {
    if(!fg_umjob.load(std::memory_order_relaxed)||!nrreproject::fg_ativo()){umjob_saida=0;umjob_T=0;umjob_n=0;return false;}
    if(umjob_saida){
        const long long nat=chegada-umjob_saida;
        if(nat<=0||nat>=100'000'000){umjob_saida=chegada;umjob_n=0;umjob_T=0;return true;}
        if(!umjob_T){umjob_nat[umjob_n++]=nat;if(umjob_n==16){long long c[16];std::copy(umjob_nat,umjob_nat+16,c);std::sort(c,c+16);umjob_T=c[8];}}
    }
    long long saida=chegada;
    if(umjob_T){
        const int idade=nrreproject::idade_composta.load(std::memory_order_relaxed);
        if(idade>=2){umjob_T+=500'000;++umjob_faltas;}else if(idade==1)umjob_T-=10'000;
        umjob_T=(std::max)(5'000'000LL,(std::min)(umjob_T,33'333'333LL));
        const long long alvo=umjob_saida+umjob_T;
        if(alvo>chegada){esperar_ate(alvo);saida=agora_ns();++umjob_esperas;umjob_espera_ns+=saida-chegada;}
    }
    umjob_saida=saida;
    if(++umjob_quadros%1024==1)umjob_log();
    return true;
}

bool ritmo_fg(long long chegada) noexcept {
    const unsigned P=fg_ritmo.load(std::memory_order_relaxed);
    if(!P||!nrreproject::fg_ativo()){ritmo_saida=0;ritmo_n=ritmo_i=0;ritmo_T=0;return false;}
    if(ritmo_saida){const long long nat=chegada-ritmo_saida;if(nat>0&&nat<100'000'000){ritmo_nat[ritmo_i]=nat;ritmo_i=(ritmo_i+1)%64;if(ritmo_n<64)++ritmo_n;}
                   else{ritmo_n=ritmo_i=0;}}
    long long saida=chegada;
    if(ritmo_n>=16){
        long long c[64];for(unsigned i=0;i<ritmo_n;++i)c[i]=ritmo_nat[i];std::sort(c,c+ritmo_n);
        ritmo_T=c[(ritmo_n-1)*P/100];
        const long long alvo=ritmo_saida+ritmo_T;
        if(alvo>chegada){esperar_ate(alvo);saida=agora_ns();++ritmo_esperas;ritmo_espera_ns+=saida-chegada;}
    }
    ritmo_saida=saida;
    if(++ritmo_quadros%1024==1)ritmo_log(ritmo_T);
    return true;
}
long long qpc_frequencia() noexcept {LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return f.QuadPart;}
void registrar(const char* evento,unsigned fps) noexcept {
    try{
        const auto arquivo=log_limitador();if(arquivo.empty())return;
        const auto c=contagem();
        std::ofstream log(arquivo,std::ios::app);
        log<<"LIMITADOR evento="<<evento<<" fps="<<fps<<" modo="<<c.modo<<" quadros="<<c.quadros<<" atrasados="<<c.atrasados
           <<" esperados="<<c.esperados<<" pulados="<<c.pulados
           <<" espera_media_ms="<<(c.quadros?c.espera_ms/double(c.quadros):0.)
           <<" alinhados="<<c.alinhados<<" espera_depois_media_ms="<<(c.quadros?c.espera_depois_ms/double(c.quadros):0.)
           <<" folga_ms="<<c.folga_ms
           <<" alta_resolucao="<<(c.alta_resolucao?1:0)<<" thread="<<GetCurrentThreadId()
           <<" pid="<<GetCurrentProcessId()<<" tick="<<GetTickCount64()<<'\n';
    }catch(...){}
}
void carregar_ini(void* base) noexcept {
    bool esperado=false;
    if(!carregando.compare_exchange_strong(esperado,true,std::memory_order_acq_rel))return;
    unsigned valor=0;
    try{

        const auto ini=nrinipath::resolve(base,reinterpret_cast<const void*>(&ritmar));
        if(!ini.empty()){
            valor=aceitar(NRCFG_INT(L"DlssNrOnAmd",L"NeuralFrameCap",0,ini.c_str()));
            const auto m=NRCFG_INT(L"DlssNrOnAmd",L"NeuralFrameCapMode",0,ini.c_str());
            modo.store(m<=2?unsigned(m):0u,std::memory_order_release);
            const auto r=NRCFG_INT(L"DlssNrOnAmd",L"NeuralFgRitmo",0,ini.c_str());
            fg_ritmo.store(r>=50&&r<=95?unsigned(r):0u,std::memory_order_release);
            fg_umjob.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralFgUmJob",0,ini.c_str())==1?1u:0u,std::memory_order_release);
        }
    }catch(...){valor=0;}
    bool livre=false;
    if(definido.compare_exchange_strong(livre,true,std::memory_order_acq_rel))pedido.store(valor,std::memory_order_release);
}
}
long long agora_ns() noexcept {
    static const long long frequencia=qpc_frequencia();
    LARGE_INTEGER t{};QueryPerformanceCounter(&t);
    const long long s=t.QuadPart/frequencia,r=t.QuadPart%frequencia;
    return s*1'000'000'000LL+r*1'000'000'000LL/frequencia;
}

void esperar_ate(long long prazo) noexcept {
    constexpr long long giro=300'000;
    if(!timer_tentado){
        timer_tentado=true;
        timer=CreateWaitableTimerExW(nullptr,nullptr,CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,TIMER_ALL_ACCESS);
        alta_resolucao=timer!=nullptr;
        if(!timer)timer=CreateWaitableTimerExW(nullptr,nullptr,0,TIMER_ALL_ACCESS);
    }
    const long long resta=prazo-agora_ns();
    if(timer&&resta>giro){
        LARGE_INTEGER due{};due.QuadPart=-((resta-giro)/100);
        if(due.QuadPart<0&&SetWaitableTimerEx(timer,&due,0,nullptr,nullptr,nullptr,0))
            WaitForSingleObject(timer,50);
    }
    while(agora_ns()<prazo)YieldProcessor();
}
Contagem contagem() noexcept {
    return {quadros.load(),atrasados.load(),esperados.load(),pulados.load(),alinhados.load(),double(espera_ns.load())/1e6,
            double(espera_depois_ns.load())/1e6,double(folga_atual.load())/1e6,modo.load(),alta_resolucao};
}
namespace {

long long passo(unsigned fps) noexcept {
    const unsigned antes=agenda.armado?agenda.fps:0;
    const long long agora=agora_ns();
    const long long prazo=agenda.planejar(agora,fps);
    if(!fps){ativo.store(false,std::memory_order_relaxed);folga.limpar();folga_atual.store(0);registrar("desligado",antes);return 0;}
    const bool rearmou=agenda.rearmou;
    if(!ativo.exchange(true,std::memory_order_relaxed)||antes!=fps){registrar(antes?"troca":"ligado",fps);}
    else if(rearmou)registrar("pausa",fps);
    quadros.fetch_add(1,std::memory_order_relaxed);
    if(prazo>agora){
        esperar_ate(prazo);
        esperados.fetch_add(1,std::memory_order_relaxed);
        espera_ns.fetch_add(static_cast<unsigned long long>(agora_ns()-agora),std::memory_order_relaxed);
    }else if(!rearmou)atrasados.fetch_add(1,std::memory_order_relaxed);
    const auto total=quadros.load(std::memory_order_relaxed);
    if(total-quadros_no_log>=1024){quadros_no_log=total;registrar("periodo",fps);}
    return rearmou?0:prazo;
}
}
void ritmar(void* base) noexcept {

    if(!carregando.load(std::memory_order_acquire))carregar_ini(base);
    unsigned fps=pedido.load(std::memory_order_relaxed);
    if(!fps&&(fg_ritmo.load(std::memory_order_relaxed)||fg_umjob.load(std::memory_order_relaxed))){
        static std::atomic_flag ritmo_ocupado=ATOMIC_FLAG_INIT;
        if(!ritmo_ocupado.test_and_set(std::memory_order_acquire)){
            const long long t=agora_ns();
            const bool tratou=umjob_fg(t)||ritmo_fg(t);ritmo_ocupado.clear(std::memory_order_release);
            if(tratou)return;
        }
    }
    if(!fps&&!ativo.load(std::memory_order_relaxed))return;
    if(ocupado.test_and_set(std::memory_order_acquire)){pulados.fetch_add(1,std::memory_order_relaxed);return;}
    struct Soltar{~Soltar(){ocupado.clear(std::memory_order_release);}} soltar;
    modo_quadro=modo.load(std::memory_order_relaxed);
    if(modo_quadro==2){pendente.store(true,std::memory_order_relaxed);return;}
    const long long prazo=passo(fps);
    if(modo_quadro==0&&prazo>0){prazo_quadro=prazo;fim_antes=agora_ns();pendente.store(true,std::memory_order_relaxed);}
    else pendente.store(false,std::memory_order_relaxed);
}
void depois() noexcept {
    if(!pendente.load(std::memory_order_relaxed))return;
    if(ocupado.test_and_set(std::memory_order_acquire))return;
    struct Soltar{~Soltar(){ocupado.clear(std::memory_order_release);}} soltar;
    if(!pendente.exchange(false,std::memory_order_relaxed))return;
    if(modo_quadro==2){(void)passo(pedido.load(std::memory_order_relaxed));return;}
    const unsigned fps=pedido.load(std::memory_order_relaxed);
    if(!fps||fps!=agenda.fps)return;
    const long long agora=agora_ns();
    folga.registrar(agora-fim_antes);
    const long long f=folga.valor(Agenda::periodo_ns(fps));
    folga_atual.store(f,std::memory_order_relaxed);
    const long long alvo=prazo_quadro+f;
    if(alvo>agora){
        esperar_ate(alvo);
        alinhados.fetch_add(1,std::memory_order_relaxed);
        espera_depois_ns.fetch_add(static_cast<unsigned long long>(agora_ns()-agora),std::memory_order_relaxed);
    }
}
}
