#include "geracao_quadros.h"
#include "registro.h"
#include "fsr2_fg_traducao.h"
#include "fg_externo.h"
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dx12/ffx_api_dx12.h>
#include <dx12/ffx_api_framegeneration_dx12.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace f2fg {
namespace {
using nrffx::fsr2fg::AoPresent;

constexpr int kSlotCreateSwapChain=10,kSlotCreateSwapChainForHwnd=15,kSlotRelease=2,kSlotPresent=8,kSlotPresent1=22;
constexpr int kOrdemEstourosMax=8;
using PfnCreateSwapChain=HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory*,IUnknown*,DXGI_SWAP_CHAIN_DESC*,IDXGISwapChain**);
using PfnCreateSwapChainForHwnd=HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*,IUnknown*,HWND,const DXGI_SWAP_CHAIN_DESC1*,
                                                            const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*,IDXGIOutput*,IDXGISwapChain1**);
using PfnRelease=ULONG(STDMETHODCALLTYPE*)(IUnknown*);
using PfnPresent=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*,UINT,UINT);
using PfnPresent1=HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain1*,UINT,UINT,const DXGI_PRESENT_PARAMETERS*);
struct Ffx{PfnFfxCreateContext create=nullptr;PfnFfxDestroyContext destroy=nullptr;PfnFfxConfigure configure=nullptr;PfnFfxQuery query=nullptr;PfnFfxDispatch dispatch=nullptr;};
struct Global{
  bool tentou_ffx=false,ffx_ok=false;Ffx ffx{};
  void** vt_fabrica=nullptr;PfnCreateSwapChain orig_cs=nullptr;PfnCreateSwapChainForHwnd orig_csh=nullptr;
  void** vt_sc=nullptr;PfnRelease orig_release=nullptr;PfnPresent orig_present=nullptr;PfnPresent1 orig_present1=nullptr;
  IDXGISwapChain4* sc=nullptr;ffxContext ctx_sc=nullptr;
  ffxContext fg=nullptr;ID3D12Device* fg_device=nullptr;nrffx::fsr2fg::Chave chave{};
  bool reset_proximo=true,upscale_visto=false;
  nrffx::fsr2fg::Ritmo ritmo{};
  HANDLE mapa=nullptr;nrfgext::Fg* pub=nullptr;
  bool aviso_sem_sc=false,aviso_removido=false,aviso_slot=false,falha_criar=false;

  bool aviso_criar=false;nrffx::fsr2fg::Chave chave_falha{};
  Estado e{};
};
Global g;
std::recursive_mutex g_trava;
thread_local int t_criando=0;
thread_local int t_apresentando=0;

std::atomic<bool> g_instalado{false},g_tentou{false};
std::mutex g_ordem_m;std::condition_variable g_ordem_cv;bool g_pendente=false;std::atomic<bool> g_ordem_off{false};int g_estouros_seguidos=0;

template<class... A>void log(const char* formato,A... a) noexcept{f2f::logf(formato,a...);}
void falha(const char* t) noexcept{log("FG FALHA: %s",t);}
void ordem_marcar(bool pendente) noexcept{
  {std::lock_guard<std::mutex> l(g_ordem_m);g_pendente=pendente;}
  if(!pendente)g_ordem_cv.notify_all();
}
struct Espera{bool esperou=false,estourou=false;double ms=0;};
Espera ordem_esperar() noexcept{
  Espera r;if(f2f::ini().fg_ordem_ms<=0||g_ordem_off.load(std::memory_order_relaxed))return r;
  std::unique_lock<std::mutex> l(g_ordem_m);if(!g_pendente)return r;
  r.esperou=true;const auto t0=std::chrono::steady_clock::now();
  r.estourou=!g_ordem_cv.wait_for(l,std::chrono::milliseconds(f2f::ini().fg_ordem_ms),[]{return !g_pendente;});
  r.ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
  if(r.estourou)g_pendente=false;
  return r;
}
bool gravar_ptr(void** slot,void* novo) noexcept{
  DWORD velho=0;if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&velho))return false;
  *slot=novo;DWORD x=0;VirtualProtect(slot,sizeof(void*),velho,&x);return true;
}
std::wstring pasta_de(HMODULE m){
  wchar_t c[MAX_PATH]{};const DWORD n=GetModuleFileNameW(m,c,MAX_PATH);if(!n||n>=MAX_PATH)return {};
  std::wstring s(c,n);const auto k=s.find_last_of(L"\\/");return k==std::wstring::npos?std::wstring{}:s.substr(0,k);
}

