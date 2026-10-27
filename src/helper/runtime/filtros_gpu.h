#pragma once

#include "estado_ffx.h"
#include "filtros.h"
#include <windows.h>
#include "log_fila.h"
#include "pasta_helper.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <atomic>
#include <wrl/client.h>
#include "d3d12_comum.h"
#include "gpu_perdida.h"
#include "tempo_gpu.h"
#include "estencil_externo.h"
#include "ao_gpu.h"
#include "il_gpu.h"
#include "fg_externo.h"

#if defined(NR_FILTROS_CS) && NR_FILTROS_CS
#include "nr_filtros_main.h"
#include "nr_filtros_reduz.h"
#include "nr_filtros_borra_h.h"
#include "nr_filtros_borra_v.h"
#include "nr_filtros_main_m.h"
#include "nr_filtros_reduz_m.h"
#include "nr_filtros_borra_h_m.h"
#include "nr_filtros_borra_v_m.h"
#include "nr_filtros_ceu.h"
#endif
namespace nrfiltrosgpu {
using Microsoft::WRL::ComPtr;
inline const char* hlsl=R"(
// rost (filtros-rosto; dedup item 2): centro, raio (x) e o semi-eixo y da elipse da composicao (0 = circulo de raio x)
cbuffer K:register(b0){uint w,h,hdr,quais;float expo,cas_k,ls_k,ls_lim,cl_k,cl_meios;uint mw,mh;float sigma;int raio;float niv_p,niv_b,tgama,texpo,tsat,tbleach,tdefog,clzona,q1,q2;
 uint nrost;float niv_sp,niv_sb;uint rcont;float4 rost[4];uint4 hq[2];uint hq8;float hsuave;uint hpele;float wb_r,wb_b;uint pele_g;uint ms_w,ms_h,ms_pitch,ms_bits;};   // lua-ao (26/10): ms_bits bit 8 = a tabela de bordas (era o ms_tab)
Texture2D<float4> src:register(t0);Texture2D<float4> aux:register(t1);RWTexture2D<float4> dst:register(u0);
ByteAddressBuffer msk:register(t2);   // sottr-mascara: stencil do personagem (so nas entradas _m)
SamplerState lin:register(s0);
// lua-ao (26/10, analysis/rdr2-lua-ao-20261026 secao 2; "- Nitidez no céu" Desligado): mascara do ceu deste quadro (resolucao de render, 1 =
// ceu; o passe ceu no fim deste texto), na 2a tabela da raiz. quais 2048 = ligada: CAS/LumaSharpen (razao -> 1) e o Clarity somem no ceu;
// o xadrez de 2 px das nuvens do RDR2 ficava x2,9-5,1. Amostra bilinear na mesma posicao relativa: borda do horizonte suave (~1 px).
Texture2D<float> ceuM:register(t3);
float ceu_peso(int2 p){return (quais&2048u)!=0u?ceuM.SampleLevel(lin,(float2(p)+0.5)/float2(w,h),0):0.0;}
float3 P(float3 x){x=max(x,0.0);if(hdr!=0){x*=expo;return x/(1.0+x);}return saturate(x);}
float3 Pi(float3 y){if(hdr!=0){y=min(y,0.9999);return y/(1.0-y)/expo;}return y;}
float Y(float3 c){return dot(c,float3(0.2126,0.7152,0.0722));}
float3 G(float3 x){return hdr!=0?pow(max(x,0.0),1.0/2.2):x;}
float3 Gi(float3 x){return hdr!=0?pow(max(x,0.0),2.2):x;}
float3 at(int2 q){return P(src.Load(int3(clamp(q,int2(0,0),int2(int(w)-1,int(h)-1)),0)).rgb);}
[numthreads(8,8,1)]void reduz(uint3 id:SV_DispatchThreadID){
 if(id.x>=mw||id.y>=mh)return;float s=0;
 [unroll]for(int y=0;y<4;++y)[unroll]for(int x=0;x<4;++x)s+=Y(at(int2(id.xy)*4+int2(x,y)));
 dst[id.xy]=float4(s/16.0,0,0,0);}
float mapa(int2 q){return src.Load(int3(clamp(q,int2(0,0),int2(int(mw)-1,int(mh)-1)),0)).r;}
// rev-morto (19/10, P3): os pesos da gaussiana (raio <= 24, CPU) saem 1x por grupo na memoria de grupo, nao 1 exp por amostra.
groupshared float pesos[25];
void pesos_grupo(uint gi){if(gi<=uint(raio)&&gi<25u)pesos[gi]=exp(-0.5*float(int(gi)*int(gi))/(sigma*sigma));GroupMemoryBarrierWithGroupSync();}
[numthreads(8,8,1)]void borra_h(uint3 id:SV_DispatchThreadID,uint gi:SV_GroupIndex){
 pesos_grupo(gi);
 if(id.x>=mw||id.y>=mh)return;float s=0,ws=0;
 for(int k=-raio;k<=raio;++k){float g=pesos[abs(k)];s+=g*mapa(int2(id.xy)+int2(k,0));ws+=g;}
 dst[id.xy]=float4(s/ws,0,0,0);}
[numthreads(8,8,1)]void borra_v(uint3 id:SV_DispatchThreadID,uint gi:SV_GroupIndex){
 pesos_grupo(gi);
 if(id.x>=mw||id.y>=mh)return;float s=0,ws=0;
 for(int k=-raio;k<=raio;++k){float g=pesos[abs(k)];s+=g*mapa(int2(id.xy)+int2(0,k));ws+=g;}
 dst[id.xy]=float4(s/ws,0,0,0);}
// pele-rosto (24/10): o peso da pele (regiao x cor, descrito em pele() abaixo); uma fonte so, usada pela Pele do rosto e pelo
// "Clarity fora da pele" (quais 512).
float pele_regiao(int2 p){
 float w=0.0;
 for(uint k=0;k<nrost&&k<4u;++k){float r=max(rost[k].z,1.0),ry=rost[k].w>0.0?rost[k].w:r;float2 d=float2(p)+0.5-rost[k].xy;
  float dd=length(float2(d.x/(1.5*r),d.y/(d.y<0.0?1.6*ry:4.4*ry)));w=max(w,1.0-smoothstep(0.55,1.0,dd));}
 return w;}
float pele_cor(float3 c){
 c=saturate(c);float mx=max(c.r,max(c.g,c.b)),mn=min(c.r,min(c.g,c.b)),ch=mx-mn;if(!(ch>1e-5))return 0.0;
 float hh=mx==c.r?(c.g-c.b)/ch:(mx==c.g?2.0+(c.b-c.r)/ch:4.0+(c.r-c.g)/ch);hh=frac(hh/6.0+1.0)*360.0;float S=ch/mx;
 return smoothstep(4.0,12.0,hh)*(1.0-smoothstep(42.0,55.0,hh))*smoothstep(0.10,0.20,S)*(1.0-smoothstep(0.62,0.75,S))*smoothstep(0.28,0.42,mx);}
float pele_peso(int2 p,float3 o){float w=pele_regiao(p);return w>0.0?w*pele_cor(o):0.0;}
)"
R"(// rdr2-halo (26/10, quais 8192 = "- Clarity fora do rosto" em Cabeça): a cabeca inteira, sem olhar a cor (cabelo, barba, chapeu, a pele
// de costas e o fundo junto dela). Elipse assimetrica por cabeca de rost[] (r = metade do lado da caixa, w = semi-eixo y ou 0): 1,8 r
// para o lado, 2,0 ry para cima (chapeu), 2,4 ry para baixo (queixo, barba, pescoco); peso 1 ate 0,7 e some em 1,0. Folgada de proposito:
// no RDR2 a caixa do rastreio fica ate ~0,25 cabeca do centro real (F9 30060: 40 px@4K, na nuca).
float cabeca_regiao(int2 p){
 float w=0.0;
 for(uint k=0;k<nrost&&k<4u;++k){float r=max(rost[k].z,1.0),ry=rost[k].w>0.0?rost[k].w:r;float2 d=float2(p)+0.5-rost[k].xy;
  float dd=length(float2(d.x/(1.8*r),d.y/(d.y<0.0?2.0*ry:2.4*ry)));w=max(w,1.0-smoothstep(0.7,1.0,dd));}
 return w;}
// filtros-luz (20/10, q1 = NeuralFiltroNitidezLocal): CAS e LumaSharpen na luminancia LINEAR da cena, Clarity com razao limitada;
// o resultado e uma razao de luma aplicada ao RGB (matiz e saturacao ficam). Em luz linear o CAS so ve razoes entre vizinhos
// (amp = sqrt(min/max), pesos somando 1): o mesmo resultado em qualquer escala/exposicao do jogo e sem energia nova no bloom.
// O caminho antigo (Reinhard por canal + volta y/(1-y)/expo) levava ponto de luz a 67-357x no RE (exposicao real do jogo).
float YL(int2 q){return Y(max(src.Load(int3(clamp(q,int2(0,0),int2(int(w)-1,int(h)-1)),0)).rgb,0.0));}
float YS(float2 uv){return Y(max(src.SampleLevel(lin,uv,0).rgb,0.0));}
// rosto-contorno (24/10, NeuralFiltroRostoContorno; q2 = a forca, 0 = desligado; rcont = Min | Max em meia precisao): a razao da nitidez
// dentro do rosto so nos contornos. Contraste de Michelson da luma linear media 2x2 (1 amostra bilinear no canto de 4 px) em 3x3 pontos
// a 2 px (~6x6 px): a media tira o jitter e o ruido de 1 px, a janela pega os 2-3 px em volta da borda onde a nitidez age. (max-min)/
// (max+min) e uma razao: igual em qualquer exposicao. F9 14192 do RE (rosto 467 px): pele 0,03-0,14, olho/labio/sobrancelha 0,3-0,5.
float rosto_contorno(int2 p,float r){
 float2 px=1.0/float2(w,h);float2 c=(float2(p)+1.0)*px;float mn=1e30,mx=0.0;
 [unroll]for(int j=-1;j<=1;++j)[unroll]for(int i=-1;i<=1;++i){float v=YS(c+float2(i,j)*(2.0*px));mn=min(mn,v);mx=max(mx,v);}
 float m=smoothstep(f16tof32(rcont&0xffffu),f16tof32(rcont>>16),(mx-mn)/max(mx+mn,1e-30));
 return clamp(1.0+(r-1.0)*q2*m,0.5,2.0);}
