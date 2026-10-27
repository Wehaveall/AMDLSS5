#include "fsr2_ffx.h"
#include "../active-scheduling/fsr2_ffx_recurso_dx12.h"
#include "../active-scheduling/fsr2_ffx_traducao.h"
#include "../compat-diag/compat.h"
#include "../../../runtime/ini_jogo.h"
#include <windows.h>
#include <d3d12.h>
#include <dx12/ffx_api_dx12.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace nrfsr2ffx {
namespace {
constexpr std::uint64_t kDespachosAteDestruir=8;
constexpr std::int32_t kFsr2Ok=0;
struct Runtime{
  bool tentou=false;HMODULE modulo=nullptr;char nome[48]{};
  PfnFfxCreateContext create=nullptr;PfnFfxDestroyContext destroy=nullptr;PfnFfxQuery query=nullptr;PfnFfxDispatch dispatch=nullptr;
  unsigned char inicio_create[16]{};bool gancho_visto=false;
  bool provedores_listados=false;std::uint64_t provedor=0;
};
struct Aposentado{ffxContext ffx;std::uint64_t ate;};
struct Contexto{
  bool tem_desc=false,recusado=false,visto=false,reset_pendente=false,logado=false;nrffx::fsr2ffx::Descricao desc{};
  ffxContext ffx=nullptr;ID3D12Device* device=nullptr;unsigned recriacoes=0;std::uint64_t despachos=0;std::vector<Aposentado> aposentados;
  std::uint64_t chave_entradas=~0ull;
};
std::mutex trava;Runtime rt;std::unordered_map<void*,Contexto> contextos;
std::atomic<std::uint64_t> despachos_{0},destruidos_{0};

void falha(Contexto& c,const char* motivo) noexcept{
  c.recusado=true;
  char t[224];std::snprintf(t,sizeof t,"%s: o contexto do FSR 2 segue pelo caminho de hoje (generico + FSR 2 do jogo)",motivo);
  nrcompat::tolerado("tolerado_fsr2_ffx_falha",t);
}
bool legivel(const void* p,std::size_t n) noexcept{
  MEMORY_BASIC_INFORMATION m{};if(!p||!VirtualQuery(p,&m,sizeof m)||m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;
  return reinterpret_cast<std::uintptr_t>(p)+n<=reinterpret_cast<std::uintptr_t>(m.BaseAddress)+m.RegionSize;
}
std::wstring pasta_de(HMODULE m){
  wchar_t c[MAX_PATH]{};const DWORD n=GetModuleFileNameW(m,c,MAX_PATH);if(!n||n>=MAX_PATH)return {};
  std::wstring s(c,n);const auto k=s.find_last_of(L"\\/");return k==std::wstring::npos?std::wstring{}:s.substr(0,k);
}
void estreito(const wchar_t* w,char* out,std::size_t n){std::size_t i=0;for(;w&&w[i]&&i+1<n;++i)out[i]=w[i]<128?static_cast<char>(w[i]):'?';if(n)out[i]=0;}

bool desviado(const unsigned char* p) noexcept{return p[0]==0xE9||(p[0]==0xFF&&p[1]==0x25);}

bool carregar() noexcept{
  if(rt.tentou)return rt.modulo!=nullptr;
  rt.tentou=true;
  const wchar_t* nomes[]{L"amd_fidelityfx_loader_dx12.dll",L"amd_fidelityfx_dx12.dll"};
  std::vector<std::wstring> pastas{pasta_de(nullptr)};
  if(HMODULE f=GetModuleHandleW(L"ffx_fsr2_api_x64.dll")){auto p=pasta_de(f);if(!p.empty()&&_wcsicmp(p.c_str(),pastas[0].c_str())!=0)pastas.push_back(p);}
  for(const wchar_t* nome:nomes){
    HMODULE m=GetModuleHandleW(nome);
    for(std::size_t i=0;!m&&i<pastas.size();++i){
      const std::wstring caminho=pastas[i]+L"\\"+nome;
      if(pastas[i].empty()||GetFileAttributesW(caminho.c_str())==INVALID_FILE_ATTRIBUTES)continue;
      m=LoadLibraryExW(caminho.c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
    }
    if(!m)continue;
    rt.create=reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(m,"ffxCreateContext"));
    rt.destroy=reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(m,"ffxDestroyContext"));
    rt.query=reinterpret_cast<PfnFfxQuery>(GetProcAddress(m,"ffxQuery"));
    rt.dispatch=reinterpret_cast<PfnFfxDispatch>(GetProcAddress(m,"ffxDispatch"));
    if(!rt.create||!rt.destroy||!rt.query||!rt.dispatch)continue;
    rt.modulo=m;estreito(nome,rt.nome,sizeof rt.nome);
    std::memcpy(rt.inicio_create,reinterpret_cast<const void*>(rt.create),sizeof rt.inicio_create);
    rt.gancho_visto=desviado(rt.inicio_create);
    return true;
  }
  return false;
}

bool gancho_chegou() noexcept{
  if(!rt.modulo)return false;
  if(!rt.gancho_visto&&std::memcmp(reinterpret_cast<const void*>(rt.create),rt.inicio_create,sizeof rt.inicio_create)!=0)rt.gancho_visto=true;
  return rt.gancho_visto;
}
void mensagem(uint32_t tipo,const wchar_t* msg){
  char t[256];estreito(msg,t,sizeof t);
  nrcompat::report(tipo==FFX_API_MESSAGE_TYPE_ERROR?"fsr2_ffx_mensagem_erro":"fsr2_ffx_mensagem_aviso",t);
}

void escolher_provedor(ID3D12Device* device) noexcept{
  if(rt.provedores_listados)return;
  rt.provedores_listados=true;
  wchar_t pedido_w[32]{};if(!nrinijogo::ler_texto(L"NeuralFsr2FfxProvedor",pedido_w,32))return;
  char pedido[32];estreito(pedido_w,pedido,sizeof pedido);
  std::uint64_t n=0;ffxQueryDescGetVersions v{};v.header.type=FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
  v.createDescType=FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;v.device=device;v.outputCount=&n;
  if(rt.query(nullptr,&v.header)!=FFX_API_RETURN_OK||n==0||n>64)return;
  std::vector<std::uint64_t> ids(static_cast<std::size_t>(n));std::vector<const char*> nomes(static_cast<std::size_t>(n));
  v.versionIds=ids.data();v.versionNames=nomes.data();
  if(rt.query(nullptr,&v.header)!=FFX_API_RETURN_OK)return;
  for(std::size_t i=0;i<ids.size();++i)if(nomes[i]&&std::strncmp(nomes[i],pedido,std::strlen(pedido))==0){rt.provedor=ids[i];break;}
  char t[128];std::snprintf(t,sizeof t,"NeuralFsr2FfxProvedor=%s -> %s",pedido,rt.provedor?"achado":"nao existe (o loader escolhe)");
  nrcompat::report("fsr2_ffx_provedor",t);
}
void destruir(ffxContext& f) noexcept{
  if(f&&rt.destroy&&rt.destroy(&f,nullptr)==FFX_API_RETURN_OK)destruidos_.fetch_add(1,std::memory_order_relaxed);
  f=nullptr;
}

bool criar(Contexto& c,ID3D12Device* device) noexcept{
  escolher_provedor(device);
  const bool com_gancho=gancho_chegou();
  ffxCreateContextDescUpscale up{};nrffx::fsr2ffx::criar_upscale(c.desc,up);up.fpMessage=&mensagem;
  ffxCreateBackendDX12Desc be{};be.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;be.device=device;
  ffxOverrideVersion ov{};ov.header.type=FFX_API_DESC_TYPE_OVERRIDE_VERSION;ov.versionId=rt.provedor;
  ffxCreateContextDescUpscaleVersion ver{};ver.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE_VERSION;ver.version=FFX_UPSCALER_VERSION;
  up.header.pNext=&be.header;ffxApiHeader* fim=&be.header;
  if(rt.provedor){fim->pNext=&ov.header;fim=&ov.header;}
  fim->pNext=&ver.header;
  ffxReturnCode_t r=rt.create(&c.ffx,&up.header,nullptr);
  if(r!=FFX_API_RETURN_OK){fim->pNext=nullptr;c.ffx=nullptr;r=rt.create(&c.ffx,&up.header,nullptr);}
  if(r!=FFX_API_RETURN_OK||!c.ffx){
    c.ffx=nullptr;char t[160];std::snprintf(t,sizeof t,"ffxCreateContext (%s) devolveu %u (flags FFX 0x%x, %ux%u -> %ux%u)",rt.nome,r,up.flags,
      c.desc.max_w,c.desc.max_h,c.desc.tela_w,c.desc.tela_h);
    falha(c,t);return false;
  }
  c.visto=com_gancho;c.reset_pendente=true;c.logado=false;
  ffxQueryGetProviderVersion pv{};pv.header.type=FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;
  const bool tem_nome=rt.query(&c.ffx,&pv.header)==FFX_API_RETURN_OK&&pv.versionName;
  char t[256];std::snprintf(t,sizeof t,"%s: contexto %p, flags FSR 2 0x%x -> FFX 0x%x, %ux%u -> %ux%u, provedor %s, %s (recriacao %u)",rt.nome,
    static_cast<void*>(c.ffx),c.desc.flags,up.flags,c.desc.max_w,c.desc.max_h,c.desc.tela_w,c.desc.tela_h,tem_nome?pv.versionName:"?",
    com_gancho?"com o gancho do proxy":"ANTES do gancho do proxy (recria quando ele chegar)",c.recriacoes);
  nrcompat::tolerado("tolerado_fsr2_ffx_contexto",t);
  return true;
}
void soltar(Contexto& c) noexcept{
  destruir(c.ffx);for(auto& a:c.aposentados)destruir(a.ffx);c.aposentados.clear();
  if(c.device){c.device->Release();c.device=nullptr;}
}
}
bool pedido() noexcept{
  static const bool v=[]{double x=0.;return nrinijogo::ler_double(L"NeuralFsr2ParaFfx",x)&&x>.5;}();
  return v;
}
void ao_criar(void* ctx,const void* descricao) noexcept{
  if(!ctx||!descricao||!pedido())return;
  nrffx::fsr2ffx::Descricao d{};if(!nrffx::fsr2ffx::descricao_plausivel(descricao,d))return;
  try{
    std::lock_guard<std::mutex> l(trava);
    auto it=contextos.find(ctx);if(it!=contextos.end())soltar(it->second);
    Contexto c;c.tem_desc=true;c.desc=d;contextos[ctx]=std::move(c);
  }catch(...){}
}
bool despachar(void* ctx,const void* desc,const nrffx::fsr2::Leiaute& L,std::size_t n,std::int32_t& retorno) noexcept{
  if(!ctx||!desc||!pedido())return false;
  try{
    std::lock_guard<std::mutex> l(trava);
    Contexto& c=contextos[ctx];
    if(c.recusado)return false;

    if(!c.tem_desc){
      if(!legivel(ctx,sizeof(nrffx::fsr2ffx::Descricao))||!nrffx::fsr2ffx::descricao_plausivel(ctx,c.desc)){falha(c,"descricao do contexto implausivel");return false;}
      c.tem_desc=true;
    }
    if(!carregar()){falha(c,"nenhuma DLL da FidelityFX API (amd_fidelityfx_loader_dx12.dll ou amd_fidelityfx_dx12.dll) na pasta do jogo");return false;}

    if(c.ffx&&!c.visto&&gancho_chegou()){c.aposentados.push_back({c.ffx,c.despachos+kDespachosAteDestruir});c.ffx=nullptr;++c.recriacoes;}
    if(!c.ffx){
      if(!c.device){
        void* saida=nullptr;std::memcpy(&saida,static_cast<const std::byte*>(desc)+L.output,sizeof saida);
        if(!saida||FAILED(static_cast<ID3D12Resource*>(saida)->GetDevice(IID_PPV_ARGS(&c.device)))){c.device=nullptr;falha(c,"sem o dispositivo D3D12 da saida");return false;}
      }
      if(!criar(c,c.device))return false;
    }
    for(std::size_t i=0;i<c.aposentados.size();)
      if(c.despachos>=c.aposentados[i].ate){destruir(c.aposentados[i].ffx);c.aposentados.erase(c.aposentados.begin()+static_cast<std::ptrdiff_t>(i));}
      else ++i;
    ffxDispatchDescUpscale d{};
    if(!nrffx::fsr2ffx::traduzir(desc,n,L,c.desc,&nrffx::fsr2ffx::descrever_dx12,nullptr,d)){
      static std::atomic<bool> avisou{false};
      if(!avisou.exchange(true))nrcompat::tolerado("tolerado_fsr2_ffx_falha","despacho sem lista/cor/MV/saida: esse quadro vai pelo caminho de hoje");
      return false;
    }
    if(c.reset_pendente)d.reset=true;
    const ffxReturnCode_t r=rt.dispatch(&c.ffx,&d.header);
    if(r!=FFX_API_RETURN_OK){
      char t[96];std::snprintf(t,sizeof t,"ffxDispatch devolveu %u",r);
      c.aposentados.push_back({c.ffx,c.despachos+kDespachosAteDestruir});c.ffx=nullptr;falha(c,t);return false;
    }
    c.reset_pendente=false;++c.despachos;

    {nrffx::fsr2ffx::Auto22 a{};nrffx::fsr2ffx::auto_reativo_22(desc,n,L,a);const std::uint64_t k=nrffx::fsr2ffx::chave_entradas(d,a);
     if(k!=c.chave_entradas){c.chave_entradas=k;char t[512];nrffx::fsr2ffx::texto_entradas(d,a,t,sizeof t);nrcompat::report("fsr2_ffx_entradas",t);}}
    const auto total=despachos_.fetch_add(1,std::memory_order_relaxed)+1;
    if(!c.logado){
      c.logado=true;char t[200];
      std::snprintf(t,sizeof t,"1o despacho pela FFX (%s) do contexto %p: render %ux%u -> %ux%u, reset %u, nitidez %u, total %llu",rt.nome,
        static_cast<void*>(c.ffx),d.renderSize.width,d.renderSize.height,d.upscaleSize.width,d.upscaleSize.height,d.reset?1u:0u,
        d.enableSharpening?1u:0u,static_cast<unsigned long long>(total));
      nrcompat::tolerado("tolerado_fsr2_ffx_despacho",t);
    }
    retorno=kFsr2Ok;return true;
  }catch(...){return false;}
}
void ao_destruir(void* ctx) noexcept{
  if(!ctx)return;
  try{
    std::lock_guard<std::mutex> l(trava);
    auto it=contextos.find(ctx);if(it==contextos.end())return;
    soltar(it->second);contextos.erase(it);
  }catch(...){}
}
std::uint64_t despachos() noexcept{return despachos_.load(std::memory_order_relaxed);}
namespace teste {
bool estado(void* ctx,Estado& e) noexcept{
  std::lock_guard<std::mutex> l(trava);auto it=contextos.find(ctx);if(it==contextos.end())return false;const auto& c=it->second;
  e={c.ffx,nrffx::fsr2ffx::flags_para_ffx(c.desc.flags),c.desc.max_w,c.desc.max_h,c.desc.tela_w,c.desc.tela_h,c.recriacoes,c.visto,c.recusado,c.despachos};
  return true;
}
unsigned vivos() noexcept{
  std::lock_guard<std::mutex> l(trava);unsigned n=0;
  for(const auto& [k,c]:contextos){(void)k;n+=c.ffx?1u:0u;for(const auto& a:c.aposentados)n+=a.ffx?1u:0u;}
  return n;
}
std::uint64_t destruidos() noexcept{return destruidos_.load(std::memory_order_relaxed);}
}
}
