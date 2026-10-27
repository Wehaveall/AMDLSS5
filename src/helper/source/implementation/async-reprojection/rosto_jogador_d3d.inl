#include "../../../runtime/rosto_jogador_state.h"
#include "../../../runtime/camada_dela.h"
#include "../../../runtime/rosto_cor_luz.h"
#include "../../../runtime/reuso_state.h"
#include "../../../runtime/orcamento_vram.h"
#include "../../../runtime/geometria_nativa.h"
#include "../../../runtime/rosto_camada_fila.h"
#include "../../../runtime/jogador_externo.h"
#include <thread>
#include <deque>
#include <climits>
#include "nr_reproject_rjrecorte.h"
#include "nr_reproject_rjcompor.h"
#include "nr_reproject_rjgrade.h"
#include "nr_reproject_rjcomporr.h"
#include "nr_reproject_rjcamada.h"
#include "nr_reproject_rjcaminho.h"
#include "nr_reproject_rjcabeca.h"
#include "nr_reproject_rjrostoimg.h"
#include "nr_reproject_rjruido.h"
#include "nr_reproject_rjrecorteruido.h"
namespace nrreproject {
namespace {
struct RjK{unsigned w,h;int rx,ry;unsigned lado,n,off_ent,off_tag;unsigned job,idade,fatia,mascara;unsigned historico;int fx,fy;unsigned fw;
           unsigned fh;float expo;unsigned tonemap;float ganho;float hx,hy,prof_rel,prof_abs;float camada0,camada1;unsigned off_ent2,off_tag2;
           unsigned job2,idade2;int rx2,ry2;unsigned lado2,n2;float hx2,hy2;float peso,limiar;unsigned sozinho,pad3;
           float a_baixa,a_alta,gama;unsigned acum;unsigned quadro,geracao,bytes_meio,janela;
           unsigned rap_r,pad_r0,pad_r1,pad_r2;
           float bx_forca,a_lenta,a_rapida,mov_px;
           float cor_hx,cor_hy,cor_hl,pos_mov;
           unsigned off_t0;
           float gama_parado;};
static_assert(sizeof(RjK)==248);

void rj_aposentar(ComPtr<ID3D12Resource>& r) noexcept;
struct RjLimpa{ComPtr<ID3D12Resource> r;DXGI_FORMAT fmt{};UINT64 w=0;UINT h=0;std::uint64_t quadro=~0ull,preparado=~0ull;bool falhou=false,em_copia_dest=true;unsigned long long copias=0,regioes=0,sem_rosto=0;};
inline RjLimpa& rj_limpa() noexcept {static RjLimpa L;return L;}

constexpr unsigned rj_janela=2304,rj_hcab=64;

inline unsigned rj_jan() noexcept {
 if(nrrj::canon2_inst()&&nrorc::aplicada()){const auto& d=nrorc::decisao();if(d.k_perto>0&&nrrj::inst_atual()>d.k_perto&&d.jan_peq)return d.jan_peq;}
 if(nrrj::todos_inst()&&nrorc::aplicada()){const auto& d=nrorc::decisao();if(d.k_perto>0&&nrrj::inst_atual()<=d.k_perto&&d.jan_perto)return d.jan_perto;}
 return nrrj::inst_atual()>=1?((nrrj::npc_leve.load(std::memory_order_relaxed)&1)?nrrj::npc_leve_jan.load(std::memory_order_relaxed):1536u):rj_janela;
}
inline UINT64 rj_hist_b() noexcept {return 2ull*(rj_hcab+UINT64(rj_jan())*rj_jan()*16ull);}

constexpr unsigned rj_srvs=3,rj_uavs=4,rj_set=rj_srvs+rj_uavs,rj_passes=6u+unsigned(nrrj::npc_slots)+4u,rj_aneis=8,rj_compor_max=3;
constexpr unsigned rj_passe_cabeca=6u+unsigned(nrrj::npc_slots),rj_passe_pontos=rj_passe_cabeca+1u;
struct RjCabK{int x0,y0;float cw;unsigned n,fatia,inv,off,pad0;};
constexpr unsigned rj_passe_caminho=rj_passes-1,rj_passe_camada=rj_passes-2;
struct RjGradeK{unsigned gw,gh,off_d,off_mv,fatia,inv,pad0,pad1;};
static_assert(sizeof(RjGradeK)==32);
struct RjGlobal{
 ComPtr<ID3D12Device> device;ComPtr<ID3D12RootSignature> root;ComPtr<ID3D12PipelineState> recorte,compor,compor_r;bool compor_r_tentado{};

 std::uint64_t seg_quadro{},seg_ger{},segurados{};unsigned seg_n{};int seg_fx{},seg_fy{};unsigned seg_fw{},seg_fh{},seg_ow{},seg_oh{};
 ComPtr<ID3D12DescriptorHeap> heap;unsigned stride{};
 ComPtr<ID3D12Resource> compartilhado,marcador;
 UINT64 off_comp{},off_hist{};

 ComPtr<ID3D12PipelineState> grade_pso;ComPtr<ID3D12Resource> grade_buf;bool grade_pronto{},grade_falhou{};std::uint64_t grade_seq{};
 ComPtr<ID3D12Resource> hist;bool hist_tentado{};int job_ult{};std::uint64_t quadro_troca{};float k_quadros{4.f};std::uint64_t acumulados{};
 bool pronto{},pre_logado{};std::uint64_t quadro{},seq{};unsigned variante{};
 nrrj::Rampa rampa;bool composto_ant{},composto{};
 std::uint64_t recortes{},composicoes{},quadros_compostos{},sem_slot{},recusas{},sem_delta{},geracao_velha{},medias{},reusos{},pulados_cada{},teto_tela_n{};bool teto_tela_avisou{};

 unsigned jobs_sem{};std::uint64_t ger_recorte{};

 std::uint64_t quadro_recorte{},por_quadros{},sem_rosto{};unsigned seg_motivo{};
 unsigned n_ult{};std::uint64_t compor_elipse_n{},compor_px{};
 nrrj::RedeAdapt adapt{};unsigned adapt_log{};std::uint64_t adapt_trocas{};
 unsigned geo_log_de{},geo_log_antes{};
 std::uint64_t epoca_vista{},epocas{};

 ComPtr<ID3D12PipelineState> caminho_pso;ComPtr<ID3D12Resource> caminho_buf,caminho_rbuf;bool caminho_tentado{};std::uint64_t idade_hist[18]{};std::uint64_t sj_compostos{};

 ComPtr<ID3D12Resource> copia;DXGI_FORMAT copia_fmt{};unsigned copia_w{},copia_h{};bool copia_falhou{};std::uint64_t tetos{};std::uint64_t cor0_n{};

 std::uint64_t limpa_q{~0ull};int limpa_x0{},limpa_y0{};unsigned limpa_w{},limpa_h{};

 ComPtr<ID3D12PipelineState> rosto_pso;ComPtr<ID3D12Resource> rosto_buf,rosto_rb;bool rosto_tentado{},rosto_ok{};
 std::uint32_t rosto_seq{};unsigned rosto_i{};LONGLONG rosto_qpc{};
 ComPtr<ID3D12Resource> roi_buf[nrrj::npc_slots],roi_rb[nrrj::npc_slots];bool roi_tentado{},roi_ok{};std::uint32_t roi_seq[nrrj::npc_slots]{};unsigned roi_i[nrrj::npc_slots]{};LONGLONG roi_qpc{};
 unsigned rosto_n{};
 std::uint64_t ger_visto{},quadro_ger{};bool ger_pend{};

 ComPtr<ID3D12PipelineState> cab_pso;ComPtr<ID3D12Resource> cab_buf,cab_rb;bool cab_tentado{},cab_ok{};const float* cab_m{};std::uint32_t cab_seq{};
 struct CabEnt{std::uint32_t seq{};std::uint64_t quadro{};float x0{},y0{},cw{},esc{},z{},lado{},px{},py{},jx{},jy{};bool lido{true};} cab_ent[nrrj::cab_aneis];
 nrrj::CabecaMedida cab;
 std::uint64_t cab_medidas{},cab_recusas{},cab_usadas{},cab_disp{};
 ComPtr<ID3D12Resource> p3_buf,p3_rb;bool p3_ok{};

 ComPtr<ID3D12PipelineState> ruido_pso,recorte_ruido_pso;ComPtr<ID3D12Resource> ruido_buf;unsigned ruido_jan{};bool ruido_tentado{},ruido_falhou{};
 std::uint64_t ruido_q{~0ull},ruido_passes{},ruido_px{},ruido_recortes{},ruido_grande{};unsigned ruido_e{1};
 std::uint64_t pulados_agenda{};
};
RjGlobal& rjg(){static auto* g=new RjGlobal[nrrj::n_inst];return g[nrrj::inst_atual()];}

static void rj_agenda_planejar(bool sem_job,std::uint64_t quadro) noexcept {
 try{
  nrag::Cand c[nrrj::npc_slots]{};const int nn=nrrj::npc_n();const unsigned kmax=nrrj::npc_orc_kmax.load(std::memory_order_relaxed);
  for(int k=1;k<=nn;++k){nrrj::Inst ik(k);auto& Gk=rjg();const auto d=nrrj::ler_deteccao();const auto ger=nrrj::ger().load(std::memory_order_acquire);
   auto& ck=c[k-1];
   ck.quer=!sem_job&&nrrj::ativo()&&(d.on||Gk.rampa.nivel>0.f)&&d.tem&&!d.pend;
   ck.forcado=nrag::forcado(Gk.ger_recorte!=ger,Gk.recortes==0,Gk.jobs_sem,kmax);
   ck.cab4k=d.cab4k;ck.jobs_sem=Gk.jobs_sem;
   const unsigned cap=(std::max)(nrrj::n_cap(),256u),nguess=Gk.n_ult?Gk.n_ult:(std::min)((std::max)(d.rl&~127u,256u),cap);
   ck.custo_ms=nrrj::custo_previsto(k,nguess);}
  const auto pl=nrag::planejar(c,unsigned(nn),nrrj::npc_orc_ms.load(std::memory_order_relaxed),nrrj::npc_orc_grande.load(std::memory_order_relaxed));
  nrrj::agenda_bits.store(pl.bits<<1,std::memory_order_relaxed);nrrj::agenda_quadro.store(quadro,std::memory_order_release);
  auto& A=nrrj::agenda_conta();const auto np=A.planos.fetch_add(1,std::memory_order_relaxed)+1;
  A.querem.fetch_add(pl.querem,std::memory_order_relaxed);A.escolhidos.fetch_add(pl.escolhidos,std::memory_order_relaxed);A.forcados.fetch_add(pl.forcados,std::memory_order_relaxed);
  A.grandes.fetch_add(pl.grandes,std::memory_order_relaxed);A.adiados.fetch_add(pl.adiados,std::memory_order_relaxed);A.usado.store(A.usado.load(std::memory_order_relaxed)+pl.usado_ms,std::memory_order_relaxed);
  if(np<=4||(np&255u)==0){const double q=double(np);char l[300]{};NRLOGF(l,sizeof l,"ROSTO_NPC_AGENDA planos=%llu por_job: querem=%.2f escolhidos=%.2f forcados=%.2f grandes=%.2f adiados=%.2f usado_ms=%.1f orcamento_ms=%.1f max_jobs=%u grande_px=%.0f",
   static_cast<unsigned long long>(np),double(A.querem.load())/q,double(A.escolhidos.load())/q,double(A.forcados.load())/q,double(A.grandes.load())/q,double(A.adiados.load())/q,
   A.usado.load()/q,double(nrrj::npc_orc_ms.load()),kmax,double(nrrj::npc_orc_grande.load()));nrrj::log(l);}
 }catch(...){nrrj::agenda_quadro.store(0,std::memory_order_relaxed);}
}

template<class Impl> int& rj_slot_de(Impl& p){
 if(nrrj::inst_atual()>=1)return p.rj_slotn[nrrj::inst_atual()-1];
 return p.rj_slot;}
bool rj_buffer(ID3D12Device* d,D3D12_HEAP_TYPE tipo,UINT64 bytes,D3D12_RESOURCE_STATES estado,D3D12_RESOURCE_FLAGS flags,ComPtr<ID3D12Resource>& r,const wchar_t* nome,D3D12_HEAP_FLAGS hf=D3D12_HEAP_FLAG_NONE){
 D3D12_HEAP_PROPERTIES hp{};hp.Type=tipo;D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=bytes;bd.Height=1;bd.DepthOrArraySize=1;bd.MipLevels=1;
 bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;bd.Flags=flags;
 const bool ok=SUCCEEDED(d->CreateCommittedResource(&hp,hf,&bd,estado,nullptr,IID_PPV_ARGS(&r)));
 if(ok)nrd3d::nomear(r.Get(),nome);
 if(ok)nrorc_vram::registrar(nrorc::Api::Recurso,tipo==D3D12_HEAP_TYPE_DEFAULT?bytes:0u,"rosto");
 return ok;
}
D3D12_CPU_DESCRIPTOR_HANDLE rj_cpu(RjGlobal& G,unsigned i){auto h=G.heap->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(i)*G.stride;return h;}
unsigned rj_base(RjGlobal& G,unsigned passe){return ((unsigned(G.quadro%rj_aneis))*rj_passes+passe)*rj_set;}

template<class Impl> void rj_srv_hist(RjGlobal& G,Impl& p,unsigned base,bool mv=true){
 D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
 sv.Texture2DArray.MipLevels=1;sv.Texture2DArray.ArraySize=Policy::history_count;
 if(mv){sv.Format=DXGI_FORMAT_R16G16_FLOAT;G.device->CreateShaderResourceView(p.history[1].Get(),&sv,rj_cpu(G,base+1));}
 sv.Format=DXGI_FORMAT_R32_FLOAT;G.device->CreateShaderResourceView(p.history[2].Get(),&sv,rj_cpu(G,base+2));}

template<class Impl> void rj_k_base(const Impl& p,RjK& k,float exposure,unsigned tone){
 k.fatia=p.constants.slice;k.mascara=p.constants.mask;k.historico=Policy::history_count;k.expo=exposure;k.tonemap=tone&1u;
 k.prof_rel=p.constants.depth_relative;k.prof_abs=p.constants.depth_absolute;

 k.camada0=nrrj::borda_camada_z0.load(std::memory_order_relaxed);k.camada1=nrrj::borda_camada_z1.load(std::memory_order_relaxed);}
void rj_nulos(RjGlobal& G,unsigned base){
 {D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Texture2D.MipLevels=1;
  G.device->CreateShaderResourceView(nullptr,&v,rj_cpu(G,base));}
 {D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=DXGI_FORMAT_R16G16_FLOAT;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
  v.Texture2DArray.MipLevels=1;v.Texture2DArray.ArraySize=Policy::history_count;G.device->CreateShaderResourceView(nullptr,&v,rj_cpu(G,base+1));}
 {D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_FLOAT;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
  v.Texture2DArray.MipLevels=1;v.Texture2DArray.ArraySize=Policy::history_count;G.device->CreateShaderResourceView(nullptr,&v,rj_cpu(G,base+2));}
 {D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.NumElements=1;v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
  G.device->CreateUnorderedAccessView(nullptr,nullptr,&v,rj_cpu(G,base+3));}
 {D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;  G.device->CreateUnorderedAccessView(nullptr,nullptr,&v,rj_cpu(G,base+4));}
 {D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.NumElements=1;v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
  G.device->CreateUnorderedAccessView(nullptr,nullptr,&v,rj_cpu(G,base+5));}
 {D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.NumElements=1;v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
  G.device->CreateUnorderedAccessView(nullptr,nullptr,&v,rj_cpu(G,base+6));}
}

struct RjCamadaG{
 ComPtr<ID3D12Resource> buf;unsigned w{},h{};ComPtr<ID3D12PipelineState> pso;bool tentado{},falhou{};unsigned falhas{};
 std::uint64_t quadro_ult{~0ull},passes{},px{};int Bm[2][4]{};
 nrrjcam::Fila fila;
};
RjCamadaG& rjc(){static auto* g=new RjCamadaG;return *g;}

constexpr unsigned rj_apos_margem=3;
struct RjAposentadas{std::mutex m;std::vector<std::pair<ComPtr<ID3D12Resource>,std::uint64_t>> v;std::uint64_t capturas{};};
RjAposentadas& rj_apos(){static auto* a=new RjAposentadas;return *a;}
void rj_aposentar(ComPtr<ID3D12Resource>& r) noexcept {
 if(!r)return;auto& A=rj_apos();
 try{std::lock_guard<std::mutex> l(A.m);A.v.emplace_back(std::move(r),A.capturas+rj_apos_margem);}catch(...){}
 r.Reset();}
void rj_apos_passo() noexcept {
 auto& A=rj_apos();
 try{std::lock_guard<std::mutex> l(A.m);++A.capturas;
  if(!A.v.empty())A.v.erase(std::remove_if(A.v.begin(),A.v.end(),[&](const auto& x){return x.second<=A.capturas;}),A.v.end());}catch(...){}}

bool rj_copia_garantir(RjGlobal& G,DXGI_FORMAT fr) noexcept {
 if(G.copia&&G.copia_fmt==fr)return true;
 if(G.copia_falhou||!G.device)return false;
 rj_aposentar(G.copia);
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=rj_jan();td.Height=rj_jan();
 td.DepthOrArraySize=1;td.MipLevels=1;td.Format=fr;td.SampleDesc.Count=1;td.Layout=D3D12_TEXTURE_LAYOUT_UNKNOWN;
 if(FAILED(G.device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,read_state,nullptr,IID_PPV_ARGS(&G.copia)))){G.copia.Reset();G.copia_falhou=true;nrrj::log("ROSTO_JOGADOR_TETO sem memoria para a copia: desligado");return false;}
 G.copia_fmt=fr;G.copia_w=unsigned(td.Width);G.copia_h=td.Height;G.limpa_q=~0ull;nrd3d::nomear(G.copia.Get(),L"nr_rosto_copia");nrrj::log("ROSTO_JOGADOR_TETO copia criada");
 nrorc_vram::registrar(nrorc::Api::Recurso,G.device->GetResourceAllocationInfo(0,1,&td).SizeInBytes,"rosto_copia");
 return true;}
void rj_limpa_preparar(ID3D12GraphicsCommandList* c,ID3D12Resource* dst,unsigned estado,std::uint64_t quadro) noexcept{
 try{

  const bool fresco=nrrj::frescor_borrado.load(std::memory_order_relaxed)&&nrrj::frescor.load(std::memory_order_relaxed)>0.f;
  if(!c||!dst||!((nrrjcl::compoe_cor()&&!nrrj::delta_inteiro())||fresco))return;
  auto& L=rj_limpa();if(L.preparado==quadro)return;
  L.preparado=quadro;
  const auto dd=dst->GetDesc();const int ow=int(dd.Width),oh=int(dd.Height);
  if(nrorc::modo()==1&&!nrorc::estado().preparado.load(std::memory_order_acquire))return;

  struct Alvo{int inst,x0,y0,x1,y1;};Alvo alvos[1+nrrj::npc_slots]{};unsigned n=0;bool tela=false;
  const int nn=nrrj::npc2.load(std::memory_order_relaxed)?nrrj::npc_n():0;
  for(int i=0;i<=nn;++i){
   nrrj::Inst ii(i);if(!nrrj::ativo())continue;auto& G=rjg();
   if(!G.pronto||!nrrj::acumula.load(std::memory_order_relaxed)||!nrrj::compor_rapido.load(std::memory_order_relaxed)||!nrrj::temporal_inst()||(G.compor_r_tentado&&!G.compor_r))continue;
   const auto ger=nrrj::ger().load(std::memory_order_acquire);int x0=INT_MAX,y0=INT_MAX,x1=INT_MIN,y1=INT_MIN;
   for(unsigned s=0;s<nrrj::slots;++s){const auto& S=nrrj::global().slot[s];
    if(nrrj::estado_de(S.palavra.load(std::memory_order_acquire))==nrrj::Livre||S.geracao!=ger||int(S.ow)!=ow||int(S.oh)!=oh||!S.L)continue;
    const int M=std::clamp(int(S.L/4u),32,256);
    x0=(std::min)(x0,(std::max)(S.x-M,0));y0=(std::min)(y0,(std::max)(S.y-M,0));x1=(std::max)(x1,(std::min)(S.x+int(S.L)+M,ow));y1=(std::max)(y1,(std::min)(S.y+int(S.L)+M,oh));}
   if(x1<=x0||y1<=y0)continue;
   if(unsigned(x1-x0)>rj_jan()||unsigned(y1-y0)>rj_jan()||G.copia_falhou){tela=true;continue;}
   alvos[n++]={i,x0,y0,x1,y1};}
  if(!n&&!tela){++L.sem_rosto;return;}
  nrd3d::marcador(c,L"nr_rosto_limpa");
  transition(c,dst,D3D12_RESOURCE_STATES(estado),D3D12_RESOURCE_STATE_COPY_SOURCE);
  D3D12_TEXTURE_COPY_LOCATION b{};b.pResource=dst;b.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;b.SubresourceIndex=0;
  for(unsigned k=0;k<n;++k){const auto& A=alvos[k];nrrj::Inst ii(A.inst);auto& G=rjg();
   if(!rj_copia_garantir(G,dd.Format)||unsigned(A.x1-A.x0)>G.copia_w||unsigned(A.y1-A.y0)>G.copia_h){tela=true;continue;}
   transition(c,G.copia.Get(),read_state,D3D12_RESOURCE_STATE_COPY_DEST);
   D3D12_TEXTURE_COPY_LOCATION a{};a.pResource=G.copia.Get();a.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;a.SubresourceIndex=0;
   const D3D12_BOX box{UINT(A.x0),UINT(A.y0),0,UINT(A.x1),UINT(A.y1),1};c->CopyTextureRegion(&a,0,0,0,&b,&box);
   transition(c,G.copia.Get(),D3D12_RESOURCE_STATE_COPY_DEST,read_state);
   G.limpa_q=quadro;G.limpa_x0=A.x0;G.limpa_y0=A.y0;G.limpa_w=unsigned(A.x1-A.x0);G.limpa_h=unsigned(A.y1-A.y0);++L.regioes;}
  if(tela&&!L.falhou){
   if(!L.r||L.fmt!=dd.Format||L.w!=dd.Width||L.h!=dd.Height){rj_aposentar(L.r);
    ComPtr<ID3D12Device> dev;if(FAILED(dst->GetDevice(IID_PPV_ARGS(&dev)))||!dev){L.falhou=true;}
    else{D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC td=dd;td.Flags=D3D12_RESOURCE_FLAG_NONE;td.MipLevels=1;td.DepthOrArraySize=1;
     if(FAILED(dev->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&L.r)))){L.r.Reset();L.falhou=true;nrrj::log("ROSTO_COR_LIMPA sem memoria para a copia de tela inteira (reserva da regiao)");}
     else{nrd3d::nomear(L.r.Get(),L"nr_rosto_cor_limpa");L.fmt=dd.Format;L.w=dd.Width;L.h=dd.Height;L.em_copia_dest=true;
      nrorc_vram::registrar(nrorc::Api::Recurso,dev->GetResourceAllocationInfo(0,1,&td).SizeInBytes,"rosto_cor_limpa");
      char l[160]{};NRLOGF(l,sizeof l,"ROSTO_COR_LIMPA copia de tela inteira criada %llux%u formato=%u (regiao maior que a copia)",static_cast<unsigned long long>(dd.Width),dd.Height,unsigned(dd.Format));nrrj::log(l);}}}
   if(L.r){if(!L.em_copia_dest)transition(c,L.r.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
    c->CopyResource(L.r.Get(),dst);
    transition(c,L.r.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE);L.em_copia_dest=false;
    L.quadro=quadro;++L.copias;}}
  transition(c,dst,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATES(estado));
 }catch(...){}
}