float razao_local(int2 p,float3 rgb){
 float yc=Y(rgb);if(!(yc>1e-12))return 1.0;float ys=yc;
 if(quais&1u){
  float a=YL(p+int2(-1,-1)),b=YL(p+int2(0,-1)),c=YL(p+int2(1,-1)),d=YL(p+int2(-1,0)),f=YL(p+int2(1,0)),g=YL(p+int2(-1,1)),hh=YL(p+int2(0,1)),i=YL(p+int2(1,1));
  float mn=min(min(min(d,yc),min(f,b)),hh);mn+=min(mn,min(min(a,c),min(g,i)));
  float mx=max(max(max(d,yc),max(f,b)),hh);mx+=max(mx,max(max(a,c),max(g,i)));
  float wv=sqrt(saturate(mn/max(mx,1e-30)))*(-1.0/lerp(8.0,5.0,saturate(cas_k)));
  ys+=max((b+d+f+hh)*wv+yc,0.0)/(1.0+4.0*wv)-yc;}
 if(quais&2u){
  float2 px=1.0/float2(w,h);float2 uv=(float2(p)+0.5)*px;
  float yb=0.25*(YS(uv+float2( 0.4,-1.2)*px)+YS(uv+float2(-1.2,-0.4)*px)+YS(uv+float2( 1.2, 0.4)*px)+YS(uv+float2(-0.4, 1.2)*px));
  // O limite do LumaSharpen e de tela (0..1): aqui relativo a luz local (0,035 = 14% de um meio-tom).
  float lim=2.0*ls_lim*(yc+yb);ys+=clamp((yc-yb)*ls_k,-lim,lim);}
 float r=clamp(ys/yc,0.5,2.0);
 const float cw=ceu_peso(p);r=lerp(r,1.0,cw);   // lua-ao (26/10): sem CAS/LumaSharpen no ceu (0 fora dele: r intacto)
 // filtros-rosto: dentro das cabecas ligadas (Grace e NPCs) a nitidez some (com o contorno: so nos contornos), borda suave de 0,85 a
 // 1,15 do raio; o Clarity continua.
 if(nrost>0u&&(quais&3u)&&!(quais&256u)){float pr=0.0;
  for(uint k=0;k<nrost&&k<4u;++k){float d=rost[k].w>0.0?length((float2(p)+0.5-rost[k].xy)/max(rost[k].zw,1.0)):length((float2(p)+0.5-rost[k].xy)/max(rost[k].z,1.0));pr=max(pr,1.0-smoothstep(0.85,1.15,d));}
  if(q2>0.0&&pr>0.0)r=lerp(r,rosto_contorno(p,r),pr);else r=lerp(r,1.0,pr);}
 if(quais&4u){
  float y=Y(P(rgb));float b=aux.SampleLevel(lin,(float2(p)+0.5)/(4.0*float2(mw,mh)),0).r;
  float mid=lerp(1.0,1.0-abs(2.0*y-1.0),cl_meios);float dl=clamp((y-b)*cl_k*mid,-0.25,0.25);
  if(clzona>0.5){float m=dot(G(P(rgb)),1.0/3.0);dl*=1.0-smoothstep(0.8*100.0/255.0,1.2*100.0/255.0,m);}
  if(quais&512u)dl*=1.0-pele_peso(p,G(P(rgb)));   // pele-rosto: Clarity fora da pele (multiplica junto com a Zona)
  if(quais&8192u)dl*=1.0-cabeca_regiao(p);   // rdr2-halo: Clarity fora da cabeca
  dl*=1.0-cw;   // lua-ao (26/10): sem Clarity no ceu
  // A razao e a da curva (y2/y, como o caminho antigo aplicava a luma): nos meios-tons ~ a mesma em luz linear; perto de 1 a
  // volta do Reinhard explodiria (+49% de energia numa linha de luz), aqui o realce quase nao muda. No maximo 1,5x por lado.
  r*=clamp(saturate(y+dl)/max(y,1e-30),2.0/3.0,1.5);}
 return r;}
// rev-hsl (22/10): Cor por faixa (HSL), no dominio de tom do display (depois de Niveis/Tom). Ajustes da faixa f (0 Todas, 1..8 =
// Vermelhos, Laranjas, Amarelos, Verdes, Cianos, Azuis, Roxos, Magentas) em hq: byte 0 matiz em graus, 1 saturacao %, 2 luminosidade %,
// cada um + 128. Faixas = particao do circulo do matiz entre os centros 0/30/60/120/180/220/270/300: entre dois centros vizinhos o peso
// passa de um para o outro por um smoothstep de largura hsuave (fracao da distancia; soma sempre 1, sem degrau). Os ajustes das faixas
// pesam pela saturacao (cinza e branco nao ganham cor); Todas vale em tudo. Saturacao multiplicativa (-100 = cinza); luminosidade
// escurece/clareia no L do HSL (matiz e saturacao ficam). Ajuste somado = 0 devolve o pixel intacto (bit a bit).
static const float hcent[9]={0.0,30.0,60.0,120.0,180.0,220.0,270.0,300.0,360.0};
float3 hsl_aj(uint f){uint v=f<8u?hq[f>>2][f&3u]:hq8;return float3(int3(int(v&255u),int((v>>8)&255u),int((v>>16)&255u))-128);}
float3 hsl(float3 c){
 c=saturate(c);float mx=max(c.r,max(c.g,c.b)),mn=min(c.r,min(c.g,c.b)),ch=mx-mn,L=0.5*(mx+mn);
 float S=ch>1e-6?saturate(ch/max(1.0-abs(2.0*L-1.0),1e-6)):0.0;float hh=0.0;
 float3 a=hsl_aj(0u);
 if(ch>1e-6){
  hh=mx==c.r?(c.g-c.b)/ch:(mx==c.g?2.0+(c.b-c.r)/ch:4.0+(c.r-c.g)/ch);hh=frac(hh/6.0+1.0)*360.0;
  uint i=0u;[unroll]for(uint k=1u;k<8u;++k)if(hh>=hcent[k])i=k;
  float t=saturate((hh-hcent[i])/(hcent[i+1u]-hcent[i]));float wb=smoothstep(0.5-0.5*hsuave,0.5+0.5*hsuave,t),wa=1.0-wb;
  uint fa=i+1u,fb=(i+1u)%8u+1u;
  // Proteger pele: matiz ~15-45 graus, saturacao e luminosidade medias; atenua so Vermelhos e Laranjas.
  if((hpele&1u)!=0u){float pele=smoothstep(8.0,15.0,hh)*(1.0-smoothstep(45.0,55.0,hh))*smoothstep(0.08,0.18,S)*(1.0-smoothstep(0.7,0.9,S))*
                            smoothstep(0.1,0.22,L)*(1.0-smoothstep(0.82,0.93,L));
   if(fa<=2u)wa*=1.0-pele;if(fb<=2u)wb*=1.0-pele;}
  float ws=smoothstep(0.02,0.25,ch/max(mx,1e-6));
  a+=ws*(wa*hsl_aj(fa)+wb*hsl_aj(fb));}
 if(a.x==0.0&&a.y==0.0&&a.z==0.0)return c;
 float h2=frac((hh+a.x)/360.0+1.0)*360.0;float S2=saturate(S*max(1.0+a.y/100.0,0.0));
 float dl=clamp(a.z/100.0,-1.0,1.0);float L2=saturate(dl>=0.0?L+(1.0-L)*dl:L*(1.0+dl));
 float q=S2*min(L2,1.0-L2);float3 k3=fmod(float3(0.0,8.0,4.0)+h2/30.0,12.0);
 float3 r=L2-q*clamp(min(k3-3.0,9.0-k3),-1.0,1.0);
 return all(r==r)?saturate(r):c;}   // NaN: o pixel de entrada
// temperatura (24/10, NeuralFiltroTemperatura): ganhos lineares R,G,B (luma Rec.709 = 1; G sai da luma: wb_r e wb_b bastam) aplicados em
// LUZ LINEAR, sobre a cor ja com Niveis/Tom (no dominio com gama deles). No HDR: o volta ao Reinhard (Gi) e o ganho vai na luz da cena:
// P(Pi(y)*g) = y*g/(1-y+y*g) (a exposicao some da conta): cinza com o mesmo brilho na cena, como o jogo mostra. No SDR (UNORM, valor de tela
// com gama ~2,2): o ganho linear g e o*g^(1/2,2), a mesma conta. Sem mascara e sem area de rosto: e a cor da imagem toda (como Tom e HSL).
float3 ganho_linear(float3 o,float3 g){
 if(hdr!=0){float3 y=Gi(saturate(o));return G(y*g/max(1.0-y+y*g,1e-6));}
 return saturate(o*pow(max(g,0.0),1.0/2.2));}
float3 temperatura(float3 o){return ganho_linear(o,float3(wb_r,(1.0-0.2126*wb_r-0.0722*wb_b)/0.7152,wb_b));}
// pele-rosto (24/10, NeuralFiltroPele; quais 128): so a pele da cabeca e do pescoco. Peso = regiao x pele, sem estado no tempo.
// Regiao: para cada cabeca de rost[] (as mesmas da nitidez fora do rosto; r = metade do lado da caixa, w = semi-eixo y da elipse ou 0),
// uma elipse assimetrica descendo para o pescoco: semi-eixo x 1,5 r, para cima 1,6 ry, para baixo 4,4 ry (= 0,75 do tamanho da cabeca
// para o lado, 0,8 acima e 2,2 abaixo do centro); peso 1 ate 0,55 do raio normalizado e cai por um smoothstep ate 1 (borda longa).
// Pele pela cor (HSV da cor de tela, ja com Niveis/Tom/Temperatura): matiz 4-12 sobe e 42-55 graus desce (labio vermelho/rosado e
// cabelo loiro saem), saturacao 0,10-0,20 sobe e 0,62-0,75 desce (cinza, branco do olho e roupa saturada saem), valor 0,28-0,42 sobe
// (cabelo castanho escuro e sombra funda saem). Ajustes: ganho linear (temperatura da pele x 2^luz, luma 2^luz, o mesmo ganho_linear da
// Temperatura), depois saturacao em volta do cinza e matiz girando em volta do eixo cinza (+ = para o amarelo), na cor de tela.
// hpele: bit 0 = Proteger pele do HSL; bits 8-13 luz (passos de 0,02 EV + 25), 14-19 saturacao (% + 30), 20-25 matiz (meio grau + 20).
// pele_g = ganhos lineares de R e B em meia precisao (G sai da luma 2^luz).
float3 pele(int2 p,float3 o){
 float w=pele_peso(p,o);if(!(w>0.0))return o;
 float ev=(float((hpele>>8)&63u)-25.0)*0.02,s=(float((hpele>>14)&63u)-30.0)/100.0,m=radians((float((hpele>>20)&63u)-20.0)*0.5);
 float gr=f16tof32(pele_g&0xffffu),gb=f16tof32(pele_g>>16);
 float3 q=ganho_linear(o,float3(gr,(exp2(ev)-0.2126*gr-0.0722*gb)/0.7152,gb));
 float y=dot(q,1.0/3.0);float3 d=(q-y)*(1.0+s);float3 k=float3(0.57735027,0.57735027,0.57735027);
 q=y+d*cos(m)+cross(k,d)*sin(m);
 return lerp(o,saturate(q),w);}
