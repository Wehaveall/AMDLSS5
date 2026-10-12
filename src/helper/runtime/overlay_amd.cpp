#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "nr_publico.h"
#include "overlay_amd.h"
#include "native_ini_path.h"
#include "../source/implementation/compat-diag/compat.h"
#include <windows.h>
#include <tlhelp32.h>
#include <atomic>
#include <cstdio>
#include <thread>

namespace nroverlayamd {
namespace {

std::atomic<std::uint32_t> amostra_bits{0};
std::atomic<std::uint32_t> amostra_seq{0};
std::uint32_t tri(int v) noexcept {return v<0?3u:(v>0?1u:0u);}
int destri(std::uint32_t v) noexcept {return v==3u?-1:int(v);}
std::uint32_t empacotar(const Sinais& s) noexcept {
 return tri(s.reg_enable)|(tri(s.reg_estado)<<2)|(std::uint32_t(s.presentmon)<<4)|(std::uint32_t(s.janela)<<5)|
        (std::uint32_t(s.ihk)<<6)|(std::uint32_t(s.game_manager)<<7)|(tri(s.show_rs)<<8)|(tri(s.dvr)<<10)|
        (tri(s.desk_rec)<<12)|(tri(s.camera)<<14)|(tri(s.replay)<<16);
}
Sinais desempacotar(std::uint32_t b) noexcept {
 Sinais s{};s.reg_enable=destri(b&3u);s.reg_estado=destri((b>>2)&3u);s.presentmon=(b>>4)&1u;s.janela=(b>>5)&1u;s.ihk=(b>>6)&1u;
 s.game_manager=(b>>7)&1u;s.show_rs=destri((b>>8)&3u);s.dvr=destri((b>>10)&3u);
 s.desk_rec=destri((b>>12)&3u);s.camera=destri((b>>14)&3u);s.replay=destri((b>>16)&3u);
 return s;
}
int ler_dword(const wchar_t* chave,const wchar_t* valor,HKEY raiz=HKEY_CURRENT_USER) noexcept {
 DWORD v=0,n=sizeof v;
 return RegGetValueW(raiz,chave,valor,RRF_RT_REG_DWORD,nullptr,&v,&n)==ERROR_SUCCESS?int(v):-1;
}

int ler_bool_texto(const wchar_t* chave,const wchar_t* valor) noexcept {
 wchar_t t[16]{};DWORD n=sizeof t;
 if(RegGetValueW(HKEY_CURRENT_USER,chave,valor,RRF_RT_REG_SZ,nullptr,t,&n)!=ERROR_SUCCESS)return -1;
 return igual_ci(t,L"true")?1:igual_ci(t,L"false")?0:-1;
}
constexpr const wchar_t* perf=L"Software\\AMD\\CN\\Performance";
constexpr const wchar_t* dvr=L"Software\\AMD\\DVR";
bool procurar_presentmon() noexcept {
 HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snap==INVALID_HANDLE_VALUE)return false;
 PROCESSENTRY32W e{};e.dwSize=sizeof e;bool achou=false;
 for(BOOL ok=Process32FirstW(snap,&e);ok&&!achou;ok=Process32NextW(snap,&e)){
  if(!igual_ci(e.szExeFile,L"PresentMon-x64.exe"))continue;
  wchar_t caminho[MAX_PATH]{};DWORD n=MAX_PATH;
  if(HANDLE h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,e.th32ProcessID)){if(!QueryFullProcessImageNameW(h,0,caminho,&n))caminho[0]=0;CloseHandle(h);}
  achou=presentmon_amd(e.szExeFile,caminho);
 }
 CloseHandle(snap);return achou;
}
bool procurar_janela() noexcept {
 for(HWND w=FindWindowExW(nullptr,nullptr,L"AMDDVROVERLAYWINDOWCLASS",nullptr);w;w=FindWindowExW(nullptr,w,L"AMDDVROVERLAYWINDOWCLASS",nullptr)){
  RECT r{};if(!GetWindowRect(w,&r))continue;
  if(janela_overlay(L"AMDDVROVERLAYWINDOWCLASS",IsWindowVisible(w)!=FALSE,int(r.right-r.left),int(r.bottom-r.top)))return true;
 }
 return false;
}
Sinais sondar() noexcept {
 Sinais s{};
 s.reg_enable=ler_dword(perf,L"EnableMetricsOverlay");
 s.reg_estado=ler_dword(perf,L"MetricsOverlayState");
 s.show_rs=ler_bool_texto(dvr,L"ShowRSOverlay");
 s.dvr=ler_dword(dvr,L"DvrEnabled");
 s.desk_rec=ler_dword(dvr,L"DesktopRecordingEnabled");
 s.camera=ler_dword(dvr,L"CameraEnabled");
 {const int a=ler_dword(dvr,L"InstantReplayEnabled",HKEY_LOCAL_MACHINE),b=ler_dword(dvr,L"InGameReplayEnabled",HKEY_LOCAL_MACHINE);s.replay=a>b?a:b;}
 s.presentmon=procurar_presentmon();
 s.janela=procurar_janela();
 const wchar_t* nomes[]{GetModuleHandleW(L"amdihk64.dll")?L"amdihk64.dll":L"",GetModuleHandleW(L"GameManager64.dll")?L"GameManager64.dll":L"",
                        GetModuleHandleW(L"detoured64.dll")?L"detoured64.dll":L""};
 classificar_modulos(nomes,3,s);
 return s;
}
void iniciar_sonda() noexcept {
 try{
  std::thread([]{
   for(;;){
    const Sinais s=sondar();
    amostra_bits.store(empacotar(s),std::memory_order_relaxed);amostra_seq.fetch_add(1,std::memory_order_release);
    Sleep(1000);
   }
  }).detach();
 }catch(...){}
}
void descrever(const Sinais& s,int modo,const char* acao,char* out,unsigned cap) noexcept {
 NRLOGF(out,cap,"EnableMetricsOverlay=%d MetricsOverlayState=%d presentmon=%d janela=%d amdihk64=%d gamemanager=%d ShowRSOverlay=%d DvrEnabled=%d "
        "DesktopRecordingEnabled=%d CameraEnabled=%d Replay=%d ativo=%d risco_dvr=%d modo=%d acao=%s",
        s.reg_enable,s.reg_estado,int(s.presentmon),int(s.janela),int(s.ihk),int(s.game_manager),s.show_rs,s.dvr,
        s.desk_rec,s.camera,s.replay,int(ativo(s)),int(risco_dvr(s)),modo,acao);
}
int ler_ini(void* base,const void* anchor,const wchar_t* chave,int padrao) noexcept {
 int v=padrao;
 try{const auto ini=nrinipath::resolve(base,anchor);if(!ini.empty())v=int(NRCFG_INT(L"DlssNrOnAmd",chave,padrao,ini.c_str()));}catch(...){}
 return v;
}
}

