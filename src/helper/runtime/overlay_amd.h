#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <cwchar>
namespace nroverlayamd {
struct Sinais {
 int reg_enable=-1;
 int reg_estado=-1;
 bool presentmon=false;
 bool janela=false;
 bool ihk=false;
 bool game_manager=false;
 int show_rs=-1;
 int dvr=-1;
 int desk_rec=-1;
 int camera=-1;
 int replay=-1;
};
inline bool ativo(const Sinais& s) noexcept { return s.reg_enable>0||s.reg_estado>0||s.presentmon; }
inline bool risco_dvr(const Sinais& s) noexcept { return s.dvr>0||s.desk_rec>0||s.camera>0||s.replay>0; }
inline bool risco(const Sinais& s,bool dvr_conta) noexcept { return ativo(s)||(dvr_conta&&risco_dvr(s)); }

inline const wchar_t* nome_de(const wchar_t* caminho) noexcept {
 if(!caminho)return L"";
 const wchar_t* n=caminho;for(const wchar_t* p=caminho;*p;++p)if(*p==L'\\'||*p==L'/')n=p+1;
 return n;
}
inline bool igual_ci(const wchar_t* a,const wchar_t* b) noexcept {
 if(!a||!b)return false;
 for(;*a&&*b;++a,++b){wchar_t x=*a,y=*b;if(x>=L'A'&&x<=L'Z')x=wchar_t(x-L'A'+L'a');if(y>=L'A'&&y<=L'Z')y=wchar_t(y-L'A'+L'a');if(x!=y)return false;}
 return *a==*b;
}
inline bool contem_ci(const wchar_t* s,const wchar_t* sub) noexcept {
 if(!s||!sub)return false;const std::size_t n=std::wcslen(sub);if(!n)return true;
 for(;*s;++s){std::size_t i=0;for(;i<n&&s[i];++i){wchar_t x=s[i],y=sub[i];if(x>=L'A'&&x<=L'Z')x=wchar_t(x-L'A'+L'a');if(y>=L'A'&&y<=L'Z')y=wchar_t(y-L'A'+L'a');if(x!=y)break;}
  if(i==n)return true;}
 return false;
}

inline void classificar_modulos(const wchar_t* const* nomes,unsigned n,Sinais& s) noexcept {
 for(unsigned i=0;i<n;++i){const wchar_t* b=nome_de(nomes[i]);
  if(igual_ci(b,L"amdihk64.dll"))s.ihk=true;
  else if(igual_ci(b,L"GameManager64.dll")||igual_ci(b,L"detoured64.dll"))s.game_manager=true;}
}

inline bool presentmon_amd(const wchar_t* exe,const wchar_t* caminho) noexcept {
 if(!igual_ci(nome_de(exe),L"PresentMon-x64.exe"))return false;
 return !caminho||!*caminho||contem_ci(caminho,L"\\AMD\\");
}

inline bool janela_overlay(const wchar_t* classe,bool visivel,int w,int h) noexcept {
 return visivel&&igual_ci(classe,L"AMDDVROVERLAYWINDOWCLASS")&&w>=64&&h>=24;
}

enum class Modo : int { SoLog=0, Pausa=1, SoAviso=2 };
inline Modo modo_de(int v) noexcept { return v==0?Modo::SoLog:v==2?Modo::SoAviso:Modo::Pausa; }

struct Passo {
 bool escrever=false;std::uint8_t valor=0;
 bool detectou=false,saiu=false,pausou=false,retomou=false,forcou=false;
};
struct Guarda {
 bool detectado=false,pausou_rede=false,forcado=false;unsigned desligado_seguidos=0;
 static constexpr unsigned amostras_para_sair=3;
 Passo amostra(bool ativo_agora,std::uint8_t enabled,Modo modo) noexcept {
  Passo p{};
  if(ativo_agora){
   desligado_seguidos=0;
   if(!detectado){detectado=true;p.detectou=true;}
   if(modo==Modo::Pausa&&!forcado&&enabled!=0){
    if(pausou_rede){forcado=true;pausou_rede=false;p.forcou=true;}
    else{p.escrever=true;p.valor=0;pausou_rede=true;p.pausou=true;}
   }
   return p;
  }
  if(!detectado)return p;
  if(++desligado_seguidos<amostras_para_sair)return p;
  detectado=false;desligado_seguidos=0;p.saiu=true;
  if(pausou_rede&&enabled==0){p.escrever=true;p.valor=1;p.retomou=true;}
  pausou_rede=false;forcado=false;
  return p;
 }

 int aviso(Modo modo) const noexcept {
  if(!detectado||modo==Modo::SoLog)return 0;
  return pausou_rede?1:2;
 }
};

struct Entrada {
 static constexpr unsigned n=8;
 const void* vistos[n]{};unsigned usados=0,prox=0;std::uint32_t w=0,h=0;bool tem=false;
 bool nota(const void* p,std::uint32_t largura,std::uint32_t altura) noexcept {
  if(!p)return false;
  if(!tem||largura!=w||altura!=h){const bool mudou=tem;tem=true;w=largura;h=altura;usados=0;prox=0;guardar(p);return mudou;}
  for(unsigned i=0;i<usados;++i)if(vistos[i]==p)return false;
  guardar(p);return true;
 }
 void guardar(const void* p) noexcept {vistos[prox]=p;prox=(prox+1)%n;if(usados<n)++usados;}
};

struct PausaRecria {
 bool pausou=false;std::uint64_t ate=0;std::uint32_t visto=0;bool tem_visto=false;
 static constexpr std::uint64_t espera_ms=2000;
 Passo amostra(bool armado,std::uint32_t recriacoes,std::uint8_t enabled,std::uint64_t agora) noexcept {
  Passo p{};
  const bool nova=tem_visto&&recriacoes!=visto;visto=recriacoes;tem_visto=true;
  if(pausou&&enabled!=0){pausou=false;p.forcou=true;return p;}
  if(armado&&nova){
   ate=agora+espera_ms;
   if(!pausou&&enabled!=0){pausou=true;p.escrever=true;p.valor=0;p.pausou=true;}
   return p;
  }
  if(pausou&&(!armado||agora>=ate)){pausou=false;if(enabled==0){p.escrever=true;p.valor=1;p.retomou=true;}}
  return p;
 }
};
inline std::atomic<bool> vigiar_entrada{false};
inline std::atomic<std::uint32_t> recriacoes{0};

inline void nota_entrada(const void* entrada,std::uint32_t w,std::uint32_t h) noexcept {
 static Entrada e;if(e.nota(entrada,w,h))recriacoes.fetch_add(1,std::memory_order_relaxed);
}

inline std::atomic<int> estado_aviso{0};
inline std::atomic<bool> pausa_recriacao{false};
inline std::atomic<std::uint64_t> aviso_ate_ms{0};
constexpr std::uint64_t aviso_tela_ms=20000;

void tick(void* proxy_base,const void* ini_anchor) noexcept;

int linha_aviso(std::uint64_t agora_ms,char* linha,unsigned cap) noexcept;
}
