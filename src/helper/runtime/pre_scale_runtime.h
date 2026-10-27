#pragma once
#include "nr_publico.h"
#include "pre_scale_state.h"
#include "native_ini_path.h"
#include "multipass_prescale.h"
#include "geometria_nativa.h"
#include <mutex>
namespace nrprescale_runtime {
struct Buffers {
 void* module{};void* kernel{};void* packed_kernel{};void* input{};void* output{};size_t capacity{};
 double percent=100;bool initialized{};int device=-1;void* owner{};bool guided{};void* guided_kernel{};

 std::vector<unsigned char> modulo_bytes;bool modulo_recusado{};

 size_t falha_bytes{};unsigned falha_espera{};unsigned falhas{};
};
inline Buffers& buffers(){static auto* b=new Buffers;return *b;}

inline bool inicializar(void* primary,int width,int height){
 auto& b=buffers();
 if(!b.initialized){

  const std::wstring ini=nrinipath::resolve(static_cast<char*>(primary)-nrproxy::rva_contexto,reinterpret_cast<const void*>(&buffers));
  if(ini.empty())return false;
  wchar_t scale[32]{};
  NRCFG_STR(L"DlssNrOnAmd",L"NeuralPreScalePercent",L"100",scale,32,ini.c_str());
  b.percent=wcscmp(scale,L"6.25")==0?6.25:NRCFG_INT(L"DlssNrOnAmd",L"NeuralPreScalePercent",100,ini.c_str());
  b.guided=NRCFG_INT(L"DlssNrOnAmd",L"NeuralGuidedUpscale",0,ini.c_str())==1;
  ::nrmpprescale::sem_teto.store(NRCFG_INT(L"DlssNrOnAmd",L"NeuralPassosSemTeto",0,ini.c_str())==1);
  const double lido=b.percent;b.percent=::nrprescale::accepted_percent(b.percent);

  {wchar_t chave[32]{},alvo[32]{};
   NRCFG_STR(L"DlssNrOnAmd",L"NeuralPreScaleAuto",L"0",chave,32,ini.c_str());
   NRCFG_STR(L"DlssNrOnAmd",L"NeuralPreScaleTargetPixels",L"0",alvo,32,ini.c_str());
   const std::uint64_t semente=::nrprescale::auto_alvo_de(std::wcstoll(chave,nullptr,10),std::wcstoll(alvo,nullptr,10));
   if(!::nrprescale::auto_menu_escreveu.load())::nrprescale::auto_alvo.store(semente);}

  {wchar_t post[32]{};NRCFG_STR(L"DlssNrOnAmd",L"NeuralPostPercent",L"",post,32,ini.c_str());
   if(!::nrprescale::post_menu_escreveu.load()){const unsigned c=::nrprescale::post_chave_de(post);::nrprescale::post_chave.store(c?c:100u);}}
  ::nrprescale::campo128.store(static_cast<int>(NRCFG_INT(L"DlssNrOnAmd",L"NeuralPostCampo128",0,ini.c_str())));
  b.initialized=true;
  if(const auto& pasta_h=nrpasta::helper();!pasta_h.empty()){

  }
 }
 return true;
}

struct Plano {bool post{};std::uint64_t alvo{};unsigned chave_post{};double pedido{100.},pct{100.};unsigned w{},h{};};
inline Plano planejar(int width,int height,unsigned request){
 Plano p{};
 p.post=::nrprescale::rede_na_saida(unsigned(width),unsigned(height),::nrprescale::quadro.load(std::memory_order_acquire));
 p.alvo=p.post?0:(::nrprescale::auto_alvo.load(std::memory_order_relaxed));
 p.chave_post=::nrprescale::post_chave.load(std::memory_order_relaxed);
 p.pedido=::nrprescale::pedido_percent(p.post,p.chave_post,buffers().percent,p.alvo,width,height);
 p.pct=::nrmpprescale::multipass_cap_percent(width,height,p.pedido,request);

 const unsigned alinha=p.post?::nrprescale::post_alinhamento(p.chave_post):2u;
 p.w=::nrprescale::extensao_rede(width,p.pct,alinha);p.h=::nrprescale::extensao_rede(height,p.pct,alinha);
 if(p.post){const int c=::nrprescale::campo128.load(std::memory_order_relaxed);p.w=::nrprescale::campo_eixo(p.w,c,unsigned(width));p.h=::nrprescale::campo_eixo(p.h,c,unsigned(height));}

 if(::nrgeo::ligada.load(std::memory_order_relaxed)&&p.pct<100.&&p.w>=64&&p.h>=64){
  static std::mutex trava;static unsigned pw=0,ph=0;static ::nrgeo::Tam ultimo{};
  std::lock_guard<std::mutex> t(trava);
  if(pw!=p.w||ph!=p.h){pw=p.w;ph=p.h;ultimo=::nrgeo::cena(p.w,p.h);}
  if(ultimo.w<=unsigned(width)&&ultimo.h<=unsigned(height)){p.w=ultimo.w;p.h=ultimo.h;}
 }
 return p;
}

inline bool aquecer_tamanho(void* primary,int width,int height,unsigned request,unsigned& w,unsigned& h,double& pct){
 if(!inicializar(primary,width,height))return false;
 const Plano p=planejar(width,height,request);
 if(p.pct==100||width<256||height<256||width>4096||height>4096||p.w<32||p.h<32)return false;
 if(!p.post&&::nrprescale::geometry.load(std::memory_order_acquire)!=::nrprescale::pack(unsigned(width),unsigned(height)))return false;
 w=p.w;h=p.h;pct=p.pct;return true;
}
inline bool evaluate(Eval eval,void* primary,void* source,int input_pitch,int input_format,int height,int width,
 void* destination,int output_pitch,int output_format,int tone,void* metadata,unsigned request,float* controls,bool& chained,unsigned& effective){
 chained=false;effective=request;
 auto& b=buffers();
 if(!inicializar(primary,width,height))return false;

 const Plano plano=planejar(width,height,request);
 const bool post=plano.post;
 const std::uint64_t alvo=plano.alvo;
 const double pedido=plano.pedido;

 const double pct=plano.pct;
 {static std::atomic<unsigned> cap_logs{0};static double ultimo_pct=-1,ultimo_pedido=-1;static unsigned ultimo_request=0;static int cw=0,ch=0;
  if(::nrmpprescale::prescale_chains(request)&&(pct!=ultimo_pct||pedido!=ultimo_pedido||request!=ultimo_request||width!=cw||height!=ch)&&cap_logs.fetch_add(1)<64){
   ultimo_pct=pct;ultimo_pedido=pedido;ultimo_request=request;cw=width;ch=height;
   if(const auto& pasta_h=nrpasta::helper();!pasta_h.empty()){

   }
  }}
 if(alvo){
  static std::atomic<unsigned> auto_logs{0};static double ultimo=-1;static int ultimo_w=0,ultimo_h=0;static std::uint64_t ultimo_alvo=0;
  if((pct!=ultimo||width!=ultimo_w||height!=ultimo_h||alvo!=ultimo_alvo)&&auto_logs.fetch_add(1)<64){
   ultimo=pct;ultimo_w=width;ultimo_h=height;ultimo_alvo=alvo;
   if(const auto& pasta_h=nrpasta::helper();!pasta_h.empty()){

   }
  }
 }
 if(pct==100){::nrprescale::efetivo.store(100.);return false;}

 const std::uint64_t expected=::nrprescale::pack(unsigned(width),unsigned(height));
 const std::uint64_t published=::nrprescale::geometry.load(std::memory_order_acquire);
 const unsigned meta40=metadata?read<unsigned>(metadata,0x40):0u;
 const char* reason=nullptr;

 if(!metadata)reason="metadata_null";
 else if(meta40!=1)reason="metadata_0x40";
 else if(input_format!=0 && input_format!=6)reason="input_format";
 else if(output_format!=0)reason="output_format";
 else if(width<256 || height<256 || width>4096 || height>4096)reason="extent";
 else if(input_pitch<width*(input_format==6?4:8) || input_pitch%(input_format==6?4:2)!=0)reason="input_pitch";
 else if(output_pitch<width*8 || output_pitch%2!=0)reason="output_pitch";

 else if(published!=expected&&!post)reason="geometry";
 if(reason){
  static std::atomic<unsigned> logged{0};static const char* last=nullptr;
  if(last!=reason && logged.fetch_add(1)<16){
   last=reason;
   if(const auto& pasta_h=nrpasta::helper();!pasta_h.empty()){

   }
  }
  ::nrprescale::efetivo.store(100.);return false;
 }

 const unsigned w=plano.w,h=plano.h;
 if(w<32 || h<32){::nrprescale::efetivo.store(100.);return false;}
 ::nrprescale::efetivo.store(pct);
 Hip hip;int device{};check(hip.device(&device));
 if(b.owner && (b.owner!=primary||b.device!=device))throw std::runtime_error("neural scale context/device changed");
 b.owner=primary;b.device=device;
 if(b.modulo_recusado){::nrprescale::efetivo.store(100.);return false;}
 if(!b.kernel){

  nrmodpin::Leitura leitura{};
  int r=carregar_modulo(hip,&b.module,nrmodpin::resize,b.modulo_bytes,leitura);
  const char* motivo=leitura==nrmodpin::Leitura::ausente?"modulo_ausente":leitura!=nrmodpin::Leitura::ok?"modulo_pin_diferente":"modulo_recusado";
  if(!r&&(hip.get_kernel(&b.kernel,b.module,"nr_resize")||hip.get_kernel(&b.packed_kernel,b.module,"nr_resize_rgb9e5")||
     (b.guided&&hip.get_kernel(&b.guided_kernel,b.module,"nr_guided")))){r=-1;motivo="simbolo_ausente";}
  if(r){
   (void)hip.error();b.kernel=b.packed_kernel=b.guided_kernel=nullptr;b.modulo_recusado=true;

   (void)motivo;

   ::nrprescale::efetivo.store(100.);return false;
  }
 }
 const size_t bytes=size_t(w)*h*8;

 const auto neutro=[&]{
  check(hip.set(destination,0,size_t(output_pitch)*size_t(height)));hip.completed("pre_escala_neutra");
  chained=true;effective=(request&0xff00u)|1u;return true;};
 if(bytes>b.capacity){
  if(b.falha_bytes&&bytes>=b.falha_bytes&&b.falha_espera){--b.falha_espera;return neutro();}
  void* in=nullptr;void* out=nullptr;
  int r=hip.allocate(&in,bytes);
  if(!r){r=hip.allocate(&out,bytes);if(r){(void)hip.release(in);in=nullptr;}}
  if(r){
   (void)hip.error();b.falha_bytes=bytes;b.falha_espera=60;++b.falhas;

   return neutro();
  }
  b.falha_bytes=0;b.falha_espera=0;
  hip.completed();if(b.input)check(hip.release(b.input));if(b.output)check(hip.release(b.output));
  b.input=in;b.output=out;b.capacity=bytes;
 }
 auto resize=[&](void* kernel,void* src,void* dst,unsigned sw,unsigned sh,unsigned sp,unsigned dw,unsigned dh,unsigned dp,unsigned area){
  void* args[]={&src,&dst,&sw,&sh,&sp,&dw,&dh,&dp,&area};
  check(hip.launch(kernel,(dw*dh+255)/256,1,1,256,1,1,0,nullptr,args,nullptr));
 };
 resize(input_format==6?b.packed_kernel:b.kernel,source,b.input,unsigned(width),unsigned(height),unsigned(input_pitch)/(input_format==6?4:2),w,h,w*4,1);

 eval(primary,b.input,int(w*8),0,int(h),int(w),b.output,int(w*8),0,tone,metadata);nrlento::marca(0);

 if(::nrmpprescale::prescale_chains(request)){
  chained=true;
  effective=extra_passes(eval,static_cast<char*>(primary)-nrproxy::rva_contexto,primary,int(h),int(w),b.output,int(w*8),0,metadata,request,controls,false);
 }

 if(b.guided){
  unsigned sw=w,sh=h,sp=w*4,dw=unsigned(width),dh=unsigned(height),dp=unsigned(output_pitch)/2,lp=w*8,hp=unsigned(input_pitch),packed=input_format==6?1u:0u;
  void* args[]={&b.output,&destination,&b.input,&source,&sw,&sh,&sp,&dw,&dh,&dp,&lp,&hp,&packed};
  check(hip.launch(b.guided_kernel,(dw*dh+255)/256,1,1,256,1,1,0,nullptr,args,nullptr));
 }else resize(b.kernel,b.output,destination,w,h,w*4,unsigned(width),unsigned(height),unsigned(output_pitch)/2,0);
 hip.completed();

 static unsigned visto_w=0,visto_h=0;static bool visto_post=false;static unsigned trocas_logadas=0;
 if(visto_w!=0u&&(w!=visto_w||h!=visto_h||post!=visto_post)&&trocas_logadas<1024u&&++trocas_logadas){
  if(const auto& pasta_h=nrpasta::helper();!pasta_h.empty()){

  }
 }
 visto_w=w;visto_h=h;visto_post=post;
 static bool reported=false;
 if(!reported){
  if(const auto& pasta_h=nrpasta::helper();!pasta_h.empty()){

  }reported=true;
 }
 return true;
}
}
