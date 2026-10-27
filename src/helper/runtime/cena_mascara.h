#pragma once

#include <algorithm>
#include <cmath>
#include <atomic>
#include <cstring>
namespace nrcena {
struct Config {
 int modo=0;
 float perto_k0=2.f,perto_k1=3.f;
 float forca=1.f;
 float t0=2.f,t1=4.f;
 float rel=.08f;
 float base_px=.3f;
 int raio=4;
 int borra=4;
 float decai=.85f;
 int passo=8;
 int iter=4;
 float c=.5f;
 float ganho=1.f;
 int escala=100;
 bool pele_ligada=false;float pele=0.f;

 float parado=0.f;

 int cada=1;
 int parado_max=0;
 int reusa=1;

 float guia=0.f;

 float cam_px=0.f;
 float cam_frac=.5f;

 int cam_reusa=0;

 int dilata_sep=1;
 int mascara_pulo=0;
 int crono=0;
 int menos_esperas=0;
 int copia_async=0;

 float ret_mov=0.f;
 int ret_tudo=0;
 float ret_idade=1.f;
 float ret_gama=0.f;
 float ret_gama_mov=0.f;
 float ret_escuro=0.f;
 int ret_raio=2;
 int alinha=0;

 bool retifica() const noexcept {return ret_mov>0.f||ret_gama>0.f||ret_escuro>0.f;}

 int fade=0;

 int fade_sobe=0;

 int protege=0;

 int protege_sessao=0;
 int protege_ini_s=20;
 float protege_frac=.002f;
 int fade_curva=0;

 int pre_carrega=0;
 int trava_ms=0;
 int trava_n=2;
 int trava_janela_s=120;

 int cont=0;
 int cont_escala=0;
 float cont_raio=8.f;
 float cont_forca=1.f;
 int cont_portao=0;
 float cont_mov=0.f;
 int cont_mascara=1;

