#include "../../../runtime/rosto_jogador_state.h"
#include "../../../runtime/reuso_state.h"
#include "nr_reproject_rjrecorte.h"
#include "nr_reproject_rjcompor.h"
#include "nr_reproject_rjgrade.h"
#include "nr_reproject_rjcomporr.h"
namespace nrreproject {
namespace {
struct RjK{unsigned w,h;int rx,ry;unsigned lado,n,off_ent,off_tag;unsigned job,idade,fatia,mascara;unsigned historico;int fx,fy;unsigned fw;
           unsigned fh;float expo;unsigned tonemap;float ganho;float hx,hy,prof_rel,prof_abs;float camada0,camada1;unsigned off_ent2,off_tag2;
           unsigned job2,idade2;int rx2,ry2;unsigned lado2,n2;float hx2,hy2;float peso,limiar;unsigned sozinho,pad3;
           float a_baixa,a_alta,gama;unsigned acum;unsigned quadro,geracao,bytes_meio,janela;
           unsigned rap_r,pad_r0,pad_r1,pad_r2;
           float bx_forca,a_lenta,a_rapida,mov_px;};
static_assert(sizeof(RjK)==224);

constexpr unsigned rj_janela=2304,rj_hcab=64;
constexpr UINT64 rj_hist_bytes=2ull*(rj_hcab+UINT64(rj_janela)*rj_janela*16ull);

constexpr unsigned rj_srvs=3,rj_uavs=3,rj_set=rj_srvs+rj_uavs,rj_passes=5,rj_aneis=8,rj_compor_max=3;
struct RjGradeK{unsigned gw,gh,off_d,off_mv,fatia,inv,pad0,pad1;};
static_assert(sizeof(RjGradeK)==32);
struct RjGlobal{
 ComPtr<ID3D12Device> device;ComPtr<ID3D12RootSignature> root;ComPtr<ID3D12PipelineState> recorte,compor,compor_r;bool compor_r_tentado{};

 std::uint64_t seg_quadro{},seg_ger{},segurados{};unsigned seg_n{};int seg_fx{},seg_fy{};unsigned seg_fw{},seg_fh{},seg_ow{},seg_oh{};
 ComPtr<ID3D12DescriptorHeap> heap;unsigned stride{};
 ComPtr<ID3D12Resource> compartilhado,marcador;

 ComPtr<ID3D12PipelineState> grade_pso;ComPtr<ID3D12Resource> grade_buf;bool grade_pronto{},grade_falhou{};std::uint64_t grade_seq{};
 ComPtr<ID3D12Resource> hist;bool hist_tentado{};int job_ult{};std::uint64_t quadro_troca{};float k_quadros{4.f};std::uint64_t acumulados{};
 bool pronto{},pre_logado{};std::uint64_t quadro{},seq{};unsigned variante{};
 nrrj::Rampa rampa;bool composto_ant{},composto{};
 std::uint64_t recortes{},composicoes{},quadros_compostos{},sem_slot{},recusas{},sem_delta{},geracao_velha{},medias{},reusos{},pulados_cada{};

 unsigned jobs_sem{};std::uint64_t ger_recorte{};

 std::uint64_t quadro_recorte{},por_quadros{},sem_rosto{};unsigned seg_motivo{};
 unsigned n_ult{};std::uint64_t compor_elipse_n{},compor_px{};