const Ffx* carregar_ffx() noexcept{
  if(g.tentou_ffx)return g.ffx_ok?&g.ffx:nullptr;
  g.tentou_ffx=true;
  const wchar_t* nome=L"amd_fidelityfx_framegeneration_dx12.dll";
  HMODULE m=GetModuleHandleW(nome);
  std::vector<std::wstring> pastas{pasta_de(nullptr)};
  if(HMODULE f=GetModuleHandleW(L"ffx_fsr2_api_x64.dll")){auto p=pasta_de(f);if(!p.empty()&&_wcsicmp(p.c_str(),pastas[0].c_str())!=0)pastas.push_back(p);}
  for(std::size_t i=0;!m&&i<pastas.size();++i){
    if(pastas[i].empty())continue;const std::wstring c=pastas[i]+L"\\"+nome;
    if(GetFileAttributesW(c.c_str())!=INVALID_FILE_ATTRIBUTES)m=LoadLibraryExW(c.c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
  }
  if(m){
    g.ffx.create=reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(m,"ffxCreateContext"));
    g.ffx.destroy=reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(m,"ffxDestroyContext"));
    g.ffx.configure=reinterpret_cast<PfnFfxConfigure>(GetProcAddress(m,"ffxConfigure"));
    g.ffx.query=reinterpret_cast<PfnFfxQuery>(GetProcAddress(m,"ffxQuery"));
    g.ffx.dispatch=reinterpret_cast<PfnFfxDispatch>(GetProcAddress(m,"ffxDispatch"));
    g.ffx_ok=g.ffx.create&&g.ffx.destroy&&g.ffx.configure&&g.ffx.query&&g.ffx.dispatch;
  }
  if(!g.ffx_ok)falha(m?"amd_fidelityfx_framegeneration_dx12.dll sem os exports da FidelityFX API: sem FG"
                       :"sem amd_fidelityfx_framegeneration_dx12.dll na pasta do jogo: sem FG");
  return g.ffx_ok?&g.ffx:nullptr;
}

void publicar() noexcept{
  if(!g.pub)return;
  g.pub->disponivel=g.sc?1u:0u;
  g.pub->ligado=g.e.usuario&&g.sc?1u:0u;
  g.pub->geracoes=g.e.geracoes;
  g.pub->tick_ms=GetTickCount64();
  std::snprintf(g.pub->provedor,sizeof g.pub->provedor,"%s",g.e.provedor);
}
ffxReturnCode_t cb_gerar(ffxDispatchDescFrameGeneration* params,void* ctx){

  if(!g.ffx_ok||!ctx)return FFX_API_RETURN_ERROR_PARAMETER;
  const ffxReturnCode_t r=g.ffx.dispatch(static_cast<ffxContext*>(ctx),&params->header);
  ++g.e.geracoes;
  if(r!=FFX_API_RETURN_OK&&g.e.erros++<16)log("ERRO dispatch da geracao devolveu %u (frameID %llu)",r,static_cast<unsigned long long>(params->frameID));
  return r;
}
void configurar(bool ligado,const char* motivo=nullptr) noexcept{
  if(!g.fg||!g.sc)return;
  ffxConfigureDescFrameGeneration c{};
  nrffx::fsr2fg::configurar(g.sc,ligado,++g.e.frame_id,g.chave.tela_w,g.chave.tela_h,&cb_gerar,&g.fg,c);
  const ffxReturnCode_t r=g.ffx.configure(&g.fg,&c.header);
  if(r!=FFX_API_RETURN_OK&&g.e.erros++<16)log("ERRO ffxConfigure(%s) devolveu %u",ligado?"ligado":"desligado",r);
  if(ligado!=g.e.fg_ligado){
    log("FG %s (frameID %llu)",ligado?"ligado":(motivo?motivo:"desligado pelo menu do mod"),static_cast<unsigned long long>(g.e.frame_id));
    if(!ligado)++g.e.desligados;
  }
  if(!ligado){g.reset_proximo=true;ordem_marcar(false);}
  g.e.fg_ligado=ligado;
}
void destruir_fg() noexcept{
  if(!g.fg)return;
  configurar(false,"desligado: contexto de FG saindo");
  const ffxReturnCode_t r=g.ffx.destroy(&g.fg,nullptr);
  log("contexto de FG %p destruido (%u)",static_cast<void*>(g.fg),r);
  g.fg=nullptr;g.e.fg_ligado=false;g.reset_proximo=true;
  if(g.fg_device)g.fg_device->Release();
  g.fg_device=nullptr;g.e.contexto=false;
}