[numthreads(8,8,1)]void main(uint3 id:SV_DispatchThreadID){
 if(id.x>=w||id.y>=h)return;int2 p=int2(id.xy);
 float4 c0=src.Load(int3(p,0));float3 e=P(c0.rgb);float3 o=e;
 if(q1>0.5){
  float3 cl=max(c0.rgb,0.0);if(quais&7u)cl*=razao_local(p,cl);
  if(!(quais&248u)){dst[p]=float4(hdr!=0?cl:saturate(cl),c0.a);return;}
  o=P(cl);
 }else{
 if(quais&1u){
  float3 a=at(p+int2(-1,-1)),b=at(p+int2(0,-1)),c=at(p+int2(1,-1)),d=at(p+int2(-1,0)),f=at(p+int2(1,0)),g=at(p+int2(-1,1)),hh=at(p+int2(0,1)),i=at(p+int2(1,1));
  float3 mn=min(min(min(d,e),min(f,b)),hh);float3 mn2=min(mn,min(min(a,c),min(g,i)));mn+=mn2;
  float3 mx=max(max(max(d,e),max(f,b)),hh);float3 mx2=max(mx,max(max(a,c),max(g,i)));mx+=mx2;
  float3 amp=sqrt(saturate(min(mn,2.0-mx)/max(mx,1e-5)));
  float3 wv=amp*(-1.0/lerp(8.0,5.0,saturate(cas_k)));
  o+=saturate((b*wv+d*wv+f*wv+hh*wv+e)/(1.0+4.0*wv))-e;}
 if(quais&2u){
  float2 px=1.0/float2(w,h);float2 uv=(float2(p)+0.5)*px;
  float3 bl=P(src.SampleLevel(lin,uv+float2( 0.4,-1.2)*px,0).rgb)+P(src.SampleLevel(lin,uv+float2(-1.2,-0.4)*px,0).rgb)+
            P(src.SampleLevel(lin,uv+float2( 1.2, 0.4)*px,0).rgb)+P(src.SampleLevel(lin,uv+float2(-0.4, 1.2)*px,0).rgb);
  o+=clamp(Y(e-bl*0.25)*ls_k,-ls_lim,ls_lim);}
 if(quais&4u){
  float y=Y(e);float b=aux.SampleLevel(lin,(float2(p)+0.5)/(4.0*float2(mw,mh)),0).r;
  float mid=lerp(1.0,1.0-abs(2.0*y-1.0),cl_meios);float dl=clamp((y-b)*cl_k*mid,-0.25,0.25);
  // Zona Sombras: some entre a media 0,31 e 0,47 da tela (com gama), como o BlendIf claro 100 do preset KK.
  if(clzona>0.5){float m=dot(G(e),1.0/3.0);dl*=1.0-smoothstep(0.8*100.0/255.0,1.2*100.0/255.0,m);}
  if(quais&512u)dl*=1.0-pele_peso(p,G(e));   // pele-rosto: Clarity fora da pele
  if(quais&8192u)dl*=1.0-cabeca_regiao(p);   // rdr2-halo: Clarity fora da cabeca
  float yo=Y(o);float y2=saturate(yo+dl);o=yo>1e-5?o*(y2/yo):o+(y2-yo);}
 }
 o=saturate(o);
 // Niveis e Tom foram feitos para a imagem com gama (0..1 da tela): no HDR, P vai para ~gama 2,2 e volta depois.
 if(quais&248u)o=G(o);
 if(quais&8u)o=saturate((o-niv_p)/max(niv_b-niv_p,1e-3));
 if((quais&8u)&&(niv_sp!=0.0||niv_sb!=1.0))o=niv_sp+o*(niv_sb-niv_sp);   // niveis-saida: 0/255 = nem faz a conta (bit a bit o de antes)
 if(quais&16u){
  // Defog na cor de neblina do preset KK (RGB 15/15/12), x2,55 como no Tonemap.fx.
  o=saturate(o-tdefog*float3(15.0,15.0,12.0)/255.0*2.55);o*=exp2(texpo);o=pow(max(o,0.0),tgama);
  float l=Y(o);float L=saturate(10.0*(l-0.45));float3 a2=tbleach*o;
  float3 nc=lerp(2.0*o*l,1.0-2.0*(1.0-l)*(1.0-o),L);o+=(1.0-a2)*(a2*nc);
  float3 dc=o-dot(o,1.0/3.0);o=(o+dc*tsat)/(1.0+dc*tsat);}
 if(quais&64u)o=temperatura(o);   // temperatura (24/10): depois de Niveis/Tom, antes do HSL
 if(quais&128u)o=pele(p,o);   // pele-rosto (24/10): depois da Temperatura, antes do HSL
 if(quais&32u)o=hsl(o);   // rev-hsl: por ultimo, sobre a cor ja com Niveis/Tom
 if(quais&248u)o=Gi(saturate(o));
 // Cor linear (HDR): Niveis/exposicao levam realces a 1, que a volta do Reinhard viraria luz quase infinita (bloom do jogo
 // estoura). O realce nao passa do original nem de 0,97 (~32x a exposicao).
 if(hdr!=0&&(quais&248u))o=min(o,max(e,0.97));
 dst[p]=float4(Pi(saturate(o)),c0.a);}

)"
R"(// sottr-mascara (23/10, NeuralFiltroMascara): entradas _m, so com o stencil do personagem (t2, buffer raw: linha y no byte y*ms_pitch).
// Classe de um px da saida = o texel do stencil mais perto. reduz_m/borra_*_m: o mapa do Clarity em 4 canais (soma da luma e peso do
// personagem, soma e peso do fundo), borrado igual; main_m = main com: distancia da borda em 8 direcoes (passos 1..6 px) -> CAS e
// LumaSharpen somem ate 3 px da saida e voltam em rampa ate 6; o Clarity compara com a media borrada SO da classe do pixel.
// main e razao_local nao mudam (copias abaixo: razao_m e main_m).
bool MB(int2 q){q=clamp(q,int2(0,0),int2(int(ms_w)-1,int(ms_h)-1));uint a=uint(q.y)*ms_pitch+uint(q.x);return ((msk.Load(a&~3u)>>((a&3u)*8u))&(ms_bits&255u))!=0u;}
bool CL(int2 p){return MB(int2(floor((float2(p)+0.5)*float2(ms_w,ms_h)/float2(w,h))));}
static const int2 d8m[8]={int2(-1,-1),int2(0,-1),int2(1,-1),int2(-1,0),int2(1,0),int2(-1,1),int2(0,1),int2(1,1)};
[numthreads(8,8,1)]void reduz_m(uint3 id:SV_DispatchThreadID){
 if(id.x>=mw||id.y>=mh)return;float4 s=0;
 [unroll]for(int y=0;y<4;++y)[unroll]for(int x=0;x<4;++x){int2 q=int2(id.xy)*4+int2(x,y);float v=Y(at(q));s+=CL(min(q,int2(int(w)-1,int(h)-1)))?float4(v,1,0,0):float4(0,0,v,1);}
 dst[id.xy]=s/16.0;}
float4 mapa4(int2 q){return src.Load(int3(clamp(q,int2(0,0),int2(int(mw)-1,int(mh)-1)),0));}
[numthreads(8,8,1)]void borra_h_m(uint3 id:SV_DispatchThreadID,uint gi:SV_GroupIndex){
 pesos_grupo(gi);
 if(id.x>=mw||id.y>=mh)return;float4 s=0;float ws=0;
 for(int k=-raio;k<=raio;++k){float g=pesos[abs(k)];s+=g*mapa4(int2(id.xy)+int2(k,0));ws+=g;}
 dst[id.xy]=s/ws;}
[numthreads(8,8,1)]void borra_v_m(uint3 id:SV_DispatchThreadID,uint gi:SV_GroupIndex){
 pesos_grupo(gi);
 if(id.x>=mw||id.y>=mh)return;float4 s=0;float ws=0;
 for(int k=-raio;k<=raio;++k){float g=pesos[abs(k)];s+=g*mapa4(int2(id.xy)+int2(0,k));ws+=g;}
 dst[id.xy]=s/ws;}