 ComPtr<ID3D12Resource> copia;DXGI_FORMAT copia_fmt{};bool copia_falhou{};std::uint64_t tetos{};
};
RjGlobal& rjg(){static auto* g=new RjGlobal;return *g;}
bool rj_buffer(ID3D12Device* d,D3D12_HEAP_TYPE tipo,UINT64 bytes,D3D12_RESOURCE_STATES estado,D3D12_RESOURCE_FLAGS flags,ComPtr<ID3D12Resource>& r,D3D12_HEAP_FLAGS hf=D3D12_HEAP_FLAG_NONE){
 D3D12_HEAP_PROPERTIES hp{};hp.Type=tipo;D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=bytes;bd.Height=1;bd.DepthOrArraySize=1;bd.MipLevels=1;
 bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;bd.Flags=flags;
 return SUCCEEDED(d->CreateCommittedResource(&hp,hf,&bd,estado,nullptr,IID_PPV_ARGS(&r)));
}
D3D12_CPU_DESCRIPTOR_HANDLE rj_cpu(RjGlobal& G,unsigned i){auto h=G.heap->GetCPUDescriptorHandleForHeapStart();h.ptr+=SIZE_T(i)*G.stride;return h;}
unsigned rj_base(RjGlobal& G,unsigned passe){return ((unsigned(G.quadro%rj_aneis))*rj_passes+passe)*rj_set;}
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
}

bool rj_hist(RjGlobal& G,unsigned base){
 if(!G.hist&&!G.hist_tentado){G.hist_tentado=true;
  if(!rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_DEFAULT,rj_hist_bytes,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,G.hist)){
   G.hist.Reset();nrrj::log("ROSTO_JOGADOR_ACUMULA sem memoria para o historico: acumulacao desligada");return false;}
  char l[128]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_ACUMULA historico %.1f MB",double(rj_hist_bytes)/1048576.0);nrrj::log(l);}
 if(!G.hist)return false;
 D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.NumElements=UINT(rj_hist_bytes/4);v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
 G.device->CreateUnorderedAccessView(G.hist.Get(),nullptr,&v,rj_cpu(G,base+5));
 return true;
}
void rj_compartilhado(RjGlobal& G,unsigned base){
 D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.NumElements=UINT(nrrj::bytes_total/4);v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
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
 for(unsigned s=0;s<rj_passes*rj_aneis;++s)rj_nulos(G,s*rj_set);

 HANDLE handle{};
 if(!rj_buffer(d,D3D12_HEAP_TYPE_DEFAULT,nrrj::bytes_total,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,G.compartilhado,D3D12_HEAP_FLAG_SHARED)||
    FAILED(d->CreateSharedHandle(G.compartilhado.Get(),nullptr,GENERIC_ALL,nullptr,&handle))||!handle){nrrj::envenenar("buffer compartilhado do rosto");return false;}
 void* m{};
 if(!rj_buffer(d,D3D12_HEAP_TYPE_READBACK,256,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE,G.marcador)||FAILED(G.marcador->Map(0,nullptr,&m))||!m){nrrj::envenenar("marcador do rosto");return false;}
 std::memset(m,0,256);
 auto& g=nrrj::global();
 g.marcador.store(static_cast<const volatile std::uint32_t*>(m),std::memory_order_release);
 g.bytes.store(nrrj::bytes_total);g.handle.store(handle,std::memory_order_release);
 G.pronto=true;
 char l[128]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_D3D_PRONTO bytes=%llu",static_cast<unsigned long long>(nrrj::bytes_total));nrrj::log(l);
 return true;
}
}

bool rj_grade_garantir(RjGlobal& G){
 if(G.grade_pronto)return true;if(G.grade_falhou)return false;
 HANDLE handle{};
 if(!pipeline(G.device.Get(),G.root.Get(),g_nr_reproject_rjgrade,G.grade_pso)||
    !rj_buffer(G.device.Get(),D3D12_HEAP_TYPE_DEFAULT,nrrj::grade_bytes,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,G.grade_buf,D3D12_HEAP_FLAG_SHARED)||
    FAILED(G.device->CreateSharedHandle(G.grade_buf.Get(),nullptr,GENERIC_ALL,nullptr,&handle))||!handle){
  G.grade_falhou=true;nrrj::log("ROSTO_JOGADOR_GRADE_FALHOU pso ou buffer compartilhado da grade (o rosto fica sem deteccao neste jogo)");return false;}
 nrrj::grade().handle.store(handle,std::memory_order_release);G.grade_pronto=true;
 char l[160]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_GRADE_PRONTA bytes=%llu slots=%u",static_cast<unsigned long long>(nrrj::grade_bytes),nrrj::grade_slots);nrrj::log(l);
 return true;
}
void Backend::rj_sem_job(bool v) noexcept{if(p_)p_->rj_sem_job=v;}
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
 {D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT_R16G16_FLOAT;sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
  sv.Texture2DArray.MipLevels=1;sv.Texture2DArray.ArraySize=Policy::history_count;G.device->CreateShaderResourceView(p.history[1].Get(),&sv,rj_cpu(G,base+1));}
 {D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT_R32_FLOAT;sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
  sv.Texture2DArray.MipLevels=1;sv.Texture2DArray.ArraySize=Policy::history_count;G.device->CreateShaderResourceView(p.history[2].Get(),&sv,rj_cpu(G,base+2));}
 {D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;v.Buffer.NumElements=UINT(nrrj::grade_bytes/4);v.Buffer.Flags=D3D12_BUFFER_UAV_FLAG_RAW;
  G.device->CreateUnorderedAccessView(G.grade_buf.Get(),nullptr,&v,rj_cpu(G,base+3));}
 RjGradeK k{gw,gh,unsigned(nrrj::grade_off_d(unsigned(s))),unsigned(nrrj::grade_off_mv(unsigned(s))),p.constants.slice,unsigned(GG.invertida.load(std::memory_order_relaxed)!=0),0,0};
 transition(c,G.grade_buf.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);
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
}

