#pragma once
#include "nr_publico.h"
#include "pre_scale_state.h"
#include "native_ini_path.h"
#include "multipass_prescale.h"
namespace nrprescale_runtime {
struct Buffers {
 void* module{};void* kernel{};void* packed_kernel{};void* input{};void* output{};size_t capacity{};
 double percent=100;bool initialized{};int device=-1;void* owner{};bool guided{};void* guided_kernel{};
};
inline Buffers& buffers(){static auto* b=new Buffers;return *b;}
inline bool evaluate(Eval eval,void* primary,void* source,int input_pitch,int input_format,int height,int width,
 void* destination,int output_pitch,int output_format,int tone,void* metadata,unsigned request,float* controls,bool& chained,unsigned& effective){
 chained=false;effective=request;
 auto& b=buffers();
 if(!b.initialized){

  const std::wstring ini=nrinipath::resolve(static_cast<char*>(primary)-0x93f48,reinterpret_cast<const void*>(&buffers));
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
  wchar_t path[32768]{};HMODULE self{};
  if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&buffers),&self)&&GetModuleFileNameW(self,path,32768)){

  }
 }

 const bool post=::nrprescale::rede_na_saida(unsigned(width),unsigned(height),::nrprescale::quadro.load(std::memory_order_acquire));
 const std::uint64_t alvo=post?0:(::nrprescale::auto_alvo.load(std::memory_order_relaxed));
 const unsigned chave_post=::nrprescale::post_chave.load(std::memory_order_relaxed);
 const double pedido=::nrprescale::pedido_percent(post,chave_post,b.percent,alvo,width,height);

 const double pct=::nrmpprescale::multipass_cap_percent(width,height,pedido,request);
 {static std::atomic<unsigned> cap_logs{0};static double ultimo_pct=-1,ultimo_pedido=-1;static unsigned ultimo_request=0;static int cw=0,ch=0;
  if(::nrmpprescale::prescale_chains(request)&&(pct!=ultimo_pct||pedido!=ultimo_pedido||request!=ultimo_request||width!=cw||height!=ch)&&cap_logs.fetch_add(1)<64){
   ultimo_pct=pct;ultimo_pedido=pedido;ultimo_request=request;cw=width;ch=height;
   wchar_t path[32768]{};HMODULE self{};
   if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&buffers),&self)&&GetModuleFileNameW(self,path,32768)){

   }
  }}
 if(alvo){
  static std::atomic<unsigned> auto_logs{0};static double ultimo=-1;static int ultimo_w=0,ultimo_h=0;static std::uint64_t ultimo_alvo=0;
  if((pct!=ultimo||width!=ultimo_w||height!=ultimo_h||alvo!=ultimo_alvo)&&auto_logs.fetch_add(1)<64){
   ultimo=pct;ultimo_w=width;ultimo_h=height;ultimo_alvo=alvo;
   wchar_t path[32768]{};HMODULE self{};
   if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&buffers),&self)&&GetModuleFileNameW(self,path,32768)){

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
   wchar_t path[32768]{};HMODULE self{};
   if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&buffers),&self)&&GetModuleFileNameW(self,path,32768)){

   }
  }
  ::nrprescale::efetivo.store(100.);return false;
 }

 const unsigned alinha=post?::nrprescale::post_alinhamento(chave_post):2u;
 unsigned w=::nrprescale::extensao_rede(width,pct,alinha),h=::nrprescale::extensao_rede(height,pct,alinha);
 if(post){const int c=::nrprescale::campo128.load(std::memory_order_relaxed);w=::nrprescale::campo_eixo(w,c,unsigned(width));h=::nrprescale::campo_eixo(h,c,unsigned(height));}
 if(w<32 || h<32){::nrprescale::efetivo.store(100.);return false;}
 ::nrprescale::efetivo.store(pct);
 Hip hip;int device{};check(hip.device(&device));
 if(b.owner && (b.owner!=primary||b.device!=device))throw std::runtime_error("neural scale context/device changed");
 b.owner=primary;b.device=device;
 if(!b.kernel){
  wchar_t path[32768]{};HMODULE self{};
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&buffers),&self)||!GetModuleFileNameW(self,path,32768))throw std::runtime_error("neural scale module path");

  const auto hsaco=std::filesystem::path(path).parent_path()/L"nr_resize.hsaco";
  std::error_code ec;
  if(!std::filesystem::is_regular_file(hsaco,ec)){
   static std::atomic<bool> avisado{false};
   if(!avisado.exchange(true)){

   }
   ::nrprescale::efetivo.store(100.);return false;
  }
  auto file=hsaco.string();
  check(hip.load_module(&b.module,file.c_str()));check(hip.get_kernel(&b.kernel,b.module,"nr_resize"));
  check(hip.get_kernel(&b.packed_kernel,b.module,"nr_resize_rgb9e5"));
  if(b.guided)check(hip.get_kernel(&b.guided_kernel,b.module,"nr_guided"));
 }
 const size_t bytes=size_t(w)*h*8;
 if(bytes>b.capacity){
  void* in=nullptr;void* out=nullptr;check(hip.allocate(&in,bytes));
  try{check(hip.allocate(&out,bytes));}catch(...){hip.release(in);throw;}
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
  effective=extra_passes(eval,static_cast<char*>(primary)-0x93f48,primary,int(h),int(w),b.output,int(w*8),0,metadata,request,controls,false);
 }else hip.completed();
 if(b.guided){
  unsigned sw=w,sh=h,sp=w*4,dw=unsigned(width),dh=unsigned(height),dp=unsigned(output_pitch)/2,lp=w*8,hp=unsigned(input_pitch),packed=input_format==6?1u:0u;
  void* args[]={&b.output,&destination,&b.input,&source,&sw,&sh,&sp,&dw,&dh,&dp,&lp,&hp,&packed};
  check(hip.launch(b.guided_kernel,(dw*dh+255)/256,1,1,256,1,1,0,nullptr,args,nullptr));
 }else resize(b.kernel,b.output,destination,w,h,w*4,unsigned(width),unsigned(height),unsigned(output_pitch)/2,0);
 hip.completed();

 static unsigned visto_w=0,visto_h=0;static bool visto_post=false;static unsigned trocas_logadas=0;
 if(visto_w!=0u&&(w!=visto_w||h!=visto_h||post!=visto_post)&&trocas_logadas<1024u&&++trocas_logadas){
  wchar_t path[32768]{};HMODULE self{};
  if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&buffers),&self)&&GetModuleFileNameW(self,path,32768)){

  }
 }
 visto_w=w;visto_h=h;visto_post=post;
 static bool reported=false;
 if(!reported){
  wchar_t path[32768]{};HMODULE self{};
  if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&buffers),&self)&&GetModuleFileNameW(self,path,32768)){

  }reported=true;
 }
 return true;
}
}
