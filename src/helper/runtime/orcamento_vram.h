#pragma once

#include "nr_publico.h"
#include "orcamento.h"
#include "log_fila.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dxgi1_4.h>
#include <algorithm>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>
namespace nrorc_vram {
inline const std::wstring& pasta() noexcept {
 static const std::wstring p=[]{HMODULE self{};wchar_t path[32768]{};
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&pasta),&self)||
     !GetModuleFileNameW(self,path,32768))return std::wstring();
  std::wstring s(path);const auto k=s.find_last_of(L"\\/");return k==std::wstring::npos?std::wstring():s.substr(0,k+1);}();
 return p;}
inline std::atomic<int> job_visto{0};

inline void log(const char* fmt,...) noexcept {
 static std::atomic<unsigned> n{0};if(n.fetch_add(1,std::memory_order_relaxed)>=4000u)return;
 char t[700]{};va_list a;va_start(a,fmt);std::vsnprintf(t,sizeof t,fmt,a);va_end(a);
 static const std::wstring arq=pasta()+L"nr_orcamento.log";
 nrlogfila::linha(arq.c_str(),"pid=%lu tick=%llu %s\n",GetCurrentProcessId(),static_cast<unsigned long long>(GetTickCount64()),t);
}
inline int env_int(const char* k,int padrao) noexcept {
 char b[16]{};std::size_t n=0;if(getenv_s(&n,b,sizeof b,k)==0&&n>1)return std::atoi(b);
 return padrao;}
inline bool ctx_separado() noexcept {return env_int("NR_ORC_CTX_SEPARADO",0)==1;}

inline void registrar(nrorc::Api a,std::uint64_t bytes,const char* sitio,nrorc::Origem o) noexcept {
 auto& G=nrorc::estado().guarda;
 if(!G.registrar(a,o,bytes))return;
 char l[260]{};nrorc::linha_tardia(l,sizeof l,G.tardias.load(std::memory_order_relaxed),a,o,bytes,sitio,job_visto.load(std::memory_order_relaxed));
 log("%s thread=%lu",l,GetCurrentThreadId());}
inline void registrar(nrorc::Api a,std::uint64_t bytes,const char* sitio) noexcept {registrar(a,bytes,sitio,nrorc::origem_tl);}
using Malloc=int(*)(void**,std::size_t);
inline std::atomic<Malloc> malloc_real{nullptr};
inline Malloc resolver_malloc() noexcept {
 if(auto m=malloc_real.load(std::memory_order_acquire))return m;
 HMODULE h=GetModuleHandleW(L"amdhip64_7.dll");const auto m=h?reinterpret_cast<Malloc>(GetProcAddress(h,"hipMalloc")):nullptr;
 malloc_real.store(m,std::memory_order_release);return m;}

struct VivosHip {std::mutex m;nrorc::Vivos v;};
inline VivosHip& vivos_hip() noexcept {static auto* p=new VivosHip;return *p;}
inline void vivo_criar(void* p,std::uint64_t n,nrorc::Origem o) noexcept {
 if(!nrorc::modo()||!p||!n)return;
 nrorc::Vivos::V velho{};bool havia=false;
 try{auto& V=vivos_hip();std::lock_guard<std::mutex> l(V.m);havia=V.v.criar(p,n,o,velho);}catch(...){return;}
 if(havia)nrorc::estado().guarda.liberar(velho.o,velho.n);}
inline void vivo_soltar(void* p) noexcept {
 if(!p)return;nrorc::Vivos::V v{};bool achou=false;
 try{auto& V=vivos_hip();std::lock_guard<std::mutex> l(V.m);achou=V.v.soltar(p,v);}catch(...){return;}
 if(achou)nrorc::estado().guarda.liberar(v.o,v.n);}

inline int hip_malloc_contado(void** p,std::size_t n) noexcept {
 const auto m=resolver_malloc();if(!m)return 1;
 const int r=m(p,n);if(!r){registrar(nrorc::Api::HipMalloc,n,"helper");if(p)vivo_criar(*p,n,nrorc::origem_tl);}return r;}
using Free=int(*)(void*);
inline std::atomic<Free> free_real{nullptr};
inline Free resolver_free() noexcept {
 if(auto f=free_real.load(std::memory_order_acquire))return f;
 HMODULE h=GetModuleHandleW(L"amdhip64_7.dll");const auto f=h?reinterpret_cast<Free>(GetProcAddress(h,"hipFree")):nullptr;
 free_real.store(f,std::memory_order_release);return f;}

inline int hip_free_contado(void* p) noexcept {
 const auto f=resolver_free();if(!f)return 1;
 vivo_soltar(p);return f(p);}

struct PoolVivo {std::mutex m;nrorc::PoolRede p;unsigned long long reais_pos_selo{};bool ligado{};};
inline PoolVivo& pool_rede() noexcept {static auto* p=new PoolVivo;return *p;}

