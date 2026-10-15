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

inline bool inicializar(void* primary,int width,int height){
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

std::ofstream log(std::filesystem::path(path).parent_path()/L"nr_pre_scale.log",std::ios::app);
   log<<"PRE_SCALE_CONFIG pid="<<GetCurrentProcessId()<<" percent="<<b.percent<<" lido="<<lido<<" dims="<<width<<'x'<<height<<"->"<<::nrprescale::scaled_extent(width,b.percent)<<'x'<<::nrprescale::scaled_extent(height,b.percent)<<" guided="<<b.guided<<" auto_alvo="<<::nrprescale::auto_alvo.load()<<" origem="<<(nrinipath::native(static_cast<char*>(primary)-0x93f48).empty()?"modulo":"proxy")<<" ini="<<std::filesystem::path(ini).string()<<std::endl;

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
   wchar_t path[32768]{};HMODULE self{};
   if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&buffers),&self)&&GetModuleFileNameW(self,path,32768)){

std::ofstream log(std::filesystem::path(path).parent_path()/L"nr_pre_scale.log",std::ios::app);
    log<<"PRE_SCALE_MULTIPASS_CAP pid="<<GetCurrentProcessId()<<" request="<<request<<" extent="<<width<<'x'<<height<<" pedido="<<pedido
       <<" teto720="<<::nrmpprescale::multipass_cap_percent(width,height,100,request)<<" percent="<<pct
       <<" neural="<<::nrprescale::scaled_extent(width,pct)<<'x'<<::nrprescale::scaled_extent(height,pct)<<std::endl;

   }
  }}
 if(alvo){
  static std::atomic<unsigned> auto_logs{0};static double ultimo=-1;static int ultimo_w=0,ultimo_h=0;static std::uint64_t ultimo_alvo=0;
  if((pct!=ultimo||width!=ultimo_w||height!=ultimo_h||alvo!=ultimo_alvo)&&auto_logs.fetch_add(1)<64){
   ultimo=pct;ultimo_w=width;ultimo_h=height;ultimo_alvo=alvo;
   wchar_t path[32768]{};HMODULE self{};
   if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&buffers),&self)&&GetModuleFileNameW(self,path,32768)){

std::ofstream log(std::filesystem::path(path).parent_path()/L"nr_pre_scale.log",std::ios::app);
    log<<"PRE_SCALE_AUTO pid="<<GetCurrentProcessId()<<" alvo="<<alvo<<" extent="<<width<<'x'<<height<<" percent="<<pct
       <<" neural="<<::nrprescale::scaled_extent(width,pct)<<'x'<<::nrprescale::scaled_extent(height,pct)
       <<" area_preenchida="<<::nrprescale::area_rede(width,height,pct)<<" qpc="<<[]{LARGE_INTEGER q{};QueryPerformanceCounter(&q);return q.QuadPart;}()<<std::endl;

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

std::ofstream log(std::filesystem::path(path).parent_path()/L"nr_pre_scale.log",std::ios::app);
    log<<"PRE_SCALE_SKIP pid="<<GetCurrentProcessId()<<" reason="<<reason<<" percent="<<pct<<" request="<<request
       <<" metadata="<<(metadata?1:0)<<" metadata_0x40="<<meta40<<" input_format="<<input_format<<" output_format="<<output_format
       <<" extent="<<width<<'x'<<height<<" input_pitch="<<input_pitch<<" output_pitch="<<output_pitch
       <<" geometry_published="<<(published>>32)<<'x'<<(published&0xffffffffu)<<" geometry_expected="<<width<<'x'<<height<<std::endl;

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
 if(!b.kernel){
  wchar_t path[32768]{};HMODULE self{};
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&buffers),&self)||!GetModuleFileNameW(self,path,32768))throw std::runtime_error("neural scale module path");

  const auto hsaco=std::filesystem::path(path).parent_path()/L"nr_resize.hsaco";
  std::error_code ec;
  if(!std::filesystem::is_regular_file(hsaco,ec)){
   static std::atomic<bool> avisado{false};
   if(!avisado.exchange(true)){

std::ofstream log(std::filesystem::path(path).parent_path()/L"nr_pre_scale.log",std::ios::app);
    log<<"PRE_SCALE_SKIP pid="<<GetCurrentProcessId()<<" reason=modulo_ausente arquivo=nr_resize.hsaco (rodar o instalador com -Sincronizar)"<<std::endl;

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

std::ofstream log(std::filesystem::path(path).parent_path()/L"nr_pre_scale.log",std::ios::app);
   log<<"PRE_SCALE_TROCA pid="<<GetCurrentProcessId()<<" tick="<<GetTickCount64()<<" stage="<<(post?"POST":"PRE")<<" effective="<<pct<<" neural="<<w<<'x'<<h<<" antes="<<visto_w<<'x'<<visto_h<<" output="<<width<<'x'<<height<<"\n";

  }
 }
 visto_w=w;visto_h=h;visto_post=post;
 static bool reported=false;
 if(!reported){
  wchar_t path[32768]{};HMODULE self{};
  if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&buffers),&self)&&GetModuleFileNameW(self,path,32768)){

std::ofstream log(std::filesystem::path(path).parent_path()/L"nr_pre_scale.log",std::ios::app);
   log<<"PRE_SCALE pid="<<GetCurrentProcessId()<<" stage="<<(post?"POST":"PRE")<<" effective="<<pct<<" source="<<width<<'x'<<height<<" neural="<<w<<'x'<<h<<" at "<<w<<'x'<<h<<" output="<<width<<'x'<<height<<" input_format="<<input_format<<" guided="<<b.guided<<" fsr_descriptor_unchanged=1 residual=1\n";

  }reported=true;
 }
 return true;
}
}
