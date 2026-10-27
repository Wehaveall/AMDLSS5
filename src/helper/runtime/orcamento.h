#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include <vector>
namespace nrorc {
inline constexpr std::uint64_t MB=1024ull*1024ull;

inline constexpr unsigned slots_rosto=6;
inline constexpr unsigned janela_grace_padrao=2304;
inline constexpr unsigned janela_npc_padrao=1536;

inline constexpr std::uint64_t bytes_compartilhado(unsigned n) noexcept {return std::uint64_t(slots_rosto)*2u*std::uint64_t(n)*n*8u+4096u;}

inline constexpr std::uint64_t bytes_hist(unsigned janela) noexcept {return 2u*(64u+std::uint64_t(janela)*janela*16u);}

inline constexpr std::uint64_t bytes_grades(unsigned n) noexcept {const std::uint64_t n2=std::uint64_t(n)*n;return n2*48u+(n2/16u)*32u+n2*12u+16u;}
inline constexpr std::uint64_t bytes_lin(unsigned n) noexcept {return std::uint64_t(n)*n*4u;}
inline constexpr std::uint64_t bytes_passo2(unsigned n) noexcept {return 2u*std::uint64_t(n)*n*8u;}

inline constexpr std::uint64_t bytes_ruido(unsigned n) noexcept {return std::uint64_t(n)*n*4u+32u;}

struct Contexto {std::uint64_t fixo{},por_px8{};};
inline constexpr std::uint64_t bytes_contexto(const Contexto& c,unsigned n) noexcept {return c.fixo+(c.por_px8*std::uint64_t(n)*n+7u)/8u;}

inline constexpr std::uint64_t rede_lado_px8=1357u,rede_grande_px8=1024u;
inline constexpr std::uint64_t bytes_rede_lado(unsigned n) noexcept {return (rede_lado_px8*std::uint64_t(n)*n+7u)/8u;}
inline constexpr std::uint64_t bytes_rede_grande(unsigned n) noexcept {return rede_grande_px8*std::uint64_t(n)*n/8u;}
inline constexpr std::uint64_t ctx_fixo_bytes=147686204ull;
inline constexpr std::uint64_t ctx_por_px8=rede_lado_px8+rede_grande_px8;
static_assert(ctx_por_px8==2381u,"297,625 B/px (FASE1-MEDIDAS.md D2)");
inline constexpr Contexto contexto_medido{ctx_fixo_bytes,ctx_por_px8};

struct Config {
 unsigned teto_grace{1024};

 bool grace_fixa{};
 std::uint64_t npc_reserva{};
 unsigned janela_grace{janela_grace_padrao},janela_npc{janela_npc_padrao};
 bool npc{};int npc_max{0};unsigned teto_npc{1024};
 bool acumula{},npc_acumula{true},frescor{};unsigned passos{1};
 bool ruido_grace{},ruido_npc{};
 bool ctx_unico{true};
 int perto{0};unsigned jan_peq{0};
 bool todos{};unsigned todos_rede{768},todos_jan{768};int todos_max{2};
 bool leve{};unsigned leve_rede{512};
 Contexto ctx{};
 std::uint64_t fixo{};