bool rj_camada_pronta(RjGlobal& G,unsigned ow,unsigned oh){
 auto& C=rjc();if(C.falhou||!nrrj::camada.load(std::memory_order_relaxed))return false;
 if(!C.pso&&!C.tentado){C.tentado=true;if(!pipeline(G.device.Get(),G.root.Get(),g_nr_reproject_rjcamada,C.pso)){C.pso.Reset();C.falhou=true;nrrj::log("ROSTO_CAMADA PSO recusado: camada desligada");return false;}}
 if(!C.pso)return false;
 if(C.buf&&C.w==ow&&C.h==oh)return true;
 rj_aposentar(C.buf);
 if(!rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_DEFAULT,UINT64(ow)*oh*16ull,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,C.buf,L"nr_rosto_camada")){
  C.buf.Reset();C.falhou=true;nrrj::log("ROSTO_CAMADA sem memoria para a camada: desligada");return false;}
 C.w=ow;C.h=oh;C.quadro_ult=~0ull;std::memset(C.Bm,0,sizeof C.Bm);C.fila.limpar();
 char l[128]{};NRLOGF(l,sizeof l,"ROSTO_CAMADA camada %ux%u (%.1f MB)",ow,oh,double(UINT64(ow)*oh*16ull)/1048576.0);nrrj::log(l);return true;}
void rj_camada_uav(RjGlobal& G,unsigned base){
 auto& C=rjc();D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;
 v.Buffer.NumElements=UINT(UINT64(C.w)*C.h*4ull);v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;G.device->CreateUnorderedAccessView(C.buf.Get(),nullptr,&v,rj_cpu(G,base+6));}

bool rj_camada_antes(RjGlobal& G,ID3D12GraphicsCommandList* c,unsigned base,unsigned ow,unsigned oh,std::uint64_t quadro,RjK& k){
 if(nrrj::diag_hist.load(std::memory_order_relaxed)||!rj_camada_pronta(G,ow,oh))return false;
 rj_camada_uav(G,base);k.rap_r|=32u;k.pad_r2=unsigned(quadro&1u);transition(c,rjc().buf.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);return true;}
void rj_camada_depois(ID3D12GraphicsCommandList* c,std::uint64_t quadro,const RjK& k){
 auto& C=rjc();order_uav(c,C.buf.Get());transition(c,C.buf.Get(),write_state,D3D12_RESOURCE_STATE_COMMON);
 if(k.fw&&k.fh)C.fila.por({quadro,nrrj::epoca_backend.load(std::memory_order_acquire),nrrj::inst_atual(),k.fx,k.fy,k.fx+int(k.fw),k.fy+int(k.fh)});}

bool rj_hist_criar(RjGlobal& G){
 G.hist_tentado=true;
 if(!rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_DEFAULT,rj_hist_b(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,G.hist,L"nr_rosto_hist")){
  G.hist.Reset();nrrj::log("ROSTO_JOGADOR_ACUMULA sem memoria para o historico: acumulacao desligada");return false;}
 char l[128]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ACUMULA historico %.1f MB",double(rj_hist_b())/1048576.0);nrrj::log(l);return true;}
bool rj_hist(RjGlobal& G,unsigned base){
 if(!G.hist&&!G.hist_tentado)(void)rj_hist_criar(G);
 if(!G.hist)return false;
 D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.FirstElement=G.off_hist/4;
 v.Buffer.NumElements=UINT(rj_hist_b()/4);v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
 G.device->CreateUnorderedAccessView(G.hist.Get(),nullptr,&v,rj_cpu(G,base+5));
 return true;
}
void rj_compartilhado(RjGlobal& G,unsigned base){
 D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.FirstElement=G.off_comp/4;
 v.Buffer.NumElements=UINT(nrrj::bytes_total_i()/4);v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
 G.device->CreateUnorderedAccessView(G.compartilhado.Get(),nullptr,&v,rj_cpu(G,base+3));
}
void rj_bind(RjGlobal& G,ID3D12GraphicsCommandList* c,ID3D12PipelineState* pso,unsigned base,const RjK& k){
 ID3D12DescriptorHeap* hs[]{G.heap.Get()};c->SetDescriptorHeaps(1,hs);c->SetComputeRootSignature(G.root.Get());c->SetPipelineState(pso);
 c->SetComputeRoot32BitConstants(0,sizeof(RjK)/4,&k,0);
 auto gp=G.heap->GetGPUDescriptorHandleForHeapStart();auto t=gp;t.ptr+=UINT64(base)*G.stride;auto u=gp;u.ptr+=UINT64(base+rj_srvs)*G.stride;
 c->SetComputeRootDescriptorTable(1,t);c->SetComputeRootDescriptorTable(2,u);
}

bool rj_garantir(RjGlobal& G,ID3D12Device* d){
 if(G.pronto)return G.device.Get()==d||(nrrj::envenenar("dispositivo diferente do primeiro"),false);
 if(nrrj::global().envenenado.load())return false;
 G.device=d;G.stride=d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
 if(!root(d,sizeof(RjK)/4,rj_srvs,rj_uavs,G.root)||!pipeline(d,G.root.Get(),g_nr_reproject_rjrecorte,G.recorte)||!pipeline(d,G.root.Get(),g_nr_reproject_rjcompor,G.compor)){
  nrrj::envenenar("raiz ou PSO do rosto");return false;}
 D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,rj_set*rj_passes*rj_aneis,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
 if(FAILED(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&G.heap)))){nrrj::envenenar("heap do rosto");return false;}
 nrorc_vram::registrar(nrorc::Api::Heap,0,"rosto_heap");
 for(unsigned s=0;s<rj_passes*rj_aneis;++s)rj_nulos(G,s*rj_set);

 HANDLE handle{};
 if(!rj_buffer(d,D3D12_HEAP_TYPE_DEFAULT,nrrj::bytes_total_i(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,G.compartilhado,L"nr_rosto_compartilhado",D3D12_HEAP_FLAG_SHARED)||
    FAILED(d->CreateSharedHandle(G.compartilhado.Get(),nullptr,GENERIC_ALL,nullptr,&handle))||!handle){
  G.compartilhado.Reset();nrrj::passageira(nrrj::Falha::Render,"buffer compartilhado do rosto");return false;}
 void* m{};
 if(!rj_buffer(d,D3D12_HEAP_TYPE_READBACK,256,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE,G.marcador,L"nr_rosto_marcador")||FAILED(G.marcador->Map(0,nullptr,&m))||!m){nrrj::envenenar("marcador do rosto");return false;}
 std::memset(m,0,256);
 auto& g=nrrj::global();
 g.marcador.store(static_cast<const volatile std::uint32_t*>(m),std::memory_order_release);
 g.bytes.store(nrrj::bytes_total_i());g.handle.store(handle,std::memory_order_release);
 G.pronto=true;
 char l[128]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_D3D_PRONTO bytes=%llu",static_cast<unsigned long long>(nrrj::bytes_total_i()));nrrj::log(l);
 return true;
}
void rj_caminho_garantir(RjGlobal& G);
}