using Range=int(*)(void**,std::size_t*,void*);
inline Range range_real{};
inline int range_pool(void** base,std::size_t* size,void* p) noexcept {
 {auto& P=pool_rede();std::lock_guard<std::mutex> l(P.m);if(const auto q=P.p.pedido_de(p);q&&base&&size){*base=p;*size=std::size_t(q);return 0;}}
 return range_real?range_real(base,size,p):1;}
inline void log_pool(int job,const char* quando) noexcept {
 auto& P=pool_rede();unsigned long long sv{},re{},rs{},dv{};std::size_t vi{},li{};std::uint64_t by{},bl{};bool on{};
 {std::lock_guard<std::mutex> l(P.m);on=P.ligado;sv=P.p.servidos;re=P.p.reais;rs=P.reais_pos_selo;dv=P.p.devolvidos;vi=P.p.vivos.size();li=P.p.livres.size();by=P.p.bytes;bl=P.p.bytes_livres();}
 log("ORC_POOL %s job=%d ligado=%d servidos=%llu reais=%llu reais_pos_selo=%llu devolvidos=%llu blocos_com_proxy=%zu livres=%zu pool_mb=%.1f livre_mb=%.1f",
     quando,job,on?1:0,sv,re,rs,dv,vi,li,double(by)/double(nrorc::MB),double(bl)/double(nrorc::MB));}
inline std::atomic<IDXGIAdapter3*> adaptador_v{nullptr};
inline IDXGIAdapter3* adaptador() noexcept {return adaptador_v.load(std::memory_order_acquire);}
inline void guardar_adaptador(IDXGIAdapter3* a) noexcept {if(!a)return;a->AddRef();IDXGIAdapter3* nada=nullptr;if(!adaptador_v.compare_exchange_strong(nada,a))a->Release();}
struct Vram {bool ok{};std::uint64_t budget{},uso{},budget_nl{},uso_nl{};};
inline Vram medir(IDXGIAdapter3* a) noexcept {
 Vram v{};if(!a)return v;DXGI_QUERY_VIDEO_MEMORY_INFO l{},n{};
 if(FAILED(a->QueryVideoMemoryInfo(0,DXGI_MEMORY_SEGMENT_GROUP_LOCAL,&l))||FAILED(a->QueryVideoMemoryInfo(0,DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL,&n)))return v;
 v.ok=true;v.budget=l.Budget;v.uso=l.CurrentUsage;v.budget_nl=n.Budget;v.uso_nl=n.CurrentUsage;
 if(const int f=env_int("NR_ORC_BUDGET_MB",0);f>0)v.budget=std::uint64_t(f)*nrorc::MB;
 return v;}
inline bool hip_mem(std::uint64_t& livre,std::uint64_t& total) noexcept {
 using Info=int(*)(std::size_t*,std::size_t*);
 static const Info f=[]{HMODULE h=GetModuleHandleW(L"amdhip64_7.dll");return h?reinterpret_cast<Info>(GetProcAddress(h,"hipMemGetInfo")):nullptr;}();
 std::size_t a{},b{};if(!f||f(&a,&b))return false;livre=a;total=b;return true;}
inline std::wstring arq_calib(){return pasta()+L"nr_orcamento.ini";}
inline nrorc::Calib ler_calib() noexcept {
 nrorc::Calib c{};
 try{const auto a=arq_calib();
  c.outros_pico=std::uint64_t(GetPrivateProfileIntW(L"Orcamento",L"OutrosPicoMb",0,a.c_str()))*nrorc::MB;
  c.budget=std::uint64_t(GetPrivateProfileIntW(L"Orcamento",L"BudgetMb",0,a.c_str()))*nrorc::MB;}catch(...){}
 return c;}
inline void gravar_calib(const nrorc::Calib& c) noexcept {
 try{const auto a=arq_calib();wchar_t v[32]{};
  swprintf_s(v,L"%llu",static_cast<unsigned long long>(c.outros_pico/nrorc::MB));WritePrivateProfileStringW(L"Orcamento",L"OutrosPicoMb",v,a.c_str());
  swprintf_s(v,L"%llu",static_cast<unsigned long long>(c.budget/nrorc::MB));WritePrivateProfileStringW(L"Orcamento",L"BudgetMb",v,a.c_str());
  swprintf_s(v,L"%lu",GetCurrentProcessId());WritePrivateProfileStringW(L"Orcamento",L"Pid",v,a.c_str());}catch(...){}
}

inline void configurar(int modo,unsigned folga_mb,unsigned outros_mb) noexcept {
 auto& E=nrorc::estado();E.folga_mb.store(folga_mb);E.outros_mb.store(outros_mb);
 E.selo_min.store((std::max)(1,env_int("NR_ORC_SELO_JOB",nrorc::selo_job_padrao)));E.serie_cada.store((std::max)(1,env_int("NR_ORC_SERIE_CADA",256)));
 E.calib=ler_calib();E.pico_arquivo.store(E.calib.outros_pico);
 E.guarda.modo.store(modo,std::memory_order_release);}