void tick(void* base,const void* ini_anchor) noexcept {
 if(!base)return;
 static int modo_ini=-1;static bool recria_ini=true,dvr_ini=true;
 if(modo_ini<0){
  int v=ler_ini(base,ini_anchor,L"NeuralOverlayAmd",0);if(v<0||v>2)v=0;
 modo_ini=v;
  recria_ini=ler_ini(base,ini_anchor,L"NeuralOverlayAmdRecria",0)!=0;
  dvr_ini=ler_ini(base,ini_anchor,L"NeuralOverlayAmdRiscoDvr",0)!=0;
  iniciar_sonda();
 }
 const Modo modo=modo_de(modo_ini);
 auto& enabled=*reinterpret_cast<volatile std::uint8_t*>(static_cast<std::uint8_t*>(base)+0x94A8C);
 static Guarda guarda;static PausaRecria recria;static Sinais s{};static bool tem_amostra=false;
 char texto[400];
 static std::uint32_t visto=0;
 const std::uint32_t seq=amostra_seq.load(std::memory_order_acquire);
 if(seq!=visto){
  visto=seq;s=desempacotar(amostra_bits.load(std::memory_order_relaxed));
  const Passo p=guarda.amostra(ativo(s),enabled,modo);
  if(p.escrever)enabled=p.valor;

  if(!tem_amostra){tem_amostra=true;descrever(s,modo_ini,"inicio",texto,sizeof texto);nrcompat::report("overlay_amd_inicio",texto);}
  const char* acao=p.pausou?"rede_pausada":p.retomou?"rede_retomada":p.forcou?"usuario_forcou_f10":p.detectou?"detectado":p.saiu?"saiu":nullptr;
  if(acao){
   descrever(s,modo_ini,acao,texto,sizeof texto);
   nrcompat::report(p.saiu?"overlay_amd_saiu":p.forcou?"overlay_amd_forcado":"overlay_amd_detectado",texto);
  }
  if(p.detectou||p.pausou)aviso_ate_ms.store(GetTickCount64()+aviso_tela_ms,std::memory_order_relaxed);
  if(p.saiu||p.forcou)aviso_ate_ms.store(0,std::memory_order_relaxed);

  int e=guarda.aviso(modo);
  static bool dvr_visto=false;
  const bool so_dvr=e==0&&modo!=Modo::SoLog&&dvr_ini&&risco_dvr(s);
  if(so_dvr){e=3;if(!dvr_visto){dvr_visto=true;aviso_ate_ms.store(GetTickCount64()+aviso_tela_ms,std::memory_order_relaxed);
   descrever(s,modo_ini,"risco_dvr",texto,sizeof texto);nrcompat::report("overlay_amd_risco_dvr",texto);}}
  else if(!risco_dvr(s))dvr_visto=false;
  estado_aviso.store(e,std::memory_order_relaxed);
 }
 if(!tem_amostra)return;

 const bool armado=recria_ini&&modo!=Modo::SoLog&&risco(s,dvr_ini)&&!guarda.pausou_rede;
 vigiar_entrada.store(armado||recria.pausou,std::memory_order_relaxed);
 const Passo r=recria.amostra(armado,recriacoes.load(std::memory_order_relaxed),enabled,GetTickCount64());
 if(r.escrever)enabled=r.valor;
 pausa_recriacao.store(recria.pausou,std::memory_order_relaxed);
 if(r.pausou||r.retomou||r.forcou){
  NRLOGF(texto,sizeof texto,"acao=%s recriacoes=%u ativo=%d risco_dvr=%d modo=%d",r.pausou?"rede_pausada_recriacao":r.retomou?"rede_retomada_recriacao":"usuario_forcou_f10",
         recriacoes.load(std::memory_order_relaxed),int(ativo(s)),int(risco_dvr(s)),modo_ini);
  nrcompat::report("overlay_amd_recriacao",texto);
 }
}

int linha_aviso(std::uint64_t agora,char* linha,unsigned cap) noexcept {
 const int e=estado_aviso.load(std::memory_order_relaxed);
 if(!e||aviso_ate_ms.load(std::memory_order_relaxed)<=agora)return 0;
 if(e==1)NRLOGF(linha,cap,"OVERLAY DA AMD LIGADO: DLSS 5 PAUSADO. DESLIGUE COM CTRL+SHIFT+O");
 else if(e==2)NRLOGF(linha,cap,"OVERLAY DA AMD LIGADO: RISCO DE TRAVAR A GPU. DESLIGUE COM CTRL+SHIFT+O");
 else NRLOGF(linha,cap,"GRAVACAO/CAMERA/REPLAY DA AMD LIGADOS: PODE TRAVAR A GPU. DESLIGUE NO ADRENALIN");
 return 3;
}
}