 int sobe=0;
 float sobe_luma=0.f;
 float sobe_prof=.05f;
 float sobe_raio=1.f;
 bool fold_proprio() const noexcept {return modo>0||ganho!=1.f||escala<100;}
 bool pula() const noexcept {return parado>0.f||cada>1||cam_px>0.f;}
};

inline Config limitar(Config c) noexcept {
 c.modo=std::clamp(c.modo,0,2);c.perto_k0=std::clamp(c.perto_k0,1.f,50.f);c.perto_k1=std::clamp(c.perto_k1,c.perto_k0+.01f,60.f);c.forca=std::clamp(c.forca,0.f,1.f);
 c.t0=std::clamp(c.t0,0.f,100.f);c.t1=std::clamp(c.t1,c.t0,100.f);c.rel=std::clamp(c.rel,0.f,2.f);c.base_px=std::clamp(c.base_px,.01f,20.f);
 c.raio=std::clamp(c.raio,0,32);c.borra=std::clamp(c.borra,0,32);c.decai=std::clamp(c.decai,0.f,.99f);
 c.passo=std::clamp(c.passo,2,32);c.iter=std::clamp(c.iter,1,8);c.c=std::clamp(c.c,.05f,20.f);c.ganho=std::clamp(c.ganho,0.f,4.f);
 c.pele=std::clamp(c.pele,-1.f,2.f);c.escala=std::clamp(c.escala,25,100);
 c.parado=std::clamp(c.parado,0.f,1.f);c.cada=std::clamp(c.cada,1,16);c.parado_max=std::clamp(c.parado_max,0,64);
 c.reusa=std::clamp(c.reusa,0,1);c.guia=std::clamp(c.guia,0.f,1.f);
 c.cam_px=std::clamp(c.cam_px,0.f,64.f);c.cam_frac=std::clamp(c.cam_frac,0.f,1.f);c.cam_reusa=std::clamp(c.cam_reusa,0,1);
 c.crono=std::clamp(c.crono,0,2);c.copia_async=std::clamp(c.copia_async,0,1);c.menos_esperas=std::clamp(c.menos_esperas,0,1);c.dilata_sep=std::clamp(c.dilata_sep,0,1);c.mascara_pulo=std::clamp(c.mascara_pulo,0,1);
 c.ret_mov=std::clamp(c.ret_mov,0.f,256.f);c.ret_tudo=std::clamp(c.ret_tudo,0,1);c.ret_idade=std::clamp(c.ret_idade,0.f,8.f);
 c.ret_gama=std::clamp(c.ret_gama,0.f,16.f);c.ret_gama_mov=std::clamp(c.ret_gama_mov,0.f,256.f);c.ret_escuro=std::clamp(c.ret_escuro,0.f,1.f);
 c.ret_raio=std::clamp(c.ret_raio,1,3);
 c.alinha=std::clamp(c.alinha,0,64);
 c.fade=std::clamp(c.fade,0,64);
 c.protege=std::clamp(c.protege,0,1);c.protege_ini_s=std::clamp(c.protege_ini_s,0,600);c.protege_frac=std::clamp(c.protege_frac,0.f,1.f);
 c.fade_sobe=std::clamp(c.fade_sobe,0,600);c.fade_curva=std::clamp(c.fade_curva,0,1);
 c.pre_carrega=std::clamp(c.pre_carrega,0,1);c.trava_ms=c.trava_ms<=0?0:std::clamp(c.trava_ms,200,10000);
 c.trava_n=std::clamp(c.trava_n,1,8);c.trava_janela_s=std::clamp(c.trava_janela_s,1,3600);
 c.cont=std::clamp(c.cont,0,1);c.cont_escala=c.cont_escala<=0?0:std::clamp(c.cont_escala,25,100);c.cont_raio=std::clamp(c.cont_raio,1.f,64.f);
 c.cont_forca=std::clamp(c.cont_forca,0.f,1.f);c.cont_portao=std::clamp(c.cont_portao,0,1);c.cont_mov=std::clamp(c.cont_mov,0.f,256.f);c.cont_mascara=std::clamp(c.cont_mascara,1,16);
 c.sobe=std::clamp(c.sobe,0,1);c.sobe_luma=std::clamp(c.sobe_luma,0.f,4.f);c.sobe_prof=std::clamp(c.sobe_prof,0.f,4.f);c.sobe_raio=std::clamp(c.sobe_raio,1.f,2.f);
 return c;
}

inline bool calcula_extra(const Config& c,float frac,int sem,bool tem) noexcept {
 if(!c.pula()||!tem||(frac<0.f&&c.parado>0.f))return true;
 if(c.parado_max>0&&sem>=c.parado_max)return true;
 if(sem+1<c.cada)return false;
 return c.parado<=0.f||frac<=c.parado;
}

inline bool camera_anda(const Config& c,float frac_cam) noexcept {
 return c.cam_px>0.f&&frac_cam>=0.f&&frac_cam>c.cam_frac;
}

inline float idade_ret(const Config& c,int entre,unsigned long long desde_ancora) noexcept {
 const float d=desde_ancora>64ull?64.f:float(desde_ancora);
 return c.ret_idade*float(entre<1?1:entre)+d;
}

inline float passo_fade(float f,bool alvo,int k,int n) noexcept {
 if(n<=0)return alvo?1.f:0.f;
 const float s=float(k<1?1:k)/float(n);f=alvo?f+s:f-s;return f<0.f?0.f:(f>1.f?1.f:f);
}

inline float passo_fade2(float w,bool alvo,int k,int n_desce,int n_sobe,int curva) noexcept {
 if(!alvo)return passo_fade(w,false,k,n_desce);
 const int n=n_sobe>0?n_sobe:n_desce;
 if(n<=0)return 1.f;
 w=w<0.f?0.f:(w>1.f?1.f:w);
 const float s=float(k<1?1:k)/float(n);
 if(!curva)return w+s>1.f?1.f:w+s;
 float p=.5f-std::sin(std::asin(1.f-2.f*w)/3.f);
 p+=s;if(p>=1.f)return 1.f;
 return p*p*(3.f-2.f*p);
}

inline bool protege_job(const Config& c,double seg,float frac_zero) noexcept {
 if(!c.protege)return false;
 if(seg>=0.0&&seg<double(c.protege_ini_s))return true;
 return frac_zero>=0.f&&frac_zero<=c.protege_frac;
}

struct Trava {unsigned long long t[8]{};int n=0;bool desligado=false;};
inline bool trava_registra(Trava& g,const Config& c,double ms,unsigned long long agora_ms) noexcept {
 if(c.trava_ms<=0)return false;
 if(g.desligado)return true;
 if(!(ms>=double(c.trava_ms)))return false;
 const unsigned long long jan=(unsigned long long)c.trava_janela_s*1000ull;int k=0;
 for(int i=0;i<g.n;++i)if(agora_ms>=g.t[i]&&agora_ms-g.t[i]<jan)g.t[k++]=g.t[i];
 g.n=k;if(g.n<8)g.t[g.n++]=agora_ms;
 if(g.n>=c.trava_n)g.desligado=true;
 return g.desligado;
}

struct Bandas {float baixa=1.f,detalhe=1.f,portao_baixa=1.f;};
inline Bandas bandas(const Config& c,float v,bool reusa) noexcept {
 v=v<0.f?0.f:(v>1.f?1.f:v);Bandas b{};
 b.baixa=c.cont_forca+(1.f-c.cont_forca)*v;b.detalhe=v;
 b.portao_baixa=(reusa||c.cont_portao)?1.f:v;
 return b;
}

inline bool continuo(const Config& c) noexcept {return c.cont>0&&c.cam_px>0.f&&c.fade>0&&c.escala<100;}

inline int escala_job(const Config& c,bool andando,float v) noexcept {
 return (continuo(c)&&andando&&v<=0.f&&c.cont_escala>0)?c.cont_escala:c.escala;
}

inline int quadros_entre(unsigned long long agora,unsigned long long antes) noexcept {
 if(!agora||!antes||agora<=antes)return 2;
 const unsigned long long d=agora-antes;return d>8ull?8:int(d);
}

struct MedidaCamera {
 struct E{unsigned long long q=0;float lim=-1.f,f=-1.f;};E e[2];unsigned i=0;unsigned long long reusos=0;
 bool lembra(unsigned long long q,float lim,float& f) noexcept {if(!q)return false;for(const auto& x:e)if(x.q==q&&x.lim==lim){f=x.f;++reusos;return true;}return false;}
 void guarda(unsigned long long q,float lim,float f) noexcept {if(q)e[i++&1u]={q,lim,f};}
};
inline std::atomic<bool> quer_mv{false};
inline thread_local unsigned char params[0x70]{};inline thread_local bool params_ok=false;
inline void guardar(const void* p) noexcept {std::memcpy(params,p,sizeof params);params_ok=true;}
}