inline void hip_inicio() noexcept {
 std::uint64_t l=0,t=0;if(hip_mem(l,t)){auto& E=nrorc::estado();E.hip_livre.store(l);E.hip_total.store(t);}
 log("NPC_ORCAMENTO_HIP livre_mb=%llu total_mb=%llu modo=%d",static_cast<unsigned long long>(l/nrorc::MB),static_cast<unsigned long long>(t/nrorc::MB),nrorc::modo());}
inline int formatar_decisao(char* b,std::size_t n,const nrorc::Config& c,const nrorc::Medida& m,const nrorc::Decisao& d,const Vram& v) noexcept {
 char mt[96];nrorc::motivo_texto(d.motivo,mt,sizeof mt);static const char* const fontes[]{"ini","arquivo","uso","fracao"};
 const auto M=[](std::uint64_t x){return static_cast<unsigned long long>(x/nrorc::MB);};auto& E=nrorc::estado();
 return std::snprintf(b,n,"NPC_ORCAMENTO modo=%d dxgi=%d budget_mb=%llu uso_mb=%llu nl_mb=%llu hip_livre_mb=%llu hip_total_mb=%llu outros_mb=%llu fonte=%s folga_mb=%llu livre_mb=%llu"
  " grace=%u npc=%d recorte_npc=%u total_mb=%llu motivo=%s pedido=%u/%d/%u ctx_unico=%d custos_mb grace1024=%llu grace1536=%llu grace2048=%llu npc1024=%llu npc768=%llu npc512=%llu",
  nrorc::modo(),v.ok?1:0,M(m.budget),M(m.uso),M(m.uso_nl),M(E.hip_livre.load()),M(E.hip_total.load()),M(d.outros),fontes[unsigned(d.fonte)&3u],M(m.folga),M(d.livre),
  d.cap_grace,d.k_npc,d.cap_npc,M(d.total),mt,c.teto_grace,c.npc_max,c.teto_npc,c.ctx_unico?1:0,
  M(nrorc::custo_grace(c,1024)),M(nrorc::custo_grace(c,1536)),M(nrorc::custo_grace(c,2048)),M(nrorc::custo_npc(c,1024)),M(nrorc::custo_npc(c,768)),M(nrorc::custo_npc(c,512)));}

inline void amostra(int job) noexcept {
 auto& E=nrorc::estado();const auto v=medir(adaptador());std::uint64_t hl=0,ht=0;(void)hip_mem(hl,ht);
 LARGE_INTEGER q{},f{};QueryPerformanceCounter(&q);QueryPerformanceFrequency(&f);
 const auto& G=E.guarda;const std::uint64_t ro=G.bytes_rosto();const auto C=[&](nrorc::Api a){return G.criados[unsigned(a)].load();};
 log("NPC_ORCAMENTO_SERIE job=%d qpc=%lld qpf=%lld budget_mb=%llu uso_mb=%llu nl_mb=%llu hip_livre_mb=%llu rosto_mb=%llu cena_mb=%llu tardias=%llu selado=%d criados=%llu/%llu/%llu/%llu/%llu/%llu/%llu",
  job,q.QuadPart,f.QuadPart,static_cast<unsigned long long>(v.budget/nrorc::MB),static_cast<unsigned long long>(v.uso/nrorc::MB),static_cast<unsigned long long>(v.uso_nl/nrorc::MB),
  static_cast<unsigned long long>(hl/nrorc::MB),static_cast<unsigned long long>(ro/nrorc::MB),static_cast<unsigned long long>(G.bytes[unsigned(nrorc::Origem::Cena)].load()/nrorc::MB),
  G.tardias.load(),G.selado.load()?1:0,C(nrorc::Api::HipMalloc),C(nrorc::Api::HipMallocProxy),C(nrorc::Api::Recurso),C(nrorc::Api::Pso),C(nrorc::Api::Raiz),C(nrorc::Api::Heap),C(nrorc::Api::RecriaCena));
 if(nrorc::modo()==1)log_pool(job,"serie");
 if(v.ok&&E.decidido.load(std::memory_order_acquire)&&nrorc::atualizar_pico(E.calib,nrorc::outros_agora(v.uso,v.uso_nl,E.nl_inicio.load(),ro),v.budget))gravar_calib(E.calib);}

inline void por_job(int job) noexcept {
 const int m=nrorc::modo();if(!m)return;
 job_visto.store(job,std::memory_order_relaxed);
 auto& E=nrorc::estado();
 if(nrorc::deve_selar(m,E.guarda.selado.load(std::memory_order_acquire),job,E.preparado.load(std::memory_order_acquire),E.selo_min.load(std::memory_order_relaxed))){
  E.guarda.selar(job);
  log("NPC_ORCAMENTO_SELO job=%d modo=%d rosto_mb=%llu cena_mb=%llu",job,m,static_cast<unsigned long long>(E.guarda.bytes_rosto()/nrorc::MB),
      static_cast<unsigned long long>(E.guarda.bytes[unsigned(nrorc::Origem::Cena)].load()/nrorc::MB));}
 if(job==1||job%E.serie_cada.load(std::memory_order_relaxed)==0)amostra(job);}
}