ULONG STDMETHODCALLTYPE meu_release(IUnknown* s){
  if(!g.sc||s!=static_cast<IUnknown*>(g.sc))return g.orig_release(s);
  std::lock_guard<std::recursive_mutex> t(g_trava);
  const ULONG n=g.orig_release(s);
  if(n!=1||s!=static_cast<IUnknown*>(g.sc)||!g.ctx_sc)return n;

  log("o jogo soltou a swapchain de FG %p (Presents %llu)",static_cast<void*>(g.sc),static_cast<unsigned long long>(g.e.presents));
  ffxContext c=g.ctx_sc;g.ctx_sc=nullptr;
  destruir_fg();
  g.sc=nullptr;g.e.swapchain=false;g.e.swapchain_ptr=nullptr;
  publicar();
  g.ffx.destroy(&c,nullptr);
  return 0;
}
void antes_do_present() noexcept{
  ++g.e.presents;
  ordem_marcar(false);
  if(nrffx::fsr2fg::ao_present(g.ritmo,g.e.fg_ligado)==AoPresent::Desliga)configurar(false,"desligado: Presents sem upscale (menu/carregamento)");
}

HRESULT STDMETHODCALLTYPE meu_present(IDXGISwapChain* s,UINT sync,UINT flags){
  if(t_apresentando||s!=static_cast<IDXGISwapChain*>(g.sc))return g.orig_present(s,sync,flags);
  if(flags&DXGI_PRESENT_TEST){++g.e.presents_teste;return S_OK;}
  std::lock_guard<std::recursive_mutex> t(g_trava);
  antes_do_present();
  ++t_apresentando;const HRESULT r=g.orig_present(s,sync,flags);--t_apresentando;
  return r;
}
HRESULT STDMETHODCALLTYPE meu_present1(IDXGISwapChain1* s,UINT sync,UINT flags,const DXGI_PRESENT_PARAMETERS* p){
  if(t_apresentando||s!=static_cast<IDXGISwapChain1*>(g.sc))return g.orig_present1(s,sync,flags,p);
  if(flags&DXGI_PRESENT_TEST){++g.e.presents_teste;return S_OK;}
  std::lock_guard<std::recursive_mutex> t(g_trava);
  antes_do_present();
  ++t_apresentando;const HRESULT r=g.orig_present1(s,sync,flags,p);--t_apresentando;
  return r;
}
void enganchar_swapchain(IDXGISwapChain4* sc) noexcept{
  void** vt=*reinterpret_cast<void***>(sc);
  if(vt==g.vt_sc)return;
  g.orig_release=reinterpret_cast<PfnRelease>(vt[kSlotRelease]);
  g.orig_present=reinterpret_cast<PfnPresent>(vt[kSlotPresent]);
  g.orig_present1=reinterpret_cast<PfnPresent1>(vt[kSlotPresent1]);
  const bool ok=gravar_ptr(&vt[kSlotRelease],reinterpret_cast<void*>(&meu_release))&&gravar_ptr(&vt[kSlotPresent],reinterpret_cast<void*>(&meu_present))&&
                gravar_ptr(&vt[kSlotPresent1],reinterpret_cast<void*>(&meu_present1));
  g.vt_sc=vt;
  log("Release/Present/Present1 da swapchain de FG enganchados (vtable %p): %s",static_cast<void*>(vt),ok?"ok":"FALHOU");
}
void adotar(ffxContext ctx,IDXGISwapChain4* sc,const char* via) noexcept{
  destruir_fg();
  g.sc=sc;g.ctx_sc=ctx;++g.e.swapchains_criadas;g.e.swapchain=true;g.e.swapchain_ptr=sc;g.ritmo={};
  g.falha_criar=false;
  enganchar_swapchain(sc);
  publicar();
  DXGI_SWAP_CHAIN_DESC1 d{};sc->GetDesc1(&d);
  char t[200];std::snprintf(t,sizeof t,"swapchain de FG %p criada por %s: %ux%u, formato DXGI %u, %u buffers, efeito %u (FG desligado ate o menu ligar)",
    static_cast<void*>(sc),via,d.Width,d.Height,static_cast<unsigned>(d.Format),d.BufferCount,static_cast<unsigned>(d.SwapEffect));
  log("FG: %s",t);
}
bool fila_d3d12(IUnknown* dev,ID3D12CommandQueue** fila) noexcept{
  *fila=nullptr;return dev&&SUCCEEDED(dev->QueryInterface(IID_PPV_ARGS(fila)))&&*fila;
}
template<class Desc>ffxReturnCode_t criar_sc(Desc& d,ffxContext& ctx) noexcept{
  ffxCreateContextDescFrameGenerationSwapChainVersionDX12 v{};
  v.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_VERSION_DX12;v.version=FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION;
  d.header.pNext=&v.header;
  ++t_criando;const ffxReturnCode_t r=g.ffx.create(&ctx,&d.header,nullptr);--t_criando;
  return r;
}
HRESULT STDMETHODCALLTYPE meu_create_swapchain(IDXGIFactory* f,IUnknown* dev,DXGI_SWAP_CHAIN_DESC* desc,IDXGISwapChain** out){
  if(t_criando||!desc||!out)return g.orig_cs(f,dev,desc,out);
  ID3D12CommandQueue* fila=nullptr;if(!fila_d3d12(dev,&fila))return g.orig_cs(f,dev,desc,out);
  std::lock_guard<std::recursive_mutex> t(g_trava);
  if(!carregar_ffx()){fila->Release();return g.orig_cs(f,dev,desc,out);}
  IDXGISwapChain4* sc=nullptr;ffxContext ctx=nullptr;
  ffxCreateContextDescFrameGenerationSwapChainNewDX12 d{};
  d.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_NEW_DX12;d.swapchain=&sc;d.desc=desc;d.dxgiFactory=f;d.gameQueue=fila;
  const ffxReturnCode_t r=criar_sc(d,ctx);fila->Release();
  if(r!=FFX_API_RETURN_OK||!sc){
    char m[160];std::snprintf(m,sizeof m,"swapchain de FG (CreateSwapChain %ux%u fmt %u) devolveu %u: swapchain normal, sem FG",desc->BufferDesc.Width,
      desc->BufferDesc.Height,static_cast<unsigned>(desc->BufferDesc.Format),r);falha(m);
    return g.orig_cs(f,dev,desc,out);
  }
  adotar(ctx,sc,"CreateSwapChain");*out=sc;return S_OK;
}
HRESULT STDMETHODCALLTYPE meu_create_swapchain_hwnd(IDXGIFactory2* f,IUnknown* dev,HWND hwnd,const DXGI_SWAP_CHAIN_DESC1* desc,
                                                    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* tela,IDXGIOutput* saida,IDXGISwapChain1** out){
  if(t_criando||!desc||!out||saida)return g.orig_csh(f,dev,hwnd,desc,tela,saida,out);
  ID3D12CommandQueue* fila=nullptr;if(!fila_d3d12(dev,&fila))return g.orig_csh(f,dev,hwnd,desc,tela,saida,out);
  std::lock_guard<std::recursive_mutex> t(g_trava);
  if(!carregar_ffx()){fila->Release();return g.orig_csh(f,dev,hwnd,desc,tela,saida,out);}
  IDXGISwapChain4* sc=nullptr;ffxContext ctx=nullptr;
  DXGI_SWAP_CHAIN_DESC1 d1=*desc;DXGI_SWAP_CHAIN_FULLSCREEN_DESC tela_copia{};if(tela)tela_copia=*tela;
  ffxCreateContextDescFrameGenerationSwapChainForHwndDX12 d{};
  d.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12;d.swapchain=&sc;d.hwnd=hwnd;d.desc=&d1;
  d.fullscreenDesc=tela?&tela_copia:nullptr;d.dxgiFactory=f;d.gameQueue=fila;
  const ffxReturnCode_t r=criar_sc(d,ctx);fila->Release();
  if(r!=FFX_API_RETURN_OK||!sc){
    char m[160];std::snprintf(m,sizeof m,"swapchain de FG (CreateSwapChainForHwnd %ux%u fmt %u) devolveu %u: swapchain normal, sem FG",desc->Width,desc->Height,
      static_cast<unsigned>(desc->Format),r);falha(m);
    return g.orig_csh(f,dev,hwnd,desc,tela,saida,out);
  }
  adotar(ctx,sc,"CreateSwapChainForHwnd");*out=sc;return S_OK;
}