float razao_m(int2 p,float3 rgb,float bw,float bc){
 float yc=Y(rgb);if(!(yc>1e-12))return 1.0;float ys=yc;
 if(quais&1u){
  float a=YL(p+int2(-1,-1)),b=YL(p+int2(0,-1)),c=YL(p+int2(1,-1)),d=YL(p+int2(-1,0)),f=YL(p+int2(1,0)),g=YL(p+int2(-1,1)),hh=YL(p+int2(0,1)),i=YL(p+int2(1,1));
  float mn=min(min(min(d,yc),min(f,b)),hh);mn+=min(mn,min(min(a,c),min(g,i)));
  float mx=max(max(max(d,yc),max(f,b)),hh);mx+=max(mx,max(max(a,c),max(g,i)));
  float wv=sqrt(saturate(mn/max(mx,1e-30)))*(-1.0/lerp(8.0,5.0,saturate(cas_k)));
  ys+=max((b+d+f+hh)*wv+yc,0.0)/(1.0+4.0*wv)-yc;}
 if(quais&2u){
  float2 px=1.0/float2(w,h);float2 uv=(float2(p)+0.5)*px;
  float yb=0.25*(YS(uv+float2( 0.4,-1.2)*px)+YS(uv+float2(-1.2,-0.4)*px)+YS(uv+float2( 1.2, 0.4)*px)+YS(uv+float2(-0.4, 1.2)*px));
  // O limite do LumaSharpen e de tela (0..1): aqui relativo a luz local (0,035 = 14% de um meio-tom).
  float lim=2.0*ls_lim*(yc+yb);ys+=clamp((yc-yb)*ls_k,-lim,lim);}
 float r=clamp(ys/yc,0.5,2.0);
 const float cw=ceu_peso(p);r=lerp(r,1.0,cw);   // lua-ao (26/10): sem CAS/LumaSharpen no ceu (0 fora dele: r intacto)
 // filtros-rosto: dentro das cabecas ligadas (Grace e NPCs) a nitidez some (com o contorno: so nos contornos), borda suave de 0,85 a
 // 1,15 do raio; o Clarity continua.
 if(nrost>0u&&(quais&3u)&&!(quais&256u)){float pr=0.0;
  for(uint k=0;k<nrost&&k<4u;++k){float d=rost[k].w>0.0?length((float2(p)+0.5-rost[k].xy)/max(rost[k].zw,1.0)):length((float2(p)+0.5-rost[k].xy)/max(rost[k].z,1.0));pr=max(pr,1.0-smoothstep(0.85,1.15,d));}
  if(q2>0.0&&pr>0.0)r=lerp(r,rosto_contorno(p,r),pr);else r=lerp(r,1.0,pr);}
 r=lerp(r,1.0,bw);   // sottr-mascara: sem CAS/LumaSharpen perto da borda do personagem
 if(quais&4u){
  float y=Y(P(rgb));float b=bc;   // sottr-mascara: a media borrada so da classe do pixel
  float mid=lerp(1.0,1.0-abs(2.0*y-1.0),cl_meios);float dl=clamp((y-b)*cl_k*mid,-0.25,0.25);
  if(clzona>0.5){float m=dot(G(P(rgb)),1.0/3.0);dl*=1.0-smoothstep(0.8*100.0/255.0,1.2*100.0/255.0,m);}
  if(quais&512u)dl*=1.0-pele_peso(p,G(P(rgb)));   // pele-rosto: Clarity fora da pele (multiplica junto com a Zona)
  if(quais&8192u)dl*=1.0-cabeca_regiao(p);   // rdr2-halo: Clarity fora da cabeca
  dl*=1.0-cw;   // lua-ao (26/10): sem Clarity no ceu
  // A razao e a da curva (y2/y, como o caminho antigo aplicava a luma): nos meios-tons ~ a mesma em luz linear; perto de 1 a
  // volta do Reinhard explodiria (+49% de energia numa linha de luz), aqui o realce quase nao muda. No maximo 1,5x por lado.
  r*=clamp(saturate(y+dl)/max(y,1e-30),2.0/3.0,1.5);}
 return r;}
// sottr-otim (23/10): bloco de 8x8 px sem borda de classe no alcance da busca de 6 px = dmin 99 (o mesmo resultado) sem as 48 leituras
// por px. dedup (23/10, item 3): a tabela dos blocos e a do backend (BordasPost em compose_post.hlsl, com a janela daqui incluida),
// uma por quadro; antes cada grupo refazia a sua (gsMisto). Sem a tabela deste quadro (ms_tab 0: PRE, backend sem a mascara) todo
// bloco vai ao caminho cheio -- o mesmo resultado, mais lento.
RWTexture2D<uint> bordasM:register(u1);
[numthreads(8,8,1)]void main_m(uint3 id:SV_DispatchThreadID,uint3 gid:SV_GroupID){
 if(id.x>=w||id.y>=h)return;int2 p=int2(id.xy);
 const bool misto=(ms_bits&256u)==0u||bordasM[gid.xy]!=0u;   // lua-ao (26/10): a tabela no bit 8 do ms_bits (a raiz _m ganhou a tabela do ceu)
 const bool cm=CL(p);float dmin=99.0;if(misto){[unroll]for(int km=0;km<8;++km)[unroll]for(int sm=1;sm<=6;++sm)if(CL(p+d8m[km]*sm)!=cm)dmin=min(dmin,float(sm));}
 const float bw=1.0-smoothstep(3.0,6.0,dmin);float bc=0.0;   // px da saida: o raio do CAS/LumaSharpen e 1-1,2 px em qualquer resolucao
 if(quais&4u){float4 v=aux.SampleLevel(lin,(float2(p)+0.5)/(4.0*float2(mw,mh)),0);float so=cm?v.x:v.z,no=cm?v.y:v.w;
  bc=no>1e-5?so/no:(v.x+v.z)/max(v.y+v.w,1e-6);}
 float4 c0=src.Load(int3(p,0));float3 e=P(c0.rgb);float3 o=e;
 if(q1>0.5){
  float3 cl=max(c0.rgb,0.0);if(quais&7u)cl*=razao_m(p,cl,bw,bc);
  if(!(quais&248u)){dst[p]=float4(hdr!=0?cl:saturate(cl),c0.a);return;}
  o=P(cl);
 }else{
 if(quais&1u){
  float3 a=at(p+int2(-1,-1)),b=at(p+int2(0,-1)),c=at(p+int2(1,-1)),d=at(p+int2(-1,0)),f=at(p+int2(1,0)),g=at(p+int2(-1,1)),hh=at(p+int2(0,1)),i=at(p+int2(1,1));
  float3 mn=min(min(min(d,e),min(f,b)),hh);float3 mn2=min(mn,min(min(a,c),min(g,i)));mn+=mn2;
  float3 mx=max(max(max(d,e),max(f,b)),hh);float3 mx2=max(mx,max(max(a,c),max(g,i)));mx+=mx2;
  float3 amp=sqrt(saturate(min(mn,2.0-mx)/max(mx,1e-5)));
  float3 wv=amp*(-1.0/lerp(8.0,5.0,saturate(cas_k)));
  o+=saturate((b*wv+d*wv+f*wv+hh*wv+e)/(1.0+4.0*wv))-e;}
 if(quais&2u){
  float2 px=1.0/float2(w,h);float2 uv=(float2(p)+0.5)*px;
  float3 bl=P(src.SampleLevel(lin,uv+float2( 0.4,-1.2)*px,0).rgb)+P(src.SampleLevel(lin,uv+float2(-1.2,-0.4)*px,0).rgb)+
            P(src.SampleLevel(lin,uv+float2( 1.2, 0.4)*px,0).rgb)+P(src.SampleLevel(lin,uv+float2(-0.4, 1.2)*px,0).rgb);
  o+=clamp(Y(e-bl*0.25)*ls_k,-ls_lim,ls_lim);}
 if(quais&4u){
  float y=Y(e);float b=aux.SampleLevel(lin,(float2(p)+0.5)/(4.0*float2(mw,mh)),0).r;
  float mid=lerp(1.0,1.0-abs(2.0*y-1.0),cl_meios);float dl=clamp((y-b)*cl_k*mid,-0.25,0.25);
  // Zona Sombras: some entre a media 0,31 e 0,47 da tela (com gama), como o BlendIf claro 100 do preset KK.
  if(clzona>0.5){float m=dot(G(e),1.0/3.0);dl*=1.0-smoothstep(0.8*100.0/255.0,1.2*100.0/255.0,m);}
  if(quais&512u)dl*=1.0-pele_peso(p,G(e));   // pele-rosto: Clarity fora da pele
  if(quais&8192u)dl*=1.0-cabeca_regiao(p);   // rdr2-halo: Clarity fora da cabeca
  float yo=Y(o);float y2=saturate(yo+dl);o=yo>1e-5?o*(y2/yo):o+(y2-yo);}
 }
 o=saturate(o);
 // Niveis e Tom foram feitos para a imagem com gama (0..1 da tela): no HDR, P vai para ~gama 2,2 e volta depois.
 if(quais&248u)o=G(o);
 if(quais&8u)o=saturate((o-niv_p)/max(niv_b-niv_p,1e-3));
 if((quais&8u)&&(niv_sp!=0.0||niv_sb!=1.0))o=niv_sp+o*(niv_sb-niv_sp);   // niveis-saida: 0/255 = nem faz a conta (bit a bit o de antes)
 if(quais&16u){
  // Defog na cor de neblina do preset KK (RGB 15/15/12), x2,55 como no Tonemap.fx.
  o=saturate(o-tdefog*float3(15.0,15.0,12.0)/255.0*2.55);o*=exp2(texpo);o=pow(max(o,0.0),tgama);
  float l=Y(o);float L=saturate(10.0*(l-0.45));float3 a2=tbleach*o;
  float3 nc=lerp(2.0*o*l,1.0-2.0*(1.0-l)*(1.0-o),L);o+=(1.0-a2)*(a2*nc);
  float3 dc=o-dot(o,1.0/3.0);o=(o+dc*tsat)/(1.0+dc*tsat);}
 if(quais&64u)o=temperatura(o);   // temperatura (24/10): depois de Niveis/Tom, antes do HSL
 if(quais&128u)o=pele(p,o);   // pele-rosto (24/10): depois da Temperatura, antes do HSL
 if(quais&32u)o=hsl(o);   // rev-hsl: por ultimo, sobre a cor ja com Niveis/Tom
 if(quais&248u)o=Gi(saturate(o));
 // Cor linear (HDR): Niveis/exposicao levam realces a 1, que a volta do Reinhard viraria luz quase infinita (bloom do jogo
 // estoura). O realce nao passa do original nem de 0,97 (~32x a exposicao).
 if(hdr!=0&&(quais&248u))o=min(o,max(e,0.97));
 dst[p]=float4(Pi(saturate(o)),c0.a);}
// lua-ao (26/10): a mascara do ceu na resolucao de render (u0 = R8, 1 = ceu) pela profundidade do jogo em t0 (.r). Constantes deste passe:
// w/h = o tamanho de render, hdr = 1 com o Z invertido (ceu em 0) e 0 com o normal (ceu em 1); NaN = ceu (a regra da AO, ao_gpu.h zlin).
[numthreads(8,8,1)]void ceu(uint3 id:SV_DispatchThreadID){
 if(id.x>=w||id.y>=h)return;const float d=src.Load(int3(id.xy,0)).r;
 const bool c=hdr!=0u?!(d>1e-7):!(d<1.0-1e-7);dst[id.xy]=float4(c?1.0:0.0,0.0,0.0,0.0);}
)";

inline D3D12_RESOURCE_STATES estado_ffx(unsigned s){const auto t=nrffx_estado::traduzir(s);
 return t.d3d12==nrffx_estado::recusa?D3D12_RESOURCE_STATES(~0u):D3D12_RESOURCE_STATES(t.d3d12);}

