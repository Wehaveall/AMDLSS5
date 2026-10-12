#pragma once
#include <cstdint>

namespace nrreuso {
inline int janela_fronteira(int corte) noexcept {return corte==4?2:corte==3?1:corte==2?0:-1;}
enum class Acao {lancar,lancar_e_pular_depois,pular,restaurar_e_lancar,guardar_e_lancar,abortar_e_lancar};
struct Maquina {
 unsigned n{};
 unsigned pausa_ms{250};
 bool pulando{};bool entre{};
 bool cache_ok{};
 unsigned desde_cheio{};
 unsigned cheios_pendentes{};
 unsigned geracao{};
 std::uint64_t ultimo_ms{};bool tem_ultimo{};
 unsigned job{};unsigned jobs_reuso{};unsigned long long pulados{};unsigned abortos{};unsigned refrescos{};unsigned pausas{};
 unsigned forcados{};

 Acao antes(bool fronteira,unsigned modo,bool pulavel,unsigned refresco=0,std::uint64_t agora_ms=0,bool forcar_cheio=false,unsigned n_max=0) noexcept {
  if(!n)return Acao::lancar;
  if(pulando){
   if(fronteira&&modo==8){pulando=false;entre=false;return Acao::restaurar_e_lancar;}
   if(pulavel){++pulados;return Acao::pular;}

   pulando=false;entre=false;cache_ok=false;++abortos;return Acao::abortar_e_lancar;
  }
  if(fronteira&&modo==4){
   ++job;entre=true;
   if(refresco!=geracao){geracao=refresco;cheios_pendentes=2;++refrescos;}
   if(pausa_ms&&tem_ultimo&&agora_ms-ultimo_ms>pausa_ms){cache_ok=false;++pausas;}
   ultimo_ms=agora_ms;tem_ultimo=true;
   if(cheios_pendentes){--cheios_pendentes;cache_ok=false;}

   const unsigned n_ef=(n_max&&n_max<n)?n_max:n;
   const bool pode=cache_ok&&n>=2&&desde_cheio+1<n;
   const bool reuso=pode&&n_ef>=2&&desde_cheio+1<n_ef&&!forcar_cheio;
   if(pode&&!reuso)++forcados;
   if(reuso){++desde_cheio;++jobs_reuso;return Acao::lancar_e_pular_depois;}
   desde_cheio=0;
   return Acao::lancar;
  }
  if(fronteira&&modo==8){entre=false;return Acao::guardar_e_lancar;}
  return Acao::lancar;
 }

 void comecar_pulo() noexcept {pulando=true;}
};

inline std::uint64_t fnv(const unsigned char* p,std::uint64_t n) noexcept {
 std::uint64_t h=1469598103934665603ull;for(std::uint64_t i=0;i<n;++i){h^=p[i];h*=1099511628211ull;}return h;
}
}