bool criar_fg(ID3D12Device* device,const nrffx::fsr2fg::Chave& k,std::uint32_t flags_upscale,DXGI_FORMAT formato) noexcept{
  ffxCreateBackendDX12Desc be{};be.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;be.device=device;
  ffxCreateContextDescFrameGenerationVersion ver{};ver.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_VERSION;ver.version=FFX_FRAMEGENERATION_VERSION;
  ffxCreateContextDescFrameGeneration fd{};
  nrffx::fsr2fg::criar_fg(flags_upscale,k.tela_w,k.tela_h,k.render_w,k.render_h,ffxApiGetSurfaceFormatDX12(formato),fd);
  fd.header.pNext=&be.header;be.header.pNext=&ver.header;
  ffxContext ctx=nullptr;
  const ffxReturnCode_t r=g.ffx.create(&ctx,&fd.header,nullptr);
  if(r!=FFX_API_RETURN_OK||!ctx){
    ++g.e.erros;g.falha_criar=true;g.chave_falha=k;
    if(!g.aviso_criar){g.aviso_criar=true;char m[200];std::snprintf(m,sizeof m,"ffxCreateContext(FRAMEGENERATION) devolveu %u (tela %ux%u, teto %ux%u, formato DXGI %u, flags 0x%X): sem FG",
      r,k.tela_w,k.tela_h,fd.maxRenderSize.width,fd.maxRenderSize.height,static_cast<unsigned>(formato),fd.flags);falha(m);}
    return false;
  }
  g.fg=ctx;device->AddRef();g.fg_device=device;
  g.chave=k;g.chave.render_w=fd.maxRenderSize.width;g.chave.render_h=fd.maxRenderSize.height;
  g.reset_proximo=true;g.falha_criar=false;g.e.contexto=true;
  ffxQueryGetProviderVersion pv{};pv.header.type=FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;
  const bool ok=g.ffx.query(&g.fg,&pv.header)==FFX_API_RETURN_OK&&pv.versionName;
  std::snprintf(g.e.provedor,sizeof g.e.provedor,"%s",ok?pv.versionName:"?");
  char t[220];std::snprintf(t,sizeof t,"contexto de FG %p: provedor %s, tela %ux%u, teto %ux%u, formato DXGI %u (FFX %u), flags 0x%X",static_cast<void*>(ctx),
    g.e.provedor,k.tela_w,k.tela_h,fd.maxRenderSize.width,fd.maxRenderSize.height,static_cast<unsigned>(formato),fd.backBufferFormat,fd.flags);
  log("FG: %s",t);
  return true;
}
void aplicar_pedido() noexcept{
  if(!g.pub)return;
  const long p=InterlockedExchange(&g.pub->pedido,-1);
  const bool novo=nrffx::fsr2fg::aplicar_pedido(p,g.e.usuario);
  if(novo!=g.e.usuario)log("pedido do menu: %s",novo?"ligar":"desligar");
  g.e.usuario=novo;
}
}