inline DXGI_FORMAT vista(DXGI_FORMAT f,bool& hdr){
 hdr=false;switch(f){
 case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:hdr=true;return f;
 case DXGI_FORMAT_R16G16B16A16_TYPELESS:case DXGI_FORMAT_R16G16B16A16_FLOAT:hdr=true;return DXGI_FORMAT_R16G16B16A16_FLOAT;
 case DXGI_FORMAT_R32G32B32A32_TYPELESS:case DXGI_FORMAT_R32G32B32A32_FLOAT:hdr=true;return DXGI_FORMAT_R32G32B32A32_FLOAT;
 case DXGI_FORMAT_R11G11B10_FLOAT:hdr=true;return f;
 case DXGI_FORMAT_R10G10B10A2_TYPELESS:case DXGI_FORMAT_R10G10B10A2_UNORM:return DXGI_FORMAT_R10G10B10A2_UNORM;
 case DXGI_FORMAT_R8G8B8A8_TYPELESS:case DXGI_FORMAT_R8G8B8A8_UNORM:case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:return DXGI_FORMAT_R8G8B8A8_UNORM;
 case DXGI_FORMAT_B8G8R8A8_TYPELESS:case DXGI_FORMAT_B8G8R8A8_UNORM:case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:return DXGI_FORMAT_B8G8R8A8_UNORM;
 default:return DXGI_FORMAT_UNKNOWN;}}
struct Constantes{std::uint32_t w,h,hdr,quais;float expo,cas,luma_forca,luma_limite,clar_forca,clar_meios;std::uint32_t mw,mh;float sigma;std::int32_t raio;float niv_preto,niv_branco,tom_gama,tom_expo,tom_sat,tom_bleach,tom_defog,clar_zona,q1,q2;
 std::uint32_t nrost;float niv_saida_preto,niv_saida_branco;std::uint32_t rcont;float rost[16];
 std::uint32_t hsl[9];float hsl_suave;std::uint32_t hsl_pele;
 float temp_r,temp_b;
 std::uint32_t pele_g;};
static_assert(sizeof(Constantes)==232,"58 constantes (+ a tabela = 59 de 64 DWORDs da raiz)");

struct ConstantesM{Constantes c;std::uint32_t ms_w,ms_h,ms_pitch,ms_bits;};
static_assert(sizeof(ConstantesM)==248,"62 constantes");

constexpr unsigned desc_filtros=16u,desc_filtros_m=21u;

inline bool raiz_ceu(ID3D12Device* d,unsigned constantes,unsigned srvs,unsigned uavs,ComPtr<ID3D12RootSignature>& raiz) noexcept {
    D3D12_DESCRIPTOR_RANGE r[3]{};r[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,srvs,0,0,0};r[1]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,uavs,0,0,srvs};r[2]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,3,0,0};
    D3D12_ROOT_PARAMETER p[3]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[0].Constants={0,0,constantes};
    p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[1].DescriptorTable={2,r};p[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[2].DescriptorTable={1,r+2};
    D3D12_STATIC_SAMPLER_DESC ss{};ss.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;ss.AddressU=ss.AddressV=ss.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    ss.MaxLOD=D3D12_FLOAT32_MAX;ss.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
    D3D12_ROOT_SIGNATURE_DESC rd{3,p,1,&ss,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> b,er;
    return SUCCEEDED(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&b,&er))&&SUCCEEDED(d->CreateRootSignature(0,b->GetBufferPointer(),b->GetBufferSize(),IID_PPV_ARGS(&raiz)));}
struct Estado{
 std::mutex m;bool falhou=false;
 ComPtr<ID3D12Device> dev;ComPtr<ID3D12RootSignature> root;ComPtr<ID3D12PipelineState> pso_main,pso_reduz,pso_h,pso_v;ComPtr<ID3D12DescriptorHeap> heap;
 ComPtr<ID3D12Resource> temp,mapa_a,mapa_b;D3D12_RESOURCE_DESC temp_desc{};unsigned mw=0,mh=0;unsigned slot=0;unsigned long long feitos=0;
 bool temp_uav=false;

 ComPtr<ID3D12RootSignature> root_m;ComPtr<ID3D12PipelineState> pso_main_m,pso_reduz_m,pso_h_m,pso_v_m;ComPtr<ID3D12DescriptorHeap> heap_m;
 ComPtr<ID3D12Resource> mapa_ma,mapa_mb;unsigned mw_m=0,mh_m=0;nrestext::Leitor ms;
 unsigned long long m_feitos=0,m_sem=0,m_tabela=0,m_sem_tabela=0;const char* m_motivo=nullptr;

 nrd3d::Aposentados<32> aposentados;
 unsigned long long suspensos_fg=0;bool fg_susp=false;

 ComPtr<ID3D12PipelineState> pso_ceu;ComPtr<ID3D12Resource> ceu_masc;unsigned ceu_w=0,ceu_h=0;bool ceu_pronto=false;
 unsigned long long ceu_feitos=0,ceu_sem=0;const char* ceu_motivo=nullptr;
 void aposentar(ComPtr<ID3D12Resource>& r){
  if(aposentados.aposentar(r,slot))nrlogfila::linha(L"nr_filtros.log","pid=%lu APOSENTADOS cheios: recurso trocado fica vivo (nao solto em voo)\n",GetCurrentProcessId());}
 void soltar_aposentados(){aposentados.soltar(slot);}
};

inline Estado& estado_(){static Estado* e=new Estado;return *e;}
inline void log(const char* t){
 (void)t;
}

inline std::atomic<bool> teste_compilar{false};

