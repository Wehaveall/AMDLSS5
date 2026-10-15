#pragma once
#include <atomic>
#include <cstdint>
#include <cmath>
namespace nrprescale {

inline std::atomic<std::uint64_t> geometry{};
inline std::uint64_t pack(unsigned w,unsigned h){return (std::uint64_t(w)<<32)|h;}

inline std::uint64_t geometria(bool pre_pedido,unsigned estagio_efetivo,unsigned resolucao_trabalho,bool decodificado,
                               unsigned render_w,unsigned render_h,unsigned output_w,unsigned output_h){
 if(!decodificado||resolucao_trabalho!=100)return 0;
 if(pre_pedido)return estagio_efetivo==1?pack(render_w,render_h):0;
 return estagio_efetivo==2?pack(output_w,output_h):0;
}

inline double accepted_percent(double p){
 if(p==6.25)return 6.25;
 if(p>=25 && p<=100 && p==double(int(p)))return p;
 return 100;
}

inline unsigned scaled_extent(int extent,double percent){return unsigned(extent*percent/100)&~1u;}

inline unsigned extensao_rede(int extent,double percent,unsigned alinhamento){
 const unsigned a=alinhamento>2u?alinhamento:2u;return scaled_extent(extent,percent)&~(a-1u);
}

inline std::atomic<int> campo128{0};

inline std::atomic<unsigned> passos_pedidos{1};
inline unsigned campo_eixo(unsigned n,int modo,unsigned teto){
 if(modo<1||modo>2||n<256||!(n&127u))return n;
 const unsigned d=n&~127u,u=(n+127u)&~127u;
 return modo==1?d:(u<=teto?u:n);
}

inline std::atomic<double> efetivo{100.};

inline constexpr std::uint64_t auto_alvo_padrao=921600;
inline std::uint64_t pixels_16x9(unsigned altura){return std::uint64_t(altura*16u/9u)*altura;}
inline std::uint64_t auto_alvo_de(long long chave,long long alvo_pixels){
 if(chave==1){
  if(alvo_pixels<=0)return auto_alvo_padrao;
  return alvo_pixels<65536?65536u:alvo_pixels>8294400?8294400u:std::uint64_t(alvo_pixels);
 }
 if(chave>=144 && chave<=2160)return pixels_16x9(unsigned(chave));
 return 0;
}

inline unsigned pad128(unsigned v){return (v+127u)&~127u;}
inline std::uint64_t area_rede(int largura,int altura,double percent){
 return std::uint64_t(pad128(scaled_extent(largura,percent)))*pad128(scaled_extent(altura,percent));
}
inline double auto_percent(int largura,int altura,std::uint64_t alvo){
 if(largura<=0 || altura<=0 || !alvo)return 100;
 const unsigned ah=unsigned(std::sqrt(double(alvo)*9./16.)+.5);
 const unsigned aw=unsigned(double(alvo)/double(ah?ah:1u)+.5);
 const std::uint64_t teto=std::uint64_t(pad128(aw))*pad128(ah);
 for(int p=100;p>25;--p)if(area_rede(largura,altura,p)<=teto)return p;
 return 25;
}

inline double post_percent(unsigned chave){return chave==33?100.0/3.0:chave==50?50.:chave==67?720.5*100.0/1080.0:chave==75?75.:chave==85?920.0*100.0/1080.0:chave==95?1024.5*100.0/1080.0:chave==107?1152.5*100.0/1080.0:chave==111?1200.5*100.0/1080.0:chave==100?100.:chave==133?400.0/3.0:chave==200?200.:0.;}

inline unsigned post_alinhamento(unsigned chave){return chave==67||chave==111?16u:2u;}
inline unsigned post_chave_de(const wchar_t* texto){
 if(!texto||!*texto)return 100;
 unsigned v=0;for(const wchar_t* c=texto;*c;++c){if(*c<L'0'||*c>L'9'||v>1000)return 0;v=v*10u+unsigned(*c-L'0');}
 return post_percent(v)>0?v:0;
}

inline std::atomic<unsigned> post_chave{100};

inline std::atomic<unsigned> post_teto_centi{10000};
inline double post_teto(){return double(post_teto_centi.load(std::memory_order_relaxed))/100.0;}

inline double post_efetivo(unsigned chave,double teto){
 const double p=post_percent(chave)>0?post_percent(chave):100.;
 return teto>0&&teto<100.&&teto<p?teto:p;
}

inline constexpr double post_base_linhas=1080.0;
inline double post_saida_percent(unsigned chave,double teto,int altura){
 const double p=post_efetivo(chave,teto);
 const double s=altura>0?p*post_base_linhas/double(altura):p;
 return s>100.?100.:s;
}
inline std::atomic<bool> post_menu_escreveu{false};

struct Quadro { unsigned saida_w{},saida_h{},render_w{},render_h{};bool post_pedido{}; };
inline std::uint64_t pack_quadro(const Quadro& q){
 auto m=[](unsigned v){return std::uint64_t(v&0x7fffu);};
 return m(q.saida_w)|(m(q.saida_h)<<15)|(m(q.render_w)<<30)|(m(q.render_h)<<45)|(std::uint64_t(q.post_pedido?1:0)<<60);
}
inline Quadro unpack_quadro(std::uint64_t v){
 return {unsigned(v&0x7fffu),unsigned((v>>15)&0x7fffu),unsigned((v>>30)&0x7fffu),unsigned((v>>45)&0x7fffu),((v>>60)&1u)!=0};
}
inline std::atomic<std::uint64_t> quadro{0};
inline bool rede_na_saida(unsigned w,unsigned h,std::uint64_t v){
 const auto q=unpack_quadro(v);
 if(!q.saida_w||!q.saida_h||w!=q.saida_w||h!=q.saida_h)return false;
 return q.saida_w!=q.render_w||q.saida_h!=q.render_h||q.post_pedido;
}

struct ModoPost { unsigned chave,linhas;bool no_ciclo; };
inline constexpr ModoPost modos[]{{0,0,true},{33,360,true},{50,540,true},{67,720,true},{75,810,true},{85,920,true},{95,1024,true},
                                  {100,1080,true},{107,1152,false},{111,1200,true},{133,1440,true},{200,2160,true}};
inline constexpr unsigned modo_total=unsigned(sizeof modos/sizeof modos[0]);
inline constexpr unsigned modo_chave(unsigned i){return modos[i%modo_total].chave;}

inline constexpr unsigned modo_linhas_de(unsigned i){return modos[i%modo_total].linhas;}
inline unsigned modo_indice(bool pre,unsigned chave){if(pre)return 0;for(unsigned i=1;i<modo_total;++i)if(modos[i].chave==chave)return i;return 0;}
inline unsigned modo_proximo(unsigned indice,int delta){
 unsigned i=indice%modo_total;
 for(unsigned n=0;n<modo_total;++n){i=(i+(delta<0?modo_total-1u:1u))%modo_total;if(modos[i].no_ciclo)return i;}
 return i;
}

inline constexpr unsigned modo_pre_linhas[4]{360,540,720,1080};
inline constexpr unsigned modo_pre_total=unsigned(sizeof modo_pre_linhas/sizeof modo_pre_linhas[0]);
inline constexpr unsigned modo2_total=modo_pre_total+modo_total-1u;
inline unsigned modo2_indice(bool pre,std::uint64_t alvo,unsigned chave){
 if(pre){
  if(!alvo)return modo_pre_total-1u;
  const double linhas=std::sqrt(double(alvo)*9./16.);unsigned melhor=0;double d=1e30;
  for(unsigned i=0;i<modo_pre_total;++i){const double e=std::fabs(linhas-double(modo_pre_linhas[i]));if(e<d){d=e;melhor=i;}}
  return melhor;
 }
 const unsigned j=modo_indice(false,chave);
 return modo_pre_total-1u+(j?j:modo_indice(false,100));
}
inline bool modo2_pre(unsigned indice){return indice%modo2_total<modo_pre_total;}
inline bool modo2_no_ciclo(unsigned indice){const unsigned i=indice%modo2_total;return i<modo_pre_total||modos[i-(modo_pre_total-1u)].no_ciclo;}
inline unsigned modo2_proximo(unsigned indice,int delta){
 unsigned i=indice%modo2_total;
 for(unsigned n=0;n<modo2_total;++n){i=(i+(delta<0?modo2_total-1u:1u))%modo2_total;if(modo2_no_ciclo(i))return i;}
 return i;
}
inline unsigned modo2_linhas(unsigned indice){const unsigned i=indice%modo2_total;return i<modo_pre_total?modo_pre_linhas[i]:modos[i-(modo_pre_total-1u)].linhas;}
inline unsigned modo2_chave(unsigned indice){const unsigned i=indice%modo2_total;return i<modo_pre_total?0u:modos[i-(modo_pre_total-1u)].chave;}
inline double pedido_percent(bool post,unsigned chave,double percent_ini,std::uint64_t alvo,int largura,int altura){
 if(post)return post_saida_percent(chave,post_teto(),altura);
 return alvo?accepted_percent(auto_percent(largura,altura,alvo)):percent_ini;
}

inline std::atomic<std::uint64_t> auto_alvo{0};
inline std::atomic<bool> auto_menu_escreveu{false};

using AquecerJob1=bool(*)(void* base,int largura,int altura) noexcept;
inline std::atomic<AquecerJob1> aquecer_job1{nullptr};
}