void Backend::rj_recorte(void* command,void* color,unsigned vista,bool post) noexcept{
 if(!p_)return;auto& p=*p_;p.rj_slot=-1;p.rj_grade=-1;
 if(!nrrj::ativo())return;
 auto& G=rjg();auto& g=nrrj::global();
 try{
  ++G.quadro;G.variante=0;

  const auto det=nrrj::ler_deteccao();const auto ger=nrrj::geracao.load(std::memory_order_acquire);
  G.rampa.passo(det.on,G.composto,ger);G.composto_ant=G.composto;G.composto=false;
  auto& q=nrrj::quadro_render();q=nrrj::QuadroRender{};q.on=det.on?1:0;q.nivel=G.rampa.nivel;
  const std::uint64_t agora=nrreusostate::quadros.load(std::memory_order_relaxed);
  const float dq=det.quadro&&agora>det.quadro?float((std::min<std::uint64_t>)(agora-det.quadro,16u)):0.f;
  if(det.tem){q.cx=det.cx+det.vx*dq;q.cy=det.cy+det.vy*dq;q.lado=det.lado;}
  if(!post){if(!G.pre_logado){G.pre_logado=true;nrrj::log("ROSTO_JOGADOR_PRE sem recorte: o rosto so roda no POST (a captura e a saida do FSR)");}return;}

  p.rj_grade=-1;if(!p.rj_sem_job)rj_grade_escrever(G,p,static_cast<ID3D12GraphicsCommandList*>(command));
  if(p.rj_sem_job||!(det.on||G.rampa.nivel>0.f)||!det.tem)return;
  if(nrrj::ablacao.load(std::memory_order_relaxed)&4u)return;

  {const unsigned N=nrrj::cada.load(std::memory_order_relaxed);
   if(N>1&&G.ger_recorte==ger&&G.jobs_sem+1u<N){

    const unsigned Q=nrrj::cada_quadros_efetivo();
    if(!(Q>0&&G.quadro>=G.quadro_recorte+Q)){++G.pulados_cada;return;}
    ++G.por_quadros;}}
  auto* c=static_cast<ID3D12GraphicsCommandList*>(command);auto* cor=static_cast<ID3D12Resource*>(color);
  if(!c||!cor||!p.device)return;
  const auto cd=cor->GetDesc();const unsigned ow=unsigned(cd.Width),oh=cd.Height;
  const auto r=nrrj::recorte_saida(det,ow,oh,nrrj::recorte_fator.load(std::memory_order_relaxed),nrrj::grade_rede.load(std::memory_order_relaxed),nrrj::teto.load(std::memory_order_relaxed));if(r.vazio())return;
  if(!rj_garantir(G,p.device.Get()))return;
  ComPtr<ID3D12GraphicsCommandList2> c2;
  if(FAILED(c->QueryInterface(IID_PPV_ARGS(&c2)))||!c2){nrrj::envenenar("sem ID3D12GraphicsCommandList2");return;}
  const int s=nrrj::slot_para_recorte();if(s<0){++G.sem_slot;return;}
  auto& S=g.slot[unsigned(s)];
  const unsigned base=rj_base(G,0);
  {D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT(vista);sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sv.Texture2D.MipLevels=1;
   G.device->CreateShaderResourceView(cor,&sv,rj_cpu(G,base));}
  rj_compartilhado(G,base);
  RjK k{};k.w=ow;k.h=oh;k.rx=r.x;k.ry=r.y;k.lado=r.L;k.n=nrrj::rede_de(r.L,nrrj::teto.load(std::memory_order_relaxed));

  {const float alvo=nrrj::rede_alvo.load(std::memory_order_relaxed);if(alvo>0.f){k.n=nrrj::rede_por_cabeca(r.L,nrrj::teto.load(std::memory_order_relaxed),det.lado,alvo,G.n_ult);G.n_ult=k.n;}}k.off_ent=unsigned(nrrj::offset_ent(unsigned(s)));k.off_tag=unsigned(nrrj::offset_tag(unsigned(s)));
  transition(c,G.compartilhado.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);
  rj_bind(G,c,G.recorte.Get(),base,k);c->Dispatch((k.n+7)/8,(k.n+7)/8,1);
  order_uav(c,G.compartilhado.Get());transition(c,G.compartilhado.Get(),write_state,D3D12_RESOURCE_STATE_COMMON);
  const std::uint32_t seq=std::uint32_t(++G.seq);
  {const D3D12_GPU_VIRTUAL_ADDRESS va=G.marcador->GetGPUVirtualAddress()+4u*unsigned(s);
   const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER marca{va,UINT(seq)};const D3D12_WRITEBUFFERIMMEDIATE_MODE modo=D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_OUT;
   c2->WriteBufferImmediate(1,&marca,&modo);}
  S.seq=seq;S.geracao=ger;S.x=r.x;S.y=r.y;S.L=r.L;S.n=k.n;S.ow=ow;S.oh=oh;S.hx=q.cx;S.hy=q.cy;S.hl=det.lado;S.nivel=G.rampa.nivel;
  S.palavra.store(nrrj::palavra(nrrj::Gravado,0),std::memory_order_release);
  p.rj_slot=s;++G.recortes;g.recortes.fetch_add(1);G.ger_recorte=ger;G.quadro_recorte=G.quadro;
  q.recorte_x=r.x;q.recorte_y=r.y;q.recorte_l=int(r.L);
 }catch(...){nrrj::envenenar("excecao no recorte");}
}
void Backend::rj_aceito(int job) noexcept{
 if(!p_)return;auto& p=*p_;

 if(p.rj_grade>=0){auto& GS=nrrj::grade().slot[unsigned(p.rj_grade)];p.rj_grade=-1;auto w=GS.palavra.load(std::memory_order_acquire);
  if(job>0&&(w&255u)==nrrj::GGravado)GS.palavra.compare_exchange_strong(w,nrrj::palavra_g(nrrj::GEscrito,job),std::memory_order_acq_rel);}
 if(nrrj::ativo()){auto& G=rjg();if(p.rj_slot>=0)G.jobs_sem=0;else ++G.jobs_sem;}
 if(p.rj_slot<0)return;
 auto& S=nrrj::global().slot[unsigned(p.rj_slot)];p.rj_slot=-1;
 if(job>0&&nrrj::estado_de(S.palavra.load(std::memory_order_acquire))==nrrj::Gravado)S.palavra.store(nrrj::palavra(nrrj::Escrito,job),std::memory_order_release);
}