bool rj_grade_garantir(RjGlobal& G){
 if(G.grade_pronto)return true;if(G.grade_falhou)return false;
 HANDLE handle{};
 if((!G.grade_pso&&!pipeline(G.device.Get(),G.root.Get(),g_nr_reproject_rjgrade,G.grade_pso))||
    !rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_DEFAULT,nrrj::grade_bytes,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,G.grade_buf,L"nr_rosto_grade",D3D12_HEAP_FLAG_SHARED)||
    FAILED(G.device->CreateSharedHandle(G.grade_buf.Get(),nullptr,GENERIC_ALL,nullptr,&handle))||!handle){
  G.grade_falhou=true;nrrj::log("ROSTO_JOGADOR_GRADE_FALHOU pso ou buffer compartilhado da grade (o rosto fica sem deteccao neste jogo)");return false;}
 nrrj::grade().handle.store(handle,std::memory_order_release);G.grade_pronto=true;
 char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_GRADE_PRONTA bytes=%llu slots=%u",static_cast<unsigned long long>(nrrj::grade_bytes),nrrj::grade_slots);nrrj::log(l);
 return true;
}
void Backend::rj_sem_job(bool v) noexcept{if(p_)p_->rj_sem_job=v;}
namespace {

void rj_caminho_garantir(RjGlobal& G){
 if(G.caminho_tentado||nrrj::inst_atual()!=0||!nrrj::sj_q())return;G.caminho_tentado=true;void* m{};
 if(!pipeline(G.device.Get(),G.root.Get(),g_nr_reproject_rjcaminho,G.caminho_pso)||
    !rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_DEFAULT,nrrj::caminho_total,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,G.caminho_buf,L"nr_rosto_caminho")||
    !rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_READBACK,nrrj::caminho_total,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE,G.caminho_rbuf,L"nr_rosto_caminho_leitura")||FAILED(G.caminho_rbuf->Map(0,nullptr,&m))||!m){
  G.caminho_pso.Reset();G.caminho_buf.Reset();G.caminho_rbuf.Reset();nrrj::sem_job_q.store(0);nrrj::log("ROSTO_JOGADOR_SEMJOB_FALHOU PSO ou buffers da caminhada: o recorte volta a ser so no job");return;}
 std::memset(m,0xff,size_t(nrrj::caminho_total));nrrj::caminho_rb.store(static_cast<const nrrj::CaminhoEnt*>(m),std::memory_order_release);
 char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_SEMJOB pronto: recorte da protagonista a cada %u quadros, com ou sem job",nrrj::sj_q());nrrj::log(l);}
}
namespace {

template<class Impl> void rj_grade_escrever(RjGlobal& G,Impl& p,ID3D12GraphicsCommandList* c){
 auto& GG=nrrj::grade();p.rj_grade=-1;
 if(!GG.quer.load(std::memory_order_relaxed)||!c||!p.device||!p.histories_read||!p.history[1]||!p.history[2])return;
 if(!rj_garantir(G,p.device.Get())||!rj_grade_garantir(G))return;
 const auto hd=p.history[2]->GetDesc();const unsigned hw=unsigned(hd.Width),hh=hd.Height;if(!hw||!hh)return;
 unsigned gh=(std::min)(nrrj::grade_h_max,hh),gw=unsigned(std::lround(double(gh)*hw/hh));
 if(gw>nrrj::grade_w_max){gw=nrrj::grade_w_max;gh=unsigned(std::lround(double(gw)*hh/hw));}
 if(gw<64||gh<64)return;
 const int s=nrrj::grade_slot_livre();if(s<0){GG.sem_slot.fetch_add(1);return;}
 ComPtr<ID3D12GraphicsCommandList2> c2;
 if(FAILED(c->QueryInterface(IID_PPV_ARGS(&c2)))||!c2){nrrj::envenenar("sem ID3D12GraphicsCommandList2");return;}
 const unsigned base=rj_base(G,4);
 rj_srv_hist(G,p,base);
 {D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.NumElements=UINT(nrrj::grade_bytes/4);v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
  G.device->CreateUnorderedAccessView(G.grade_buf.Get(),nullptr,&v,rj_cpu(G,base+3));}
 RjGradeK k{gw,gh,unsigned(nrrj::grade_off_d(unsigned(s))),unsigned(nrrj::grade_off_mv(unsigned(s))),p.constants.slice,unsigned(GG.invertida.load(std::memory_order_relaxed)!=0),0,0};
 nrd3d::marcador(c,L"nr_rosto_grade");transition(c,G.grade_buf.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);
 {ID3D12DescriptorHeap* hs[]{G.heap.Get()};c->SetDescriptorHeaps(1,hs);c->SetComputeRootSignature(G.root.Get());c->SetPipelineState(G.grade_pso.Get());
  c->SetComputeRoot32BitConstants(0,sizeof(RjGradeK)/4,&k,0);
  auto gp=G.heap->GetGPUDescriptorHandleForHeapStart();auto t=gp;t.ptr+=UINT64(base)*G.stride;auto u=gp;u.ptr+=UINT64(base+rj_srvs)*G.stride;
  c->SetComputeRootDescriptorTable(1,t);c->SetComputeRootDescriptorTable(2,u);}
 c->Dispatch((gw+7)/8,(gh+7)/8,1);
 order_uav(c,G.grade_buf.Get());transition(c,G.grade_buf.Get(),write_state,D3D12_RESOURCE_STATE_COMMON);
 const std::uint32_t seq=std::uint32_t(++G.grade_seq);
 {const D3D12_GPU_VIRTUAL_ADDRESS va=G.marcador->GetGPUVirtualAddress()+4u*(nrrj::grade_marca0+unsigned(s));
  const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER marca{va,UINT(seq)};const D3D12_WRITEBUFFERIMMEDIATE_MODE modo=D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_OUT;
  c2->WriteBufferImmediate(1,&marca,&modo);}
 auto& S=GG.slot[unsigned(s)];S.seq=seq;S.w=int(gw);S.h=int(gh);S.palavra.store(nrrj::palavra_g(nrrj::GGravado,0),std::memory_order_release);
 p.rj_grade=s;
 if(GG.gravadas.fetch_add(1)==0){char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_GRADE primeira grade %ux%u (captura %ux%u, fatia %u, invertida %u)",gw,gh,hw,hh,p.constants.slice,k.inv);nrrj::log(l);}
}

template<class Impl> void rj_caminhar(RjGlobal& G,Impl& p,ID3D12GraphicsCommandList* c,unsigned ow,unsigned oh,float hx,float hy,float lado){
 if(!G.caminho_pso||!G.caminho_buf||!G.caminho_rbuf||!c||!ow||!oh||!p.histories_read||!p.history[1]||!p.history[2]){nrrj::sj_conta().sem_tabela.fetch_add(1,std::memory_order_relaxed);return;}
 const unsigned base=rj_base(G,rj_passe_caminho);
 rj_srv_hist(G,p,base);
 {D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.NumElements=UINT(nrrj::caminho_total/4);v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
  G.device->CreateUnorderedAccessView(G.caminho_buf.Get(),nullptr,&v,rj_cpu(G,base+3));}
 const unsigned e=unsigned(p.policy.frame%nrrj::caminho_anel);
 RjK kc{};kc.w=ow;kc.h=oh;kc.hx=hx;kc.hy=hy;kc.fatia=p.constants.slice;kc.mascara=p.constants.mask;kc.historico=Policy::history_count;
 kc.prof_rel=p.constants.depth_relative;kc.prof_abs=p.constants.depth_absolute;kc.off_ent=e*nrrj::caminho_bytes;kc.quadro=unsigned(p.policy.frame);kc.ganho=lado;
 nrd3d::marcador(c,L"nr_rosto_caminho");transition(c,G.caminho_buf.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);
 rj_bind(G,c,G.caminho_pso.Get(),base,kc);c->Dispatch(1,1,1);
 order_uav(c,G.caminho_buf.Get());transition(c,G.caminho_buf.Get(),write_state,D3D12_RESOURCE_STATE_COPY_SOURCE);
 c->CopyBufferRegion(G.caminho_rbuf.Get(),UINT64(e)*nrrj::caminho_bytes,G.caminho_buf.Get(),UINT64(e)*nrrj::caminho_bytes,nrrj::caminho_bytes);
 transition(c,G.caminho_buf.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
}

void rj_ruido_uav(RjGlobal& G,unsigned base){
 D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;
 v.Buffer.NumElements=UINT((2ull*(64ull+UINT64(G.ruido_jan)*G.ruido_jan*8ull))/4ull);v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
 G.device->CreateUnorderedAccessView(G.ruido_buf.Get(),nullptr,&v,rj_cpu(G,base+6));}
void rj_ruido_k(RjK& k,RjGlobal& G,std::uint64_t ger){
 k.quadro=unsigned(G.quadro);k.geracao=unsigned(ger);k.janela=G.ruido_jan;
 k.a_baixa=nrrj::ruido_alfa.load(std::memory_order_relaxed);k.a_alta=nrrj::ruido_alfa_var.load(std::memory_order_relaxed);k.gama=nrrj::ruido_gama.load(std::memory_order_relaxed);
 k.a_lenta=nrrj::ruido_pixel.load(std::memory_order_relaxed);k.mov_px=nrrj::ruido_passo.load(std::memory_order_relaxed);
 k.limiar=nrrj::ruido_t0.load(std::memory_order_relaxed);k.peso=nrrj::ruido_t1.load(std::memory_order_relaxed);
 if(nrrj::ceu_fora.load(std::memory_order_relaxed))k.rap_r|=512u;
 k.n2=(std::max)(nrrj::ruido_escala.load(std::memory_order_relaxed),1u);if(nrrj::ruido_mistura.load(std::memory_order_relaxed))k.rap_r|=1u;}
template<class Impl> void rj_ruido(RjGlobal& G,Impl& p,ID3D12GraphicsCommandList* c,ID3D12Resource* cor,unsigned vista,const nrrj::Deteccao& det,std::uint64_t ger){
 if(!c||!cor||!p.device||G.ruido_falhou||!p.histories_read||!p.history[1]||!p.history[2])return;
 const auto cd=cor->GetDesc();const unsigned ow=unsigned(cd.Width),oh=cd.Height;
 const auto r=nrrj::recorte_saida(det,ow,oh,nrrj::recorte_fator.load(std::memory_order_relaxed),nrrj::grade_rede.load(std::memory_order_relaxed),nrrj::teto_inst());if(r.vazio())return;
 if(!rj_garantir(G,p.device.Get()))return;
 const unsigned e=(std::max)(nrrj::ruido_escala.load(std::memory_order_relaxed),1u);
 if(!G.ruido_tentado){G.ruido_tentado=true;const unsigned jan=(rj_jan()+e-1u)/e+2u;
  const UINT64 bytes=2ull*(64ull+UINT64(jan)*jan*8ull);
  if(!pipeline(G.device.Get(),G.root.Get(),g_nr_reproject_rjruido,G.ruido_pso)||!pipeline(G.device.Get(),G.root.Get(),g_nr_reproject_rjrecorteruido,G.recorte_ruido_pso)||
     !rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_DEFAULT,bytes,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,G.ruido_buf,L"nr_rosto_ruido")){
   G.ruido_pso.Reset();G.recorte_ruido_pso.Reset();G.ruido_buf.Reset();G.ruido_falhou=true;nrrj::log("ROSTO_RUIDO PSO ou memoria recusados: ruido desligado nesta instancia");return;}
  G.ruido_jan=jan;G.ruido_e=e;char l[160]{};NRLOGF(l,sizeof l,"ROSTO_RUIDO inst=%d janela %u celulas de %u px (%.1f MB)",nrrj::inst_atual(),jan,e,double(bytes)/1048576.0);nrrj::log(l);}
 if(e!=G.ruido_e)return;

 if(r.L>(G.ruido_jan-2u)*e){++G.ruido_grande;return;}
 const int m=int(nrrj::ruido_margem.load(std::memory_order_relaxed)),J=int(G.ruido_jan*e);
 int x0=(std::max)(r.x-m,0),y0=(std::max)(r.y-m,0),x1=(std::min)(r.x+int(r.L)+m,int(ow)),y1=(std::min)(r.y+int(r.L)+m,int(oh));
 if(x1-x0>J){const int f=x1-x0-J;x0+=f/2;x1=x0+J;}
 if(y1-y0>J){const int f=y1-y0-J;y0+=f/2;y1=y0+J;}
 if(x1-x0<8||y1-y0<8)return;
 const unsigned base=rj_base(G,0);
 {D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT(vista);sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sv.Texture2D.MipLevels=1;
  G.device->CreateShaderResourceView(cor,&sv,rj_cpu(G,base));}
 rj_srv_hist(G,p,base);
 rj_ruido_uav(G,base);
 RjK k{};k.w=ow;k.h=oh;k.fx=x0;k.fy=y0;k.fw=(unsigned(x1-x0)+e-1u)/e;k.fh=(unsigned(y1-y0)+e-1u)/e;
 if(k.fw>G.ruido_jan||k.fh>G.ruido_jan)return;
 k.fatia=p.constants.slice;k.mascara=p.constants.mask;k.historico=Policy::history_count;k.prof_rel=p.constants.depth_relative;k.prof_abs=p.constants.depth_absolute;
 rj_ruido_k(k,G,ger);
 nrd3d::marcador(c,L"nr_rosto_ruido");transition(c,G.ruido_buf.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);
 rj_bind(G,c,G.ruido_pso.Get(),base,k);c->Dispatch((k.fw+7)/8,(k.fh+7)/8,1);
 order_uav(c,G.ruido_buf.Get());transition(c,G.ruido_buf.Get(),write_state,D3D12_RESOURCE_STATE_COMMON);
 G.ruido_q=G.quadro;++G.ruido_passes;G.ruido_px+=std::uint64_t(k.fw)*k.fh;
 if(G.ruido_passes<=3||(G.ruido_passes&4095u)==0){char l[220]{};NRLOGF(l,sizeof l,"ROSTO_RUIDO inst=%d passes=%llu janela=%d,%d,%ux%u px_med=%llu recortes=%llu grandes=%llu",nrrj::inst_atual(),
  static_cast<unsigned long long>(G.ruido_passes),k.fx,k.fy,k.fw,k.fh,static_cast<unsigned long long>(G.ruido_px/G.ruido_passes),static_cast<unsigned long long>(G.ruido_recortes),static_cast<unsigned long long>(G.ruido_grande));nrrj::log(l);}
}

template<class Impl> void rj_rosto_img(RjGlobal& G,Impl& p,ID3D12GraphicsCommandList* c,ID3D12Resource* cor,unsigned vista){
 unsigned hz=nrrj::rosto_hz.load(std::memory_order_relaxed);
 const unsigned hb=nrrj::rosto_hz_busca.load(std::memory_order_relaxed);const bool busca=hz&&hb&&(!nrrj::grace_on.load(std::memory_order_relaxed)||nrrj::busca_sempre.load(std::memory_order_relaxed));
 if(busca&&hb>hz)hz=hb;
 if(!nrrj::npc2.load(std::memory_order_relaxed)||!hz||!c||!cor||!p.device)return;
 LARGE_INTEGER agora{},freq{};QueryPerformanceCounter(&agora);QueryPerformanceFrequency(&freq);
 if(G.rosto_qpc&&agora.QuadPart-G.rosto_qpc<freq.QuadPart/LONGLONG(hz))return;
 if(!rj_garantir(G,p.device.Get()))return;
 if(!G.rosto_ok){
  if(G.rosto_tentado)return;G.rosto_tentado=true;void* m{};
  if(!pipeline(G.device.Get(),G.root.Get(),g_nr_reproject_rjrostoimg,G.rosto_pso)||
     !rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_DEFAULT,nrrj::rosto_bytes,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,G.rosto_buf,L"nr_rosto_detector")||
     !rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_READBACK,nrrj::rosto_bytes*nrrj::rosto_aneis,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE,G.rosto_rb,L"nr_rosto_detector_leitura")||
     FAILED(G.rosto_rb->Map(0,nullptr,&m))||!m){nrrj::log("ROSTO_JOGADOR_ROSTOS_FALHOU pso ou buffers da imagem do detector de rosto");return;}
  nrrj::rosto_img().base.store(static_cast<const unsigned char*>(m),std::memory_order_release);G.rosto_ok=true;
  char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ROSTOS_D3D pronto (%u por segundo, anel %u x %llu bytes)",hz,nrrj::rosto_aneis,static_cast<unsigned long long>(nrrj::rosto_bytes));nrrj::log(l);}
 ComPtr<ID3D12GraphicsCommandList2> c2;if(FAILED(c->QueryInterface(IID_PPV_ARGS(&c2)))||!c2)return;
 const auto cd=cor->GetDesc();const unsigned ow=unsigned(cd.Width),oh=cd.Height;if(ow<64||oh<64)return;
 const unsigned wi=nrrj::yunet_inteira.load(std::memory_order_relaxed);
 const unsigned wmax=wi?wi:nrrj::rosto_w_max,hmax=wi?nrrj::inteira_h_max:nrrj::rosto_h_max;
 unsigned iw=wmax,ih=unsigned(std::lround(double(iw)*oh/ow));
 if(ih>hmax){ih=hmax;iw=unsigned(std::lround(double(ih)*ow/oh));}
 if(iw<32||ih<32)return;

 int rx0=0,ry0=0,rw=int(ow),rh=int(oh);
 double y0f=.25;
 if(nrrj::npc2.load(std::memory_order_relaxed)&&nrrj::faixa_da_vez(G.rosto_n++,nrrj::giro_inteira.load(std::memory_order_relaxed),busca,y0f)){
  rw=int(ow);rh=int(std::lround(double(oh)*(960.0/2160.0)));ry0=(std::min)(int(std::lround(double(oh)*y0f)),int(oh)-rh);
  iw=nrrj::faixa_w;ih=nrrj::faixa_h;

  if(const unsigned fw=nrrj::yunet_faixa.load(std::memory_order_relaxed)){iw=fw;ih=fw/4u;}}
 const unsigned i=(G.rosto_i++)%nrrj::rosto_aneis,base=rj_base(G,5);
 {D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT(vista);sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sv.Texture2D.MipLevels=1;
  G.device->CreateShaderResourceView(cor,&sv,rj_cpu(G,base));}
 {D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.NumElements=UINT(nrrj::rosto_bytes/4);v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
  G.device->CreateUnorderedAccessView(G.rosto_buf.Get(),nullptr,&v,rj_cpu(G,base+3));}
 RjK k{};k.w=ow;k.h=oh;k.fw=iw;k.fh=ih;k.off_ent=0;
 if(ry0>0||rh<int(oh)){k.rx=rx0;k.ry=ry0;k.lado=unsigned(rw);k.n=unsigned(rh);}
 nrd3d::marcador(c,L"nr_rosto_detector");transition(c,G.rosto_buf.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);
 rj_bind(G,c,G.rosto_pso.Get(),base,k);c->Dispatch((iw+7)/8,(ih+7)/8,1);
 order_uav(c,G.rosto_buf.Get());transition(c,G.rosto_buf.Get(),write_state,D3D12_RESOURCE_STATE_COPY_SOURCE);
 c->CopyBufferRegion(G.rosto_rb.Get(),UINT64(i)*nrrj::rosto_bytes,G.rosto_buf.Get(),0,UINT64(iw)*ih*8u);
 transition(c,G.rosto_buf.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
 const std::uint32_t seq=++G.rosto_seq;
 {const D3D12_GPU_VIRTUAL_ADDRESS va=G.marcador->GetGPUVirtualAddress()+4u*(nrrj::rosto_marca0+i);
  const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER marca{va,UINT(seq)};const D3D12_WRITEBUFFERIMMEDIATE_MODE modo=D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_OUT;
  c2->WriteBufferImmediate(1,&marca,&modo);}
 auto& E=nrrj::rosto_img().ent[i];E.w=int(iw);E.h=int(ih);E.ow=int(ow);E.oh=int(oh);E.x0=rx0;E.y0=ry0;E.rw=rw;E.rh=rh;E.quadro=nrreusostate::quadros.load(std::memory_order_relaxed);
 E.t=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
 E.seq.store(seq,std::memory_order_release);nrrj::rosto_img().gravadas.fetch_add(1,std::memory_order_relaxed);
 nrrj::acordar_rostos();
 G.rosto_qpc=agora.QuadPart;
}

template<class Impl> void rj_rosto_roi(RjGlobal& G,Impl& p,ID3D12GraphicsCommandList* c,ID3D12Resource* cor,unsigned vista){
 if(!nrrj::npc2.load(std::memory_order_relaxed)||!G.rosto_ok||!c||!cor||!p.device)return;
 LARGE_INTEGER agora{},freq{};QueryPerformanceCounter(&agora);QueryPerformanceFrequency(&freq);
 if(G.roi_qpc&&agora.QuadPart-G.roi_qpc<freq.QuadPart/LONGLONG(nrrj::roi_hz))return;
 if(!G.roi_ok){
  if(G.roi_tentado)return;G.roi_tentado=true;
  for(int kn=0;kn<nrrj::npc_slots;++kn){void* m{};
   if(!rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_DEFAULT,nrrj::roi_bytes,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,G.roi_buf[kn],L"nr_rosto_roi")||
      !rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_READBACK,nrrj::roi_bytes*nrrj::roi_aneis,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE,G.roi_rb[kn],L"nr_rosto_roi_leitura")||
      FAILED(G.roi_rb[kn]->Map(0,nullptr,&m))||!m){nrrj::log("ROSTO_NPC2_ROI_FALHOU buffers da ROI");return;}
   nrrj::roi_img(kn).base.store(static_cast<const unsigned char*>(m),std::memory_order_release);}
  G.roi_ok=true;nrrj::log("ROSTO_NPC2_ROI pronta (uma por NPC)");}
 ComPtr<ID3D12GraphicsCommandList2> c2;if(FAILED(c->QueryInterface(IID_PPV_ARGS(&c2)))||!c2)return;
 const auto cd=cor->GetDesc();const int ow=int(cd.Width),oh=int(cd.Height);if(ow<64||oh<64)return;
 bool fez=false;
 for(int kn=0;kn<nrrj::npc_n();++kn){
  nrrj::Deteccao d1{};{nrrj::Inst i1(1+kn);d1=nrrj::ler_deteccao();}
  if(!(d1.on||d1.pend)||!d1.tem||!(d1.lado>0.f))continue;
  const int L=std::clamp(int(std::lround(nrrj::roi_fator*d1.lado)),96,(std::min)(ow,oh));
  const int x0=std::clamp(int(std::lround(d1.cx-.5f*L)),0,ow-L),y0=std::clamp(int(std::lround(d1.cy-.5f*L)),0,oh-L);
  const unsigned i=(G.roi_i[kn]++)%nrrj::roi_aneis,base=rj_base(G,6u+unsigned(kn));
  {D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT(vista);sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sv.Texture2D.MipLevels=1;
   G.device->CreateShaderResourceView(cor,&sv,rj_cpu(G,base));}
  {D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.NumElements=UINT(nrrj::roi_bytes/4);v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
   G.device->CreateUnorderedAccessView(G.roi_buf[kn].Get(),nullptr,&v,rj_cpu(G,base+3));}
  RjK k{};k.w=unsigned(ow);k.h=unsigned(oh);k.rx=x0;k.ry=y0;k.lado=unsigned(L);k.fw=nrrj::roi_lado;k.fh=nrrj::roi_lado;k.off_ent=0;
  nrd3d::marcador(c,L"nr_rosto_roi");transition(c,G.roi_buf[kn].Get(),D3D12_RESOURCE_STATE_COMMON,write_state);
  rj_bind(G,c,G.rosto_pso.Get(),base,k);c->Dispatch((nrrj::roi_lado+7)/8,(nrrj::roi_lado+7)/8,1);
  order_uav(c,G.roi_buf[kn].Get());transition(c,G.roi_buf[kn].Get(),write_state,D3D12_RESOURCE_STATE_COPY_SOURCE);
  c->CopyBufferRegion(G.roi_rb[kn].Get(),UINT64(i)*nrrj::roi_bytes,G.roi_buf[kn].Get(),0,nrrj::roi_bytes);
  transition(c,G.roi_buf[kn].Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
  const std::uint32_t seq=++G.roi_seq[kn];
  {const D3D12_GPU_VIRTUAL_ADDRESS va=G.marcador->GetGPUVirtualAddress()+4u*(nrrj::roi_marca0+unsigned(kn)*nrrj::roi_aneis+i);
   const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER marca{va,UINT(seq)};const D3D12_WRITEBUFFERIMMEDIATE_MODE modo=D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_OUT;
   c2->WriteBufferImmediate(1,&marca,&modo);}
  auto& E=nrrj::roi_img(kn).ent[i];E.x0=x0;E.y0=y0;E.l=L;E.quadro=nrreusostate::quadros.load(std::memory_order_relaxed);
  E.t=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
  E.seq.store(seq,std::memory_order_release);fez=true;}
 if(fez){G.roi_qpc=agora.QuadPart;nrrj::acordar_rostos();}
}

void rj_rosto_criar(RjGlobal& G,int k_roi){
 if(!nrrj::npc2.load(std::memory_order_relaxed))return;
 G.rosto_tentado=true;void* m{};
 if(!pipeline(G.device.Get(),G.root.Get(),g_nr_reproject_rjrostoimg,G.rosto_pso)||
    !rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_DEFAULT,nrrj::rosto_bytes,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,G.rosto_buf,L"nr_rosto_detector")||
    !rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_READBACK,nrrj::rosto_bytes*nrrj::rosto_aneis,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE,G.rosto_rb,L"nr_rosto_detector_leitura")||
    FAILED(G.rosto_rb->Map(0,nullptr,&m))||!m){nrrj::log("ROSTO_JOGADOR_ROSTOS_FALHOU pso ou buffers da imagem do detector de rosto");return;}
 nrrj::rosto_img().base.store(static_cast<const unsigned char*>(m),std::memory_order_release);G.rosto_ok=true;
 {char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ROSTOS_D3D pronto no inicio (anel %u x %llu bytes)",nrrj::rosto_aneis,static_cast<unsigned long long>(nrrj::rosto_bytes));nrrj::log(l);}
 if(!nrrj::npc2.load(std::memory_order_relaxed))return;
 G.roi_tentado=true;
 for(int kn=0;kn<k_roi&&kn<nrrj::npc_slots;++kn){void* r{};
  if(!rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_DEFAULT,nrrj::roi_bytes,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,G.roi_buf[kn],L"nr_rosto_roi")||
     !rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_READBACK,nrrj::roi_bytes*nrrj::roi_aneis,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE,G.roi_rb[kn],L"nr_rosto_roi_leitura")||
     FAILED(G.roi_rb[kn]->Map(0,nullptr,&r))||!r){nrrj::log("ROSTO_NPC2_ROI_FALHOU buffers da ROI");return;}
  nrrj::roi_img(kn).base.store(static_cast<const unsigned char*>(r),std::memory_order_release);}
 G.roi_ok=true;}