inline const char* teste_hlsl=nullptr;
inline bool preparar(Estado& e,ID3D12Device* d){
 if(e.pso_main&&e.dev.Get()==d)return true;
 e.dev=d;
 const char* entradas[9]{"main","reduz","borra_h","borra_v","main_m","reduz_m","borra_h_m","borra_v_m","ceu"};ComPtr<ID3DBlob> cs[9];const void* codigo[9]{};SIZE_T tamanho[9]{};
 bool embutido=false;
#if defined(NR_FILTROS_CS) && NR_FILTROS_CS
 if(!teste_compilar.load()){embutido=true;
  codigo[0]=g_nr_filtros_main;tamanho[0]=sizeof(g_nr_filtros_main);codigo[1]=g_nr_filtros_reduz;tamanho[1]=sizeof(g_nr_filtros_reduz);
  codigo[2]=g_nr_filtros_borra_h;tamanho[2]=sizeof(g_nr_filtros_borra_h);codigo[3]=g_nr_filtros_borra_v;tamanho[3]=sizeof(g_nr_filtros_borra_v);
  codigo[4]=g_nr_filtros_main_m;tamanho[4]=sizeof(g_nr_filtros_main_m);codigo[5]=g_nr_filtros_reduz_m;tamanho[5]=sizeof(g_nr_filtros_reduz_m);
  codigo[6]=g_nr_filtros_borra_h_m;tamanho[6]=sizeof(g_nr_filtros_borra_h_m);codigo[7]=g_nr_filtros_borra_v_m;tamanho[7]=sizeof(g_nr_filtros_borra_v_m);
  codigo[8]=g_nr_filtros_ceu;tamanho[8]=sizeof(g_nr_filtros_ceu);}
#endif
 if(!embutido){
 HMODULE c=LoadLibraryExW(L"d3dcompiler_47.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);    if(!c){log("FALHA d3dcompiler_47 ausente");return false;}
 auto compila=reinterpret_cast<decltype(&D3DCompile)>(GetProcAddress(c,"D3DCompile"));if(!compila){log("FALHA D3DCompile");return false;}
 for(int i=0;i<9;++i){ComPtr<ID3DBlob> err;
  const char* fonte=teste_hlsl?teste_hlsl:hlsl;
  if(i>=4&&i<8&&teste_hlsl&&!std::strstr(teste_hlsl,"main_m")){codigo[i]=nullptr;tamanho[i]=0;continue;}
  if(i==8&&teste_hlsl&&!std::strstr(teste_hlsl,"void ceu(")){codigo[i]=nullptr;tamanho[i]=0;continue;}
  if(FAILED(compila(fonte,std::strlen(fonte),"nr_filtros",nullptr,nullptr,entradas[i],"cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&cs[i],&err))){
   char s[600];std::snprintf(s,sizeof s,"FALHA compilacao %s: %s",entradas[i],err?static_cast<const char*>(err->GetBufferPointer()):"?");log(s);return false;}
  codigo[i]=cs[i]->GetBufferPointer();tamanho[i]=cs[i]->GetBufferSize();}
 }

 if(!raiz_ceu(d,58,2,1,e.root)){log("FALHA root");return false;}
 ComPtr<ID3D12PipelineState>* psos[9]{&e.pso_main,&e.pso_reduz,&e.pso_h,&e.pso_v,&e.pso_main_m,&e.pso_reduz_m,&e.pso_h_m,&e.pso_v_m,&e.pso_ceu};
 for(int i=0;i<4;++i){D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=e.root.Get();pd.CS={codigo[i],tamanho[i]};
  if(FAILED(d->CreateComputePipelineState(&pd,IID_PPV_ARGS(psos[i]->ReleaseAndGetAddressOf())))){log("FALHA pso");return false;}}
 e.pso_ceu.Reset();
 if(codigo[8]){D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=e.root.Get();pd.CS={codigo[8],tamanho[8]};
  if(FAILED(d->CreateComputePipelineState(&pd,IID_PPV_ARGS(e.pso_ceu.ReleaseAndGetAddressOf())))){e.pso_ceu.Reset();log("FALHA pso do ceu: o ceu fica com a nitidez");}}

 if(codigo[4]&&raiz_ceu(d,62,3,2,e.root_m)){bool ok=true;
  for(int i=4;i<8&&ok;++i){D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=e.root_m.Get();pd.CS={codigo[i],tamanho[i]};
   ok=SUCCEEDED(d->CreateComputePipelineState(&pd,IID_PPV_ARGS(psos[i]->ReleaseAndGetAddressOf())));}
  D3D12_DESCRIPTOR_HEAP_DESC hm{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,8*desc_filtros_m,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
  if(!ok||FAILED(d->CreateDescriptorHeap(&hm,IID_PPV_ARGS(&e.heap_m)))){e.pso_main_m.Reset();e.root_m.Reset();log("FALHA mascara (pso/heap): filtros sem a mascara");}}
 D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,8*desc_filtros,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
 if(FAILED(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&e.heap)))){log("FALHA heap");return false;}
 log(embutido?"FILTROS pronto (bytecode do build)":"FILTROS pronto (HLSL compilado agora)");return true;}
inline void teste_recriar(){auto& e=estado_();std::lock_guard<std::mutex> l(e.m);e.pso_main.Reset();e.pso_reduz.Reset();e.pso_h.Reset();e.pso_v.Reset();e.root.Reset();e.dev.Reset();
 e.pso_main_m.Reset();e.pso_reduz_m.Reset();e.pso_h_m.Reset();e.pso_v_m.Reset();e.root_m.Reset();e.pso_ceu.Reset();}

inline ID3D12Resource* teste_mascara=nullptr;inline ID3D12Resource* teste_bordas=nullptr;
inline unsigned teste_mascara_w=0,teste_mascara_h=0,teste_mascara_pitch=0;
inline bool suporta_uav(ID3D12Device* d,DXGI_FORMAT f){
 D3D12_FEATURE_DATA_FORMAT_SUPPORT s{f};if(FAILED(d->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&s,sizeof s)))return false;
 return (s.Support1&D3D12_FORMAT_SUPPORT1_SHADER_LOAD)&&(s.Support2&D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE);}

inline bool mascara_pronta(Estado& e,ID3D12Device* d){
 auto sem=[&](const char* m){++e.m_sem;if(e.m_motivo!=m){e.m_motivo=m;char t[120];std::snprintf(t,sizeof t,"MASCARA_SEM motivo=%s usados=%llu sem=%llu",m,e.m_feitos,e.m_sem);log(t);}return false;};
 if(!e.pso_main_m||!e.heap_m)return sem("sem_pso");
 if(teste_mascara){e.ms.buf=teste_mascara;e.ms.larg=teste_mascara_w;e.ms.alt=teste_mascara_h;e.ms.pitch=teste_mascara_pitch;e.ms.bytes=teste_mascara->GetDesc().Width;e.m_motivo=nullptr;return true;}

 {ComPtr<ID3D12Resource> velho;bool novo=false;
  if(const char* m=nrestext::pronto(e.ms,d,velho,novo))return sem(m);
  if(novo){e.aposentar(velho);char t[120];std::snprintf(t,sizeof t,"MASCARA estencil geracao=%u %ux%u pitch=%u",e.ms.geracao,e.ms.larg,e.ms.alt,e.ms.pitch);log(t);}}
 e.m_motivo=nullptr;return true;}

inline float expo_filtros(){float ex=nrfiltros::expo.load(std::memory_order_relaxed);return ex>0.f&&ex<1e6f?ex:1.f;}
inline nraogpu::Mascara mascara_ao(Estado& e,ID3D12Device* d,const nrfiltros::Parametros& par);
inline void encher_temp(Estado& e,ID3D12Device* d,ID3D12GraphicsCommandList* lista,ID3D12Resource* out,D3D12_RESOURCE_STATES s0,DXGI_FORMAT vf,bool hdr,bool ao,
                        const nrfiltros::Parametros& par,unsigned w,unsigned h){
 auto tr=[&](ID3D12Resource* r,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){nrd3d::transicao(lista,r,a,b);};
 constexpr auto UAV=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;constexpr auto SRV=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
 if(s0==UAV)nrd3d::barreira_uav(lista,out);
 if(ao){tr(out,s0,SRV);tr(e.temp.Get(),D3D12_RESOURCE_STATE_COPY_DEST,UAV);
  nraogpu::aplicar_cor(d,lista,out,e.temp.Get(),vf,w,h,hdr,expo_filtros(),mascara_ao(e,d,par));
  tr(out,SRV,UAV);tr(e.temp.Get(),UAV,SRV);return;}
 tr(out,s0,D3D12_RESOURCE_STATE_COPY_SOURCE);
 D3D12_TEXTURE_COPY_LOCATION a{},b{};a.pResource=e.temp.Get();a.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;b.pResource=out;b.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
 lista->CopyTextureRegion(&a,0,0,0,&b,nullptr);
 tr(out,D3D12_RESOURCE_STATE_COPY_SOURCE,UAV);tr(e.temp.Get(),D3D12_RESOURCE_STATE_COPY_DEST,SRV);}

inline Constantes constantes(const nrfiltros::Parametros& par,unsigned w,unsigned h,bool hdr,unsigned mw,unsigned mh);

inline bool ceu_mascara(Estado& e,ID3D12Device* d,ID3D12GraphicsCommandList* lista){
 e.ceu_pronto=false;
 auto sem=[&](const char* m){++e.ceu_sem;if(e.ceu_motivo!=m){e.ceu_motivo=m;char t[140];std::snprintf(t,sizeof t,"CEU sem mascara motivo=%s feitos=%llu sem=%llu (o ceu leva a nitidez)",m,e.ceu_feitos,e.ceu_sem);log(t);}return false;};
 const auto& en=nrao::entrada;
 if(!e.pso_ceu)return sem("sem_pso");if(!en.valido||!en.prof)return sem("sem_profundidade");
 auto* prof=static_cast<ID3D12Resource*>(en.prof);const auto sp=estado_ffx(en.prof_estado);if(sp==D3D12_RESOURCE_STATES(~0u))return sem("estado");
 if(lista->GetType()==D3D12_COMMAND_LIST_TYPE_COMPUTE&&!nrffx_estado::valido_em_compute(unsigned(sp)))return sem("estado_compute");
 const auto pd=prof->GetDesc();const DXGI_FORMAT pv=nrformato::vista_profundidade(pd.Format);if(!nrformato::entrada(prof,pv))return sem("formato");
 const unsigned rw=(std::min)(en.render_w?en.render_w:unsigned(pd.Width),unsigned(pd.Width)),rh=(std::min)(en.render_h?en.render_h:pd.Height,pd.Height);
 if(rw<2u||rh<2u)return sem("render_pequeno");
 if(!e.ceu_masc||e.ceu_w!=rw||e.ceu_h!=rh){
  e.aposentar(e.ceu_masc);D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC md{};md.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;md.Width=rw;md.Height=rh;
  md.DepthOrArraySize=1;md.MipLevels=1;md.Format=DXGI_FORMAT_R8_UNORM;md.SampleDesc.Count=1;md.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  if(FAILED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&md,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&e.ceu_masc)))){e.ceu_masc.Reset();return sem("recurso");}
  nrd3d::nomear(e.ceu_masc.Get(),L"nr_filtros_ceu");e.ceu_w=rw;e.ceu_h=rh;char t[96];std::snprintf(t,sizeof t,"CEU mascara %ux%u",rw,rh);log(t);}
 const int inv_ini=nrao::opcoes().invertida;const bool inv=(inv_ini>=0?inv_ini:en.invertida)!=0;
 const auto inc=d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);const unsigned base=(e.slot%8u)*desc_filtros;
 auto hc=[&](unsigned i){auto c=e.heap->GetCPUDescriptorHandleForHeapStart();c.ptr+=SIZE_T(base+i)*inc;return c;};
 auto hg=[&](unsigned i){auto c=e.heap->GetGPUDescriptorHandleForHeapStart();c.ptr+=UINT64(base+i)*inc;return c;};
 D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=pv;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.Texture2D.MipLevels=1;
 d->CreateShaderResourceView(prof,&sv,hc(13));
 D3D12_SHADER_RESOURCE_VIEW_DESC sn=sv;sn.Format=DXGI_FORMAT_R8_UNORM;d->CreateShaderResourceView(nullptr,&sn,hc(14));
 D3D12_UNORDERED_ACCESS_VIEW_DESC uv{};uv.Format=DXGI_FORMAT_R8_UNORM;uv.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;d->CreateUnorderedAccessView(e.ceu_masc.Get(),nullptr,&uv,hc(15));
 constexpr auto UAV=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;constexpr auto SRV=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
 auto para_leitura=[](D3D12_RESOURCE_STATES s){return (s&SRV)?s:SRV;};
 nrd3d::transicao(lista,prof,sp,para_leitura(sp));
 Constantes k{};k.w=rw;k.h=rh;k.hdr=inv?1u:0u;
 ID3D12DescriptorHeap* hs[]{e.heap.Get()};lista->SetDescriptorHeaps(1,hs);lista->SetComputeRootSignature(e.root.Get());lista->SetComputeRoot32BitConstants(0,58,&k,0);
 lista->SetPipelineState(e.pso_ceu.Get());lista->SetComputeRootDescriptorTable(1,hg(13));lista->SetComputeRootDescriptorTable(2,hg(14));
 lista->Dispatch((rw+7u)/8u,(rh+7u)/8u,1);
 nrd3d::transicao(lista,prof,para_leitura(sp),sp);nrd3d::transicao(lista,e.ceu_masc.Get(),UAV,SRV);
 e.ceu_pronto=true;e.ceu_motivo=nullptr;
 if(++e.ceu_feitos==1||e.ceu_feitos%6000==0){char t[140];std::snprintf(t,sizeof t,"CEU mascara %llu quadros render %ux%u invertida %d sem=%llu",e.ceu_feitos,rw,rh,int(inv),e.ceu_sem);log(t);}
 return true;}

inline void ceu_srv(Estado& e,ID3D12Device* d,D3D12_CPU_DESCRIPTOR_HANDLE h){
 D3D12_SHADER_RESOURCE_VIEW_DESC sc{};sc.Format=DXGI_FORMAT_R8_UNORM;sc.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sc.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sc.Texture2D.MipLevels=1;
 d->CreateShaderResourceView(e.ceu_pronto?e.ceu_masc.Get():nullptr,&sc,h);}
inline void ceu_fim(Estado& e,ID3D12GraphicsCommandList* lista){
 if(!e.ceu_pronto)return;nrd3d::transicao(lista,e.ceu_masc.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);e.ceu_pronto=false;}