 std::uint64_t camada{},detector{},copia{};
};
inline constexpr std::uint64_t bytes_camada(unsigned w,unsigned h) noexcept {return std::uint64_t(w)*h*16u;}
inline constexpr std::uint64_t bytes_detector(std::uint64_t cheia,std::uint64_t grade,unsigned conjuntos) noexcept {return std::uint64_t(conjuntos)*(cheia*4u+grade*12u);}
inline constexpr std::uint64_t bytes_copia(unsigned janela,unsigned bpp) noexcept {return std::uint64_t(janela)*janela*bpp;}
struct Medida {
 std::uint64_t budget{},uso{},uso_nl{};
 std::uint64_t outros_ini{};
 std::uint64_t outros_pico{};
 std::uint64_t folga{256*MB};
 unsigned fracao_outros_pct{92};
 std::uint64_t budget_pior{};
};
enum Motivo:unsigned{Cabe=0,GraceMenor=1u,NpcMenor=2u,MenosNpc=4u,SemNpc=8u,Insuficiente=16u,SemMedida=32u,GraceFixaAperto=64u};
enum class FonteOutros:unsigned{Ini=0,Arquivo=1,Uso=2,Fracao=3};
struct Decisao {
 unsigned cap_grace{},cap_npc{};int k_npc{};unsigned motivo{};FonteOutros fonte{FonteOutros::Fracao};
 std::uint64_t outros{},livre{},grace{},npc_cada{},npc_comum{},total{};
 int k_perto{};unsigned cap_perto{};unsigned jan_peq{};std::uint64_t npc_perto{};unsigned jan_perto{};
};
inline std::uint64_t custo_grace(const Config& c,unsigned g) noexcept {
 return bytes_compartilhado(g)+(c.acumula?bytes_hist(c.janela_grace):0u)+bytes_grades(g)+(c.acumula&&c.frescor?bytes_lin(g):0u)
       +(c.passos>=2?bytes_passo2(g):0u)+bytes_contexto(c.ctx,g)+c.fixo+c.camada+c.detector+c.copia+(c.ruido_grace?bytes_ruido(g):0u);}
inline std::uint64_t custo_npc(const Config& c,unsigned r) noexcept {return bytes_compartilhado(r)+(c.acumula&&c.npc_acumula?bytes_hist(c.janela_npc):0u)+(c.ruido_npc?bytes_ruido(r):0u);}
inline std::uint64_t custo_npc_comum(const Config& c,unsigned r) noexcept {return c.ctx_unico?0u:bytes_contexto(c.ctx,r);}
inline std::uint64_t custo_total(const Config& c,unsigned g,unsigned r,int k) noexcept {
 return custo_grace(c,g)+(k>0?custo_npc_comum(c,r)+std::uint64_t(k)*custo_npc(c,r):0u);}

inline std::uint64_t outros(const Medida& m,FonteOutros& f) noexcept {
 if(m.outros_ini){f=FonteOutros::Ini;return m.outros_ini;}
 const std::uint64_t base=m.outros_pico?m.outros_pico:m.budget/100u*m.fracao_outros_pct;
 if(m.uso>base){f=FonteOutros::Uso;return m.uso;}
 f=m.outros_pico?FonteOutros::Arquivo:FonteOutros::Fracao;return base;}
inline constexpr int npc_k_alvo=3;
inline Decisao decidir(const Config& c,const Medida& m) noexcept {
 Decisao d{};
 const unsigned piso=(std::min)(c.teto_grace,1024u);
 const bool quer_npc=c.npc&&c.npc_max>0;
 if(!m.budget){d.cap_grace=piso;d.motivo=SemMedida|(quer_npc?SemNpc:0u);d.grace=custo_grace(c,piso);d.total=d.grace;return d;}
 d.outros=outros(m,d.fonte);
 d.livre=m.budget>d.outros+m.folga?m.budget-d.outros-m.folga:0u;

 if(c.grace_fixa){d.cap_grace=c.teto_grace;if(custo_grace(c,d.cap_grace)>d.livre)d.motivo|=GraceFixaAperto;}
 else{
 const unsigned gs[3]={c.teto_grace,1536u,1024u};
 for(unsigned g:gs)if(g<=c.teto_grace&&custo_grace(c,g)<=d.livre){d.cap_grace=g;break;}
 if(!d.cap_grace){d.cap_grace=piso;d.motivo|=Insuficiente;}
 else if(d.cap_grace<c.teto_grace)d.motivo|=GraceMenor;
 }
 d.grace=custo_grace(c,d.cap_grace);

 std::uint64_t livre_n=d.livre;
 if(c.grace_fixa&&m.budget_pior&&m.budget_pior<m.budget)livre_n=(std::min)(livre_n,m.budget_pior>d.outros+m.folga?m.budget_pior-d.outros-m.folga:std::uint64_t{0});

 if(quer_npc&&!(d.motivo&Insuficiente)){

  const std::uint64_t sobra=(c.grace_fixa&&c.npc_reserva)?c.npc_reserva:(livre_n>d.grace?livre_n-d.grace:0u);const int alvo=c.leve?c.npc_max:(std::min)(c.npc_max,npc_k_alvo);
  const unsigned lr=c.leve_rede>=384u?c.leve_rede:512u;
  const unsigned rs[3]={c.leve?(std::min)(c.teto_npc,lr):c.teto_npc,c.leve?384u:768u,c.leve?384u:512u};int bk=0;unsigned br=0;
  for(unsigned r:rs){
   if(r>c.teto_npc||r>d.cap_grace||r<256u)continue;
   const std::uint64_t comum=custo_npc_comum(c,r),cada=custo_npc(c,r);
   const int k=sobra>comum?int((std::min)(std::uint64_t(c.npc_max),(sobra-comum)/cada)):0;
   if(k>bk){bk=k;br=r;}
   if(k>=alvo)break;}
  d.k_npc=bk;d.cap_npc=bk?br:0u;
  if(!bk)d.motivo|=SemNpc;
  else{if(br<c.teto_npc)d.motivo|=NpcMenor;if(bk<c.npc_max)d.motivo|=MenosNpc;}
 }else if(quer_npc)d.motivo|=SemNpc;
 d.npc_cada=d.k_npc?custo_npc(c,d.cap_npc):0u;d.npc_comum=d.k_npc?custo_npc_comum(c,d.cap_npc):0u;
 d.total=custo_total(c,d.cap_grace,d.cap_npc,d.k_npc);

 if(c.perto>0&&d.k_npc>0){const unsigned jp=c.jan_peq?c.jan_peq:c.janela_npc;
  auto cp=[&](unsigned r,unsigned j){return bytes_compartilhado(r)+(c.acumula&&c.npc_acumula?bytes_hist(j):0u);};
  const std::uint64_t base=d.grace+d.npc_comum,peq=cp(d.cap_npc,jp);
  const int ok[4]={2,2,1,1};const unsigned orr[4]={1024u,768u,1024u,768u};
  for(int i=0;i<4;++i){if(ok[i]>c.perto||ok[i]>d.k_npc||orr[i]>d.cap_grace||orr[i]<=d.cap_npc)continue;
   const std::uint64_t t=base+std::uint64_t(ok[i])*cp(orr[i],c.janela_npc)+std::uint64_t(d.k_npc-ok[i])*peq;
   if(t<=livre_n){d.k_perto=ok[i];d.cap_perto=orr[i];d.jan_peq=jp;d.npc_perto=cp(orr[i],c.janela_npc);d.npc_cada=peq;d.total=t;break;}}}

 if(c.todos&&d.k_npc>0&&c.todos_rede>d.cap_npc&&c.todos_rede<=d.cap_grace){
  auto cp=[&](unsigned r,unsigned j){return bytes_compartilhado(r)+(c.acumula&&c.npc_acumula?bytes_hist(j):0u);};
  const std::uint64_t base=d.grace+d.npc_comum,peq=cp(d.cap_npc,c.janela_npc),gr=cp(c.todos_rede,c.todos_jan);
  for(int k=(std::min)(c.todos_max,d.k_npc);k>=1;--k){const std::uint64_t t=base+std::uint64_t(k)*gr+std::uint64_t(d.k_npc-k)*peq;
   if(t<=livre_n){d.k_perto=k;d.cap_perto=c.todos_rede;d.jan_perto=c.todos_jan;d.jan_peq=c.janela_npc;d.npc_perto=gr;d.npc_cada=peq;d.total=t;break;}}}
 return d;}
inline void motivo_texto(unsigned m,char* b,std::size_t n) noexcept {
 static const char* const nomes[]{"grace_menor","npc_menor","menos_npc","sem_npc","insuficiente","sem_medida","grace_fixa_aperto"};
 std::size_t k=0;if(!n)return;b[0]=0;
 for(unsigned i=0;i<7;++i)if(m&(1u<<i)){const int r=std::snprintf(b+k,n-k,"%s%s",k?"+":"",nomes[i]);if(r<0||std::size_t(r)>=n-k)break;k+=std::size_t(r);}
 if(!k)std::snprintf(b,n,"cabe");}

struct Pool {
 std::uint64_t por_inst{};int k{};
 static constexpr std::uint64_t alinha=65536u;
 std::uint64_t passo() const noexcept {return (por_inst+alinha-1u)/alinha*alinha;}
 std::uint64_t por_perto{};int k_perto{};
 std::uint64_t passo_p() const noexcept {return (por_perto+alinha-1u)/alinha*alinha;}
 std::uint64_t offset(int inst) const noexcept {if(!(inst>=1&&inst<=k))return ~0ull;const int kp=(std::min)(k_perto,k);
  return inst<=kp?std::uint64_t(inst-1)*passo_p():std::uint64_t(kp)*passo_p()+std::uint64_t(inst-1-kp)*passo();}
 std::uint64_t total() const noexcept {const int kp=(std::min)(k_perto,k);return k>0?std::uint64_t(kp)*passo_p()+std::uint64_t(k-kp)*passo():0u;}
};

enum class Api:unsigned{HipMalloc,HipMallocProxy,Recurso,Pso,Raiz,Heap,RecriaCena,n};
enum class Origem:unsigned{Cena,Rosto,Npc,F9,RedeRosto,n};
inline const char* nome(Api a) noexcept {static const char* const t[]{"hipMalloc","hipMalloc_proxy","CreateCommittedResource","CreateComputePipelineState","CreateRootSignature","CreateDescriptorHeap","recria_cena"};
 return unsigned(a)<unsigned(Api::n)?t[unsigned(a)]:"?";}
inline const char* nome(Origem o) noexcept {static const char* const t[]{"cena","rosto","npc","f9","rede_rosto"};return unsigned(o)<unsigned(Origem::n)?t[unsigned(o)]:"?";}
inline thread_local Origem origem_tl=Origem::Cena;

inline thread_local int rede_rosto_tl=0;
struct EscopoRedeRosto {EscopoRedeRosto() noexcept {++rede_rosto_tl;}~EscopoRedeRosto(){--rede_rosto_tl;}
 EscopoRedeRosto(const EscopoRedeRosto&)=delete;EscopoRedeRosto& operator=(const EscopoRedeRosto&)=delete;};
struct EscopoOrigem {Origem ant;explicit EscopoOrigem(Origem o) noexcept:ant(origem_tl){origem_tl=o;}~EscopoOrigem(){origem_tl=ant;}
 EscopoOrigem(const EscopoOrigem&)=delete;EscopoOrigem& operator=(const EscopoOrigem&)=delete;};
struct Guarda {
 std::atomic<int> modo{0};std::atomic<bool> selado{false};std::atomic<int> selo_job{0};
 std::atomic<unsigned long long> tardias{0};
 std::atomic<std::uint64_t> bytes[unsigned(Origem::n)]{};
 std::atomic<unsigned long long> criados[unsigned(Api::n)]{};