void Backend::rj_segurar(void* command,void* destination,int job,float exposure,unsigned tone) noexcept{
 if(!p_)return;auto& p=*p_;auto& G=rjg();
 try{
  const unsigned N=nrrj::segura.load(std::memory_order_relaxed);
  if(!N||!nrrj::compor_rapido.load(std::memory_order_relaxed)||!nrrj::acumula.load(std::memory_order_relaxed)||!G.hist||!G.compor_r)return;
  const auto ger=nrrj::geracao.load(std::memory_order_acquire);

  G.seg_motivo=G.seg_quadro+1!=G.quadro?1u:G.seg_ger!=unsigned(ger)?2u:G.seg_n>=N?3u:(!G.seg_fw||!G.seg_fh)?4u:0u;
  if(G.seg_motivo)return;
  auto* c=static_cast<ID3D12GraphicsCommandList*>(command);auto* dst=static_cast<ID3D12Resource*>(destination);if(!c||!dst)return;
  const auto dd=dst->GetDesc();const unsigned ow=unsigned(dd.Width),oh=dd.Height;
  if(ow!=G.seg_ow||oh!=G.seg_oh||G.variante>=rj_compor_max)return;
  const auto det=nrrj::ler_deteccao();
  const float ganho=det.on?(std::min)(1.f,G.rampa.nivel+1.f/nrrj::rampa_quadros):G.rampa.nivel;if(!(ganho>0.f))return;
  const unsigned base=rj_base(G,1+G.variante++);
  {D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT_R16G16_FLOAT;sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
   sv.Texture2DArray.MipLevels=1;sv.Texture2DArray.ArraySize=Policy::history_count;G.device->CreateShaderResourceView(p.history[1].Get(),&sv,rj_cpu(G,base+1));}
  {D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT_R32_FLOAT;sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
   sv.Texture2DArray.MipLevels=1;sv.Texture2DArray.ArraySize=Policy::history_count;G.device->CreateShaderResourceView(p.history[2].Get(),&sv,rj_cpu(G,base+2));}
  rj_compartilhado(G,base);
  {D3D12_UNORDERED_ACCESS_VIEW_DESC uv{};uv.Format=dd.Format;uv.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;G.device->CreateUnorderedAccessView(dst,nullptr,&uv,rj_cpu(G,base+4));}
  if(!rj_hist(G,base))return;

  const int dx=det.tem?int(std::lround(det.vx)):0,dy=det.tem?int(std::lround(det.vy)):0;
  const int fx=std::clamp(G.seg_fx+dx,0,(std::max)(0,int(ow)-int(G.seg_fw))),fy=std::clamp(G.seg_fy+dy,0,(std::max)(0,int(oh)-int(G.seg_fh)));
  RjK k{};k.w=ow;k.h=oh;k.lado=1;k.n=1;k.off_tag=unsigned(nrrj::offset_tag(0));k.job=0xffffffffu;k.job2=0;
  k.fatia=p.constants.slice;k.mascara=p.constants.mask;k.historico=Policy::history_count;
  k.fx=fx;k.fy=fy;k.fw=G.seg_fw;k.fh=G.seg_fh;k.expo=exposure;k.tonemap=tone&1u;k.ganho=ganho;
  k.prof_rel=p.constants.depth_relative;k.prof_abs=p.constants.depth_absolute;k.camada0=1.15f;k.camada1=1.45f;
  k.acum=1u;k.gama=nrrj::acum_gama.load(std::memory_order_relaxed);k.quadro=unsigned(G.quadro);k.geracao=unsigned(ger);
  k.bytes_meio=unsigned(nrrj::bytes_meio);k.janela=rj_janela;k.rap_r=1u;
  k.bx_forca=nrrj::baixa_forca.load(std::memory_order_relaxed);
  transition(c,G.compartilhado.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);transition(c,G.hist.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);
  rj_bind(G,c,G.compor_r.Get(),base,k);c->Dispatch((k.fw+7)/8,(k.fh+7)/8,1);
  order_uav(c,dst);order_uav(c,G.hist.Get());transition(c,G.hist.Get(),write_state,D3D12_RESOURCE_STATE_COMMON);transition(c,G.compartilhado.Get(),write_state,D3D12_RESOURCE_STATE_COMMON);
  G.composto=true;++G.segurados;++G.seg_n;G.seg_quadro=G.quadro;G.seg_fx=fx;G.seg_fy=fy;
  auto& q=nrrj::quadro_render();q.composto=1;q.job=job;q.ganho=ganho;
 }catch(...){nrrj::envenenar("excecao ao segurar o rosto");}
}