inline unsigned aplicar_m(Estado& e,ID3D12Device* d,ID3D12GraphicsCommandList* lista,ID3D12Resource* out,D3D12_RESOURCE_STATES s0,DXGI_FORMAT vf,bool hdr,
                          const nrfiltros::Parametros& par,unsigned w,unsigned h,unsigned mw,unsigned mh,bool clar,bool ao){
 const auto inc=d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);const unsigned base=(e.slot++%8u)*desc_filtros_m;
 auto hc=[&](unsigned i){auto c=e.heap_m->GetCPUDescriptorHandleForHeapStart();c.ptr+=SIZE_T(base+i)*inc;return c;};
 auto hg=[&](unsigned i){auto c=e.heap_m->GetGPUDescriptorHandleForHeapStart();c.ptr+=UINT64(base+i)*inc;return c;};
 D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=vf;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.Texture2D.MipLevels=1;
 D3D12_UNORDERED_ACCESS_VIEW_DESC uv{};uv.Format=vf;uv.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
 auto sm=sv;sm.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;D3D12_UNORDERED_ACCESS_VIEW_DESC um{};um.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;um.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
 D3D12_SHADER_RESOURCE_VIEW_DESC sb{};sb.Format=DXGI_FORMAT_R32_TYPELESS;sb.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;sb.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
 sb.Buffer.NumElements=UINT(e.ms.bytes/4u);sb.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;
 for(unsigned t=0;t<4;++t)d->CreateShaderResourceView(e.ms.buf.Get(),&sb,hc(5*t+2));
 d->CreateShaderResourceView(e.temp.Get(),&sv,hc(15));d->CreateUnorderedAccessView(out,nullptr,&uv,hc(18));

 const auto& bq=nrestext::bordas_quadro;
 ID3D12Resource* tab=teste_bordas?teste_bordas:(bq.tabela&&bq.w==w&&bq.h==h&&bq.execucao==e.ms.exec_ult&&!teste_mascara)?bq.tabela:nullptr;
 {D3D12_UNORDERED_ACCESS_VIEW_DESC ut{};ut.Format=DXGI_FORMAT_R32_UINT;ut.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
  for(unsigned t=0;t<4;++t)d->CreateUnorderedAccessView(t==3u?tab:nullptr,nullptr,&ut,hc(5*t+4));}
 if(tab)++e.m_tabela;else ++e.m_sem_tabela;
 if(clar){
  d->CreateShaderResourceView(e.temp.Get(),&sv,hc(0));d->CreateShaderResourceView(e.temp.Get(),&sv,hc(1));d->CreateUnorderedAccessView(e.mapa_ma.Get(),nullptr,&um,hc(3));
  d->CreateShaderResourceView(e.mapa_ma.Get(),&sm,hc(5));d->CreateShaderResourceView(e.mapa_ma.Get(),&sm,hc(6));d->CreateUnorderedAccessView(e.mapa_mb.Get(),nullptr,&um,hc(8));
  d->CreateShaderResourceView(e.mapa_mb.Get(),&sm,hc(10));d->CreateShaderResourceView(e.mapa_mb.Get(),&sm,hc(11));d->CreateUnorderedAccessView(e.mapa_ma.Get(),nullptr,&um,hc(13));
  d->CreateShaderResourceView(e.mapa_ma.Get(),&sm,hc(16));
 }else d->CreateShaderResourceView(e.temp.Get(),&sv,hc(16));
 ceu_srv(e,d,hc(20));
 auto tr=[&](ID3D12Resource* r,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){nrd3d::transicao(lista,r,a,b);};
 auto ub=[&](ID3D12Resource* r){D3D12_RESOURCE_BARRIER u{};u.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;u.UAV.pResource=r;lista->ResourceBarrier(1,&u);};
 constexpr auto UAV=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;constexpr auto SRV=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
 encher_temp(e,d,lista,out,s0,vf,hdr,ao,par,w,h);
 ID3D12DescriptorHeap* hs[]{e.heap_m.Get()};lista->SetDescriptorHeaps(1,hs);lista->SetComputeRootSignature(e.root_m.Get());lista->SetComputeRootDescriptorTable(2,hg(20));
 ConstantesM k{constantes(par,w,h,hdr,mw,mh),e.ms.larg,e.ms.alt,e.ms.pitch,par.mascara|(tab?256u:0u)};
 lista->SetComputeRoot32BitConstants(0,62,&k,0);
 if(clar){
  const unsigned gx=(mw+7)/8,gy=(mh+7)/8;
  lista->SetPipelineState(e.pso_reduz_m.Get());lista->SetComputeRootDescriptorTable(1,hg(0));lista->Dispatch(gx,gy,1);
  tr(e.mapa_ma.Get(),UAV,SRV);
  lista->SetPipelineState(e.pso_h_m.Get());lista->SetComputeRootDescriptorTable(1,hg(5));lista->Dispatch(gx,gy,1);
  tr(e.mapa_mb.Get(),UAV,SRV);tr(e.mapa_ma.Get(),SRV,UAV);
  lista->SetPipelineState(e.pso_v_m.Get());lista->SetComputeRootDescriptorTable(1,hg(10));lista->Dispatch(gx,gy,1);
  tr(e.mapa_ma.Get(),UAV,SRV);
 }
 if(tab)ub(tab);
 lista->SetPipelineState(e.pso_main_m.Get());lista->SetComputeRootDescriptorTable(1,hg(15));lista->Dispatch((w+7)/8,(h+7)/8,1);
 ub(out);tr(out,UAV,s0);tr(e.temp.Get(),SRV,D3D12_RESOURCE_STATE_COPY_DEST);ceu_fim(e,lista);
 if(clar){tr(e.mapa_ma.Get(),SRV,UAV);tr(e.mapa_mb.Get(),SRV,UAV);}
 if(++e.m_feitos==1||e.m_feitos%6000==0){char t[200];std::snprintf(t,sizeof t,"FILTROS com a mascara %llu quadros quais=%u bits=0x%02X sem=%llu tabela_do_backend=%llu sem_tabela=%llu",e.m_feitos,par.quais,par.mascara,e.m_sem,e.m_tabela,e.m_sem_tabela);log(t);}
 return 0;}

inline std::uint32_t meia(float v){if(!(v>0.f))return 0u;std::uint32_t u;std::memcpy(&u,&v,4);const int e=int((u>>23)&255u)-127+15;
 if(e<=0)return 0u;if(e>=31)return 0x7bffu;return (std::uint32_t(e)<<10)|((u>>13)&1023u);}

inline Constantes constantes(const nrfiltros::Parametros& par,unsigned w,unsigned h,bool hdr,unsigned mw,unsigned mh){
  float ex=nrfiltros::expo.load(std::memory_order_relaxed);if(!(ex>0.f&&ex<1e6f))ex=1.f;
  const float sigma=(std::max)(.75f,par.clar_sigma*float(h)/2160.f);
  Constantes k{w,h,hdr?1u:0u,par.quais,ex,par.cas,par.luma_forca,par.luma_limite,par.clar_forca,par.clar_meios,mw,mh,sigma,(std::min)(24,int(std::ceil(3.f*sigma))),
                par.niv_preto,par.niv_branco,par.tom_gama,par.tom_expo,par.tom_sat,par.tom_bleach,par.tom_defog,par.clar_zona,par.local?1.f:0.f,
                par.contorno?par.contorno_forca:0.f};
  k.rcont=meia(par.contorno_min)|(meia(par.contorno_max)<<16);
  k.niv_saida_preto=par.niv_saida_preto;k.niv_saida_branco=par.niv_saida_branco;
  if(nrfiltros::areas_rosto(par))for(unsigned i=0;i<nrfiltros::n_cabecas&&i<4u;++i){const auto& c=nrfiltros::cabecas[i];
   if(!(c.r>1.f))continue;float* r=k.rost+4u*k.nrost++;r[0]=c.cx;r[1]=c.cy;r[2]=c.r;r[3]=c.ry>1.f?c.ry:0.f;}

  for(unsigned f=0;f<9u;++f){std::uint32_t v=0;for(unsigned c=0;c<3u;++c){const float x=par.hsl[f][c];const long n=std::isfinite(x)?std::lround(x):0;
    v|=std::uint32_t((n<-127?-127:n>127?127:n)+128)<<(8u*c);}k.hsl[f]=v;}
  k.hsl_suave=par.hsl_suave>0.f&&par.hsl_suave<=1.f?par.hsl_suave:.6f;k.hsl_pele=par.hsl_pele?1u:0u;
  k.temp_r=par.temp[0];k.temp_b=par.temp[2];
  if(par.quais&128u){
   auto cod=[](float v,float passo,long meio,long max){const long n=std::isfinite(v)?std::lround(v/passo)+meio:meio;return std::uint32_t(n<0?0:n>max?max:n);};
   k.hsl_pele|=(cod(par.pele_ev,.02f,25,50)<<8)|(cod(par.pele_sat,1.f,30,60)<<14)|(cod(par.pele_matiz,.5f,20,40)<<20);
   k.pele_g=meia(par.pele_g[0])|(meia(par.pele_g[2])<<16);
}
  if((par.quais&(128u|512u|8192u))&&!nrfiltros::nitidez_rostos(par))k.quais|=256u;
  return k;}

inline nraogpu::Mascara mascara_ao(Estado& e,ID3D12Device* d,const nrfiltros::Parametros& par){

 nraogpu::Mascara m{};if(!(nrao::opcoes().personagem>0.f||(nraogpu::estado_().il.tex&&nraogpu::estado_().il.pers>0.f))||!par.mascara||!mascara_pronta(e,d))return m;
 m.buf=e.ms.buf.Get();m.w=e.ms.larg;m.h=e.ms.alt;m.pitch=e.ms.pitch;m.bits=par.mascara;m.bytes=e.ms.bytes;return m;}

inline bool fg_do_mod_ligado() noexcept {return nrfgext::disponivel()&&(nrfgext::ligado()||nrfgext::gerando());}

inline bool suspenso_pelo_fg(Estado& e) noexcept {
 const bool s=!nrfiltros::com_fg.load(std::memory_order_relaxed)&&fg_do_mod_ligado();
 if(s!=e.fg_susp){e.fg_susp=s;char t[200];
  std::snprintf(t,sizeof t,s?"FILTROS suspensos: geracao de quadros do mod ligada (filtros e AO voltam com ela desligada; NeuralFiltrosComFg=1 libera)"
                            :"FILTROS de volta: geracao de quadros desligada (%llu quadros suspensos ate aqui)",e.suspensos_fg);log(t);}
 if(s){++e.suspensos_fg;nraogpu::estado_().hist_ok=false;nrilgpu::estado_().hist_ok=false;}
 return s;}
