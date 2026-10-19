#include "../../../runtime/nr_publico.h"
#include "../live-transition-binding/late_state.h"
#include <exception>
namespace nrreproject {
namespace {
struct LateFlags {ComPtr<ID3D12Device> device;ComPtr<ID3D12Resource> upload;std::uint64_t* map{};};

LateFlags* late_flags_for(ID3D12Device* d){
 static std::mutex mutex;static std::vector<LateFlags*> todas;
 std::lock_guard<std::mutex> lock(mutex);
 for(auto* f:todas)if(f->device.Get()==d)return f;
 auto* f=new(std::nothrow) LateFlags;if(!f||!d)return nullptr;
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_UPLOAD;
 D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=UINT64(nrlate::flag_slots)*nrlate::flag_stride_u64*8;bd.Height=1;bd.DepthOrArraySize=1;bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
 void* map{};
 if(FAILED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&f->upload)))||
    FAILED(f->upload->Map(0,nullptr,&map))||!map){delete f;return nullptr;}
 std::memset(map,0,std::size_t(bd.Width));_mm_sfence();
 f->map=static_cast<std::uint64_t*>(map);f->device=d;todas.push_back(f);return f;
}
struct LateLogico {
 bool composed_frame;unsigned est_cur;bool est_valido;unsigned ganho_cur;bool ganho_valido,ganho_dividido;std::uint64_t ganho_quadro,ganho_limitados;
 unsigned luz_cur;bool luz_valido;std::uint64_t luz_quadro,luz_quadros,luz_filtrados;

 bool post,bil;
 bool operator==(const LateLogico& o)const{return composed_frame==o.composed_frame&&est_cur==o.est_cur&&est_valido==o.est_valido&&ganho_cur==o.ganho_cur&&ganho_valido==o.ganho_valido&&
  ganho_dividido==o.ganho_dividido&&ganho_quadro==o.ganho_quadro&&ganho_limitados==o.ganho_limitados&&luz_cur==o.luz_cur&&luz_valido==o.luz_valido&&luz_quadro==o.luz_quadro&&
  luz_quadros==o.luz_quadros&&luz_filtrados==o.luz_filtrados&&post==o.post&&bil==o.bil;}
};