void Backend::rj_compor(void* command,void* destination,unsigned formato,int job,int age,float exposure,unsigned tone) noexcept{
 if(!p_||!nrrj::ativo())return;auto& p=*p_;auto& G=rjg();
 try{
  if(!G.pronto||job<=0||age<0||!p.histories_read||!p.history[1]||!p.history[2])return;

  const unsigned teto=nrrj::teto_jobs();
  int s=nrrj::slot_do_job(job),jp=job,ap=age;
  if(s<0){
   const int sa=nrrj::slot_anterior(job);const int ja=sa>=0?nrrj::job_de(nrrj::global().slot[unsigned(sa)].palavra.load(std::memory_order_acquire)):0;const int aa=ja>0?p.policy.age(ja):-1;
   if(teto<2||sa<0||job-ja>int(teto)||aa<0||unsigned(aa)>=Policy::history_count){++G.sem_delta;const auto seg0=G.segurados;G.seg_motivo=9;rj_segurar(command,destination,job,exposure,tone);
    if(G.segurados==seg0){++G.sem_rosto;
     if(G.sem_rosto<=64){char l[260]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_SEM_ROSTO n=%llu quadro=%llu job=%d idade=%d ja=%d aa=%d teto=%u motivo=%u seg_n=%u seg_quadro=%llu ult_recorte=%llu jobs_sem=%u",
      static_cast<unsigned long long>(G.sem_rosto),static_cast<unsigned long long>(G.quadro),job,age,ja,aa,teto,G.seg_motivo,G.seg_n,static_cast<unsigned long long>(G.seg_quadro),static_cast<unsigned long long>(G.quadro_recorte),G.jobs_sem);nrrj::log(l);}}
    return;}
   s=sa;jp=ja;ap=aa;++G.reusos;}
  const auto& S=nrrj::global().slot[unsigned(s)];
  if(S.geracao!=nrrj::geracao.load(std::memory_order_acquire)){++G.geracao_velha;return;}
  auto* c=static_cast<ID3D12GraphicsCommandList*>(command);auto* dst=static_cast<ID3D12Resource*>(destination);
  if(!c||!dst)return;
  const auto dd=dst->GetDesc();const unsigned ow=unsigned(dd.Width),oh=dd.Height;
  if(ow!=S.ow||oh!=S.oh){++G.recusas;return;}
  if(G.variante>=rj_compor_max)return;
  const unsigned base=rj_base(G,1+G.variante++);
  {D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT_R16G16_FLOAT;sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
   sv.Texture2DArray.MipLevels=1;sv.Texture2DArray.ArraySize=Policy::history_count;G.device->CreateShaderResourceView(p.history[1].Get(),&sv,rj_cpu(G,base+1));}
  {D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT_R32_FLOAT;sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
   sv.Texture2DArray.MipLevels=1;sv.Texture2DArray.ArraySize=Policy::history_count;G.device->CreateShaderResourceView(p.history[2].Get(),&sv,rj_cpu(G,base+2));}
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
    const float ax=.575f*hl+24.f+mx,ay=.69f*hl+24.f+my;
    a0=int(std::floor(hx-ax));b0=int(std::floor(hy-ay));a1=int(std::ceil(hx+ax));b1=int(std::ceil(hy+ay));};
   int a0,b0,a1,b1;caixa(S.hx,S.hy,S.hl,a0,b0,a1,b1);
   ex0=(std::max)(x0,a0);ey0=(std::max)(y0,b0);ex1=(std::min)(x1,a1);ey1=(std::min)(y1,b1);
  }
  const float ganho=det.on?(std::min)(1.f,G.rampa.nivel+1.f/nrrj::rampa_quadros):G.rampa.nivel;
  if(!(ganho>0.f))return;
  RjK k{};k.w=ow;k.h=oh;k.rx=S.x;k.ry=S.y;k.lado=S.L;k.n=S.n;k.off_ent=unsigned(nrrj::offset_ent(unsigned(s)));k.off_tag=unsigned(nrrj::offset_tag(unsigned(s)));
  k.job=unsigned(jp);k.idade=unsigned(ap);k.fatia=p.constants.slice;k.mascara=p.constants.mask;k.historico=Policy::history_count;
  k.fx=x0;k.fy=y0;k.fw=unsigned(x1-x0);k.fh=unsigned(y1-y0);
  if(ex1>ex0&&ey1>ey0&&(ex0!=x0||ey0!=y0||ex1!=x1||ey1!=y1)){k.fx=ex0;k.fy=ey0;k.fw=unsigned(ex1-ex0);k.fh=unsigned(ey1-ey0);++G.compor_elipse_n;}
  G.compor_px+=std::uint64_t(k.fw)*k.fh;k.expo=exposure;k.tonemap=tone&1u;k.ganho=ganho*p.q_rosto_ganho;
  k.hx=S.hx;k.hy=S.hy;k.prof_rel=p.constants.depth_relative;k.prof_abs=p.constants.depth_absolute;k.camada0=1.15f;k.camada1=1.45f;

  if(jp>G.job_ult){if(G.job_ult>0&&G.quadro>G.quadro_troca)G.k_quadros=.8f*G.k_quadros+.2f*std::clamp(float(G.quadro-G.quadro_troca),1.f,8.f);G.job_ult=jp;G.quadro_troca=G.quadro;}
  if(nrrj::acumula.load(std::memory_order_relaxed)&&k.fw<=rj_janela&&k.fh<=rj_janela&&rj_hist(G,base)){
   const float kq=std::clamp(G.k_quadros,1.f,8.f);
   auto por_quadro=[&](float a){a=std::clamp(a,.001f,1.f);return a>=1.f?1.f:1.f-std::pow(1.f-a,1.f/kq);};
   k.acum=1u;k.a_baixa=por_quadro(nrrj::acum_baixa.load(std::memory_order_relaxed));k.a_alta=por_quadro(nrrj::acum_alta.load(std::memory_order_relaxed));
   k.gama=nrrj::acum_gama.load(std::memory_order_relaxed);k.quadro=unsigned(G.quadro);k.geracao=unsigned(nrrj::geracao.load(std::memory_order_acquire));
   k.bytes_meio=unsigned(nrrj::bytes_meio);k.janela=rj_janela;++G.acumulados;

   k.bx_forca=nrrj::baixa_forca.load(std::memory_order_relaxed);
   {const float al=nrrj::baixa_lenta.load(std::memory_order_relaxed);if(al>0.f){const float ar=nrrj::baixa_rapida.load(std::memory_order_relaxed);
    k.a_lenta=por_quadro(al);k.a_rapida=ar>0.f?por_quadro(ar):k.a_baixa;k.mov_px=nrrj::baixa_mov_px.load(std::memory_order_relaxed);}}

   {const float t=nrrj::frescor.load(std::memory_order_relaxed);if(t>0.f&&nrrj::frescor_pronto.load(std::memory_order_acquire))std::memcpy(&k.pad3,&t,4);}
  }

  {
   constexpr float peso=.5f,limiar=.03f;
   k.peso=peso;k.limiar=limiar;
   {const float t4=nrrj::acum_limiar.load(std::memory_order_relaxed);if(k.acum&&t4>0.f)k.limiar=t4;}

   const int s2=nrrj::slot_anterior(jp);
   if(s2>=0&&(peso<1.f||teto>=2)){const auto& S2=nrrj::global().slot[unsigned(s2)];const int j2=nrrj::job_de(S2.palavra.load(std::memory_order_acquire));const int a2=p.policy.age(j2);
    if(a2>=0&&unsigned(a2)<Policy::history_count&&S2.geracao==S.geracao&&S2.ow==S.ow&&S2.oh==S.oh&&(teto<2||job-j2<=int(teto))){k.sozinho=teto>=2?1u:0u;
     k.off_ent2=unsigned(nrrj::offset_ent(unsigned(s2)));k.off_tag2=unsigned(nrrj::offset_tag(unsigned(s2)));k.job2=unsigned(j2);k.idade2=unsigned(a2);
     k.rx2=S2.x;k.ry2=S2.y;k.lado2=S2.L;k.n2=S2.n;k.hx2=S2.hx;k.hy2=S2.hy;++G.medias;

     if(nrrj::compor_elipse.load(std::memory_order_relaxed)&&S2.hl>0.f&&(k.fx!=x0||k.fy!=y0||int(k.fw)!=x1-x0||int(k.fh)!=y1-y0)){
      const float fq=float(a2+1);
      const float mx=std::clamp(std::fabs(det.vx)*fq*1.5f+32.f,32.f,float(M)),my=std::clamp(std::fabs(det.vy)*fq*1.5f+32.f,32.f,float(M));
      const float ax=.575f*S2.hl+24.f+mx,ay=.69f*S2.hl+24.f+my;
      const int u0=(std::max)(x0,(std::min)(k.fx,int(std::floor(S2.hx-ax)))),v0=(std::max)(y0,(std::min)(k.fy,int(std::floor(S2.hy-ay))));
      const int u1=(std::min)(x1,(std::max)(k.fx+int(k.fw),int(std::ceil(S2.hx+ax)))),v1=(std::min)(y1,(std::max)(k.fy+int(k.fh),int(std::ceil(S2.hy+ay))));
      if(u1>u0&&v1>v0&&(!k.acum||(unsigned(u1-u0)<=rj_janela&&unsigned(v1-v0)<=rj_janela))){
       G.compor_px-=std::uint64_t(k.fw)*k.fh;k.fx=u0;k.fy=v0;k.fw=unsigned(u1-u0);k.fh=unsigned(v1-v0);G.compor_px+=std::uint64_t(k.fw)*k.fh;}}}}}

  {
   constexpr bool zero=false;
   if(zero||(nrrj::ablacao.load(std::memory_order_relaxed)&1u)){G.composto=true;auto& qz=nrrj::quadro_render();qz.composto=1;qz.job=job;qz.ganho=0.f;return;}}
  transition(c,G.compartilhado.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);
  if(k.acum)transition(c,G.hist.Get(),D3D12_RESOURCE_STATE_COMMON,write_state);

  ID3D12PipelineState* pso=G.compor.Get();
  if(k.acum&&nrrj::compor_rapido.load(std::memory_order_relaxed)){
   if(!G.compor_r&&!G.compor_r_tentado){G.compor_r_tentado=true;if(!pipeline(G.device.Get(),G.root.Get(),g_nr_reproject_rjcomporr,G.compor_r)){G.compor_r.Reset();nrrj::log("ROSTO_JOGADOR_COMPOR_RAPIDO PSO recusado: caminho de hoje");}else nrrj::log("ROSTO_JOGADOR_COMPOR_RAPIDO ligado");}
   if(G.compor_r)pso=G.compor_r.Get();
   if(G.compor_r&&nrrj::segura.load(std::memory_order_relaxed)&&nrrj::segura_pixel.load(std::memory_order_relaxed))k.rap_r|=2u;
   {const unsigned bs=nrrj::borda_suave.load(std::memory_order_relaxed);if(G.compor_r&&(bs&1u))k.rap_r|=4u;if(G.compor_r&&(bs&2u)){k.rap_r|=8u;const float lb=nrrj::borda_largura.load(std::memory_order_relaxed);std::memcpy(&k.pad_r0,&lb,4);}}

   if(const float gt=nrrj::borda_teto.load(std::memory_order_relaxed);gt>0.f&&G.compor_r&&!G.copia_falhou&&k.fw<=rj_janela&&k.fh<=rj_janela){
    const DXGI_FORMAT fr=dd.Format;
    if(!G.copia||G.copia_fmt!=fr){G.copia.Reset();
     D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=rj_janela;td.Height=rj_janela;
     td.DepthOrArraySize=1;td.MipLevels=1;td.Format=fr;td.SampleDesc.Count=1;td.Layout=D3D12_TEXTURE_LAYOUT_UNKNOWN;
     if(FAILED(G.device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,read_state,nullptr,IID_PPV_ARGS(&G.copia)))){G.copia.Reset();G.copia_falhou=true;nrrj::log("ROSTO_JOGADOR_TETO sem memoria para a copia: desligado");}
     else{G.copia_fmt=fr;nrrj::log("ROSTO_JOGADOR_TETO copia criada");}}
    if(G.copia){
     transition(c,dst,write_state,D3D12_RESOURCE_STATE_COPY_SOURCE);transition(c,G.copia.Get(),read_state,D3D12_RESOURCE_STATE_COPY_DEST);
     D3D12_TEXTURE_COPY_LOCATION a{};a.pResource=G.copia.Get();a.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;a.SubresourceIndex=0;
     D3D12_TEXTURE_COPY_LOCATION b{};b.pResource=dst;b.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;b.SubresourceIndex=0;
     D3D12_BOX box{UINT(k.fx),UINT(k.fy),0,UINT(k.fx)+k.fw,UINT(k.fy)+k.fh,1};
     c->CopyTextureRegion(&a,0,0,0,&b,&box);
     transition(c,G.copia.Get(),D3D12_RESOURCE_STATE_COPY_DEST,read_state);transition(c,dst,D3D12_RESOURCE_STATE_COPY_SOURCE,write_state);
     D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=DXGI_FORMAT(formato);sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sv.Texture2D.MipLevels=1;
     G.device->CreateShaderResourceView(G.copia.Get(),&sv,rj_cpu(G,base));
     k.rap_r|=16u;std::memcpy(&k.pad_r1,&gt,4);++G.tetos;
     if(G.tetos<=2||(G.tetos&1023u)==0){char l[120]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_TETO gama=%.2f quadros=%llu",double(gt),static_cast<unsigned long long>(G.tetos));nrrj::log(l);}}}}
  rj_bind(G,c,pso,base,k);c->Dispatch((k.fw+7)/8,(k.fh+7)/8,1);
  order_uav(c,dst);if(k.acum){order_uav(c,G.hist.Get());transition(c,G.hist.Get(),write_state,D3D12_RESOURCE_STATE_COMMON);}transition(c,G.compartilhado.Get(),write_state,D3D12_RESOURCE_STATE_COMMON);
  G.composto=true;++G.composicoes;nrrj::global().composicoes.fetch_add(1);
  auto& q=nrrj::quadro_render();q.composto=1;q.job=job;q.ganho=ganho;
  if(k.acum&&pso==G.compor_r.Get()){G.seg_quadro=G.quadro;G.seg_ger=k.geracao;G.seg_n=0;G.seg_fx=k.fx;G.seg_fy=k.fy;G.seg_fw=k.fw;G.seg_fh=k.fh;G.seg_ow=ow;G.seg_oh=oh;}
  if(G.composicoes<=6||(G.composicoes&255u)==0){
   char l[640]{};NRLOGF(l,sizeof l,"ROSTO_JOGADOR_QUADRO composicoes=%llu job=%d idade=%d slot=%d recorte=%d,%d,%u ganho=%.2f recortes=%llu sem_slot=%llu sem_delta=%llu geracao_velha=%llu medias=%llu reusos=%llu pulados_cada=%llu marcados=%llu avaliacoes=%llu acum=%llu kq=%.2f elipse=%llu px_med=%.0f segurados=%llu cada_quadros=%u por_quadros=%llu sem_rosto=%llu quadro=%llu",
    static_cast<unsigned long long>(G.composicoes),job,age,s,S.x,S.y,S.L,double(ganho),static_cast<unsigned long long>(G.recortes),static_cast<unsigned long long>(G.sem_slot),
    static_cast<unsigned long long>(G.sem_delta),static_cast<unsigned long long>(G.geracao_velha),static_cast<unsigned long long>(G.medias),static_cast<unsigned long long>(G.reusos),static_cast<unsigned long long>(G.pulados_cada),static_cast<unsigned long long>(nrrj::global().prontos.load()),static_cast<unsigned long long>(nrrj::global().avaliacoes.load()),static_cast<unsigned long long>(G.acumulados),double(G.k_quadros),static_cast<unsigned long long>(G.compor_elipse_n),G.composicoes?double(G.compor_px)/double(G.composicoes+1):0.,static_cast<unsigned long long>(G.segurados),
    nrrj::cada_quadros_efetivo(),static_cast<unsigned long long>(G.por_quadros),static_cast<unsigned long long>(G.sem_rosto),static_cast<unsigned long long>(G.quadro));
   nrrj::log(l);}
 }catch(...){nrrj::envenenar("excecao na composicao");}
}
}