 bool registrar(Api a,Origem o,std::uint64_t n) noexcept {
  if(!modo.load(std::memory_order_relaxed))return false;
  bytes[unsigned(o)].fetch_add(n,std::memory_order_relaxed);criados[unsigned(a)].fetch_add(1,std::memory_order_relaxed);
  if(!selado.load(std::memory_order_acquire))return false;
  tardias.fetch_add(1,std::memory_order_relaxed);return true;}

 void liberar(Origem o,std::uint64_t n) noexcept {
  if(!modo.load(std::memory_order_relaxed)||unsigned(o)>=unsigned(Origem::n))return;
  auto& b=bytes[unsigned(o)];std::uint64_t v=b.load(std::memory_order_relaxed);
  while(!b.compare_exchange_weak(v,v>n?v-n:0u,std::memory_order_relaxed)){}}
 void selar(int job) noexcept {if(!selado.exchange(true,std::memory_order_acq_rel))selo_job.store(job,std::memory_order_relaxed);}
 std::uint64_t bytes_rosto() const noexcept {return bytes[unsigned(Origem::Rosto)].load()+bytes[unsigned(Origem::Npc)].load()+bytes[unsigned(Origem::RedeRosto)].load();}
};

struct Vivos {
 struct V {std::uint64_t n{};Origem o{Origem::Cena};};
 std::unordered_map<void*,V> m;

