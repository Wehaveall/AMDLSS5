#include "nr_reproject_qmapa.h"
#include "nr_reproject_qmapa4.h"
#include "../../../runtime/rosto_jogador_state.h"
namespace nrreproject {
namespace {
struct QMapaK{unsigned w,h,fatia,bits;float gHi,gHiNeg,gLo,gLoNeg,movA,movB,gradA,gradB,profR,profA,profB,confP,edgeA,edgeB,fcx,fcy,fax,fay,fniv,pad2,movR,expo;unsigned tom;float stopsMax;};
static_assert(sizeof(QMapaK)==112);
}
bool Backend::Impl::alocar_q_post(const D3D12_UNORDERED_ACCESS_VIEW_DESC& flags_view){
 auto& g=qpost;
 if(!width||!height||!warped||!history[0]||!history[1]||!history[2]||!flags_buffer)return false;
 if(!root(device.Get(),28,4,1,g.root)||!((q_mapa.bits&(8192u|16384u))?pipeline(device.Get(),g.root.Get(),g_nr_reproject_qmapa4,g.pso):pipeline(device.Get(),g.root.Get(),g_nr_reproject_qmapa,g.pso)))return false;
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
 D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=width;td.Height=height;td.DepthOrArraySize=1;td.MipLevels=1;
 td.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
 if(FAILED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,read_state,nullptr,IID_PPV_ARGS(&g.saida))))return false;
 D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,5,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
 if(FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&g.heap)))||FAILED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&g.post_heap))))return false;
 const DXGI_FORMAT f=DXGI_FORMAT_R16G16B16A16_FLOAT;
 srv(g.heap.Get(),0,warped.Get(),f);srv(g.heap.Get(),1,history[1].Get(),DXGI_FORMAT_R16G16_FLOAT,true);srv(g.heap.Get(),2,history[2].Get(),DXGI_FORMAT_R32_FLOAT,true);
 srv(g.heap.Get(),3,history[0].Get(),f,true);
 uav(g.heap.Get(),4,g.saida.Get(),f);
 srv(g.post_heap.Get(),0,g.saida.Get(),f);srv(g.post_heap.Get(),1,g.saida.Get(),f);
 device->CreateUnorderedAccessView(flags_buffer.Get(),nullptr,&flags_view,cpu(g.post_heap.Get(),3));
 return true;
}

bool Backend::Impl::q_post(ID3D12GraphicsCommandList* c,float exposure,unsigned tone){
 auto& g=qpost;
 if(!q_mapa.ligado||!g.pso||!g.heap||!g.post_heap||!g.saida||!warped_read||!histories_read)return false;

 {const unsigned np=nrprescale::passos_pedidos.load(std::memory_order_relaxed);
  if(np>1){if(!g.fora_passos){g.fora_passos=true;nrlogfila::linha(L"nr_reproject.log","pid=%lu Q_MAPA_FORA passos=%u (o mapa de ganho so vale com 1 passo; volta sozinho com 1)\n",GetCurrentProcessId(),np);}++g.pulados_passos;return false;}
  if(g.fora_passos){g.fora_passos=false;nrlogfila::linha(L"nr_reproject.log","pid=%lu Q_MAPA_VOLTA passos=1 (pulados=%llu)\n",GetCurrentProcessId(),static_cast<unsigned long long>(g.pulados_passos));}}
 const auto& m=q_mapa;
 QMapaK k{width,height,constants.slice%Policy::history_count,m.bits,m.gHi,m.gHiNeg,m.gLo,m.gLoNeg,m.movA,m.movB,m.gradA,m.gradB,m.profR,m.profA,m.profB,m.confP,m.edgeA,m.edgeB,0.f,0.f,0.f,0.f,0.f,0.f,m.movR,(std::isfinite(exposure)&&exposure>0.f)?exposure:1.f,tone,m.stopsMax};

 if((m.bits&4096u)&&comp_w&&comp_h){const auto& qr=nrrj::quadro_render();
  if(qr.on==1&&qr.lado>0.f&&qr.nivel>0.f){const float sx=float(width)/float(comp_w),sy=float(height)/float(comp_h);
   k.fcx=qr.cx*sx;k.fcy=qr.cy*sy;k.fax=(std::max)(.575f*qr.lado*sx,1.f);k.fay=(std::max)(.69f*qr.lado*sy,1.f);k.fniv=(std::min)(qr.nivel,1.f);}}
 if(!g.logado){g.logado=true;{
  nrlogfila::linha(L"nr_reproject.log","pid=%lu Q_MAPA bits=%u hi=%.3f/%.3f lo=%.3f/%.3f mov=%.2f..%.2f grad=%.2f..%.2f prof=%.1f px %.3f..%.3f confP=%.2f borda=%.3f..%.3f viz=%.1f stops=%u stopsMax=%.2f historico=%ux%u saida=%ux%u reduzido=%u\n",
   GetCurrentProcessId(),m.bits,double(m.gHi),double(m.gHiNeg),double(m.gLo),double(m.gLoNeg),double(m.movA),double(m.movB),double(m.gradA),double(m.gradB),
   double(m.profR),double(m.profA),double(m.profB),double(m.confP),double(m.edgeA),double(m.edgeB),double(m.movR),(m.bits&16384u)?1u:0u,double(m.stopsMax),width,height,comp_w,comp_h,reduzido?1u:0u);}}
 transition(c,g.saida.Get(),read_state,write_state);
 bind(c,g.heap.Get(),g.root.Get(),g.pso.Get(),28,&k,4);c->Dispatch((width+7)/8,(height+7)/8,1);
 transition(c,g.saida.Get(),write_state,read_state);
 ++g.quadros;return true;
}
}