bool instalar() noexcept{
  if(g_instalado.load(std::memory_order_acquire))return true;
  if(g_tentou.load(std::memory_order_acquire)||f2f::ini().geracao_quadros!=1)return false;
  try{
    std::lock_guard<std::recursive_mutex> t(g_trava);
    if(g_instalado.load(std::memory_order_acquire))return true;
    if(g_tentou.exchange(true))return false;

    wchar_t nome[64];swprintf(nome,64,L"Local\\NR_FG_%lu",GetCurrentProcessId());
    HANDLE h=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(nrfgext::Fg),nome);
    if(h&&GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(h);falha("a memoria Local\\NR_FG_<pid> ja existe (FG da libxess no processo): o FG do FSR 2 fica fora");return false;}
    auto* pub=h?static_cast<nrfgext::Fg*>(MapViewOfFile(h,FILE_MAP_ALL_ACCESS,0,0,sizeof(nrfgext::Fg))):nullptr;
    if(!pub){if(h)CloseHandle(h);falha("sem a memoria Local\\NR_FG_<pid> do menu: sem FG");return false;}

    HMODULE dxgi=GetModuleHandleW(L"dxgi.dll");if(!dxgi)dxgi=LoadLibraryW(L"dxgi.dll");
    using PfnCriarFabrica=HRESULT(WINAPI*)(REFIID,void**);
    auto criar=dxgi?reinterpret_cast<PfnCriarFabrica>(GetProcAddress(dxgi,"CreateDXGIFactory1")):nullptr;
    IDXGIFactory2* f=nullptr;
    if(!criar||FAILED(criar(IID_PPV_ARGS(&f)))||!f){UnmapViewOfFile(pub);CloseHandle(h);falha("sem IDXGIFactory2: sem FG");return false;}
    void** vt=*reinterpret_cast<void***>(f);
    g.orig_cs=reinterpret_cast<PfnCreateSwapChain>(vt[kSlotCreateSwapChain]);
    g.orig_csh=reinterpret_cast<PfnCreateSwapChainForHwnd>(vt[kSlotCreateSwapChainForHwnd]);
    const bool ok=gravar_ptr(&vt[kSlotCreateSwapChain],reinterpret_cast<void*>(&meu_create_swapchain))&&
                  gravar_ptr(&vt[kSlotCreateSwapChainForHwnd],reinterpret_cast<void*>(&meu_create_swapchain_hwnd));
    f->Release();
    if(!ok){UnmapViewOfFile(pub);CloseHandle(h);falha("VirtualProtect na vtable da fabrica DXGI falhou: sem FG");return false;}
    g.vt_fabrica=vt;g.e.gancho_fabrica=true;
    g.mapa=h;g.pub=pub;g.pub->pedido=-1;g.pub->versao=1u;publicar();MemoryBarrier();g.pub->magia=nrfgext::kMagia;g.e.memoria=true;
    char tx[160];std::snprintf(tx,sizeof tx,"GeracaoQuadros=1: gancho na fabrica DXGI (vtable %p) e memoria do menu; a swapchain criada daqui em diante vira a de FG",
      static_cast<void*>(vt));
    log("FG: %s",tx);
    g_instalado.store(true,std::memory_order_release);
    return true;
  }catch(...){return false;}
}
void apos_upscale(ID3D12Device* device,const ffxDispatchDescUpscale& up,std::uint32_t flags_upscale) noexcept{
  if(!g_instalado.load(std::memory_order_acquire))return;
  try{
    const Espera esp=ordem_esperar();
    std::lock_guard<std::recursive_mutex> t(g_trava);
    if(esp.esperou&&esp.estourou){
      if(++g_estouros_seguidos>=kOrdemEstourosMax&&!g_ordem_off.exchange(true))
        log("AVISO FgOrdem: %d upscales seguidos sem o Present do quadro anterior em %d ms: a espera sai nesta sessao",kOrdemEstourosMax,f2f::ini().fg_ordem_ms);
    }else if(esp.esperou)g_estouros_seguidos=0;
    nrffx::fsr2fg::ao_upscale(g.ritmo);
    const bool primeiro=!g.upscale_visto;g.upscale_visto=true;
    if(!g.aviso_removido&&device){
      const HRESULT rr=device->GetDeviceRemovedReason();
      if(FAILED(rr)){g.aviso_removido=true;log("ERRO device removido 0x%08lX (FG %s, provedor %s, frameID %llu, geracoes %llu, prepares %llu, Presents %llu)",
        static_cast<unsigned long>(rr),g.e.fg_ligado?"ligado":"desligado",g.e.provedor,static_cast<unsigned long long>(g.e.frame_id),
        static_cast<unsigned long long>(g.e.geracoes),static_cast<unsigned long long>(g.e.prepares),static_cast<unsigned long long>(g.e.presents));}
    }
    aplicar_pedido();
    if(!g.aviso_slot&&(g.vt_fabrica[kSlotCreateSwapChain]!=reinterpret_cast<void*>(&meu_create_swapchain)||
                       g.vt_fabrica[kSlotCreateSwapChainForHwnd]!=reinterpret_cast<void*>(&meu_create_swapchain_hwnd))){
      g.aviso_slot=true;g.e.gancho_fabrica=false;log("AVISO os slots da fabrica DXGI nao sao mais nossos: outra swapchain criada agora fica sem FG");
    }
    if(!g.sc){
      if(!g.aviso_sem_sc){g.aviso_sem_sc=true;log("upscale traduzido sem swapchain de FG (criada antes do gancho, ou a criacao falhou): sem FG ate o jogo recriar a swapchain");}
      publicar();return;
    }
    if(primeiro)log("1o upscale traduzido com a swapchain de FG viva: FG pronto para o menu ligar");
    if(!g.e.usuario){if(g.e.fg_ligado)configurar(false);publicar();return;}
    if(!carregar_ffx()||!device){publicar();return;}
    DXGI_SWAP_CHAIN_DESC1 d{};if(FAILED(g.sc->GetDesc1(&d))||!d.Width||!d.Height){publicar();return;}

    const nrffx::fsr2fg::Chave k{device,d.Width,d.Height,up.renderSize.width,up.renderSize.height,nrffx::fsr2fg::flags_fg(flags_upscale),
                                 static_cast<std::uint32_t>(d.Format)};
    if(g.fg&&nrffx::fsr2fg::precisa_recriar(g.chave,k)){
      log("recriando o contexto de FG (tela %ux%u -> %ux%u, render %ux%u, flags 0x%X -> 0x%X, formato DXGI %u -> %u)",g.chave.tela_w,g.chave.tela_h,
          k.tela_w,k.tela_h,k.render_w,k.render_h,g.chave.flags,k.flags,g.chave.formato,k.formato);
      destruir_fg();
    }
    if(!g.fg&&g.falha_criar&&nrffx::fsr2fg::precisa_recriar(g.chave_falha,k))g.falha_criar=false;
    if(!g.fg&&(g.falha_criar||!criar_fg(device,k,flags_upscale,d.Format))){publicar();return;}
    configurar(true);
    ffxDispatchDescFrameGenerationPrepareV2 p{};
    if(!nrffx::fsr2fg::preparar(up,g.e.frame_id,g.reset_proximo,p)){publicar();return;}
    const ffxReturnCode_t r=g.ffx.dispatch(&g.fg,&p.header);
    ++g.e.prepares;
    if(r!=FFX_API_RETURN_OK){if(g.e.erros++<16)log("ERRO prepare devolveu %u (frameID %llu)",r,static_cast<unsigned long long>(p.frameID));publicar();return;}
    ordem_marcar(true);
    if(g.e.prepares<=3)log("prepare %llu: frameID %llu, render %ux%u, jitter %.4f %.4f, MV x%g %g, delta %.2f ms, reset %d, camera %g/%g/%.3f",
      static_cast<unsigned long long>(g.e.prepares),static_cast<unsigned long long>(p.frameID),p.renderSize.width,p.renderSize.height,
      double(p.jitterOffset.x),double(p.jitterOffset.y),double(p.motionVectorScale.x),double(p.motionVectorScale.y),double(p.frameTimeDelta),
      p.reset?1:0,double(p.cameraNear),double(p.cameraFar),double(p.cameraFovAngleVertical));
    if(g.e.prepares%3000==0)log("%llu prepares, %llu geracoes, %llu Presents (%llu de teste), desligado %llu vezes, %llu erros",
      static_cast<unsigned long long>(g.e.prepares),static_cast<unsigned long long>(g.e.geracoes),static_cast<unsigned long long>(g.e.presents),
      static_cast<unsigned long long>(g.e.presents_teste),static_cast<unsigned long long>(g.e.desligados),static_cast<unsigned long long>(g.e.erros));
    g.reset_proximo=false;
    publicar();
  }catch(...){}
}
void soltar_contexto() noexcept{
  try{std::lock_guard<std::recursive_mutex> t(g_trava);destruir_fg();publicar();}catch(...){}
}
Estado estado() noexcept{
  std::lock_guard<std::recursive_mutex> t(g_trava);
  Estado e=g.e;e.orig_csh=reinterpret_cast<void*>(g.orig_csh);
  return e;
}
}