void late_desligar(LateState& L,const char* motivo,std::uint64_t quadro){
 ++L.inconsistent;
 if(nrlate::desligada.exchange(true))return;
 {nrlogfila::linha(L"nr_reproject.log","pid=%lu frame=%llu TARDIA_DESLIGADA motivo=%s gravadas=%llu resolvidas=%llu executou_J=%llu\n",
  GetCurrentProcessId(),static_cast<unsigned long long>(quadro),motivo,static_cast<unsigned long long>(L.recorded),static_cast<unsigned long long>(L.resolved),
  static_cast<unsigned long long>(L.executed_late));}
}
}
void Backend::late_arm(int job) noexcept {
 if(!p_||job<=0||!p_->device||nrlate::desligada.load(std::memory_order_relaxed))return;auto& p=*p_;
 auto* f=late_flags_for(p.device.Get());if(!f)return;
 nrlate::flag_map.store(f->map,std::memory_order_release);

 *nrlate::flag_slot(f->map,job)=0;_mm_sfence();
 p.late.armed[unsigned(job)%LateState::ring]=job;++p.late.armed_n;
}
LateCounters Backend::late_counters() const noexcept {
 LateCounters c{};if(!p_)return c;const auto& l=p_->late;
 c.armed=l.armed_n;c.recorded=l.recorded;c.resolved=l.resolved;c.executed_late=l.executed_late;c.refused=l.refused;c.b_failed=l.b_failed;c.inconsistent=l.inconsistent;
 c.gain_frames=l.gain_frames;c.gain_ms=l.gain_ms;for(unsigned i=0;i<8;++i)c.refused_reason[i]=l.refused_reason[i];
 c.desligada=nrlate::desligada.load(std::memory_order_relaxed);return c;
}
Result Backend::compose_late(void* command,void* destination,std::uint32_t d3d_state,const LateRequest& q,std::uint32_t tone){
 if(!p_)return Result::Failed;auto& p=*p_;auto& L=p.late;

 {static const double freq=[]{LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return double(f.QuadPart);}();
  for(unsigned i=0;i<LateState::ring;++i){auto& e=L.pend[i];if(!e.ativo||!L.readback_map||p.policy.frame<e.quadro+8)continue;
   e.ativo=false;++L.resolved;const std::uint64_t v=L.readback_map[i];
   if(!v)continue;
   if(v!=std::uint64_t(unsigned(e.job_late))){late_desligar(L,"leitura_job_errado",p.policy.frame);continue;}
   if(!e.b_ok)continue;
   ++L.executed_late;L.gain_frames+=double(e.age_prev-e.age_late);
   if(e.cap_late>e.cap_prev&&freq>0)L.gain_ms+=double(e.cap_late-e.cap_prev)*1000.0/freq;}}

 p.atualizar_ao_vivo();

 if(nrlate::desligada.load(std::memory_order_relaxed))return compose(command,destination,d3d_state,q.residual_prev,q.job_prev,q.exposure_prev,tone);

 const bool reduzido=p.reduzido;
 if(reduzido&&!p.late_post)return compose(command,destination,d3d_state,q.residual_prev,q.job_prev,q.exposure_prev,tone);
 auto hoje=[&](unsigned motivo){++L.refused;++L.refused_reason[motivo&7u];return compose(command,destination,d3d_state,q.residual_prev,q.job_prev,q.exposure_prev,tone);};
 auto* c=static_cast<ID3D12GraphicsCommandList*>(command);
 auto* fonte=static_cast<ID3D12Resource*>(q.late_source);auto* anterior=static_cast<ID3D12Resource*>(q.residual_prev);
 const unsigned s=unsigned(q.job_late)%LateState::ring;

 if(!p.ready||!p.device||L.falhou||q.job_prev<=0||q.job_late!=q.job_prev+1||L.armed[s]!=q.job_late)return hoje(0);

 if(p.modo_post())return hoje(1);

 const int age_prev=p.policy.age(q.job_prev),age_late=p.policy.age(q.job_late);
 if(age_prev<0||age_late<0)return hoje(2);

 if(!c||p.composed_frame||command!=p.captured_command)return hoje(3);
 auto* flags=late_flags_for(p.device.Get());if(!flags)return hoje(4);

 if(!fonte||!anterior||!device_matches(p.device.Get(),fonte)||!texture(anterior,DXGI_FORMAT_R16G16B16A16_FLOAT)||!std::isfinite(q.exposure_late)||q.exposure_late<=0)return hoje(5);
 const auto rd=anterior->GetDesc(),sd=fonte->GetDesc();

 if(sd.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||rd.Width!=p.comp_w||rd.Height!=p.comp_h)return hoje(5);

 if(reduzido&&(!p.reduzir_pso||!p.reduzir_heap||!p.res_reduzido))return hoje(5);

 D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 total{};p.device->GetCopyableFootprints(&rd,0,1,0,&fp,nullptr,nullptr,&total);
 if(!total||sd.Width<total)return hoje(5);

 if(!L.tex||L.w!=unsigned(rd.Width)||L.h!=rd.Height||L.format!=rd.Format){
  Impl::Aposentadas lote;lote.vencimento=p.begins+3;
  for(IUnknown* o:{static_cast<IUnknown*>(L.tex.Get()),static_cast<IUnknown*>(L.heap.Get()),static_cast<IUnknown*>(L.reduzir_heap.Get())})if(o)lote.objetos.emplace_back(o);
  if(!lote.objetos.empty())p.retired.push_back(std::move(lote));
  L.tex.Reset();L.heap.Reset();L.reduzir_heap.Reset();
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC td=rd;td.Flags=D3D12_RESOURCE_FLAG_NONE;td.Alignment=0;
  D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,12,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
  if(FAILED(p.device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,read_state,nullptr,IID_PPV_ARGS(&L.tex)))||
     FAILED(p.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&L.heap)))){L.tex.Reset();L.heap.Reset();L.falhou=true;return hoje(6);}
  L.w=unsigned(rd.Width);L.h=rd.Height;L.format=rd.Format;
 }
 if(reduzido&&!L.reduzir_heap){
  D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,12,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
  if(FAILED(p.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&L.reduzir_heap)))){L.reduzir_heap.Reset();L.falhou=true;return hoje(6);}

  const DXGI_FORMAT fs[]{DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R16G16_FLOAT,DXGI_FORMAT_R32_FLOAT};
  ID3D12DescriptorHeap* h=L.reduzir_heap.Get();
  p.srv(h,0,nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);p.srv(h,1,nullptr,DXGI_FORMAT_R16G16_FLOAT);p.srv(h,2,nullptr,DXGI_FORMAT_R32_FLOAT);
  for(unsigned i=0;i<3;++i){p.srv(h,3+i,nullptr,fs[i],true);p.uav(h,8+i,nullptr,fs[i],true);}
  p.srv(h,6,nullptr,DXGI_FORMAT_R16G16B16A16_FLOAT);p.srv(h,7,nullptr,DXGI_FORMAT_R32_FLOAT);
  static bool logado=false;
  if(!logado){logado=true;
  }
 }
 if(!L.snapshot){
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=UINT64(LateState::ring)*LateState::snap_stride;bd.Height=1;bd.DepthOrArraySize=1;bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  D3D12_HEAP_PROPERTIES rp{};rp.Type=D3D12_HEAP_TYPE_READBACK;D3D12_RESOURCE_DESC rb=bd;rb.Width=UINT64(LateState::ring)*8;void* map{};
  if(FAILED(p.device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&L.snapshot)))||
     FAILED(p.device->CreateCommittedResource(&rp,D3D12_HEAP_FLAG_NONE,&rb,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&L.readback)))||
     FAILED(L.readback->Map(0,nullptr,&map))||!map){L.snapshot.Reset();L.readback.Reset();L.falhou=true;return hoje(6);}
  L.readback_map=static_cast<const volatile std::uint64_t*>(map);
 }

 {ID3D12DescriptorHeap* h=L.heap.Get();const auto& f=L.fonte;
  p.srv(h,0,f.color,f.color_format);p.srv(h,1,f.motion,f.motion_format);p.srv(h,2,f.depth,f.depth_format);p.srv(h,7,f.exposure,DXGI_FORMAT_R32_FLOAT);
  const DXGI_FORMAT formats[]{DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R16G16_FLOAT,DXGI_FORMAT_R32_FLOAT};
  for(unsigned i=0;i<3;++i){p.srv(h,3+i,p.history[i].Get(),formats[i],true);p.uav(h,8+i,p.history[i].Get(),formats[i],true);}
  p.uav(h,11,p.warped.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);}

 if(reduzido){ID3D12DescriptorHeap* h=L.reduzir_heap.Get();
  p.srv(h,6,L.tex.Get(),rd.Format);p.uav(h,11,p.res_reduzido.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT);}

 nrcusto::tardia();
 const unsigned r=unsigned(L.gravados%LateState::ring);const UINT64 off=UINT64(r)*LateState::snap_stride;
 auto* snap=L.snapshot.Get();
 c->CopyBufferRegion(snap,off,flags->upload.Get(),UINT64(s)*nrlate::flag_stride_u64*8,8);
 transition(c,snap,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PREDICATION);
 auto logico=[&]{return LateLogico{p.composed_frame,p.est_cur,p.est_valido,p.ganho_cur,p.ganho_valido,p.ganho_dividido,p.ganho_quadro,p.ganho_limitados,
                                    p.luz_cur,p.luz_valido,p.luz_quadro,p.luz_quadros,p.luz_filtrados,
                                    p.modo_post(),p.bilateral.forca>0&&p.bil_apply};};
 auto restaurar=[&](const LateLogico& v){p.composed_frame=v.composed_frame;p.est_cur=v.est_cur;p.est_valido=v.est_valido;p.ganho_cur=v.ganho_cur;p.ganho_valido=v.ganho_valido;
  p.ganho_dividido=v.ganho_dividido;p.ganho_quadro=v.ganho_quadro;p.ganho_limitados=v.ganho_limitados;p.luz_cur=v.luz_cur;p.luz_valido=v.luz_valido;p.luz_quadro=v.luz_quadro;
  p.luz_quadros=v.luz_quadros;p.luz_filtrados=v.luz_filtrados;};

 struct LateFiltros{unsigned cro_cur;bool cro_valido;std::uint64_t cro_quadro,cro_quadros;unsigned lg_cur;bool lg_valido;std::uint64_t lg_quadro,lg_quadros;
  unsigned su_cur;bool su_valido;std::uint64_t su_quadro,su_quadros;
  bool mesmo_pingue(const LateFiltros& o)const{return cro_cur==o.cro_cur&&cro_valido==o.cro_valido&&cro_quadro==o.cro_quadro&&lg_cur==o.lg_cur&&lg_valido==o.lg_valido&&
   lg_quadro==o.lg_quadro&&su_cur==o.su_cur&&su_valido==o.su_valido&&su_quadro==o.su_quadro;}};
 auto filtros=[&]{return LateFiltros{p.cro_cur,p.cro_valido,p.cro_quadro,p.cro_quadros,p.lg_cur,p.lg_valido,p.lg_quadro,p.lg_quadros,p.su_cur,p.su_valido,p.su_quadro,p.su_quadros};};
 auto restaurar_filtros=[&](const LateFiltros& v){p.cro_cur=v.cro_cur;p.cro_valido=v.cro_valido;p.cro_quadro=v.cro_quadro;p.cro_quadros=v.cro_quadros;
  p.lg_cur=v.lg_cur;p.lg_valido=v.lg_valido;p.lg_quadro=v.lg_quadro;p.lg_quadros=v.lg_quadros;p.su_cur=v.su_cur;p.su_valido=v.su_valido;p.su_quadro=v.su_quadro;p.su_quadros=v.su_quadros;};
 auto invalidar_filtros=[&]{p.cro_valido=false;p.lg_valido=false;p.su_valido=false;};
 const auto filtros_antes=filtros();const int cross_job_antes=p.cross_job;const unsigned cross_j_antes=p.cross_j;const float cross_P_antes=p.cross_P,idade_media_antes=p.idade_media;
 const auto antes=logico();const auto halo_antes=p.halo_quadros;

 struct Guarda{Impl& p;LateState& L;ID3D12GraphicsCommandList* c;bool predicado{};const int excecoes{std::uncaught_exceptions()};
  ~Guarda(){p.warp_heap_override=nullptr;p.bil_heap_override=nullptr;p.reduzir_heap_override=nullptr;p.suprimir_despejo=false;
   if(std::uncaught_exceptions()>excecoes){L.falhou=true;p.bil_job=-1;if(predicado)c->SetPredication(nullptr,0,D3D12_PREDICATION_OP_EQUAL_ZERO);}}
 } guarda{p,L,c};

 const bool bil_ligada=antes.bil;
 if(bil_ligada&&p.bil_duplo_late==q.job_late&&p.bil_duplo_prev==q.job_prev&&p.bil_duplo_quadro+1==p.policy.frame)p.bil_job=q.job_prev;
 p.bil_duplo_late=-1;
 p.suprimir_despejo=true;guarda.predicado=true;
 c->SetPredication(snap,off,D3D12_PREDICATION_OP_NOT_EQUAL_ZERO);
 const Result a=compose(command,destination,d3d_state,q.residual_prev,q.job_prev,q.exposure_prev,tone);
 if(a!=Result::Composed){

  c->SetPredication(nullptr,0,D3D12_PREDICATION_OP_EQUAL_ZERO);transition(c,snap,D3D12_RESOURCE_STATE_PREDICATION,D3D12_RESOURCE_STATE_COPY_DEST);
  p.suprimir_despejo=false;if(bil_ligada)p.bil_job=-1;++L.refused;++L.refused_reason[7];return a;
 }
 const auto depois_a=logico();const auto composed_a=p.composed;const int idade_a=nrreproject::idade_composta.load();
 const auto halo_a=p.halo_quadros;

 if(depois_a.post!=antes.post||depois_a.bil!=antes.bil){
  c->SetPredication(nullptr,0,D3D12_PREDICATION_OP_EQUAL_ZERO);transition(c,snap,D3D12_RESOURCE_STATE_PREDICATION,D3D12_RESOURCE_STATE_COPY_DEST);
  p.suprimir_despejo=false;p.ganho_valido=false;p.luz_valido=false;p.est_valido=false;p.bil_job=-1;invalidar_filtros();
  late_desligar(L,"modo_mudou_em_A",p.policy.frame);return Result::Composed;
 }
 const auto filtros_a=filtros();
 restaurar(antes);restaurar_filtros(filtros_antes);p.cross_job=cross_job_antes;p.cross_j=cross_j_antes;p.cross_P=cross_P_antes;p.idade_media=idade_media_antes;
 c->SetPredication(snap,off,D3D12_PREDICATION_OP_EQUAL_ZERO);
 transition(c,L.tex.Get(),read_state,D3D12_RESOURCE_STATE_COPY_DEST);
 {D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=L.tex.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.SubresourceIndex=0;
  src.pResource=fonte;src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=fp;c->CopyTextureRegion(&dst,0,0,0,&src,nullptr);}
 transition(c,L.tex.Get(),D3D12_RESOURCE_STATE_COPY_DEST,read_state);
 p.warp_heap_override=L.heap.Get();
 if(reduzido)p.reduzir_heap_override=L.reduzir_heap.Get();

 if(bil_ligada){p.bil_heap_override=p.bil_heap_tardia.Get();p.bil_job=-1;}
 const Result b=compose(command,destination,d3d_state,L.tex.Get(),q.job_late,q.exposure_late,tone);
 p.warp_heap_override=nullptr;p.bil_heap_override=nullptr;p.reduzir_heap_override=nullptr;

 if(reduzido)p.est_ultimo_job=q.job_prev;

 if(bil_ligada){p.bil_job=-1;if(b==Result::Composed){p.bil_duplo_prev=q.job_prev;p.bil_duplo_late=q.job_late;p.bil_duplo_quadro=p.policy.frame;}}
 c->SetPredication(nullptr,0,D3D12_PREDICATION_OP_EQUAL_ZERO);guarda.predicado=false;
 p.suprimir_despejo=false;

 transition(c,snap,D3D12_RESOURCE_STATE_PREDICATION,D3D12_RESOURCE_STATE_COPY_SOURCE);
 c->CopyBufferRegion(L.readback.Get(),UINT64(r)*8,snap,off,8);
 transition(c,snap,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
 const auto depois_b=logico();
 if(b!=Result::Composed){

  ++L.b_failed;restaurar(depois_a);restaurar_filtros(filtros_a);p.ganho_valido=false;p.luz_valido=false;p.est_valido=false;invalidar_filtros();
 }else if(!(depois_b==depois_a)){p.ganho_valido=false;p.luz_valido=false;p.est_valido=false;invalidar_filtros();p.bil_job=-1;late_desligar(L,"estado_divergente",p.policy.frame);}
 else if(!filtros().mesmo_pingue(filtros_a))invalidar_filtros();

 p.composed=composed_a;nrreproject::idade_composta.store(idade_a);p.halo_quadros=halo_a>halo_antes?halo_antes+1:halo_antes;
 L.pend[r]={true,p.policy.frame,q.job_prev,q.job_late,age_prev,age_late,q.capture_qpc_prev,q.capture_qpc_late,b==Result::Composed};
 ++L.gravados;++L.recorded;
 return Result::Composed;
}
}