 bool criar(void* p,std::uint64_t n,Origem o,V& velho) {
  if(!p||!n)return false;const auto it=m.find(p);
  if(it!=m.end()){velho=it->second;it->second=V{n,o};return true;}
  m.emplace(p,V{n,o});return false;}
 bool soltar(void* p,V& v) {if(!p)return false;const auto it=m.find(p);if(it==m.end())return false;v=it->second;m.erase(it);return true;}
 std::uint64_t vivos(Origem o) const noexcept {std::uint64_t s=0;for(const auto& x:m)if(x.second.o==o)s+=x.second.n;return s;}
};
inline int linha_tardia(char* b,std::size_t n,unsigned long long seq,Api a,Origem o,std::uint64_t bytes,const char* sitio,int job) noexcept {
 return std::snprintf(b,n,"NPC_ALOCACAO_TARDIA n=%llu api=%s origem=%s bytes=%llu sitio=%s job=%d",seq,nome(a),nome(o),static_cast<unsigned long long>(bytes),sitio?sitio:"-",job);}
inline constexpr int selo_job_padrao=8;

inline bool deve_selar(int modo,bool selado,int job,bool preparado,int selo_min) noexcept {return modo>0&&!selado&&job>=selo_min&&(modo==2||preparado);}

struct Lados {unsigned n[2]{};int quantos{};};
inline Lados lados_aquecer(const Decisao& d,bool ctx_unico) noexcept {Lados l{};l.n[l.quantos++]=d.cap_grace;if(d.k_npc>0&&!ctx_unico)l.n[l.quantos++]=d.cap_npc;return l;}

inline constexpr unsigned lado_passo=128u,lado_piso=256u;
inline int lados_ordem(unsigned teto,unsigned piso,unsigned* out,int max) noexcept {
 int k=0;if(!teto||!out)return 0;for(unsigned n=teto;n>=piso&&n>=lado_passo&&k<max;n-=lado_passo)out[k++]=n;return k;}

struct PoolRede {
 struct Bloco {void* p{};std::uint64_t cap{};};
 struct Vivo {std::uint64_t cap{},pedido{};};
 std::vector<Bloco> livres;std::unordered_map<void*,Vivo> vivos;
 std::uint64_t bytes{};unsigned long long servidos{},reais{},devolvidos{};
 void* servir(std::uint64_t n) {
  std::size_t b=livres.size();
  for(std::size_t i=0;i<livres.size();++i){const auto& x=livres[i];
   if(x.cap>=n&&(b==livres.size()||x.cap<livres[b].cap||(x.cap==livres[b].cap&&x.p<livres[b].p)))b=i;}
  if(b==livres.size())return nullptr;
  const Bloco x=livres[b];livres.erase(livres.begin()+std::ptrdiff_t(b));vivos[x.p]=Vivo{x.cap,n};++servidos;return x.p;}
 void registrar(void* p,std::uint64_t cap) {if(!p)return;vivos[p]=Vivo{cap,cap};bytes+=cap;++reais;}
 bool devolver(void* p) {
  if(!p)return false;const auto it=vivos.find(p);if(it==vivos.end())return false;
  livres.push_back({p,it->second.cap});vivos.erase(it);++devolvidos;return true;}
 std::uint64_t pedido_de(void* p) const {const auto it=vivos.find(p);return it==vivos.end()?0u:it->second.pedido;}
 std::uint64_t bytes_livres() const noexcept {std::uint64_t s=0;for(const auto& x:livres)s+=x.cap;return s;}
};

struct Calib {std::uint64_t outros_pico{},budget{};};
inline std::uint64_t outros_agora(std::uint64_t uso,std::uint64_t uso_nl,std::uint64_t nl_inicio,std::uint64_t rosto) noexcept {
 const std::uint64_t dem=uso+(uso_nl>nl_inicio?uso_nl-nl_inicio:0u);return dem>rosto?dem-rosto:0u;}
inline bool atualizar_pico(Calib& c,std::uint64_t outros_v,std::uint64_t budget) noexcept {if(outros_v<=c.outros_pico)return false;c.outros_pico=outros_v;c.budget=budget;return true;}

struct Estado {
 Guarda guarda;
 std::atomic<bool> ini_ok{false},decidido{false},d3d_pronto{false},preparado{false};
 std::atomic<unsigned> folga_mb{256},outros_mb{0};
 std::atomic<int> selo_min{selo_job_padrao},serie_cada{256};
 std::atomic<std::uint64_t> hip_livre{0},hip_total{0},nl_inicio{0},pico_arquivo{0};
 Config cfg{};Medida med{};Decisao dec{};
 Calib calib{};

 std::atomic<std::uint64_t> calib_pausa_ate{0};
};
inline Estado& estado() noexcept {static Estado* e=new Estado;return *e;}
inline int modo() noexcept {return estado().guarda.modo.load(std::memory_order_relaxed);}
inline bool aplicada() noexcept {return modo()==1&&estado().decidido.load(std::memory_order_acquire);}
inline const Decisao& decisao() noexcept {return estado().dec;}
inline void publicar(const Config& c,const Medida& m,const Decisao& d) noexcept {auto& e=estado();e.cfg=c;e.med=m;e.dec=d;e.decidido.store(true,std::memory_order_release);}
}