struct RjPool {ComPtr<ID3D12Resource> comp,hist;HANDLE handle{};nrorc::Pool pc{},ph{};};
RjPool& rj_pool(){static auto* p=new RjPool;return *p;}

bool rj_orc_npc(RjGlobal& G,const RjGlobal& G0,ID3D12Device* d,const RjPool& P,int k){
 G.device=d;G.stride=G0.stride;G.root=G0.root;G.recorte=G0.recorte;G.compor=G0.compor;G.compor_r=G0.compor_r;G.compor_r_tentado=true;
 D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,rj_set*rj_passes*rj_aneis,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
 if(FAILED(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&G.heap))))return false;
 nrorc_vram::registrar(nrorc::Api::Heap,0,"rosto_heap");
 for(unsigned s=0;s<rj_passes*rj_aneis;++s)rj_nulos(G,s*rj_set);
 G.compartilhado=P.comp;G.off_comp=P.pc.offset(k);G.hist=P.hist;G.off_hist=P.hist?P.ph.offset(k):0u;G.hist_tentado=true;
 void* m{};
 if(!rj_buffer(d,D3D12_HEAP_TYPE_READBACK,256,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE,G.marcador,L"nr_rosto_marcador")||FAILED(G.marcador->Map(0,nullptr,&m))||!m)return false;
 std::memset(m,0,256);
 auto& g=nrrj::global();
 g.marcador.store(static_cast<const volatile std::uint32_t*>(m),std::memory_order_release);
 g.offset.store(G.off_comp,std::memory_order_relaxed);g.bytes.store(P.pc.total());g.handle.store(P.handle,std::memory_order_release);
 G.pronto=true;return true;}

void rj_cab_garantir(RjGlobal& G){
 if(G.cab_tentado||nrrj::inst_atual()!=0||nrrj::caminha_dela.load(std::memory_order_relaxed)<2)return;G.cab_tentado=true;void* m{};
 if(!pipeline(G.device.Get(),G.root.Get(),g_nr_reproject_rjcabeca,G.cab_pso)||
    !rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_DEFAULT,nrrj::cab_bytes*nrrj::cab_aneis,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,G.cab_buf,L"nr_rosto_cabeca")||
    !rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_READBACK,nrrj::cab_bytes*nrrj::cab_aneis,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE,G.cab_rb,L"nr_rosto_cabeca_leitura")||
    FAILED(G.cab_rb->Map(0,nullptr,&m))||!m){nrrj::log("ROSTO_JOGADOR_CABECA_FALHOU pso ou buffers: CaminhaDela=2 fica como 1");return;}
 G.cab_m=static_cast<const float*>(m);G.cab_ok=true;nrrj::log("ROSTO_JOGADOR_CABECA pronta (cabeca dela medida em todo quadro)");
 if(nrrj::caminha_dela.load(std::memory_order_relaxed)==3){void* m3{};
  if((G.rosto_pso||pipeline(G.device.Get(),G.root.Get(),g_nr_reproject_rjrostoimg,G.rosto_pso))&&
     rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_DEFAULT,nrrj::roi_bytes,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,G.p3_buf,L"nr_rosto_pontos")&&
     rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_READBACK,nrrj::roi_bytes*nrrj::slots,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE,G.p3_rb,L"nr_rosto_pontos_leitura")&&SUCCEEDED(G.p3_rb->Map(0,nullptr,&m3))&&m3){
   nrrj::pontos().base.store(static_cast<const unsigned char*>(m3),std::memory_order_release);G.p3_ok=true;nrrj::log("ROSTO_JOGADOR_PONTOS pronta (ROI do rosto dela em cada recorte)");}
  else nrrj::log("ROSTO_JOGADOR_PONTOS_FALHOU pso ou buffers: CaminhaDela=3 fica como 2");}}
