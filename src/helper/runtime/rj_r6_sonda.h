#pragma once

namespace nrr6 {
inline thread_local int modo=0;
inline thread_local bool dentro=false;
struct Reg {const void* f{};void* e0{};void* e1{};long long t_in{},t_out{};int idle{-1};bool novo{};unsigned gx{},gy{},gz{},bx{};};
inline thread_local std::vector<Reg> regs;

inline constexpr const char* api_nomes[]={"hipDeviceSynchronize","hipEventCreate","hipEventCreateWithFlags","hipEventQuery","hipEventRecord",
 "hipEventSynchronize","hipFree","hipMalloc","hipMemcpy","hipMemcpyAsync","hipMemcpyToSymbol","hipMemset","hipMemsetAsync","hipStreamCreateWithFlags",
 "hipStreamSynchronize","hipGetLastError","hipEventElapsedTime","hipImportExternalMemory","hipExternalMemoryGetMappedBuffer","hipDestroyExternalMemory"};
inline constexpr unsigned api_slots[]={0x86b60,0x86b70,0x86b78,0x86b88,0x86b90,0x86b98,0x86ba8,0x86be0,0x86be8,0x86bf0,0x86bf8,0x86c00,0x86c08,0x86c20,
 0x86c28,0x86bc8,0x86b80,0x86bd0,0x86ba0,0x86b58};
inline constexpr unsigned api_n=sizeof(api_slots)/sizeof(api_slots[0]);
struct Api {unsigned long long n{};long long ticks{};unsigned long long bytes{};};
inline thread_local Api api[api_n]{};
inline long long freq() noexcept {static const long long f=[]{LARGE_INTEGER q{};QueryPerformanceFrequency(&q);return q.QuadPart;}();return f;}
inline long long agora() noexcept {LARGE_INTEGER q{};QueryPerformanceCounter(&q);return q.QuadPart;}
template<unsigned I,class R,class... A> struct W {
 static inline R(*proximo)(A...){};
 static R fn(A... a){
  if(!modo)return proximo(a...);
  const long long t0=agora();R r=proximo(a...);api[I].ticks+=agora()-t0;++api[I].n;
  if constexpr(I==8||I==9){api[I].bytes+=std::get<2>(std::tuple<A...>(a...));}
  return r;
 }
};
using W0=W<0,int>;using W1=W<1,int,void**>;using W2=W<2,int,void**,unsigned>;using W3=W<3,int,void*>;using W4=W<4,int,void*,void*>;
using W5=W<5,int,void*>;using W6=W<6,int,void*>;using W7=W<7,int,void**,std::size_t>;using W8=W<8,int,void*,const void*,std::size_t,int>;
using W9=W<9,int,void*,const void*,std::size_t,int,void*>;using W10=W<10,int,const void*,const void*,std::size_t,std::size_t,int>;
using W11=W<11,int,void*,int,std::size_t>;using W12=W<12,int,void*,int,std::size_t,void*>;using W13=W<13,int,void**,unsigned>;
using W14=W<14,int,void*>;using W15=W<15,int>;using W16=W<16,int,float*,void*,void*>;using W17=W<17,int,void**,const void*>;
using W18=W<18,int,void**,void*,const void*>;using W19=W<19,int,void*>;

inline bool slot_original(char* img,unsigned slot,const char* nome) noexcept {
 auto m=GetModuleHandleW(L"amdhip64_7.dll");return m&&*reinterpret_cast<void**>(img+slot)==reinterpret_cast<void*>(GetProcAddress(m,nome));
}
template<class T> inline void instalar_um(char* img,unsigned slot) noexcept {
 auto s=reinterpret_cast<void**>(img+slot);T::proximo=reinterpret_cast<decltype(T::proximo)>(*s);
 DWORD old{},x{};if(!VirtualProtect(s,sizeof(void*),PAGE_READWRITE,&old))return;
 InterlockedExchangePointer(reinterpret_cast<void* volatile*>(s),reinterpret_cast<void*>(&T::fn));VirtualProtect(s,sizeof(void*),old,&x);
}
inline std::once_flag instalado;
inline void instalar(char* img) noexcept {
 std::call_once(instalado,[img]{
  instalar_um<W1>(img,api_slots[1]);instalar_um<W2>(img,api_slots[2]);instalar_um<W3>(img,api_slots[3]);
  instalar_um<W4>(img,api_slots[4]);instalar_um<W5>(img,api_slots[5]);if(slot_original(img,api_slots[6],"hipFree")&&slot_original(img,api_slots[7],"hipMalloc")){instalar_um<W6>(img,api_slots[6]);instalar_um<W7>(img,api_slots[7]);}
  instalar_um<W8>(img,api_slots[8]);instalar_um<W9>(img,api_slots[9]);instalar_um<W10>(img,api_slots[10]);instalar_um<W11>(img,api_slots[11]);
  instalar_um<W12>(img,api_slots[12]);instalar_um<W13>(img,api_slots[13]);instalar_um<W14>(img,api_slots[14]);instalar_um<W15>(img,api_slots[15]);
  instalar_um<W16>(img,api_slots[16]);});
}

struct HipF {int(*criar)(void**,unsigned){};int(*marcar)(void*,void*){};int(*sinc)(void*){};int(*decorrido)(float*,void*,void*){};int(*consulta)(void*){};
 int(*by_symbol)(void**,const void*){};const char*(*name_ref)(void*){};};
inline const HipF& hipf() noexcept {static const HipF h=[]{HipF r{};auto m=GetModuleHandleW(L"amdhip64_7.dll");if(!m)return r;
 r.criar=reinterpret_cast<decltype(r.criar)>(GetProcAddress(m,"hipEventCreateWithFlags"));r.marcar=reinterpret_cast<decltype(r.marcar)>(GetProcAddress(m,"hipEventRecord"));
 r.sinc=reinterpret_cast<decltype(r.sinc)>(GetProcAddress(m,"hipEventSynchronize"));r.decorrido=reinterpret_cast<decltype(r.decorrido)>(GetProcAddress(m,"hipEventElapsedTime"));
 r.consulta=reinterpret_cast<decltype(r.consulta)>(GetProcAddress(m,"hipStreamQuery"));r.by_symbol=reinterpret_cast<decltype(r.by_symbol)>(GetProcAddress(m,"hipGetFuncBySymbol"));
 r.name_ref=reinterpret_cast<decltype(r.name_ref)>(GetProcAddress(m,"hipKernelNameRef"));return r;}();return h;}
inline thread_local std::vector<void*> pool;
inline void* evento() noexcept {if(!pool.empty()){void* e=pool.back();pool.pop_back();return e;}void* e{};const auto& h=hipf();if(!h.criar||h.criar(&e,0x20000000u))return nullptr;return e;}
inline std::string nome(const void* f) {
 static std::mutex m;static std::vector<std::pair<const void*,std::string>> cache;std::lock_guard<std::mutex> l(m);
 for(const auto& p:cache)if(p.first==f)return p.second;
 const auto& h=hipf();void* fn{};const char* t{};std::string n;
 if(h.by_symbol&&h.name_ref&&!h.by_symbol(&fn,f)&&fn&&(t=h.name_ref(fn))!=nullptr)n=t;else{char b[40]{};std::snprintf(b,sizeof b,"token@%p",f);n=b;}
 cache.emplace_back(f,n);return n;
}
inline void comecar(int m){modo=m;regs.clear();regs.reserve(1024);for(auto& a:api)a={};}
inline void parar(){modo=0;}
}
