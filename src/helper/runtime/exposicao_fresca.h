#pragma once
#include "nr_publico.h"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
namespace nrexpofresca {
inline std::atomic<bool> fresca{false};
inline std::atomic<bool> historico{false};
inline bool valida(float e) noexcept {return std::isfinite(e)&&e>0.f&&e<1e6f;}

inline float razao(float atual,float anterior) noexcept {
 if(!valida(atual)||!valida(anterior))return 1.f;
 const float r=atual/anterior;
 return std::fabs(r-1.f)<1e-6f?1.f:r;
}

constexpr unsigned slots=64,passo=512,etiqueta_em=256;

inline std::atomic<bool(*)(unsigned,float&,std::uint32_t&)> ler_slot{nullptr};

inline std::atomic<std::uint32_t> etiqueta_pendente{0};
struct Carimbo {std::atomic<int> job{0};std::atomic<std::uint32_t> etiqueta{0};};
inline Carimbo carimbos[16];
inline void carimbar(int job) noexcept {
 if(job<=0)return;auto& c=carimbos[unsigned(job)&15u];
 c.job.store(0,std::memory_order_relaxed);
 c.etiqueta.store(etiqueta_pendente.exchange(0,std::memory_order_acq_rel),std::memory_order_relaxed);
 c.job.store(job,std::memory_order_release);
}
inline std::uint32_t etiqueta_do_job(int job) noexcept {
 if(job<=0)return 0;const auto& c=carimbos[unsigned(job)&15u];
 if(c.job.load(std::memory_order_acquire)!=job)return 0;
 const auto e=c.etiqueta.load(std::memory_order_relaxed);
 return c.job.load(std::memory_order_acquire)==job?e:0;
}

enum Motivo:int {Fresca=0,SemPedido,SemCarimbo,SemLeitor,EtiquetaDiferente,ValorInvalido,VelhaInvalida,MotivoN};
inline const char* nome(int m) noexcept {
 static const char* n[]{"fresca","sem_pedido","sem_carimbo","sem_leitor","etiqueta_diferente","valor_invalido","velha_invalida"};
 return m>=0&&m<MotivoN?n[m]:"?";
}
struct Decisao {float usar;int motivo;};
inline Decisao decidir(bool pedido,std::uint32_t esperada,bool leu,std::uint32_t lida,float valor,float velha) noexcept {
 if(!pedido)return {velha,SemPedido};
 if(!valida(velha))return {velha,VelhaInvalida};
 if(!esperada)return {velha,SemCarimbo};
 if(!leu)return {velha,SemLeitor};
 if(lida!=esperada)return {velha,EtiquetaDiferente};
 if(!valida(valor))return {velha,ValorInvalido};
 return {valor,Fresca};
}

struct Uso {std::atomic<int> job{0};std::atomic<float> usada{0.f},velha{0.f};};
inline Uso usos[16];
inline void registrar_uso(int job,float usada,float velha) noexcept {
 if(job<=0)return;auto& u=usos[unsigned(job)&15u];
 u.job.store(0,std::memory_order_relaxed);u.usada.store(usada,std::memory_order_relaxed);u.velha.store(velha,std::memory_order_relaxed);
 u.job.store(job,std::memory_order_release);
}
inline bool uso_do_job(int job,float& usada,float& velha) noexcept {
 if(job<=0)return false;const auto& u=usos[unsigned(job)&15u];
 if(u.job.load(std::memory_order_acquire)!=job)return false;
 usada=u.usada.load(std::memory_order_relaxed);velha=u.velha.load(std::memory_order_relaxed);
 return u.job.load(std::memory_order_acquire)==job;
}

inline float para_compor(int job,float do_proxy) noexcept {
 if(!fresca.load(std::memory_order_relaxed))return do_proxy;
 float usada{},velha{};if(!uso_do_job(job,usada,velha)||!valida(usada))return do_proxy;
 return usada;
}

struct Job {int job{0};int motivo{SemPedido};float velha{0.f},usada{0.f},razao_proxy{1.f},razao_entrada{1.f};bool reprojetado{false};};
inline thread_local Job atual{};
inline float velha_anterior=0.f,usada_anterior=0.f;inline int job_anterior=0;

inline std::atomic<std::uint64_t> contagem[MotivoN]{};
inline std::atomic<std::uint64_t> trocas_import{0},trocas_razao{0},trocas_compor{0},importes{0},reprojecoes{0};
inline std::atomic<unsigned> linhas{0};
inline void log(const char* s) noexcept {
 FILE* f{};if(fopen_s(&f,"nr_expofresca.log","a")==0&&f){std::fprintf(f,"%s\n",s);std::fclose(f);}
}
inline bool ligado() noexcept {return fresca.load(std::memory_order_relaxed)||historico.load(std::memory_order_relaxed);}

inline void no_eval(int job,const void* metadado) noexcept {
 if(!ligado()||job<=0||!metadado){atual=Job{};return;}
 if(job==atual.job)return;
 float velha{};std::memcpy(&velha,static_cast<const char*>(metadado)+0x3C,4);
 const bool pedido=fresca.load(std::memory_order_relaxed);
 const std::uint32_t esperada=pedido?etiqueta_do_job(job):0;
 float valor=0.f;std::uint32_t lida=0;bool leu=false;
 if(esperada){if(auto ler=ler_slot.load(std::memory_order_acquire))leu=ler(esperada%slots,valor,lida);}
 const auto d=decidir(pedido,esperada,leu,lida,valor,velha);
 contagem[d.motivo].fetch_add(1,std::memory_order_relaxed);
 Job j{};j.job=job;j.motivo=d.motivo;j.velha=velha;j.usada=d.usar;
 const bool seguido=job_anterior>0&&job==job_anterior+1;
 j.razao_proxy=razao(velha,velha_anterior);j.razao_entrada=seguido?razao(d.usar,usada_anterior):j.razao_proxy;
 atual=j;velha_anterior=velha;usada_anterior=d.usar;job_anterior=job;
 registrar_uso(job,d.usar,velha);
 const unsigned n=linhas.fetch_add(1,std::memory_order_relaxed);
 if(n<64||(job%256)==0){char l[300];
  NRLOGF(l,sizeof l,"EVAL job=%d motivo=%s velha=%.6g lida=%.6g usada=%.6g dif=%+.3f%% razao_proxy=%.6g razao_entrada=%.6g etiqueta=%u/%u frescas=%llu recusas=%llu",
   job,nome(d.motivo),double(velha),double(valor),double(d.usar),valida(velha)&&valida(d.usar)?100.0*(double(d.usar)/double(velha)-1.0):0.0,
   double(j.razao_proxy),double(j.razao_entrada),lida,esperada,static_cast<unsigned long long>(contagem[Fresca].load()),
   static_cast<unsigned long long>(contagem[SemCarimbo].load()+contagem[SemLeitor].load()+contagem[EtiquetaDiferente].load()+contagem[ValorInvalido].load()));
  log(l);}
}

inline void no_import(void* params) noexcept {
 if(!params||!fresca.load(std::memory_order_relaxed)||atual.motivo!=Fresca)return;
 float e{};std::memcpy(&e,static_cast<char*>(params)+0x2C,4);
 const auto n=importes.fetch_add(1,std::memory_order_relaxed);
 if(e!=atual.velha){
  if(n<16){char l[128];NRLOGF(l,sizeof l,"IMPORT job=%d exposicao=%.6g != velha=%.6g: sem troca",atual.job,double(e),double(atual.velha));log(l);}
  return;}
 if(e!=atual.usada){std::memcpy(static_cast<char*>(params)+0x2C,&atual.usada,4);trocas_import.fetch_add(1,std::memory_order_relaxed);}
}

inline void no_reproject(void* params) noexcept {
 if(!params||!historico.load(std::memory_order_relaxed)||atual.job<=0)return;
 reprojecoes.fetch_add(1,std::memory_order_relaxed);

 if(atual.reprojetado)return;atual.reprojetado=true;
 float r{};std::memcpy(&r,static_cast<char*>(params)+0x78,4);
 if(std::fabs(r-atual.razao_proxy)>1e-6f*std::fmax(1.f,std::fabs(r)))return;
 if(r!=atual.razao_entrada){std::memcpy(static_cast<char*>(params)+0x78,&atual.razao_entrada,4);trocas_razao.fetch_add(1,std::memory_order_relaxed);}
}
}
namespace nrsementefixa {
inline std::atomic<bool> ativa{false};
inline std::atomic<std::uint64_t> seguradas{0};

inline bool aplicar(void* params) noexcept {
 if(!ativa.load(std::memory_order_relaxed)||!params)return false;
 unsigned modo{};std::memcpy(&modo,static_cast<char*>(params)+0x28,4);
 if(modo!=20)return false;
 unsigned semente{};std::memcpy(&semente,static_cast<char*>(params)+0x68,4);
 if(semente==0)return false;
 const unsigned zero=0;std::memcpy(static_cast<char*>(params)+0x68,&zero,4);
 const auto n=seguradas.fetch_add(1,std::memory_order_relaxed)+1;
 if((n&(n-1))==0){char l[96];NRLOGF(l,sizeof l,"SEMENTE n=%llu semente_do_proxy=0x%08X -> 0",static_cast<unsigned long long>(n),semente);nrexpofresca::log(l);}
 return true;
}
}