bool rj_orc_criar(ID3D12Device* d,const nrorc::Decisao& dec){
 auto& G0=rjg();
 if(!rj_garantir(G0,d))return false;
 rj_caminho_garantir(G0);
 rj_cab_garantir(G0);
 if(nrrj::acumula.load()&&!G0.hist&&!rj_hist_criar(G0))return false;
 if(nrrj::acumula.load()&&nrrj::compor_rapido.load()&&!G0.compor_r){G0.compor_r_tentado=true;
  if(!pipeline(d,G0.root.Get(),g_nr_reproject_rjcomporr,G0.compor_r)){G0.compor_r.Reset();nrrj::log("ROSTO_JOGADOR_COMPOR_RAPIDO PSO recusado: sem acumulacao");}
  else nrrj::log("ROSTO_JOGADOR_COMPOR_RAPIDO ligado");}
 if(!G0.grade_pso&&!pipeline(d,G0.root.Get(),g_nr_reproject_rjgrade,G0.grade_pso))G0.grade_pso.Reset();
 rj_rosto_criar(G0,dec.k_npc);
 std::uint64_t pool_b=0,hist_b=0;
 if(dec.k_npc>0){
  auto& P=rj_pool();
  {nrrj::Inst i1(dec.k_perto+1);P.pc=nrorc::Pool{nrrj::bytes_total_i(),dec.k_npc};P.ph=nrorc::Pool{rj_hist_b(),dec.k_npc};}
  if(dec.k_perto>0){nrrj::Inst i1(1);P.pc.por_perto=nrrj::bytes_total_i();P.pc.k_perto=dec.k_perto;P.ph.por_perto=rj_hist_b();P.ph.k_perto=dec.k_perto;}
  nrorc::EscopoOrigem eo(nrorc::Origem::Npc);
  if(!rj_buffer(d,D3D12_HEAP_TYPE_DEFAULT,P.pc.total(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,P.comp,L"nr_npc_compartilhado",D3D12_HEAP_FLAG_SHARED)||
     FAILED(d->CreateSharedHandle(P.comp.Get(),nullptr,GENERIC_ALL,nullptr,&P.handle))||!P.handle)return false;
  if(nrrj::acumula.load()&&nrrj::npc2_acumula.load()&&!rj_buffer(d,D3D12_HEAP_TYPE_DEFAULT,P.ph.total(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,P.hist,L"nr_npc_hist"))return false;
  for(int k=1;k<=dec.k_npc;++k){nrrj::Inst ik(k);if(!rj_orc_npc(rjg(),G0,d,P,k))return false;}
  pool_b=P.pc.total();hist_b=P.hist?P.ph.total():0u;}
 char l[260]{};NRLOGF(l,sizeof l,"NPC_ORCAMENTO_D3D grace=%llu hist_grace=%llu pool_npc=%llu hist_npc=%llu npcs=%d (bytes)",static_cast<unsigned long long>(nrrj::bytes_total_i()),
  static_cast<unsigned long long>(G0.hist?rj_hist_b():0u),static_cast<unsigned long long>(pool_b),static_cast<unsigned long long>(hist_b),dec.k_npc);
 nrorc_vram::log("%s",l);nrrj::log(l);
 if(dec.k_perto>0){char l2[200]{};NRLOGF(l2,sizeof l2,"NPC_ORCAMENTO_VAGAS grandes=%d x %u janela=%u pequenas=%d x %u total_mb=%llu livre_mb=%llu",dec.k_perto,dec.cap_perto,dec.jan_perto,dec.k_npc-dec.k_perto,dec.cap_npc,
  static_cast<unsigned long long>(dec.total/nrorc::MB),static_cast<unsigned long long>(dec.livre/nrorc::MB));nrorc_vram::log("%s",l2);nrrj::log(l2);}
 return true;}

IDXGIAdapter3* rj_adaptador(ID3D12Device* d) noexcept {
 if(auto* a=nrorc_vram::adaptador())return a;
 if(!d)return nullptr;
 ComPtr<IDXGIFactory4> f;ComPtr<IDXGIAdapter3> a;
 if(FAILED(CreateDXGIFactory2(0,IID_PPV_ARGS(&f)))||FAILED(f->EnumAdapterByLuid(d->GetAdapterLuid(),IID_PPV_ARGS(&a)))||!a)return nullptr;
 nrorc_vram::guardar_adaptador(a.Get());return nrorc_vram::adaptador();}

void rj_orc_hilo(ComPtr<ID3D12Device> d) noexcept {
 auto& E=nrorc::estado();
 try{
  nrorc::EscopoOrigem eo(nrorc::Origem::Rosto);
  const auto v=nrorc_vram::medir(rj_adaptador(d.Get()));
  auto cfg=nrrj::config_orcamento();cfg.ctx_unico=!nrorc_vram::ctx_separado();
  if(cfg.camada||cfg.detector||cfg.copia)nrorc_vram::log("NPC_ORCAMENTO_COMPLETO camada_mb=%.1f detector_mb=%.1f copia_mb=%.1f saida=%ux%u render=%ux%u (rev-19/10 P1-5d: no custo da Grace)",
   double(cfg.camada)/double(nrorc::MB),double(cfg.detector)/double(nrorc::MB),double(cfg.copia)/double(nrorc::MB),nrrj::orc_saida_w.load(),nrrj::orc_saida_h.load(),nrrj::orc_render_w.load(),nrrj::orc_render_h.load());
  nrorc::Medida med{};med.budget=v.ok?v.budget:0u;med.uso=v.uso;med.uso_nl=v.uso_nl;
  med.folga=std::uint64_t(E.folga_mb.load())*nrorc::MB;med.outros_ini=std::uint64_t(E.outros_mb.load())*nrorc::MB;med.outros_pico=E.pico_arquivo.load();

  if(med.outros_pico){const std::uint64_t fis=E.hip_total.load();
   if(fis&&med.outros_pico>fis){nrorc_vram::log("NPC_ORCAMENTO_PICO_INVALIDO pico_mb=%llu fisica_mb=%llu: descartado, calibracao do zero",
     static_cast<unsigned long long>(med.outros_pico/nrorc::MB),static_cast<unsigned long long>(fis/nrorc::MB));med.outros_pico=0;E.calib=nrorc::Calib{};}
   else med.budget_pior=E.calib.budget;}
  E.nl_inicio.store(v.uso_nl,std::memory_order_relaxed);
  const auto dec=nrorc::decidir(cfg,med);
  char l[700]{};nrorc_vram::formatar_decisao(l,sizeof l,cfg,med,dec,v);nrorc_vram::log("%s",l);nrrj::log(l);
  if(dec.motivo&nrorc::Insuficiente)nrorc_vram::log("NPC_ORCAMENTO_AVISO a Grace no menor teto (%u) ja passa do livre (%llu > %llu MB): sem NPCs; menos textura no jogo libera VRAM",
   dec.cap_grace,static_cast<unsigned long long>(dec.grace/nrorc::MB),static_cast<unsigned long long>(dec.livre/nrorc::MB));
  if(nrorc::modo()==1){nrrj::aplicar_decisao(cfg,med,dec);if(!rj_orc_criar(d.Get(),dec))nrrj::envenenar("orcamento: recursos D3D12 do rosto");}
  else nrorc::publicar(cfg,med,dec);
 }catch(...){nrorc_vram::log("NPC_ORCAMENTO excecao na medida");}
 E.d3d_pronto.store(true,std::memory_order_release);
}
void rj_orc_iniciar(ID3D12Device* d) noexcept {
 static std::atomic<bool> iniciado{false};
 if(!d||iniciado.exchange(true))return;
 try{std::thread(rj_orc_hilo,ComPtr<ID3D12Device>(d)).detach();}catch(...){nrorc::estado().d3d_pronto.store(true,std::memory_order_release);}
}
}

template<class Impl> void rj_cabeca(RjGlobal& G,Impl& p,ID3D12GraphicsCommandList* c,unsigned ow,unsigned oh,const nrrj::Deteccao& det,nrrj::QuadroRender& q){
 if(nrrj::caminha_dela.load(std::memory_order_relaxed)<2||nrrj::inst_atual()!=0)return;

 if(nrrj::rosto_ancora.load(std::memory_order_relaxed)>0){G.cab.t1=G.cab.t0=false;return;}
 if(!det.tem||!det.on||!(det.zela>0.f)||!(det.lado>0.f)||!c||!p.device||!ow||!oh){G.cab.t1=G.cab.t0=false;return;}
 if(!G.cab_ok){if(G.cab_tentado||!rj_garantir(G,p.device.Get()))return;rj_cab_garantir(G);if(!G.cab_ok)return;}
 auto& g=nrrj::global();const unsigned n=nrrj::cab_n;

 int ib=-1;for(unsigned i=0;i<nrrj::cab_aneis;++i){const auto& E=G.cab_ent[i];if(E.lido||!E.seq||g.marcado(nrrj::cab_marca0+i)!=E.seq)continue;if(ib<0||E.quadro>G.cab_ent[unsigned(ib)].quadro)ib=int(i);}
 if(ib>=0){auto& E=G.cab_ent[unsigned(ib)];for(auto& o:G.cab_ent)if(!o.lido&&o.quadro<=E.quadro)o.lido=true;E.lido=true;
  const float* v=G.cab_m+size_t(ib)*size_t(nrrj::cab_bytes/4u);const double L=double(E.lado)/E.esc,cw=E.cw,zl=E.z/double(nrrj::camada_dela_tol),zh=E.z*double(nrrj::camada_dela_tol);
  const int nmin=(std::max)(3,int(std::lround(.15*L/cw))),banda=(std::max)(2,int(std::lround(.6*L/cw)));
  int jt=-1;for(unsigned j=0;j<n&&jt<0;++j){int k=0;for(unsigned i=0;i<n;++i){const double z=v[j*n+i];if(z>=zl&&z<=zh)++k;}if(k>=nmin)jt=int(j);}
  bool ok=false;double hx=0,hy=0;
  if(jt>=0){double sx=0;int k=0,xmin=int(n),xmax=-1;
   for(int j=jt;j<(std::min)(int(n),jt+banda);++j)for(unsigned i=0;i<n;++i){const double z=v[unsigned(j)*n+i];if(z>=zl&&z<=zh){sx+=i+.5;++k;xmin=(std::min)(xmin,int(i));xmax=(std::max)(xmax,int(i));}}
   if(k>=4&&(xmax-xmin+1)*cw<=1.6*L){hx=(E.x0+sx/k*cw)*E.esc;hy=(E.y0+jt*cw+.45*L)*E.esc;
    ok=std::fabs(hx-E.jx)<=1.3*E.lado&&std::fabs(hy-E.jy)<=.3*E.lado;}}
  if(ok){++G.cab_medidas;auto& m=G.cab;if(m.t1){m.t0=true;m.f0=m.f1;m.x0=m.x1;m.y0=m.y1;}m.t1=true;m.f1=E.quadro;m.x1=hx;m.y1=hy;}
  else{++G.cab_recusas;G.cab.t1=G.cab.t0=false;}}

 const float jx=q.cx,jy=q.cy;

 if(nrrj::cabeca_pela_medida(G.cab,G.quadro,det.lado,nrrj::rosto_ancora.load(std::memory_order_relaxed)>0,q))++G.cab_usadas;

 if(!p.histories_read||!p.history[2])return;
 const auto hd=p.history[2]->GetDesc();const unsigned hw=unsigned(hd.Width),hh=hd.Height;if(hw<64||hh<64)return;
 const float esc=float(ow)/float(hw),L=det.lado/esc,cw=3.2f*L/float(n);if(!(cw>=1.f))return;
 const unsigned e=unsigned(G.cab_disp%nrrj::cab_aneis);auto& E=G.cab_ent[e];
 ComPtr<ID3D12GraphicsCommandList2> c2;if(FAILED(c->QueryInterface(IID_PPV_ARGS(&c2)))||!c2)return;
 RjCabK k{int(std::floor(q.cx/esc-1.6f*L)),int(std::floor(q.cy/esc-1.3f*L)),cw,n,p.constants.slice,unsigned(nrrj::grade().invertida.load(std::memory_order_relaxed)!=0),unsigned(e*nrrj::cab_bytes),0u};
 E.x0=float(k.x0);E.y0=float(k.y0);E.cw=cw;E.esc=esc;E.z=det.zela;E.lado=det.lado;E.px=q.cx;E.py=q.cy;E.jx=jx;E.jy=jy;E.quadro=G.quadro;
 const unsigned base=rj_base(G,rj_passe_cabeca);
 rj_srv_hist(G,p,base,false);
 {D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.NumElements=UINT(nrrj::cab_bytes*nrrj::cab_aneis/4);v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
  G.device->CreateUnorderedAccessView(G.cab_buf.Get(),nullptr,&v,rj_cpu(G,base+3));}
 nrd3d::marcador(c,L"nr_rosto_cabeca");transition(c,G.cab_buf.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);
 {ID3D12DescriptorHeap* hs[]{G.heap.Get()};c->SetDescriptorHeaps(1,hs);c->SetComputeRootSignature(G.root.Get());c->SetPipelineState(G.cab_pso.Get());
  c->SetComputeRoot32BitConstants(0,sizeof(RjCabK)/4,&k,0);
  auto gp=G.heap->GetGPUDescriptorHandleForHeapStart();auto t=gp;t.ptr+=UINT64(base)*G.stride;auto u=gp;u.ptr+=UINT64(base+rj_srvs)*G.stride;
  c->SetComputeRootDescriptorTable(1,t);c->SetComputeRootDescriptorTable(2,u);}
 c->Dispatch((n+7)/8,(n+7)/8,1);
 order_uav(c,G.cab_buf.Get());transition(c,G.cab_buf.Get(),write_state,D3D12_RESOURCE_STATE_COPY_SOURCE);
 c->CopyBufferRegion(G.cab_rb.Get(),UINT64(e)*nrrj::cab_bytes,G.cab_buf.Get(),UINT64(e)*nrrj::cab_bytes,nrrj::cab_bytes);
 transition(c,G.cab_buf.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
 const std::uint32_t seq=++G.cab_seq;
 {const D3D12_GPU_VIRTUAL_ADDRESS va=G.marcador->GetGPUVirtualAddress()+4u*(nrrj::cab_marca0+e);
  const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER marca{va,UINT(seq)};const D3D12_WRITEBUFFERIMMEDIATE_MODE modo=D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_OUT;
  c2->WriteBufferImmediate(1,&marca,&modo);}
 E.seq=seq;E.lido=false;++G.cab_disp;
 if(G.cab_disp==1||(G.cab_disp&4095u)==0){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_CABECA quadros=%llu medidas=%llu recusas=%llu usadas=%llu",
  static_cast<unsigned long long>(G.cab_disp),static_cast<unsigned long long>(G.cab_medidas),static_cast<unsigned long long>(G.cab_recusas),static_cast<unsigned long long>(G.cab_usadas));nrrj::log(l);}
}

void Backend::rj_recorte(void* command,void* color,unsigned vista,bool post) noexcept{
 if(!p_)return;auto& p=*p_;rj_slot_de(p)=-1;const bool inst0=nrrj::inst_atual()==0;if(inst0)p.rj_grade=-1;
 if(inst0)rj_apos_passo();
 if(nrrj::inst_atual()==1&&post&&nrrj::agenda_ligada())rj_agenda_planejar(p.rj_sem_job,p.policy.frame);
 if(!nrrj::ativo())return;
 nrorc::EscopoOrigem eo(inst0?nrorc::Origem::Rosto:nrorc::Origem::Npc);
 if(nrorc::modo()&&inst0&&post&&p.device&&nrorc::estado().ini_ok.load(std::memory_order_acquire)){

  if(!nrorc::estado().decidido.load(std::memory_order_acquire)&&nrrj::orc_completo.load(std::memory_order_relaxed)){
   if(auto* cor0=static_cast<ID3D12Resource*>(color)){const auto cd0=cor0->GetDesc();nrrj::orc_saida_w.store(unsigned(cd0.Width));nrrj::orc_saida_h.store(cd0.Height);
    const DXGI_FORMAT f0=cd0.Format;nrrj::orc_saida_bpp.store((f0>=DXGI_FORMAT_R32G32B32A32_TYPELESS&&f0<=DXGI_FORMAT_R32G32B32A32_SINT)?16u:(f0>=DXGI_FORMAT_R16G16B16A16_TYPELESS&&f0<=DXGI_FORMAT_R16G16B16A16_SINT)?8u:4u);}
   if(p.history[2]){const auto hd=p.history[2]->GetDesc();nrrj::orc_render_w.store(unsigned(hd.Width));nrrj::orc_render_h.store(hd.Height);}}
  rj_orc_iniciar(p.device.Get());}
 if(nrorc::modo()==1&&!nrorc::estado().preparado.load(std::memory_order_acquire))return;
 auto& G=rjg();auto& g=nrrj::global();
 try{
  ++G.quadro;G.variante=0;

  if(nrrj::epoca_nova(nrrj::epoca_backend.load(std::memory_order_acquire),G.epoca_vista)){nrrj::cortar();G.job_ult=0;++G.epocas;
   char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_EPOCA backend novo (troca do FSR): corte n=%llu quadro=%llu",static_cast<unsigned long long>(G.epocas),static_cast<unsigned long long>(G.quadro));nrrj::log(l);}

  const auto det=nrrj::ler_deteccao();const auto ger=nrrj::ger().load(std::memory_order_acquire);
  G.rampa.passo(det.on,G.composto,ger);G.composto_ant=G.composto;G.composto=false;
  if(ger!=G.ger_visto){if(G.ger_visto)G.ger_pend=true;G.ger_visto=ger;G.quadro_ger=G.quadro;}
  auto& q=nrrj::quadro_render();q=nrrj::QuadroRender{};q.on=det.on?1:0;q.nivel=G.rampa.nivel;
  const std::uint64_t agora=nrreusostate::quadros.load(std::memory_order_relaxed);
  const float dq=det.quadro&&agora>det.quadro?float((std::min<std::uint64_t>)(agora-det.quadro,16u)):0.f;
  if(det.tem){q.cx=det.cx+det.vx*dq;q.cy=det.cy+det.vy*dq;q.lado=det.lado;}
  q.disco_x=q.cx;q.disco_y=q.cy;

  q.quadro=agora;if(color){const auto cdq=static_cast<ID3D12Resource*>(color)->GetDesc();q.ow=unsigned(cdq.Width);q.oh=cdq.Height;}
  if(inst0&&nrrj::area_externa.load(std::memory_order_relaxed)&&nrrj::externo.load(std::memory_order_relaxed)){float eu=0,ev=0,ea=0,ed=0;bool efora=false;
   const bool eok=nrjext::ler(eu,ev,ea,ed,&efora);nrrj::cabeca_externa_no_quadro(q,q.ow,q.oh,eok,eu,ev,ea,ed,efora);}
  if(!post){if(!G.pre_logado){G.pre_logado=true;nrrj::log("ROSTO_JOGADOR_PRE sem recorte: o rosto so roda no POST (a captura e a saida do FSR)");}return;}
  if(inst0&&color){const auto cdc=static_cast<ID3D12Resource*>(color)->GetDesc();rj_cabeca(G,p,static_cast<ID3D12GraphicsCommandList*>(command),unsigned(cdc.Width),cdc.Height,det,q);}
  if(inst0){rj_rosto_img(G,p,static_cast<ID3D12GraphicsCommandList*>(command),static_cast<ID3D12Resource*>(color),vista);
   rj_rosto_roi(G,p,static_cast<ID3D12GraphicsCommandList*>(command),static_cast<ID3D12Resource*>(color),vista);}

  if(inst0){p.rj_grade=-1;if(!p.rj_sem_job)rj_grade_escrever(G,p,static_cast<ID3D12GraphicsCommandList*>(command));}

  const unsigned Qsj=inst0?nrrj::sj_q():0u;
  if((p.rj_sem_job&&!Qsj)||!(det.on||G.rampa.nivel>0.f)||!det.tem||det.pend)return;
  if(nrrj::ablacao.load(std::memory_order_relaxed)&4u)return;
  if(nrrj::ruido_inst())rj_ruido(G,p,static_cast<ID3D12GraphicsCommandList*>(command),static_cast<ID3D12Resource*>(color),vista,det,ger);

  if(!inst0&&!Qsj&&nrrj::agenda_ligada()&&nrrj::agenda_quadro.load(std::memory_order_acquire)==p.policy.frame&&
     !(nrrj::agenda_bits.load(std::memory_order_relaxed)&(1u<<unsigned(nrrj::inst_atual())))){++G.pulados_agenda;return;}
  if(Qsj){if(!G.caminho_tentado&&p.device&&rj_garantir(G,p.device.Get()))rj_caminho_garantir(G);
   if(!nrrj::sj_q())return;
   if(auto* cor0=static_cast<ID3D12Resource*>(color)){const auto cd0=cor0->GetDesc();rj_caminhar(G,p,static_cast<ID3D12GraphicsCommandList*>(command),unsigned(cd0.Width),cd0.Height,q.cx,q.cy,det.lado);}
   if(!nrrj::sj_devido(G.quadro,G.quadro_recorte,G.ger_recorte!=ger,Qsj)){++G.pulados_cada;return;}}

  if(!Qsj){const unsigned N=nrrj::cada.load(std::memory_order_relaxed);
   if(N>1&&nrrj::temporal_inst()&&G.ger_recorte==ger&&G.jobs_sem+1u<N){

    const unsigned Q=nrrj::cada_quadros_efetivo();
    if(!(Q>0&&G.quadro>=G.quadro_recorte+Q)){++G.pulados_cada;return;}
    ++G.por_quadros;}}
  auto* c=static_cast<ID3D12GraphicsCommandList*>(command);auto* cor=static_cast<ID3D12Resource*>(color);
  if(!c||!cor||!p.device)return;
  const auto cd=cor->GetDesc();const unsigned ow=unsigned(cd.Width),oh=cd.Height;
  const auto r=nrrj::recorte_saida(det,ow,oh,nrrj::recorte_fator.load(std::memory_order_relaxed),nrrj::grade_rede.load(std::memory_order_relaxed),nrrj::teto_inst());if(r.vazio())return;
  if(!rj_garantir(G,p.device.Get()))return;
  ComPtr<ID3D12GraphicsCommandList2> c2;
  if(FAILED(c->QueryInterface(IID_PPV_ARGS(&c2)))||!c2){nrrj::envenenar("sem ID3D12GraphicsCommandList2");return;}
  const int s=nrrj::slot_para_recorte();if(s<0){++G.sem_slot;return;}
  auto& S=g.slot[unsigned(s)];
  const unsigned base=rj_base(G,0);
  {D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT(vista);sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sv.Texture2D.MipLevels=1;
   G.device->CreateShaderResourceView(cor,&sv,rj_cpu(G,base));}
  rj_compartilhado(G,base);
  RjK k{};k.w=ow;k.h=oh;k.rx=r.x;k.ry=r.y;k.lado=r.L;k.n=nrrj::rede_de(r.L,nrrj::teto_inst());
  if(nrrj::canon2_inst()){k.n=nrrj::canon2_rede(r.L,nrorc::aplicada()?nrrj::n_cap():(std::min)(nrrj::npc2_teto.load(std::memory_order_relaxed),1024u),G.n_ult);G.n_ult=k.n;}
  if(nrrj::todos_inst()&&nrorc::aplicada()){k.n=nrrj::todos_rede_fino(r.L,nrrj::n_cap(),G.n_ult);G.n_ult=k.n;}

  if(inst0&&nrrj::rede_adapt.load(std::memory_order_relaxed)&&nrrj::rede_fixa.load(std::memory_order_relaxed)==0u){
   const unsigned na=nrrj::rede_adaptativa(G.adapt,r.L,nrrj::teto_inst(),det.lado,nrrj::rede_adapt_alvo.load(std::memory_order_relaxed),nrrj::rede_adapt_segura.load(std::memory_order_relaxed),G.quadro);
   if(na){if(na!=G.adapt_log){++G.adapt_trocas;char l[220]{};NRLOGF(l,sizeof l,"ROSTO_REDE_ADAPT %u->%u cabeca=%.0f recorte=%u cabeca_na_rede=%.0f quadro=%llu trocas=%llu",G.adapt_log,na,double(det.lado),r.L,double(det.lado)*double(na)/double(r.L),static_cast<unsigned long long>(G.quadro),static_cast<unsigned long long>(G.adapt_trocas));nrrj::log(l);G.adapt_log=na;}
    k.n=na;}}

  const bool ampliada=nrrj::inst_atual()==0&&nrrj::rede_fixa.load(std::memory_order_relaxed)>1u;
  {const unsigned tt=ampliada?~0u:nrrj::teto_tela(ow,oh);if(k.n>tt){
    if(!G.teto_tela_avisou){G.teto_tela_avisou=true;char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_TETO_TELA saida=%ux%u rede %u -> %u (lado menor da tela, multiplo de 128)",ow,oh,k.n,tt);nrrj::log(l);}
    k.n=tt;++G.teto_tela_n;}}

  if(nrgeo::ligada.load(std::memory_order_relaxed)){
   const unsigned antes=k.n;
   const unsigned tt=ampliada?~0u:nrrj::teto_tela(ow,oh);
   const unsigned teto=(std::min)({nrrj::n_cap(),(std::max)(antes,nrrj::teto_inst()),tt});
   k.n=nrgeo::rosto(antes,teto,r.L);
   if(k.n!=G.geo_log_de||antes!=G.geo_log_antes){G.geo_log_de=k.n;G.geo_log_antes=antes;
    char l[200]{};NRLOGF(l,sizeof l,"ROSTO_GEOMETRIA_NATIVA inst=%u rede %u -> %u (recorte %u, teto %u)",nrrj::inst_atual(),antes,k.n,r.L,teto);nrrj::log(l);}}

  k.n=(std::min)(k.n,nrrj::n_cap());
  k.off_ent=unsigned(nrrj::offset_ent(unsigned(s)));k.off_tag=unsigned(nrrj::offset_tag(unsigned(s)));
  nrd3d::marcador(c,L"nr_rosto_recorte");transition(c,G.compartilhado.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);

  const bool rr=nrrj::ruido_inst()&&G.ruido_q==G.quadro&&G.recorte_ruido_pso&&G.ruido_buf;
  if(rr){rj_ruido_k(k,G,ger);rj_ruido_uav(G,base);transition(c,G.ruido_buf.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);++G.ruido_recortes;}
  rj_bind(G,c,rr?G.recorte_ruido_pso.Get():G.recorte.Get(),base,k);c->Dispatch((k.n+7)/8,(k.n+7)/8,1);
  order_uav(c,G.compartilhado.Get());transition(c,G.compartilhado.Get(),write_state,D3D12_RESOURCE_STATE_COMMON);
  if(rr)transition(c,G.ruido_buf.Get(),write_state,D3D12_RESOURCE_STATE_COMMON);
  bool p3=false;
  if(inst0&&G.p3_ok&&nrrj::caminha_dela.load(std::memory_order_relaxed)==3&&det.lado>0.f){
   const int Lr=std::clamp(int(std::lround(2.2f*det.lado)),96,int((std::min)(ow,oh)));
   const int x0=std::clamp(int(std::lround(q.cx-.5f*Lr)),0,int(ow)-Lr),y0=std::clamp(int(std::lround(q.cy-.5f*Lr)),0,int(oh)-Lr);
   const unsigned bp=rj_base(G,rj_passe_pontos);
   {D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT(vista);sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sv.Texture2D.MipLevels=1;
    G.device->CreateShaderResourceView(cor,&sv,rj_cpu(G,bp));}
   {D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.NumElements=UINT(nrrj::roi_bytes/4);v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
    G.device->CreateUnorderedAccessView(G.p3_buf.Get(),nullptr,&v,rj_cpu(G,bp+3));}
   RjK kr{};kr.w=ow;kr.h=oh;kr.rx=x0;kr.ry=y0;kr.lado=unsigned(Lr);kr.fw=nrrj::roi_lado;kr.fh=nrrj::roi_lado;kr.off_ent=0;
   nrd3d::marcador(c,L"nr_rosto_pontos");transition(c,G.p3_buf.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);
   rj_bind(G,c,G.rosto_pso.Get(),bp,kr);c->Dispatch((nrrj::roi_lado+7)/8,(nrrj::roi_lado+7)/8,1);
   order_uav(c,G.p3_buf.Get());transition(c,G.p3_buf.Get(),write_state,D3D12_RESOURCE_STATE_COPY_SOURCE);
   c->CopyBufferRegion(G.p3_rb.Get(),UINT64(s)*nrrj::roi_bytes,G.p3_buf.Get(),0,nrrj::roi_bytes);
   transition(c,G.p3_buf.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
   auto& PE=nrrj::pontos().ent[unsigned(s)];PE.seq.store(0,std::memory_order_release);PE.x0=x0;PE.y0=y0;PE.l=Lr;p3=true;}
  const std::uint32_t seq=std::uint32_t(++G.seq);
  {const D3D12_GPU_VIRTUAL_ADDRESS va=G.marcador->GetGPUVirtualAddress()+4u*unsigned(s);
   const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER marca{va,UINT(seq)};const D3D12_WRITEBUFFERIMMEDIATE_MODE modo=D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_OUT;
   c2->WriteBufferImmediate(1,&marca,&modo);}
  S.seq=seq;S.geracao=ger;S.x=r.x;S.y=r.y;S.L=r.L;S.n=k.n;S.ow=ow;S.oh=oh;nrrj::cabeca_no_slot(S,q,nrrj::captura_pela_cabeca_do_job(inst0),det.zdx,det.zdy,det.lado,G.rampa.nivel);
  S.quadro=Qsj?p.policy.frame:0u;
  if(p3)nrrj::pontos().ent[unsigned(s)].seq.store(seq,std::memory_order_release);
  if(Qsj){
   S.palavra.store(nrrj::palavra(nrrj::Escrito,nrrj::id_de_quadro(p.policy.frame)),std::memory_order_release);nrrj::sj_conta().recortes.fetch_add(1,std::memory_order_relaxed);
   if(auto* f=nrrj::acordar_rede.load(std::memory_order_acquire))f();}
  else{S.palavra.store(nrrj::palavra(nrrj::Gravado,0),std::memory_order_release);rj_slot_de(p)=s;}
  ++G.recortes;g.recortes.fetch_add(1);G.ger_recorte=ger;G.quadro_recorte=G.quadro;
  q.recorte_x=r.x;q.recorte_y=r.y;q.recorte_l=int(r.L);
  nrrj::sucesso(nrrj::Falha::Render);
 }catch(...){nrrj::passageira(nrrj::Falha::Render,"excecao no recorte");}
}
void Backend::rj_aceito(int job) noexcept{
 if(!p_)return;auto& p=*p_;
 if(nrrj::inst_atual()==0&&nrrj::sj_q()&&job>0){nrrj::sj_marcar_job(job,p.policy.frame);
  if(job>nrrj::sj_job_aceito.load(std::memory_order_relaxed))nrrj::sj_job_aceito.store(job,std::memory_order_release);}

 if(nrrj::inst_atual()==0&&p.rj_grade>=0){auto& GS=nrrj::grade().slot[unsigned(p.rj_grade)];p.rj_grade=-1;auto w=GS.palavra.load(std::memory_order_acquire);
  if(job>0&&(w&255u)==nrrj::GGravado)GS.palavra.compare_exchange_strong(w,nrrj::palavra_g(nrrj::GEscrito,job),std::memory_order_acq_rel);}
 int& slot=rj_slot_de(p);
 if(nrorc::modo()==1&&!nrorc::estado().preparado.load(std::memory_order_acquire))return;
 if(nrrj::ativo()){auto& G=rjg();if(slot>=0)G.jobs_sem=0;else ++G.jobs_sem;}
 if(slot<0)return;
 auto& S=nrrj::global().slot[unsigned(slot)];slot=-1;
 if(job>0&&nrrj::estado_de(S.palavra.load(std::memory_order_acquire))==nrrj::Gravado)S.palavra.store(nrrj::palavra(nrrj::Escrito,job),std::memory_order_release);
}

void Backend::rj_segurar(void* command,void* destination,int job,float exposure,unsigned tone) noexcept{
 if(!p_)return;
 if(nrorc::modo()==1&&!nrorc::estado().preparado.load(std::memory_order_acquire))return;
 auto& p=*p_;auto& G=rjg();
 nrorc::EscopoOrigem eo(nrrj::inst_atual()>=1?nrorc::Origem::Npc:nrorc::Origem::Rosto);
 try{
  const unsigned N=nrrj::segura.load(std::memory_order_relaxed);
  if(!N||!nrrj::temporal_inst()||!nrrj::compor_rapido.load(std::memory_order_relaxed)||!nrrj::acumula.load(std::memory_order_relaxed)||!G.hist||!G.compor_r)return;
  const auto ger=nrrj::ger().load(std::memory_order_acquire);

  G.seg_motivo=G.seg_quadro+1!=G.quadro?1u:G.seg_ger!=unsigned(ger)?2u:G.seg_n>=N?3u:(!G.seg_fw||!G.seg_fh)?4u:0u;
  if(G.seg_motivo)return;
  auto* c=static_cast<ID3D12GraphicsCommandList*>(command);auto* dst=static_cast<ID3D12Resource*>(destination);if(!c||!dst)return;
  const auto dd=dst->GetDesc();const unsigned ow=unsigned(dd.Width),oh=dd.Height;
  if(ow!=G.seg_ow||oh!=G.seg_oh||G.variante>=rj_compor_max)return;
  const auto det=nrrj::ler_deteccao();
  if(det.pend)return;
  const float ganho=det.on?(std::min)(1.f,G.rampa.nivel+1.f/nrrj::rampa_quadros):G.rampa.nivel;if(!(ganho>0.f))return;
  const unsigned base=rj_base(G,1+G.variante++);
  rj_srv_hist(G,p,base);
  rj_compartilhado(G,base);
  {D3D12_UNORDERED_ACCESS_VIEW_DESC uv{};uv.Format=dd.Format;uv.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;G.device->CreateUnorderedAccessView(dst,nullptr,&uv,rj_cpu(G,base+4));}
  if(!rj_hist(G,base))return;

  const int dx=det.tem?int(std::lround(det.vx)):0,dy=det.tem?int(std::lround(det.vy)):0;
  const int fx=std::clamp(G.seg_fx+dx,0,(std::max)(0,int(ow)-int(G.seg_fw))),fy=std::clamp(G.seg_fy+dy,0,(std::max)(0,int(oh)-int(G.seg_fh)));
  RjK k{};k.w=ow;k.h=oh;k.lado=1;k.n=1;k.off_tag=unsigned(nrrj::offset_tag(0));k.job=0xffffffffu;k.job2=0;
  rj_k_base(p,k,exposure,tone);k.fx=fx;k.fy=fy;k.fw=G.seg_fw;k.fh=G.seg_fh;k.ganho=ganho;
  k.acum=1u;k.gama=nrrj::acum_gama.load(std::memory_order_relaxed);k.quadro=unsigned(G.quadro);k.geracao=unsigned(ger);
  k.bytes_meio=unsigned(nrrj::bytes_meio_i());k.janela=rj_jan();k.rap_r=1u;
  k.bx_forca=nrrj::baixa_forca.load(std::memory_order_relaxed);
  nrd3d::marcador(c,L"nr_rosto_segura");transition(c,G.compartilhado.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);transition(c,G.hist.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);
  const bool cam=rj_camada_antes(G,c,base,ow,oh,p.policy.frame,k);
  k.rap_r|=protege_realce_codigo()<<24;
  rj_bind(G,c,G.compor_r.Get(),base,k);c->Dispatch((k.fw+7)/8,(k.fh+7)/8,1);
  if(cam)rj_camada_depois(c,p.policy.frame,k);
  order_uav(c,dst);order_uav(c,G.hist.Get());transition(c,G.hist.Get(),write_state,D3D12_RESOURCE_STATE_COMMON);transition(c,G.compartilhado.Get(),write_state,D3D12_RESOURCE_STATE_COMMON);
  G.composto=true;++G.segurados;++G.seg_n;G.seg_quadro=G.quadro;G.seg_fx=fx;G.seg_fy=fy;
  auto& q=nrrj::quadro_render();q.composto=1;q.job=job;q.ganho=ganho;q.idade=-2;
  nrrj::sucesso(nrrj::Falha::Render);
 }catch(...){nrrj::passageira(nrrj::Falha::Render,"excecao ao segurar o rosto");}
}

void Backend::rj_compor(void* command,void* destination,unsigned formato,int job,int age,float exposure,unsigned tone) noexcept{
 if(!p_||!nrrj::ativo())return;
 if(nrorc::modo()==1&&!nrorc::estado().preparado.load(std::memory_order_acquire))return;
 auto& p=*p_;auto& G=rjg();
 nrorc::EscopoOrigem eo(nrrj::inst_atual()>=1?nrorc::Origem::Npc:nrorc::Origem::Rosto);
 try{
  if(!G.pronto||job<=0||age<0||!p.histories_read||!p.history[1]||!p.history[2])return;
  {const int fa=nrrj::teste_falha_ini.load(std::memory_order_relaxed),fb=nrrj::teste_falha_fim.load(std::memory_order_relaxed);
   if(fb>fa&&p.policy.frame>=std::uint64_t(fa)&&p.policy.frame<std::uint64_t(fb))return;}

  const unsigned teto=nrrj::teto_jobs();

  const bool sj=nrrj::inst_atual()==0&&nrrj::sj_q()>0;
  nrrj::SjVista sjv[nrrj::slots]{};
  if(sj)for(unsigned k2=0;k2<nrrj::slots;++k2){const auto& Sk=nrrj::global().slot[k2];sjv[k2].palavra=Sk.palavra.load(std::memory_order_acquire);sjv[k2].quadro=Sk.quadro;sjv[k2].geracao=Sk.geracao;}
  int s=sj?nrrj::sj_escolher(sjv,nrrj::slots,nrrj::ger().load(std::memory_order_acquire),p.policy.frame,Policy::history_count,0):nrrj::slot_do_job(job),jp=job,ap=age;
  if(sj&&s>=0){jp=nrrj::job_de(sjv[s].palavra);ap=p.policy.age_frame(sjv[s].quadro);if(ap<0)s=-1;}
  if(s<0){
   const int sa=sj?-1:nrrj::slot_anterior(job);const int ja=sa>=0?nrrj::job_de(nrrj::global().slot[unsigned(sa)].palavra.load(std::memory_order_acquire)):0;const int aa=ja>0?p.policy.age(ja):-1;
   if(sj||teto<2||sa<0||job-ja>int(teto)||aa<0||unsigned(aa)>=Policy::history_count){++G.sem_delta;const auto seg0=G.segurados;G.seg_motivo=9;rj_segurar(command,destination,job,exposure,tone);
    if(G.segurados==seg0){++G.sem_rosto;
     if(G.sem_rosto<=64&&!(nrrj::leve_npc()&&!G.recortes)){
     char l[260]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_SEM_ROSTO n=%llu quadro=%llu job=%d idade=%d ja=%d aa=%d teto=%u motivo=%u seg_n=%u seg_quadro=%llu ult_recorte=%llu jobs_sem=%u",
      static_cast<unsigned long long>(G.sem_rosto),static_cast<unsigned long long>(G.quadro),job,age,ja,aa,teto,G.seg_motivo,G.seg_n,static_cast<unsigned long long>(G.seg_quadro),static_cast<unsigned long long>(G.quadro_recorte),G.jobs_sem);nrrj::log(l);}}
    return;}
   s=sa;jp=ja;ap=aa;++G.reusos;}
  const auto& S=nrrj::global().slot[unsigned(s)];
  if(S.geracao!=nrrj::ger().load(std::memory_order_acquire)){++G.geracao_velha;return;}
  auto* c=static_cast<ID3D12GraphicsCommandList*>(command);auto* dst=static_cast<ID3D12Resource*>(destination);
  if(!c||!dst)return;
  const auto dd=dst->GetDesc();const unsigned ow=unsigned(dd.Width),oh=dd.Height;
  if(ow!=S.ow||oh!=S.oh){++G.recusas;return;}
  if(G.variante>=rj_compor_max)return;
  const unsigned base=rj_base(G,1+G.variante++);
  rj_srv_hist(G,p,base);
  rj_compartilhado(G,base);
  {D3D12_UNORDERED_ACCESS_VIEW_DESC uv{};uv.Format=DXGI_FORMAT(formato);uv.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;G.device->CreateUnorderedAccessView(dst,nullptr,&uv,rj_cpu(G,base+4));}

  const int M=std::clamp(int(S.L/4u),32,256);
  const int x0=(std::max)(S.x-M,0),y0=(std::max)(S.y-M,0),x1=(std::min)(S.x+int(S.L)+M,int(ow)),y1=(std::min)(S.y+int(S.L)+M,int(oh));
  if(x1<=x0||y1<=y0)return;
  const auto det=nrrj::ler_deteccao();

  int ex0=x0,ey0=y0,ex1=x1,ey1=y1;
  if(nrrj::compor_elipse.load(std::memory_order_relaxed)&&S.hl>0.f){
   const float fq=float(ap+1);
   const float mx=std::clamp(std::fabs(det.vx)*fq*1.5f+32.f,32.f,float(M)),my=std::clamp(std::fabs(det.vy)*fq*1.5f+32.f,32.f,float(M));
   auto caixa=[&](float hx,float hy,float hl,int& a0,int& b0,int& a1,int& b1){
    const float ax=float(NR_AREA_SEMI_X)*float(NR_AREA_BORDA1)*hl+24.f+mx,ay=float(NR_AREA_SEMI_Y)*float(NR_AREA_BORDA1)*hl+24.f+my;
    a0=int(std::floor(hx-ax));b0=int(std::floor(hy-ay));a1=int(std::ceil(hx+ax));b1=int(std::ceil(hy+ay));};
   int a0,b0,a1,b1;caixa(S.hx,S.hy,S.hl,a0,b0,a1,b1);
   ex0=(std::max)(x0,a0);ey0=(std::max)(y0,b0);ex1=(std::min)(x1,a1);ey1=(std::min)(y1,b1);
  }
  const float ganho=det.on?(std::min)(1.f,G.rampa.nivel+1.f/nrrj::rampa_quadros):G.rampa.nivel;
  if(!(ganho>0.f))return;
  RjK k{};k.w=ow;k.h=oh;k.rx=S.x;k.ry=S.y;k.lado=S.L;k.n=S.n;k.off_ent=unsigned(nrrj::offset_ent(unsigned(s)));k.off_tag=unsigned(nrrj::offset_tag(unsigned(s)));
  k.job=unsigned(jp);k.idade=unsigned(ap);rj_k_base(p,k,exposure,tone);
  k.fx=x0;k.fy=y0;k.fw=unsigned(x1-x0);k.fh=unsigned(y1-y0);
  if(ex1>ex0&&ey1>ey0&&(ex0!=x0||ey0!=y0||ex1!=x1||ey1!=y1)){k.fx=ex0;k.fy=ey0;k.fw=unsigned(ex1-ex0);k.fh=unsigned(ey1-ey0);++G.compor_elipse_n;}
  G.compor_px+=std::uint64_t(k.fw)*k.fh;k.ganho=ganho;
  k.hx=S.zx;k.hy=S.zy;

  if(jp>G.job_ult){if(G.job_ult>0&&G.quadro>G.quadro_troca)G.k_quadros=.8f*G.k_quadros+.2f*std::clamp(float(G.quadro-G.quadro_troca),1.f,8.f);G.job_ult=jp;G.quadro_troca=G.quadro;}

  const bool rapido=nrrj::compor_rapido.load(std::memory_order_relaxed);
  if(rapido&&nrrj::acumula.load(std::memory_order_relaxed)&&!G.compor_r&&!G.compor_r_tentado){G.compor_r_tentado=true;if(!pipeline(G.device.Get(),G.root.Get(),g_nr_reproject_rjcomporr,G.compor_r)){G.compor_r.Reset();nrrj::log("ROSTO_JOGADOR_COMPOR_RAPIDO PSO recusado: sem acumulacao");}else nrrj::log("ROSTO_JOGADOR_COMPOR_RAPIDO ligado");}
  if(rapido&&G.compor_r&&nrrj::acumula.load(std::memory_order_relaxed)&&nrrj::temporal_inst()&&k.fw<=rj_jan()&&k.fh<=rj_jan()&&rj_hist(G,base)){
   const float kq=std::clamp(G.k_quadros,1.f,8.f);
   auto por_quadro=[&](float a){return nrac::por_quadro(a,kq);};
   k.acum=1u;k.a_alta=por_quadro(nrrj::acum_alta.load(std::memory_order_relaxed));
   k.gama=nrrj::acum_gama.load(std::memory_order_relaxed);k.quadro=unsigned(G.quadro);k.geracao=unsigned(nrrj::ger().load(std::memory_order_acquire));
   k.bytes_meio=unsigned(nrrj::bytes_meio_i());k.janela=rj_jan();++G.acumulados;

   k.bx_forca=nrrj::baixa_forca.load(std::memory_order_relaxed);

   {const auto tb=nrac::taxas_baixa(nrrj::acum_baixa.load(std::memory_order_relaxed),nrrj::baixa_lenta.load(std::memory_order_relaxed),
        nrrj::baixa_rapida.load(std::memory_order_relaxed),nrrj::baixa_mov_px.load(std::memory_order_relaxed),kq);
    k.a_baixa=tb.a_baixa;if(tb.a_lenta>0.f){k.a_lenta=tb.a_lenta;k.a_rapida=tb.a_rapida;k.mov_px=tb.mov_px;
    k.pos_mov=nrrj::baixa_pos_mov.load(std::memory_order_relaxed);}}

   {const float gp=nrrj::acum_gama_parado.load(std::memory_order_relaxed);if(gp>0.f){k.gama_parado=gp;if(!(k.mov_px>0.f))k.mov_px=nrrj::baixa_mov_px.load(std::memory_order_relaxed);}}

   {const float t=nrrj::frescor.load(std::memory_order_relaxed);if(t>0.f&&nrrj::frescor_inst()&&nrrj::frescor_pronto.load(std::memory_order_acquire))std::memcpy(&k.pad3,&t,4);}
  }

  {
   constexpr float peso=.5f,limiar=.03f;
   k.peso=nrrj::temporal_inst()?peso:1.f;k.limiar=limiar;
   {const float t4=nrrj::acum_limiar.load(std::memory_order_relaxed);if(k.acum&&t4>0.f)k.limiar=t4;}

   {const float tn=nrrj::npc_limiar.load(std::memory_order_relaxed);if(k.acum&&nrrj::inst_atual()>=1&&tn>0.f)k.limiar=tn;}

   const int s2=sj?nrrj::sj_escolher(sjv,nrrj::slots,S.geracao,p.policy.frame,Policy::history_count,jp):nrrj::slot_anterior(jp);
   if(s2>=0&&(peso<1.f||teto>=2)){const auto& S2=nrrj::global().slot[unsigned(s2)];const int j2=nrrj::job_de(S2.palavra.load(std::memory_order_acquire));const int a2=sj?p.policy.age_frame(S2.quadro):p.policy.age(j2);
    if(a2>=0&&unsigned(a2)<Policy::history_count&&S2.geracao==S.geracao&&S2.ow==S.ow&&S2.oh==S.oh&&(sj||teto<2||job-j2<=int(teto))){k.sozinho=teto>=2?1u:0u;
     k.off_ent2=unsigned(nrrj::offset_ent(unsigned(s2)));k.off_tag2=unsigned(nrrj::offset_tag(unsigned(s2)));k.job2=unsigned(j2);k.idade2=unsigned(a2);
     k.rx2=S2.x;k.ry2=S2.y;k.lado2=S2.L;k.n2=S2.n;k.hx2=S2.zx;k.hy2=S2.zy;++G.medias;

     if(nrrj::compor_elipse.load(std::memory_order_relaxed)&&S2.hl>0.f&&(k.fx!=x0||k.fy!=y0||int(k.fw)!=x1-x0||int(k.fh)!=y1-y0)){
      const float fq=float(a2+1);
      const float mx=std::clamp(std::fabs(det.vx)*fq*1.5f+32.f,32.f,float(M)),my=std::clamp(std::fabs(det.vy)*fq*1.5f+32.f,32.f,float(M));
      const float ax=.575f*S2.hl+24.f+mx,ay=.69f*S2.hl+24.f+my;
      const int u0=(std::max)(x0,(std::min)(k.fx,int(std::floor(S2.hx-ax)))),v0=(std::max)(y0,(std::min)(k.fy,int(std::floor(S2.hy-ay))));
      const int u1=(std::min)(x1,(std::max)(k.fx+int(k.fw),int(std::ceil(S2.hx+ax)))),v1=(std::min)(y1,(std::max)(k.fy+int(k.fh),int(std::ceil(S2.hy+ay))));
      if(u1>u0&&v1>v0&&(!k.acum||(unsigned(u1-u0)<=rj_jan()&&unsigned(v1-v0)<=rj_jan()))){
       G.compor_px-=std::uint64_t(k.fw)*k.fh;k.fx=u0;k.fy=v0;k.fw=unsigned(u1-u0);k.fh=unsigned(v1-v0);G.compor_px+=std::uint64_t(k.fw)*k.fh;}}}}}

  {
   constexpr bool zero=false;
   if(zero||(nrrj::ablacao.load(std::memory_order_relaxed)&1u)){G.composto=true;auto& qz=nrrj::quadro_render();qz.composto=1;qz.job=job;qz.ganho=0.f;qz.idade=ap;return;}}
  if(det.pend&&!(k.acum&&nrrj::compor_rapido.load(std::memory_order_relaxed)&&G.compor_r&&det.camq))return;
  nrd3d::marcador(c,L"nr_rosto_compor");transition(c,G.compartilhado.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);
  if(k.acum)transition(c,G.hist.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);

  ID3D12PipelineState* pso=G.compor.Get();
  if(k.acum){
   pso=G.compor_r.Get();
   if(G.compor_r&&nrrj::diag_hist.load(std::memory_order_relaxed))k.rap_r|=256u;
   if(G.compor_r&&nrrj::ceu_fora.load(std::memory_order_relaxed))k.rap_r|=512u;
   if(G.compor_r&&nrrj::segura.load(std::memory_order_relaxed)&&nrrj::segura_pixel.load(std::memory_order_relaxed))k.rap_r|=2u;
   if(G.compor_r&&k.acum&&nrrj::acum_alta_idade.load(std::memory_order_relaxed))k.rap_r|=128u;
   if(G.compor_r&&k.acum&&nrrj::acum_alta_mov.load(std::memory_order_relaxed)){k.rap_r|=64u;if(!(k.mov_px>0.f))k.mov_px=nrrj::baixa_mov_px.load(std::memory_order_relaxed);}
   {const unsigned bs=nrrj::borda_suave.load(std::memory_order_relaxed);if(G.compor_r&&(bs&1u))k.rap_r|=4u;if(G.compor_r&&(bs&2u)){k.rap_r|=8u;const float lb=nrrj::borda_largura.load(std::memory_order_relaxed);std::memcpy(&k.pad_r0,&lb,4);}}

   if(G.compor_r&&nrrj::inst_atual()>=1&&nrrj::npc_vida.load(std::memory_order_relaxed)>=2&&det.camq&&!(k.rap_r&8u)){k.rap_r|=8u;const float lz=0.f;std::memcpy(&k.pad_r0,&lz,4);}

   const bool fb=nrrj::frescor_borrado.load(std::memory_order_relaxed)&&nrrj::frescor.load(std::memory_order_relaxed)>0.f;

   const std::uint64_t qid=p.policy.frame^(std::uint64_t(nrrj::epoca_backend.load(std::memory_order_relaxed))<<44);
   const bool reg_ok=G.limpa_q==qid&&G.copia&&G.copia_fmt==dd.Format&&k.fx>=G.limpa_x0&&k.fy>=G.limpa_y0&&
                     k.fx+int(k.fw)<=G.limpa_x0+int(G.limpa_w)&&k.fy+int(k.fh)<=G.limpa_y0+int(G.limpa_h);
   const bool tela_ok=rj_limpa().r&&rj_limpa().quadro==qid&&rj_limpa().fmt==dd.Format&&rj_limpa().w==dd.Width&&rj_limpa().h==dd.Height;
   const bool limpa_ok=reg_ok||tela_ok;
   const bool cor0=nrrjcl::compoe_cor()&&!nrrj::delta_inteiro()&&limpa_ok;
   const bool usa_limpa=limpa_ok&&(cor0||fb);

   const bool regiao_furada=G.limpa_q==qid&&!reg_ok;
   if(const float gt=nrrj::borda_teto.load(std::memory_order_relaxed);(gt>0.f||fb||cor0)&&G.compor_r&&!G.copia_falhou&&!regiao_furada&&k.fw<=rj_jan()&&k.fh<=rj_jan()){
    const DXGI_FORMAT fr=dd.Format;
    if(rj_copia_garantir(G,fr)){
     if(usa_limpa&&reg_ok){k.off_t0=unsigned(k.fx-G.limpa_x0)|(unsigned(k.fy-G.limpa_y0)<<16);}
     else{
     if(!usa_limpa)transition(c,dst,write_state,D3D12_RESOURCE_STATE_COPY_SOURCE);transition(c,G.copia.Get(),read_state,D3D12_RESOURCE_STATE_COPY_DEST);
     D3D12_TEXTURE_COPY_LOCATION a{};a.pResource=G.copia.Get();a.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;a.SubresourceIndex=0;
     D3D12_TEXTURE_COPY_LOCATION b{};b.pResource=usa_limpa?rj_limpa().r.Get():dst;b.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;b.SubresourceIndex=0;
     D3D12_BOX box{UINT(k.fx),UINT(k.fy),0,UINT(k.fx)+k.fw,UINT(k.fy)+k.fh,1};
     c->CopyTextureRegion(&a,0,0,0,&b,&box);
     transition(c,G.copia.Get(),D3D12_RESOURCE_STATE_COPY_DEST,read_state);if(!usa_limpa)transition(c,dst,D3D12_RESOURCE_STATE_COPY_SOURCE,write_state);}
     D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT(formato);sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sv.Texture2D.MipLevels=1;
     G.device->CreateShaderResourceView(G.copia.Get(),&sv,rj_cpu(G,base));
     if(gt>0.f){k.rap_r|=16u;std::memcpy(&k.pad_r1,&gt,4);}if(fb)k.rap_r|=4096u;++G.tetos;
     if(cor0&&S.hl>0.f){k.rap_r|=8192u|(nrrjcl::peso_jogo_8()<<16);k.cor_hx=S.hx;k.cor_hy=S.hy;k.cor_hl=S.hl;++G.cor0_n;
      if(G.cor0_n<=2||(G.cor0_n&1023u)==0){char l[200]{};NRLOGF(l,sizeof l,"ROSTO_COR_COMPOR inst=%d quadros=%llu copias_tela=%llu regioes=%llu sem_rosto=%llu",nrrj::inst_atual(),static_cast<unsigned long long>(G.cor0_n),rj_limpa().copias,rj_limpa().regioes,rj_limpa().sem_rosto);nrrj::log(l);}}
     if(G.tetos<=2||(G.tetos&1023u)==0){char l[120]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_TETO gama=%.2f quadros=%llu",double(gt),static_cast<unsigned long long>(G.tetos));nrrj::log(l);}}}}
  const bool cam=rj_camada_antes(G,c,base,ow,oh,p.policy.frame,k);
  k.rap_r|=protege_realce_codigo()<<24;
  rj_bind(G,c,pso,base,k);c->Dispatch((k.fw+7)/8,(k.fh+7)/8,1);
  if(cam)rj_camada_depois(c,p.policy.frame,k);
  order_uav(c,dst);if(k.acum){order_uav(c,G.hist.Get());transition(c,G.hist.Get(),write_state,D3D12_RESOURCE_STATE_COMMON);}transition(c,G.compartilhado.Get(),write_state,D3D12_RESOURCE_STATE_COMMON);
  G.composto=true;++G.composicoes;nrrj::global().composicoes.fetch_add(1);
  auto& q=nrrj::quadro_render();q.composto=1;q.job=job;q.ganho=ganho;q.idade=ap;
  ++G.idade_hist[unsigned((std::min)(ap,17))];
  if(G.ger_pend){G.ger_pend=false;
   const std::uint64_t ger=nrrj::ger().load(std::memory_order_acquire);
   char l[200]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_PRIMEIRO_DELTA quadros=%llu geracao=%llu troca_de_alvo=%d job=%d",static_cast<unsigned long long>(G.quadro-G.quadro_ger),
    static_cast<unsigned long long>(ger),ger==nrrj::geracao_sem_rampa().load(std::memory_order_relaxed)?1:0,job);nrrj::log(l);}
  if(k.acum&&pso==G.compor_r.Get()){G.seg_quadro=G.quadro;G.seg_ger=k.geracao;G.seg_n=0;G.seg_fx=k.fx;G.seg_fy=k.fy;G.seg_fw=k.fw;G.seg_fh=k.fh;G.seg_ow=ow;G.seg_oh=oh;}
  if(G.composicoes<=6||(G.composicoes&255u)==0){
   char l[640]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_QUADRO composicoes=%llu job=%d idade=%d slot=%d recorte=%d,%d,%u ganho=%.2f recortes=%llu sem_slot=%llu sem_delta=%llu geracao_velha=%llu medias=%llu reusos=%llu pulados_cada=%llu marcados=%llu avaliacoes=%llu acum=%llu kq=%.2f elipse=%llu px_med=%.0f segurados=%llu cada_quadros=%u por_quadros=%llu sem_rosto=%llu quadro=%llu",
    static_cast<unsigned long long>(G.composicoes),job,age,s,S.x,S.y,S.L,double(ganho),static_cast<unsigned long long>(G.recortes),static_cast<unsigned long long>(G.sem_slot),
    static_cast<unsigned long long>(G.sem_delta),static_cast<unsigned long long>(G.geracao_velha),static_cast<unsigned long long>(G.medias),static_cast<unsigned long long>(G.reusos),static_cast<unsigned long long>(G.pulados_cada),static_cast<unsigned long long>(nrrj::global().prontos.load()),static_cast<unsigned long long>(nrrj::global().avaliacoes.load()),static_cast<unsigned long long>(G.acumulados),double(G.k_quadros),static_cast<unsigned long long>(G.compor_elipse_n),G.composicoes?double(G.compor_px)/double(G.composicoes+1):0.,static_cast<unsigned long long>(G.segurados),
    nrrj::cada_quadros_efetivo(),static_cast<unsigned long long>(G.por_quadros),static_cast<unsigned long long>(G.sem_rosto),static_cast<unsigned long long>(G.quadro));
   nrrj::log(l);
   if(nrrj::inst_atual()==0){const auto& H=G.idade_hist;auto& C=nrrj::sj_conta();char li[640]{};NRLOGF(li,sizeof li,"ROSTO_JOGADOR_IDADES semjob=%u q0..17=%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu recortes=%llu avaliados=%llu velhos=%llu futuro=%llu sem_pub=%llu estendido=%llu zero=%llu marcador_tarde=%llu intercalados=%llu sem_tabela=%llu tabela_tarde=%llu adiados=%llu pose_pulados=%llu",
    nrrj::sj_q(),H[0],H[1],H[2],H[3],H[4],H[5],H[6],H[7],H[8],H[9],H[10],H[11],H[12],H[13],H[14],H[15],H[16],H[17],C.recortes.load(),C.avaliados.load(),C.velhos.load(),C.futuro.load(),C.sem_pub.load(),C.estendido.load(),C.zero.load(),C.marcador_tarde.load(),C.intercalados.load(),C.sem_tabela.load(),C.tabela_tarde.load(),C.adiados.load(),C.pose_pulados.load());nrrj::log(li);}}
  nrrj::sucesso(nrrj::Falha::Render);
 }catch(...){nrrj::passageira(nrrj::Falha::Render,"excecao na composicao");}
}
}

namespace nrreproject {

void Backend::rj_camada(void* command,void* destination,float exposure,unsigned tone) noexcept{
 if(!p_||!nrrj::ativo()||!nrrj::camada.load(std::memory_order_relaxed)||nrrj::diag_hist.load(std::memory_order_relaxed))return;
 auto& p=*p_;auto& C=rjc();
 try{
  nrrj::Inst i0(0);auto& G=rjg();
  if(!G.pronto||C.falhou||!p.histories_read||!p.history[1]||!p.history[2])return;
  auto* c=static_cast<ID3D12GraphicsCommandList*>(command);auto* dst=static_cast<ID3D12Resource*>(destination);if(!c||!dst)return;
  const auto dd=dst->GetDesc();const unsigned ow=unsigned(dd.Width),oh=dd.Height;
  if(!rj_camada_pronta(G,ow,oh))return;
  const std::uint64_t q=p.policy.frame;const unsigned cur=unsigned(q&1u);

  if(C.fila.cortar_epoca(nrrj::epoca_backend.load(std::memory_order_acquire))){C.quadro_ult=~0ull;
   char l[160]{};NRLOGF(l,sizeof l,"ROSTO_CAMADA_EPOCA corte (backend novo) quadro=%llu vivos=%zu cortes=%llu",static_cast<unsigned long long>(q),C.fila.v.size(),static_cast<unsigned long long>(C.fila.cortes));nrrj::log(l);}
  const bool ant_ok=C.quadro_ult!=~0ull&&C.quadro_ult+1==q;
  const int segura=nrrj::camada_segura.load(std::memory_order_relaxed);
  const float meia=nrrj::camada_meia.load(std::memory_order_relaxed),margem=nrrj::camada_margem.load(std::memory_order_relaxed);
  const int zera=segura+int(std::ceil(6.f*meia))+1;
  C.fila.podar(q,zera);
  const bool limitada=nrrj::camada_regiao_limitada.load(std::memory_order_relaxed);
  int b[4]{};nrrjcam::regiao(C.fila,q,margem,int(ow),int(oh),b,limitada);

  int r[4]{b[0],b[1],b[2],b[3]};bool vazio=b[2]<=b[0];
  for(const auto* m:{C.Bm[cur],C.Bm[cur^1u]})if(m[2]>m[0]&&m[3]>m[1]){
   if(vazio){r[0]=m[0];r[1]=m[1];r[2]=m[2];r[3]=m[3];vazio=false;}
   else{r[0]=(std::min)(r[0],m[0]);r[1]=(std::min)(r[1],m[1]);r[2]=(std::max)(r[2],m[2]);r[3]=(std::max)(r[3],m[3]);}}
  if(vazio){C.quadro_ult=q;return;}
  const unsigned base=rj_base(G,rj_passe_camada);
  rj_srv_hist(G,p,base);
  {D3D12_UNORDERED_ACCESS_VIEW_DESC uv{};uv.Format=dd.Format;uv.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;G.device->CreateUnorderedAccessView(dst,nullptr,&uv,rj_cpu(G,base+4));}
  rj_camada_uav(G,base);
  RjK k{};k.w=ow;k.h=oh;k.fx=r[0];k.fy=r[1];k.fw=unsigned(r[2]-r[0]);k.fh=unsigned(r[3]-r[1]);
  k.rx=b[0];k.ry=b[1];k.lado=unsigned(b[2]);k.n=unsigned(b[3]);
  k.fatia=p.constants.slice;k.mascara=p.constants.mask;k.historico=Policy::history_count;
  k.prof_rel=p.constants.depth_relative;k.prof_abs=p.constants.depth_absolute;k.expo=exposure;k.tonemap=tone&1u;
  k.ganho=nrrj::camada_forca.load(std::memory_order_relaxed);k.pad_r2=cur;k.idade2=ant_ok?1u:0u;
  k.sozinho=unsigned(segura);k.peso=std::pow(.5f,1.f/(std::max)(meia,1.f));{const float z=float(zera);std::memcpy(&k.pad3,&z,4);}
  if(nrrj::ceu_fora.load(std::memory_order_relaxed))k.rap_r|=512u;
   if(nrrj::camada_normaliza.load(std::memory_order_relaxed))k.rap_r|=1024u;
   if(nrrj::camada_fresco.load(std::memory_order_relaxed))k.rap_r|=2048u;
  nrd3d::marcador(c,L"nr_rosto_camada");transition(c,C.buf.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);
  k.rap_r|=protege_realce_codigo()<<24;
  rj_bind(G,c,C.pso.Get(),base,k);c->Dispatch((k.fw+7)/8,(k.fh+7)/8,1);
  order_uav(c,dst);order_uav(c,C.buf.Get());transition(c,C.buf.Get(),write_state,D3D12_RESOURCE_STATE_COMMON);
  std::memcpy(C.Bm[cur],b,sizeof b);C.quadro_ult=q;++C.passes;C.px+=std::uint64_t(k.fw)*k.fh;
  if(C.passes<=4||(C.passes&1023u)==0){char l[320]{};NRLOGF(l,sizeof l,"ROSTO_CAMADA passes=%llu quadro=%llu regiao=%d,%d,%ux%u B=%d,%d,%d,%d frescos=%llu px_med=%llu cadeia=%d vivos=%zu cortes=%llu teto=%llu limitada=%d px=%llu",
   static_cast<unsigned long long>(C.passes),static_cast<unsigned long long>(q),k.fx,k.fy,k.fw,k.fh,b[0],b[1],b[2],b[3],static_cast<unsigned long long>(C.fila.n),
   static_cast<unsigned long long>(C.px/C.passes),ant_ok?1:0,C.fila.v.size(),static_cast<unsigned long long>(C.fila.cortes),static_cast<unsigned long long>(C.fila.podados_teto),limitada?1:0,static_cast<unsigned long long>(k.fw)*k.fh);nrrj::log(l);}
  C.falhas=0;
 }catch(...){

  const unsigned mx=nrrj::falha_max.load(std::memory_order_relaxed);
  if(mx<=1u){C.falhou=true;nrrj::envenenar("excecao na camada do rosto");}
  else if(++C.falhas>=mx){C.falhou=true;nrrj::log("ROSTO_CAMADA excecao seguida: camada desligada (os rostos seguem sem ela)");}
  else nrrj::log("ROSTO_CAMADA excecao: quadro sem a camada (tenta de novo no proximo)");}
}
}