inline unsigned aplicar(void* command,void* output,unsigned ffx_state) noexcept {
 try{
  auto par=nrfiltros::parametros();if(!par.quais||!command||!output)return 1;

  const nrfiltros::Parametros par_luz=par;
  const bool quer_ao=(par.quais&1024u)!=0u,quer_il=(par.quais&4096u)!=0u;par.quais&=~(1024u|4096u);
  if(nrgpu::perdida())return 1;
  auto& e=estado_();std::lock_guard<std::mutex> l(e.m);if(e.falhou)return 1;
  if(suspenso_pelo_fg(e))return 1;
  auto* lista=static_cast<ID3D12GraphicsCommandList*>(command);auto* out=static_cast<ID3D12Resource*>(output);
  const bool ao_pode=(quer_ao||quer_il)&&nraogpu::pode(lista);if(!par.quais&&!ao_pode)return 1;
  const auto s0=estado_ffx(ffx_state);if(s0==D3D12_RESOURCE_STATES(~0u))return 1;

  if(lista->GetType()==D3D12_COMMAND_LIST_TYPE_COMPUTE&&!nrffx_estado::valido_em_compute(unsigned(s0)))return 1;
  ComPtr<ID3D12Device> d;if(FAILED(out->GetDevice(IID_PPV_ARGS(&d))))return 1;
  if(!preparar(e,d.Get())){e.falhou=true;return 2;}
  const auto od=out->GetDesc();bool hdr=false;const auto vf=vista(od.Format,hdr);
  if(od.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||od.DepthOrArraySize!=1||od.MipLevels!=1||od.SampleDesc.Count!=1||vf==DXGI_FORMAT_UNKNOWN||
     !(od.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)||od.Width>16384||od.Height>16384||!suporta_uav(d.Get(),vf)){
   char s[160];std::snprintf(s,sizeof s,"FALHA saida sem UAV tipado: formato=%u flags=0x%x mips=%u",unsigned(od.Format),unsigned(od.Flags),unsigned(od.MipLevels));log(s);e.falhou=true;return 2;}
  e.soltar_aposentados();
  const unsigned w=unsigned(od.Width),h=od.Height,mw=(w+3u)/4u,mh=(h+3u)/4u;

  const bool masc=par.mascara!=0u&&par.local&&(par.quais&7u)!=0u&&mascara_pronta(e,d.Get());
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  if(!e.temp||e.temp_desc.Width!=od.Width||e.temp_desc.Height!=od.Height||e.temp_desc.Format!=od.Format||(ao_pode&&!e.temp_uav)){

   const bool tu=ao_pode||e.temp_uav;D3D12_RESOURCE_DESC td=od;td.Flags=tu?D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS:D3D12_RESOURCE_FLAG_NONE;td.Alignment=0;e.temp_uav=tu;
   e.aposentar(e.temp);if(FAILED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&e.temp)))){log("FALHA textura");e.falhou=true;return 2;}nrd3d::nomear(e.temp.Get(),L"nr_filtros_temp");
   e.temp_desc=od;char s[160];std::snprintf(s,sizeof s,"FILTROS saida %ux%u formato=%u hdr=%d estado_ffx=0x%x",w,h,unsigned(od.Format),int(hdr),ffx_state);log(s);}
  const bool clar=(par.quais&4u)!=0;
  if(masc&&clar&&(!e.mapa_ma||e.mw_m!=mw||e.mh_m!=mh)){
   D3D12_RESOURCE_DESC md{};md.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;md.Width=mw;md.Height=mh;md.DepthOrArraySize=1;md.MipLevels=1;
   md.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;md.SampleDesc.Count=1;md.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
   e.aposentar(e.mapa_ma);e.aposentar(e.mapa_mb);
   if(FAILED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&md,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&e.mapa_ma)))||
      FAILED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&md,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&e.mapa_mb)))){log("FALHA mapa da mascara");e.falhou=true;return 2;}
   nrd3d::nomear(e.mapa_ma.Get(),L"nr_filtros_mapa_ma");nrd3d::nomear(e.mapa_mb.Get(),L"nr_filtros_mapa_mb");e.mw_m=mw;e.mh_m=mh;}
  if(!masc&&clar&&(!e.mapa_a||e.mw!=mw||e.mh!=mh)){
   D3D12_RESOURCE_DESC md{};md.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;md.Width=mw;md.Height=mh;md.DepthOrArraySize=1;md.MipLevels=1;
   md.Format=DXGI_FORMAT_R16_FLOAT;md.SampleDesc.Count=1;md.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
   e.aposentar(e.mapa_a);e.aposentar(e.mapa_b);
   if(FAILED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&md,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&e.mapa_a)))||
      FAILED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&md,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&e.mapa_b)))){log("FALHA mapa");e.falhou=true;return 2;}nrd3d::nomear(e.mapa_a.Get(),L"nr_filtros_mapa_a");nrd3d::nomear(e.mapa_b.Get(),L"nr_filtros_mapa_b");
   e.mw=mw;e.mh=mh;}
  nrtgpu::Escopo tg(lista,nrtgpu::Filtros);

  const bool prof_ok=ao_pode&&nraogpu::calcular(d.Get(),lista,par_luz,quer_ao);

  const bool il=prof_ok&&quer_il&&nrilgpu::calcular(d.Get(),lista,par_luz,out,s0,vf,hdr,w,h);
  if(quer_il&&!il)nrilgpu::estado_().hist_ok=false;
  const bool ao=prof_ok&&(quer_ao||il);if(prof_ok&&!ao)nraogpu::cancelar(lista);
  if(!par.quais){
   if(!ao)return 1;

   encher_temp(e,d.Get(),lista,out,s0,vf,hdr,false,par,w,h);
   nraogpu::aplicar_cor(d.Get(),lista,e.temp.Get(),out,vf,w,h,hdr,expo_filtros(),mascara_ao(e,d.Get(),par_luz),true);
   nrd3d::barreira_uav(lista,out);nrd3d::transicao(lista,out,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,s0);
   nrd3d::transicao(lista,e.temp.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
   if(++e.feitos==1||e.feitos%6000==0){char s[96];std::snprintf(s,sizeof s,"FILTROS aplicados %llu quadros quais=%u (so AO/IL)",e.feitos,(quer_ao?1024u:0u)|(il?4096u:0u));log(s);}
   return 0;}

  if(par.ceu&&par.local&&(par.quais&7u)!=0u&&ceu_mascara(e,d.Get(),lista))par.quais|=2048u;
  if(masc)return aplicar_m(e,d.Get(),lista,out,s0,vf,hdr,par,w,h,mw,mh,clar,ao);

  const auto inc=d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);const unsigned base=(e.slot++%8u)*desc_filtros;
  auto hc=[&](unsigned i){auto c=e.heap->GetCPUDescriptorHandleForHeapStart();c.ptr+=SIZE_T(base+i)*inc;return c;};
  auto hg=[&](unsigned i){auto c=e.heap->GetGPUDescriptorHandleForHeapStart();c.ptr+=UINT64(base+i)*inc;return c;};
  D3D12_SHADER_RESOURCE_VIEW_DESC sv{};sv.Format=vf;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.Texture2D.MipLevels=1;
  D3D12_UNORDERED_ACCESS_VIEW_DESC uv{};uv.Format=vf;uv.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
  auto sm=sv;sm.Format=DXGI_FORMAT_R16_FLOAT;D3D12_UNORDERED_ACCESS_VIEW_DESC um{};um.Format=DXGI_FORMAT_R16_FLOAT;um.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
  d->CreateShaderResourceView(e.temp.Get(),&sv,hc(9));d->CreateUnorderedAccessView(out,nullptr,&uv,hc(11));
  if(clar){
   d->CreateShaderResourceView(e.temp.Get(),&sv,hc(0));d->CreateShaderResourceView(e.temp.Get(),&sv,hc(1));d->CreateUnorderedAccessView(e.mapa_a.Get(),nullptr,&um,hc(2));
   d->CreateShaderResourceView(e.mapa_a.Get(),&sm,hc(3));d->CreateShaderResourceView(e.mapa_a.Get(),&sm,hc(4));d->CreateUnorderedAccessView(e.mapa_b.Get(),nullptr,&um,hc(5));
   d->CreateShaderResourceView(e.mapa_b.Get(),&sm,hc(6));d->CreateShaderResourceView(e.mapa_b.Get(),&sm,hc(7));d->CreateUnorderedAccessView(e.mapa_a.Get(),nullptr,&um,hc(8));
   d->CreateShaderResourceView(e.mapa_a.Get(),&sm,hc(10));
  }else d->CreateShaderResourceView(e.temp.Get(),&sv,hc(10));
  ceu_srv(e,d.Get(),hc(12));
  auto tr=[&](ID3D12Resource* r,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){nrd3d::transicao(lista,r,a,b);};
  auto ub=[&](ID3D12Resource* r){D3D12_RESOURCE_BARRIER u{};u.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;u.UAV.pResource=r;lista->ResourceBarrier(1,&u);};
  constexpr auto UAV=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;constexpr auto SRV=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

  encher_temp(e,d.Get(),lista,out,s0,vf,hdr,ao,par,w,h);
  ID3D12DescriptorHeap* hs[]{e.heap.Get()};lista->SetDescriptorHeaps(1,hs);lista->SetComputeRootSignature(e.root.Get());lista->SetComputeRootDescriptorTable(2,hg(12));
  const Constantes k=constantes(par,w,h,hdr,mw,mh);
  lista->SetComputeRoot32BitConstants(0,58,&k,0);
  if(clar){
   const unsigned gx=(mw+7)/8,gy=(mh+7)/8;
   lista->SetPipelineState(e.pso_reduz.Get());lista->SetComputeRootDescriptorTable(1,hg(0));lista->Dispatch(gx,gy,1);
   tr(e.mapa_a.Get(),UAV,SRV);
   lista->SetPipelineState(e.pso_h.Get());lista->SetComputeRootDescriptorTable(1,hg(3));lista->Dispatch(gx,gy,1);
   tr(e.mapa_b.Get(),UAV,SRV);tr(e.mapa_a.Get(),SRV,UAV);
   lista->SetPipelineState(e.pso_v.Get());lista->SetComputeRootDescriptorTable(1,hg(6));lista->Dispatch(gx,gy,1);
   tr(e.mapa_a.Get(),UAV,SRV);
  }
  lista->SetPipelineState(e.pso_main.Get());lista->SetComputeRootDescriptorTable(1,hg(9));lista->Dispatch((w+7)/8,(h+7)/8,1);
  ub(out);tr(out,UAV,s0);tr(e.temp.Get(),SRV,D3D12_RESOURCE_STATE_COPY_DEST);ceu_fim(e,lista);
  if(clar){tr(e.mapa_a.Get(),SRV,UAV);tr(e.mapa_b.Get(),SRV,UAV);}
  if(++e.feitos==1||e.feitos%6000==0){char s[96];std::snprintf(s,sizeof s,"FILTROS aplicados %llu quadros quais=%u",e.feitos,par.quais);log(s);}
  return 0;
 }catch(...){estado_().falhou=true;return 2;}
}
}
