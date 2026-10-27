#pragma once

#include "ao.h"
#include "filtros.h"
#include "d3d12_comum.h"
#include "log_fila.h"
#include "pasta_helper.h"
#include "../source/implementation/async-reprojection/formatos.h"
#include <windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#if defined(NR_FILTROS_CS) && NR_FILTROS_CS
#include "nr_ao_ao_prof.h"
#include "nr_ao_ao_principal.h"
#include "nr_ao_ao_denoise.h"
#include "nr_ao_ao_temporal.h"
#include "nr_ao_ao_aplica.h"
#endif
namespace nraogpu {
using Microsoft::WRL::ComPtr;
inline const char* hlsl=R"(
#define PI 3.14159265358979
#define PI2 1.5707963267949
static const float CEU=60000.0;
cbuffer K:register(b0){uint W,H,Wa,Ha,S,modo,fatias,passos;float lc,le,metros;uint inv;float mulx,muly,addx,addy;
 float raio,raio_mult,falloff,distrib,finos,potencia,mip_off,raio_max_px,esp;uint esp_lin,quadro;float beta;uint final_;
 float tol,margem,qmax;uint reset_,temporal;float mvx,mvy;uint ver;float djx,djy,csig,qest;uint pviz;};   // lua-ao (26/10): djx..pviz
cbuffer KA:register(b1){uint ow,oh,hdr;float expo;uint rW,rH,aW,aH,aS;float jx,jy;uint ups;float forca,intens,d_ini,d_fim,d_min,pele_k,pers_k,claro_k,c_ini,c_fim;
 uint mb,ms_w,ms_h,ms_pitch,ms_bits,nrost,ver_a,tem_entrada,no_lugar,pad1;float4 rost[4];
 uint il0,il1,il2,il3,il4,il5,il6,il7,il8;};   // ssil (26/10): a IL (il_gpu.h; il2..il8 = 2 meias cada)
Texture2D<float> profJogo:register(t0);Texture2D<float> zmip:register(t1);Texture2D<float> aoIn:register(t2);Texture2D<float> bordasIn:register(t3);
Texture2D<float4> mvTex:register(t4);Texture2D<float4> histAnt:register(t5);Texture2D<float4> cor:register(t6);Texture2D<float4> histCur:register(t7);
ByteAddressBuffer msk:register(t8);Texture2D<float4> corEntrada:register(t9);Texture2D<float4> dbgIn:register(t10);Texture2D<float4> ilTex:register(t11);Texture2D<float> aoCru:register(t12);
RWTexture2D<float> zm0:register(u0);RWTexture2D<float> zm1:register(u1);RWTexture2D<float> zm2:register(u2);RWTexture2D<float> zm3:register(u3);RWTexture2D<float> zm4:register(u4);
RWTexture2D<float> aoOut:register(u5);RWTexture2D<unorm float> bordasOut:register(u6);RWTexture2D<unorm float4> dbgOut:register(u7);RWTexture2D<float4> histOut:register(u8);
RWTexture2D<float4> dst:register(u9);
// profundidade crua -> metros (a conta do FSR); ceu (d = 0 invertida, d = 1 normal) e NaN = CEU
float zlin(float d){bool ceu=inv!=0u?!(d>1e-7):!(d<1.0-1e-7);if(ceu)return CEU;float z=abs(le/(d+lc))*metros;return z==z?clamp(z,1e-4,CEU):CEU;}
// XeGTAO_DepthMIPFilter (MIT): media ponderada, o filho mais perto que o mais longe do bloco perde peso alem de 0,75 x raio
float filtro4(float a,float b,float c,float d){float mx=max(max(a,b),max(c,d));float er=0.75*raio*raio_mult;float fr=max(falloff*er,1e-5);float ff=er*(1.0-falloff);
 float4 z4=float4(a,b,c,d);float4 w4=saturate((mx-z4)*(-1.0/fr)+(ff/fr+1.0));return dot(w4,z4)/max(dot(w4,1.0),1e-6);}
groupshared float gz[8][8];
[numthreads(8,8,1)]void ao_prof(uint3 id:SV_DispatchThreadID,uint3 gt:SV_GroupThreadID){
 int2 b=int2(id.xy),p=b*2,lim=int2(W,H)-1;
 float z0=zlin(profJogo.Load(int3(min(p,lim),0))),z1=zlin(profJogo.Load(int3(min(p+int2(1,0),lim),0))),z2=zlin(profJogo.Load(int3(min(p+int2(0,1),lim),0))),z3=zlin(profJogo.Load(int3(min(p+int2(1,1),lim),0)));
 zm0[p]=z0;zm0[p+int2(1,0)]=z1;zm0[p+int2(0,1)]=z2;zm0[p+int2(1,1)]=z3;
 float m1=filtro4(z0,z1,z2,z3);zm1[b]=m1;gz[gt.x][gt.y]=m1;GroupMemoryBarrierWithGroupSync();
 if(all((gt.xy%2u)==0u)){float m2=filtro4(gz[gt.x][gt.y],gz[gt.x+1][gt.y],gz[gt.x][gt.y+1],gz[gt.x+1][gt.y+1]);zm2[b/2]=m2;gz[gt.x][gt.y]=m2;}
 GroupMemoryBarrierWithGroupSync();
 if(all((gt.xy%4u)==0u)){float m3=filtro4(gz[gt.x][gt.y],gz[gt.x+2][gt.y],gz[gt.x][gt.y+2],gz[gt.x+2][gt.y+2]);zm3[b/4]=m3;gz[gt.x][gt.y]=m3;}
 GroupMemoryBarrierWithGroupSync();
 if(all((gt.xy%8u)==0u)){float m4=filtro4(gz[gt.x][gt.y],gz[gt.x+4][gt.y],gz[gt.x][gt.y+4],gz[gt.x+4][gt.y+4]);zm4[b/8]=m4;}}
float zm(int2 q,uint m){int2 dm=int2(max(W>>m,1u),max(H>>m,1u));return zmip.Load(int3(clamp(q,int2(0,0),dm-1),m));}
// pelo (26/10): texel instavel = material fino (pelo, folhagem): poucas bordas de profundidade em volta (a conta do qest) E borda nos dois
// eixos (L/R e T/B); a quina de parede/chao e a silhueta reta so tem borda num eixo e ficam de fora (o escurecimento estatico fica)
float instavel(float4 e){return (1.0-smoothstep(2.0,3.5,dot(e,1.0)))*saturate(2.0*(1.0-e.x*e.y))*saturate(2.0*(1.0-e.z*e.w));}
float3 vpos(float2 uv,float z){return float3((float2(mulx,muly)*uv+float2(addx,addy))*z,z);}
// XeGTAO_FastACos (entrada [-1,1], saida [0,pi])
float facos(float x){float a=min(abs(x),1.0);float r=(-0.156583*a+PI2)*sqrt(1.0-a);return x>=0.0?r:PI-r;}
// curva de Hilbert 64x64 pela definicao classica (nossa; a mesma de nrao::hilbert)
uint hilbert(uint2 p){uint x=p.x&63u,y=p.y&63u,d=0u;[unroll]for(uint s=32u;s>0u;s>>=1){uint rx=(x&s)!=0u?1u:0u,ry=(y&s)!=0u?1u:0u;d+=s*s*((3u*rx)^ry);
 x&=s-1u;y&=s-1u;if(ry==0u){if(rx==1u){x=s-1u-x;y=s-1u-y;}uint t=x;x=y;y=t;}}return d;}
// XeGTAO_CalculateEdges / PackEdges / UnpackEdges / CalculateNormal (MIT)
float4 bordas4(float zc,float zl,float zr,float zt,float zb){float4 e=float4(zl,zr,zt,zb)-zc;float slr=(e.y-e.x)*0.5,stb=(e.w-e.z)*0.5;
 float4 a=e+float4(slr,-slr,stb,-stb);e=min(abs(e),abs(a));return saturate(1.25-e/(zc*0.011));}
float empacota(float4 e){e=round(saturate(e)*2.9);return dot(e,float4(64.0/255.0,16.0/255.0,4.0/255.0,1.0/255.0));}
float4 desempacota(float v){uint p=uint(v*255.5);return saturate(float4((p>>6)&3u,(p>>4)&3u,(p>>2)&3u,p&3u)/3.0);}
float3 normal4(float4 e,float3 c,float3 l,float3 r,float3 t,float3 b){float4 w=saturate(float4(e.x*e.z,e.z*e.y,e.y*e.w,e.w*e.x)+0.01);
 l=normalize(l-c);r=normalize(r-c);t=normalize(t-c);b=normalize(b-c);return normalize(w.x*cross(l,t)+w.y*cross(t,r)+w.z*cross(r,b)+w.w*cross(b,l));}
float iarc(float h,float n,float cn){return (cn+2.0*h*sin(n)-cos(2.0*h-n))/4.0;}
// posicao no hemisferio [n - pi/2, n + pi/2]: uniforme (modo 1) ou pela integral de cosseno (modo 2; ver nrao::u_cos)
float Ah(float h,float n,float cn){float i=iarc(h,n,cn);return h>=0.0?i:-i;}
float umap(float h,float n,float cn){float lo=n-PI2;h=clamp(h,lo,n+PI2);if(modo==1u)return saturate((h-lo)/PI);float t=cn+n*sin(n);
 return t>1e-5?saturate((Ah(h,n,cn)-Ah(lo,n,cn))/t):saturate((h-lo)/PI);}
uint setores(float a,float b){if(b-a<0.5/32.0)return 0u;uint s0=min(uint(floor(a*32.0)),31u);uint c=min(uint(ceil((b-a)*32.0)),32u-s0);return c>=32u?0xFFFFFFFFu:(((1u<<c)-1u)<<s0);}
[numthreads(8,8,1)]void ao_principal(uint3 id:SV_DispatchThreadID){
 if(id.x>=Wa||id.y>=Ha)return;int2 pa=int2(id.xy);int2 r=min(pa*int(S),int2(W,H)-1);
 float zc=zm(r,0u);
 if(zc>=CEU){aoOut[pa]=1.0;bordasOut[pa]=1.0;if(ver>=2u&&ver<=5u)dbgOut[pa]=float4(ver==2u?1.0:0.0,ver==2u?1.0:0.0,ver==2u?1.0:0.0,1.0);return;}
 int s=int(S);float zl=zm(r-int2(s,0),0u),zr=zm(r+int2(s,0),0u),zt=zm(r-int2(0,s),0u),zb=zm(r+int2(0,s),0u);
 float4 e=bordas4(zc,zl,zr,zt,zb);bordasOut[pa]=empacota(e);
 // pelo (26/10): texel instavel (material fino) = espessura x 0,15 e ruido fixo (pviz bit 1)
 float inst=(pviz&2u)!=0u?instavel(desempacota(empacota(e))):0.0;
 float2 px=1.0/float2(W,H);float2 uv=(float2(r)+0.5)*px;
 float3 nrm=normal4(e,vpos(uv,zc),vpos(uv-float2(s,0)*px,zl),vpos(uv+float2(s,0)*px,zr),vpos(uv-float2(0,s)*px,zt),vpos(uv+float2(0,s)*px,zb));
 zc*=0.99999;float3 c=vpos(uv,zc);float3 v=normalize(-c);
 float er=raio*raio_mult;float fr=max(falloff*er,1e-5),ff=er*(1.0-falloff);float fmul=-1.0/fr,fadd=ff/fr+1.0;
 // raio na tela (px de render), limitado (RaioMaxPx a 1080p, escala com a altura): arma em 1a pessoa / close nao explodem
 float ssr=min(er/max(zc*mulx*px.x,1e-9),raio_max_px*float(H)/1080.0);
 float vis=saturate((10.0-ssr)/100.0)*0.5;
 float2 nz=frac(0.5+float(hilbert(uint2(pa))+288u*((inst>0.5?0u:quadro)%64u))*float2(0.7548776662,0.5698402910));
 float minS=1.3/max(ssr,1e-3);float esp_v=(esp_lin!=0u?esp*zc:esp)*lerp(1.0,0.15,inst);float fin=modo==0u?finos:0.0;
 [loop]for(uint fi=0u;fi<fatias;++fi){
  float phi=(float(fi)+nz.x)/float(fatias)*PI;float cp=cos(phi),sp=sin(phi);float2 om=float2(cp,-sp)*ssr;
  float3 dv=float3(cp,sp,0.0);float3 odv=dv-dot(dv,v)*v;float3 ax=normalize(cross(odv,v));
  float3 pn=nrm-ax*dot(nrm,ax);float pl=length(pn);float cn=saturate(dot(pn,v)/max(pl,1e-6));float n=sign(dot(odv,pn))*facos(cn);
  float lo0=cos(n+PI2),lo1=cos(n-PI2),hc0=lo0,hc1=lo1;uint masc=0u;
  [loop]for(uint k=0u;k<passos;++k){
   float sn=frac(nz.y+float(fi+k*passos)*0.6180339887498948);float sv=pow((float(k)+sn)/float(passos),distrib)+minS;
   float2 off=sv*om;uint m=uint(clamp(log2(max(length(off),1e-6))-mip_off,0.0,4.0)+0.5);off=round(off);
   int2 dm=int2(max(W>>m,1u),max(H>>m,1u));
   [unroll]for(int lado=0;lado<2;++lado){
    float2 su=uv+(lado==0?off:-off)*px;float sz=zmip.Load(int3(clamp(int2(floor(su*float2(dm))),int2(0,0),dm-1),m));
    float3 p3=vpos(su,sz);float3 dd=p3-c;float3 hv=dd/max(length(dd),1e-9);
    float wgt=saturate(length(float3(dd.xy,dd.z*(1.0+fin)))*fmul+fadd);
    float lowc=lado==0?lo0:lo1;float shc=lerp(lowc,dot(hv,v),wgt);
    if(modo==0u){if(lado==0)hc0=max(hc0,shc);else hc1=max(hc1,shc);}
    else{float3 bd=p3+normalize(p3)*esp_v-c;float bc=lerp(lowc,dot(bd/max(length(bd),1e-9),v),wgt);
     float af=facos(shc),ab=facos(bc);float a0=min(af,ab),a1=max(af,ab);
     masc|=lado==0?setores(umap(a0,n,cn),umap(a1,n,cn)):setores(umap(-a1,n,cn),umap(-a0,n,cn));}}}
  float plf=lerp(pl,1.0,0.05);
  if(modo==0u)vis+=plf*(iarc(-facos(hc1),n,cn)+iarc(facos(hc0),n,cn));
  else{float livre=1.0-float(countbits(masc))/32.0;vis+=modo==1u?livre:plf*(cn+n*sin(n))*livre;}}   // uniforme: a fracao livre da fatia (como o SSRT3: plano livre = 1)
 vis/=float(fatias);vis=max(0.03,pow(max(vis,0.0),potencia));aoOut[pa]=min(vis,1.5);
 if(ver==2u)dbgOut[pa]=float4(saturate(vis).xxx,1.0);
 else if(ver==3u)dbgOut[pa]=float4(nrm*0.5+0.5,1.0);
 else if(ver==4u)dbgOut[pa]=float4(saturate(log2(zc+1.0)/log2(1001.0)).xxx,1.0);
 else if(ver==5u)dbgOut[pa]=float4(e.x*e.y,e.z*e.w,dot(e,0.25),1.0);}
// XeGTAO_Denoise (MIT), um pixel por thread: cruz pelas bordas (simetrizadas com as do vizinho), diagonais 0,425 x produto das bordas,
// centro beta (beta/5 nas passadas do meio); vaza um pouco com 3-4 bordas (menos serrilhado no espaco e no tempo).
float aoL(int2 q){return aoIn.Load(int3(clamp(q,int2(0,0),int2(Wa,Ha)-1),0));}
float4 bdL(int2 q){return desempacota(bordasIn.Load(int3(clamp(q,int2(0,0),int2(Wa,Ha)-1),0)));}
[numthreads(8,8,1)]void ao_denoise(uint3 id:SV_DispatchThreadID){
 if(id.x>=Wa||id.y>=Ha)return;int2 p=int2(id.xy);
 float4 eC=bdL(p),eL=bdL(p+int2(-1,0)),eR=bdL(p+int2(1,0)),eT=bdL(p+int2(0,-1)),eB=bdL(p+int2(0,1));
 eC*=float4(eL.y,eR.x,eT.w,eB.z);eC=saturate(eC+(saturate(1.5-dot(eC,1.0))/1.5)*0.5);
 float wTL=0.425*(eC.x*eL.z+eC.z*eT.x),wTR=0.425*(eC.z*eT.y+eC.y*eR.z),wBL=0.425*(eC.w*eB.x+eC.x*eL.w),wBR=0.425*(eC.y*eR.w+eC.w*eB.y);
 float bl=final_!=0u?beta:beta/5.0;float sm=aoL(p)*bl,sw=bl;
 sm+=aoL(p+int2(-1,0))*eC.x+aoL(p+int2(1,0))*eC.y+aoL(p+int2(0,-1))*eC.z+aoL(p+int2(0,1))*eC.w;sw+=dot(eC,1.0);
 sm+=aoL(p+int2(-1,-1))*wTL+aoL(p+int2(1,-1))*wTR+aoL(p+int2(-1,1))*wBL+aoL(p+int2(1,1))*wBR;sw+=wTL+wTR+wBL+wBR;
 aoOut[p]=sm/sw;}
// Acumulo temporal (nosso): posicao no quadro anterior = pixel + vetor do jogo (convencao do FSR, px de render); 4 amostras bilineares
// separadas, cada uma so vale com |z_hist - z| < (TolProf + 0,01 |mv|) z; nenhuma = desoclusao (n = 1). Historico preso a [min, max]
// da vizinhanca 3x3 +- margem; n = min(n + 1, Quadros, menos com movimento rapido); ao = lerp(hist, atual, 1/n). hist = (ao, z, n).
// lua-ao (26/10, analysis/rdr2-lua-ao-20261026 secao 3; a grama do RDR2 piscava): (1) a amostra do pixel r esta em r + 0,5 - J (FSR):
// o ponto certo no quadro anterior e r + mv - (J_t - J_{t-1}) (djx/djy; 0 = o de antes); (2) pviz: a amostra do historico vale se a
// profundidade dela cai em [min, max] da 3x3 atual +- tol (folha e chao atras estao os dois na vizinhanca), peso 1 ate tol e 0 em 2 x tol;
// (3) csig > 0: historico preso a media +- (csig sigma + margem) da 3x3 (variancia) no lugar de [min, max] +- margem; (4) qest > 0: o
// teto qmax so nos pixels instaveis (bordas de profundidade do proprio texel: folhagem), qest no estavel.
[numthreads(8,8,1)]void ao_temporal(uint3 id:SV_DispatchThreadID){
 if(id.x>=Wa||id.y>=Ha)return;int2 pa=int2(id.xy);int2 r=min(pa*int(S),int2(W,H)-1);
 float zc=zm(r,0u);float cur=aoL(pa);
 if(zc>=CEU){histOut[pa]=float4(1.0,CEU,1.0,0.0);if(ver==6u)dbgOut[pa]=float4(0.0,0.0,0.0,1.0);return;}
 float n=1.0,ao=cur;float fz=(pviz&2u)!=0u?instavel(bdL(pa)):0.0,fw=fz;   // pelo: o material fino acumulado como o AO (fio que pisca nao liga/desliga o peso)
 if(temporal!=0u&&reset_==0u){
  uint mw,mh;mvTex.GetDimensions(mw,mh);float2 mv=float2(0.0,0.0);if(mw>0u&&mh>0u)mv=mvTex.Load(int3(min(r,int2(mw,mh)-1),0)).xy*float2(mvx,mvy);
  if(!all(mv==mv)||any(abs(mv)>1e5))mv=float2(0.0,0.0);
  float2 pp=(float2(r)+mv-float2(djx,djy))/float(S);float2 b=floor(pp),f=pp-b;float tl=tol+0.01*length(mv);float sw=0.0,sa=0.0,sn=0.0,sf=0.0;
  float zmn=zc,zmx=zc;
  if((pviz&1u)!=0u){[unroll]for(int j=-1;j<=1;++j)[unroll]for(int i=-1;i<=1;++i){float z=zm(r+int2(i,j)*int(S),0u);if(z<CEU){zmn=min(zmn,z);zmx=max(zmx,z);}}}
  [unroll]for(int k=0;k<4;++k){int2 o=int2(k&1,k>>1);int2 q=int2(b)+o;float wb=(o.x!=0?f.x:1.0-f.x)*(o.y!=0?f.y:1.0-f.y);
   if(q.x<0||q.y<0||q.x>=int(Wa)||q.y>=int(Ha)||!(wb>0.0))continue;float4 h=histAnt.Load(int3(q,0));
   float wz=abs(h.y-zc)<tl*zc?1.0:0.0;
   if((pviz&1u)!=0u)wz=saturate(2.0-max(max(zmn-h.y,h.y-zmx),0.0)/max(tl*zc,1e-6));
   if(wz>0.0){wb*=wz;sw+=wb;sa+=wb*h.x;sn+=wb*h.z;sf+=wb*h.w;}}
  if(sw>1e-3){float ah=sa/sw,na=sn/sw;float mn=cur,mx=cur,s1=0.0,s2=0.0;
   [unroll]for(int j=-1;j<=1;++j)[unroll]for(int i=-1;i<=1;++i){float t=aoL(pa+int2(i,j));mn=min(mn,t);mx=max(mx,t);
    float tc=(pviz&4u)!=0u?aoCru.Load(int3(clamp(pa+int2(i,j),int2(0,0),int2(Wa,Ha)-1),0)):t;s1+=tc;s2+=tc*tc;}   // pelo: a variancia do AO cru (pviz bit 2)
   if(csig>0.0){float mu=s1/9.0,sg=sqrt(max(s2/9.0-mu*mu,0.0));mn=mu-csig*sg;mx=mu+csig*sg;}
   float qpx=qmax;
   if(qest>0.0){float inst=1.0-smoothstep(2.0,3.5,dot(bdL(pa),1.0));qpx=lerp(min(qest,qmax),qmax,inst);}
   ah=clamp(ah,mn-margem,mx+margem);float qe=max(lerp(qpx,4.0,saturate(length(mv)/32.0)),1.0);n=min(na+1.0,qe);ao=lerp(ah,cur,1.0/n);fw=lerp(sf/sw,fz,1.0/n);}}
 histOut[pa]=float4(ao,zc,n,fw);if(ver==6u)dbgOut[pa]=float4(saturate(n/max(qmax,1.0)).xxx,1.0);}   // pelo: w = material fino (a aplicacao pesa menos)
)"
R"(// ---- aplicacao (resolucao de saida; a string vem em 2 pedacos: limite de literal do MSVC) ----
float3 Pf(float3 x){x=max(x,0.0);if(hdr!=0u){x*=expo;return x/(1.0+x);}return saturate(x);}
float3 Gf(float3 x){return hdr!=0u?pow(max(x,0.0),1.0/2.2):x;}
// peso da pele: a mesma conta do passe de filtros (filtros_gpu.h pele_regiao/pele_cor): elipse cabeca + pescoco x cor de pele (HSV)
float pele_regiao(int2 p){float w=0.0;
 for(uint k=0;k<nrost&&k<4u;++k){float r=max(rost[k].z,1.0),ry=rost[k].w>0.0?rost[k].w:r;float2 d=float2(p)+0.5-rost[k].xy;
  float dd=length(float2(d.x/(1.5*r),d.y/(d.y<0.0?1.6*ry:4.4*ry)));w=max(w,1.0-smoothstep(0.55,1.0,dd));}return w;}
float pele_cor(float3 c){c=saturate(c);float mx=max(c.r,max(c.g,c.b)),mn=min(c.r,min(c.g,c.b)),ch=mx-mn;if(!(ch>1e-5))return 0.0;
 float hh=mx==c.r?(c.g-c.b)/ch:(mx==c.g?2.0+(c.b-c.r)/ch:4.0+(c.r-c.g)/ch);hh=frac(hh/6.0+1.0)*360.0;float Sv=ch/mx;
 return smoothstep(4.0,12.0,hh)*(1.0-smoothstep(42.0,55.0,hh))*smoothstep(0.10,0.20,Sv)*(1.0-smoothstep(0.62,0.75,Sv))*smoothstep(0.28,0.42,mx);}
bool MB(int2 q){q=clamp(q,int2(0,0),int2(int(ms_w)-1,int(ms_h)-1));uint a=uint(q.y)*ms_pitch+uint(q.x);return ((msk.Load(a&~3u)>>((a&3u)*8u))&(ms_bits&255u))!=0u;}
// ssil (26/10): meias da IL, potencia com 0^0 = 1, luma e a volta do espaco de trabalho (HDR direto; UNORM pseudo-HDR + gama, a da AO)
float2 h2(uint v){return float2(f16tof32(v&0xFFFFu),f16tof32(v>>16));}
float potz(float x,float y){return y==0.0?1.0:(x>0.0?pow(x,y):0.0);}
float lumi(float3 c){return dot(c,float3(0.2126,0.7152,0.0722));}
float3 enc(float3 x){x=max(x,0.0);return hdr!=0u?x:pow(1.1*x/(x+1.0),1.0/2.2);}
float3 saida_dbg(float3 g){g=saturate(g);if(hdr!=0u){g=min(g,0.95);return g/(1.0-g)/(expo>0.0?expo:1.0);}return g;}
[numthreads(8,8,1)]void ao_aplica(uint3 id:SV_DispatchThreadID){
 if(id.x>=ow||id.y>=oh)return;int2 q=int2(id.xy);float4 c0=cor.Load(int3(q,0));
 float2 rp=(float2(q)+0.5)*float2(rW,rH)/float2(ow,oh)-0.5+float2(jx,jy);float2 a=rp/float(aS);float2 b=floor(a),f=a-b;int2 lim=int2(aW,aH)-1;
 float s=0.0,amx=0.0,zmn=1e30,zmx=0.0,zn=CEU,wmx=-1.0,fn=0.0;float hx[4];float wk[4];int2 qk[4];
 [unroll]for(int k=0;k<4;++k){int2 o=int2(k&1,k>>1);qk[k]=clamp(int2(b)+o,int2(0,0),lim);float4 h=histCur.Load(int3(qk[k],0));
  wk[k]=(o.x!=0?f.x:1.0-f.x)*(o.y!=0?f.y:1.0-f.y);hx[k]=h.x;s+=wk[k]*h.x;if(wk[k]>1e-4){amx=max(amx,h.x);zmn=min(zmn,h.y);zmx=max(zmx,h.y);}if(wk[k]>wmx){wmx=wk[k];zn=h.y;fn=h.w;}}
 float ao=s;
 // lua-ao (26/10): Automatico (ups 0) = conservador so na borda isolada; com muitas bordas no 2x2 (>= 6 lados de borda nos 4 texels: lamina
 // de grama, folhagem) bilinear: o maior dos 4 trocava de vizinho a cada quadro com o jitter (a grama piscava). 3 = conservador sempre.
 bool denso=false;
 if(ups==0u&&zmx>zmn*1.02){float nb=0.0;[unroll]for(int k=0;k<4;++k)nb+=dot(step(desempacota(bordasIn.Load(int3(qk[k],0))),0.5),1.0);denso=nb>=6.0;}
 if(ups!=2u&&!denso&&zmx>zmn*1.02){   // borda de profundidade entre os 4 vizinhos
  if(ups==1u&&tem_entrada!=0u){float yo=log2(max(dot(max(c0.rgb,0.0),float3(0.2126,0.7152,0.0722)),1e-6));float sa=0.0,sw=0.0;
   [unroll]for(int k=0;k<4;++k){int2 rr=min(qk[k]*int(aS),int2(rW,rH)-1);float ye=log2(max(dot(max(corEntrada.Load(int3(rr,0)).rgb,0.0),float3(0.2126,0.7152,0.0722)),1e-6));
    float w=wk[k]*exp2(-abs(yo-ye)*4.0)+1e-4;sa+=w*hx[k];sw+=w;}ao=sa/sw;}
  else ao=amx;}
 float v=saturate(ao);
 if(mb!=0u){float A=2.0404*0.5-0.3324,B=-4.7951*0.5+0.6417,C=2.7552*0.5+0.6903;v=max(v,((v*A+B)*v+C)*v);}
 float ae=intens>0.0?pow(max(v,1e-4),intens):1.0;
 float peso=zn>=CEU*0.999?0.0:forca;
 float fade=d_fim>d_ini?1.0-saturate((zn-d_ini)/(d_fim-d_ini)):(zn<d_fim?1.0:0.0);peso*=fade;if(d_min>0.0)peso*=smoothstep(0.5*d_min,d_min,zn);   // pelo (26/10): rampa (era zn/d_min), DistMin ate 10 m
 if((pad1&1u)!=0u)peso*=1.0-0.6*saturate(fn);   // pelo: o material fino pesa menos
 float3 lin=hdr!=0u?max(c0.rgb,0.0):pow(saturate(c0.rgb),2.2);
 float pw=0.0;if(pele_k>0.0&&nrost>0u){float rg=pele_regiao(q);if(rg>0.0)pw=rg*pele_cor(Gf(Pf(c0.rgb)));}peso*=1.0-pele_k*pw;
 if(pers_k>0.0&&ms_bits!=0u&&MB(int2(floor((float2(q)+0.5)*float2(ms_w,ms_h)/float2(ow,oh)))))peso*=1.0-pers_k;
 float cw=smoothstep(c_ini,c_fim,dot(lin,float3(0.2126,0.7152,0.0722))*(hdr!=0u&&expo>0.0?expo:1.0));peso*=1.0-claro_k*cw;
 float fator=lerp(1.0,ae,saturate(peso));
 // ssil (26/10): a iluminacao indireta (il_gpu.h). dC = peso x albedo x IL; o resto do caminho e o da AO quando dC = 0 (bit a bit).
 float3 dC=0.0,alb=0.0,W0=0.0;float2 i3=h2(il3);uint ilv=(il0>>2)&7u;float2 ia=0.0;int2 iD=int2(1,1);
 if((il0&1u)!=0u){uint iS=max((il0>>8)&7u,1u);iD=int2(max(il1&0xFFFFu,1u),max(il1>>16,1u));
  float zp=zmip.Load(int3(clamp(int2(floor(rp+0.5)),int2(0,0),int2(rW,rH)-1),0));ia=rp/float(iS);
  if(zp<CEU){
   // upsample bilateral pela profundidade (4 taps; tap sem profundidade parecida perde peso; todos diferentes = o mais parecido)
   float2 ib=floor(ia),fi=ia-ib;float4 sa=0.0,hm=0.0;float sw=0.0,dmin=1e30;float itol=h2(il8).x;
   [unroll]for(int k=0;k<4;++k){int2 o=int2(k&1,k>>1);int2 t=clamp(int2(ib)+o,int2(0,0),iD-1);float4 h=ilTex.Load(int3(t,0));
    float zk=zmip.Load(int3(min(t*int(iS),int2(rW,rH)-1),0));float wb=(o.x!=0?fi.x:1.0-fi.x)*(o.y!=0?fi.y:1.0-fi.y);float dz=abs(zk-zp);
    float w=zk<CEU?wb*saturate(2.0-dz/max(itol*zp,1e-6)):0.0;sa+=w*h;sw+=w;if(zk<CEU&&dz<dmin){dmin=dz;hm=h;}}
   float4 il=sw>1e-4?sa/sw:hm;
   W0=hdr!=0u?lin:min(lin,1.0)/(1.1-min(lin,1.0));float Y=lumi(W0),yb=max(il.a,0.0);
   float2 i2=h2(il2),i4=h2(il4),i5=h2(il5),i6=h2(il6),i7=h2(il7);uint am=(il0>>5)&3u;
   // albedo (il.h albedo): 0 = cor / luz local (media geometrica do pixel e da vizinhanca: independente da exposicao), 1 cinza, 2 a cor tonemapeada
   if(am==1u)alb=i7.x.xxx;else if(am==2u)alb=Pf(c0.rgb);
   else{float E=max(max(potz(Y,1.0-i6.y)*potz(yb,i6.y)/i7.x,Y/i7.y),1e-6*yb);alb=E>0.0?W0/E:float3(0.0,0.0,0.0);}
   float3 ils=max(lerp(lumi(il.rgb).xxx,il.rgb,i2.y),0.0);   // Saturacao do rebatimento
   float pk=i2.x*(d_fim>d_ini?1.0-saturate((zp-d_ini)/(d_fim-d_ini)):(zp<d_fim?1.0:0.0));if(d_min>0.0)pk*=saturate(zp/d_min);
   if(i3.y>0.0&&nrost>0u){float rg=pele_regiao(q);if(rg>0.0)pk*=1.0-i3.y*rg*pele_cor(Gf(Pf(c0.rgb)));}
   if(i4.x>0.0&&ms_bits!=0u&&MB(int2(floor((float2(q)+0.5)*float2(ms_w,ms_h)/float2(ow,oh)))))pk*=1.0-i4.x;
   pk*=1.0-i4.y*smoothstep(i5.x,i5.y,Y/max(yb,1e-6*Y+1e-30));   // pixel muito acima da media local (luz direta, emissivo): menos IL
   dC=max(pk,0.0)*alb*ils;float ld=lumi(dC),lt=i6.x*Y;if(ld>lt)dC*=lt/max(ld,1e-30);}}   // teto: ClaroMax x a luz do proprio pixel
 if(ver_a!=0u){float3 g;
  if(ver_a==1u)g=fator.xxx;else if(ver_a==7u)g=float3(pw,cw,fade);
  else g=dbgIn.Load(int3(clamp(int2(round(a)),int2(0,0),lim),0)).rgb;
  dst[q]=float4(saida_dbg(g),c0.a);return;}
 if(ilv!=0u){float3 g;   // ssil: vistas da IL (1 = dC, 2-3 na escala da imagem; 4-7 de 0 a 1)
  if(ilv==1u){dst[q]=float4(enc(dC),c0.a);return;}
  if(ilv==4u)g=alb;else g=ilTex.Load(int3(clamp(int2(round(ia)),int2(0,0),iD-1),0)).rgb;
  dst[q]=float4(ilv<=3u?enc(g):saida_dbg(g),c0.a);return;}
 bool tem_il=any(dC>0.0);
 if(!(fator<1.0)&&!tem_il){if(no_lugar==0u)dst[q]=c0;return;}   // fator 1: o pixel de entrada, bit a bit (no lugar: nem escreve)
 if(tem_il){float3 B=W0*fator,C1=B+dC;float alvo=lumi(B)+i3.x*lumi(dC);dst[q]=float4(enc(C1*(alvo/max(lumi(C1),1e-6*alvo+1e-12))),c0.a);return;}   // energia (il.h)
 float3 o;
 if(hdr!=0u)o=max(c0.rgb,0.0)*fator;
 else{float3 x=min(lin,1.0);float3 X=x/(1.1-x)*fator;o=pow(1.1*X/(X+1.0),1.0/2.2);}
 dst[q]=float4(o,c0.a);}
)";
struct K { std::uint32_t W,H,Wa,Ha,S,modo,fatias,passos;float lc,le,metros;std::uint32_t inv;float mulx,muly,addx,addy;
 float raio,raio_mult,falloff,distrib,finos,potencia,mip_off,raio_max_px,esp;std::uint32_t esp_lin,quadro;float beta;std::uint32_t final_;
 float tol,margem,qmax;std::uint32_t reset,temporal;float mvx,mvy;std::uint32_t ver;
 float djx,djy,csig,qest;std::uint32_t pviz; };
static_assert(sizeof(K)==42*4,"42 constantes em b0");
struct KA { std::uint32_t ow,oh,hdr;float expo;std::uint32_t rW,rH,aW,aH,aS;float jx,jy;std::uint32_t ups;float forca,intens,d_ini,d_fim,d_min,pele,pers,claro,c_ini,c_fim;
 std::uint32_t mb,ms_w,ms_h,ms_pitch,ms_bits,nrost,ver,tem_entrada,no_lugar,pad1;float rost[16];
 std::uint32_t il[9]; };
static_assert(sizeof(KA)==57*4,"57 constantes em b1 (rost alinhado em 16 bytes; + 9 da IL)");
enum Pso : unsigned { Prof, Principal, Denoise, Temporal, Aplica, PsoN };
inline constexpr unsigned nSrv=13,nUav=10,nDesc=nSrv+nUav,blocos=8u*8u;

struct IlEntrada { ID3D12Resource* tex=nullptr;std::uint32_t k[9]{};float pele=0.f,pers=0.f; };
struct Estado {
 bool falhou=false;ComPtr<ID3D12Device> dev;ComPtr<ID3D12RootSignature> raiz,raiz_a;ComPtr<ID3D12PipelineState> pso[PsoN];ComPtr<ID3D12DescriptorHeap> heap;unsigned bloco=0;
 ComPtr<ID3D12Resource> zmip,ao[3],bordas,dbg,hist[2];unsigned W=0,H=0,Wa=0,Ha=0,S=0;unsigned atual=0;bool hist_ok=false;float fov_ant=0.f;unsigned quadro=0;
 float jx_ant=0.f,jy_ant=0.f;
 unsigned long long feitos=0,pulados=0;const char* motivo=nullptr;nrd3d::Aposentados<64> aposentados;unsigned slot=0;
 nrd3d::Trocas trocas;

 bool pronto=false;KA ka{};ID3D12Resource* cor_entrada=nullptr;DXGI_FORMAT cor_entrada_vf=DXGI_FORMAT_UNKNOWN;unsigned cor_entrada_estado=0;
 K kq{};IlEntrada il{};
 void aposentar_(ComPtr<ID3D12Resource>& r);
};
inline void log(const char* t);
inline void Estado::aposentar_(ComPtr<ID3D12Resource>& r){if(aposentados.aposentar(r,slot))log("APOSENTADOS cheios: recurso trocado fica vivo");}
inline Estado& estado_(){static Estado* e=new Estado;return *e;}
inline void log(const char* t){
 (void)t;
}
inline std::atomic<bool> teste_compilar{false};
inline const char* teste_hlsl=nullptr;
inline void teste_recriar(){auto& e=estado_();for(auto& p:e.pso)p.Reset();e.raiz.Reset();e.raiz_a.Reset();e.dev.Reset();}
inline void teste_reiniciar(){auto& e=estado_();e.hist_ok=false;e.quadro=0;e.pronto=false;e.trocas.seguidas=0;}
inline bool raiz(ID3D12Device* d,unsigned registrador,unsigned n,ComPtr<ID3D12RootSignature>& r){
 D3D12_DESCRIPTOR_RANGE rg[2]{};rg[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,nSrv,0,0,0};rg[1]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,nUav,0,0,nSrv};
 D3D12_ROOT_PARAMETER p[2]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[0].Constants={registrador,0,n};
 p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[1].DescriptorTable={2,rg};
 D3D12_ROOT_SIGNATURE_DESC rd{2,p,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> b,er;
 return SUCCEEDED(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&b,&er))&&SUCCEEDED(d->CreateRootSignature(0,b->GetBufferPointer(),b->GetBufferSize(),IID_PPV_ARGS(&r)));}
inline bool preparar(Estado& e,ID3D12Device* d){
 if(e.pso[Aplica]&&e.dev.Get()==d)return true;
 e.dev=d;const char* entradas[PsoN]{"ao_prof","ao_principal","ao_denoise","ao_temporal","ao_aplica"};ComPtr<ID3DBlob> cs[PsoN];const void* codigo[PsoN]{};SIZE_T tam[PsoN]{};bool embutido=false;
#if defined(NR_FILTROS_CS) && NR_FILTROS_CS
 if(!teste_compilar.load()){embutido=true;codigo[0]=g_nr_ao_ao_prof;tam[0]=sizeof g_nr_ao_ao_prof;codigo[1]=g_nr_ao_ao_principal;tam[1]=sizeof g_nr_ao_ao_principal;
  codigo[2]=g_nr_ao_ao_denoise;tam[2]=sizeof g_nr_ao_ao_denoise;codigo[3]=g_nr_ao_ao_temporal;tam[3]=sizeof g_nr_ao_ao_temporal;codigo[4]=g_nr_ao_ao_aplica;tam[4]=sizeof g_nr_ao_ao_aplica;}
#endif
 if(!embutido){HMODULE c=LoadLibraryExW(L"d3dcompiler_47.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!c){log("FALHA d3dcompiler_47 ausente");return false;}
  auto compila=reinterpret_cast<decltype(&D3DCompile)>(GetProcAddress(c,"D3DCompile"));if(!compila){log("FALHA D3DCompile");return false;}
  for(unsigned i=0;i<PsoN;++i){ComPtr<ID3DBlob> er;
   const char* fonte=teste_hlsl?teste_hlsl:hlsl;
   if(FAILED(compila(fonte,std::strlen(fonte),"nr_ao",nullptr,nullptr,entradas[i],"cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&cs[i],&er))){
    char s[700];std::snprintf(s,sizeof s,"FALHA compilacao %s: %s",entradas[i],er?static_cast<const char*>(er->GetBufferPointer()):"?");log(s);return false;}
   codigo[i]=cs[i]->GetBufferPointer();tam[i]=cs[i]->GetBufferSize();}}
 if(!raiz(d,0,sizeof(K)/4,e.raiz)||!raiz(d,1,sizeof(KA)/4,e.raiz_a)){log("FALHA raiz");return false;}
 for(unsigned i=0;i<PsoN;++i){D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=i==Aplica?e.raiz_a.Get():e.raiz.Get();pd.CS={codigo[i],tam[i]};
  if(FAILED(d->CreateComputePipelineState(&pd,IID_PPV_ARGS(e.pso[i].ReleaseAndGetAddressOf())))){log("FALHA pso");return false;}}
 if(!e.heap){D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,nDesc*blocos,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
  if(FAILED(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&e.heap)))){log("FALHA heap");return false;}}
 log(embutido?"pronta (bytecode do build)":"pronta (HLSL compilado agora)");return true;}

struct Vista { ID3D12Resource* r=nullptr;DXGI_FORMAT f=DXGI_FORMAT_UNKNOWN;unsigned mip=0;unsigned mips=1;unsigned bytes=0; };
inline D3D12_GPU_DESCRIPTOR_HANDLE bloco(Estado& e,ID3D12Device* d,const Vista (&s)[nSrv],const Vista (&u)[nUav]){
 static constexpr DXGI_FORMAT fs[nSrv]{DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R16_FLOAT,DXGI_FORMAT_R8_UNORM,DXGI_FORMAT_R16G16_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT,
  DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R16_FLOAT};
 static constexpr DXGI_FORMAT fu[nUav]{DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R16_FLOAT,
  DXGI_FORMAT_R8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT};
 const auto inc=d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);const unsigned base=(e.bloco++%blocos)*nDesc;
 auto hc=[&](unsigned i){auto c=e.heap->GetCPUDescriptorHandleForHeapStart();c.ptr+=SIZE_T(base+i)*inc;return c;};
 for(unsigned i=0;i<nSrv;++i){D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  if(i==8){v.Format=DXGI_FORMAT_R32_TYPELESS;v.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;v.Buffer.Flags=D3D12_BUFFER_SRV_FLAG_RAW;v.Buffer.NumElements=s[i].r?s[i].bytes/4u:1u;}
  else{v.Format=s[i].r?s[i].f:fs[i];v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Texture2D.MipLevels=s[i].mips;}
  d->CreateShaderResourceView(s[i].r,&v,hc(i));}
 for(unsigned i=0;i<nUav;++i){D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=u[i].r?u[i].f:fu[i];v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;v.Texture2D.MipSlice=u[i].mip;
  d->CreateUnorderedAccessView(u[i].r,nullptr,&v,hc(nSrv+i));}
 auto g=e.heap->GetGPUDescriptorHandleForHeapStart();g.ptr+=UINT64(base)*inc;return g;}
inline bool textura(ID3D12Device* d,unsigned w,unsigned h,unsigned mips,DXGI_FORMAT f,D3D12_RESOURCE_STATES s,ComPtr<ID3D12Resource>& r,const wchar_t* nome){
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=w;td.Height=h;td.DepthOrArraySize=1;
 td.MipLevels=UINT16(mips);td.Format=f;td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
 if(FAILED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,s,nullptr,IID_PPV_ARGS(&r))))return false;nrd3d::nomear(r.Get(),nome);return true;}

inline bool recursos(Estado& e,ID3D12Device* d,unsigned W,unsigned H,unsigned Wa,unsigned Ha,unsigned S){
 if(e.zmip&&e.W==W&&e.H==H&&e.Wa==Wa&&e.Ha==Ha&&e.S==S)return true;
 ComPtr<ID3D12Resource>* rs[]{std::addressof(e.zmip),std::addressof(e.ao[0]),std::addressof(e.ao[1]),std::addressof(e.ao[2]),std::addressof(e.bordas),std::addressof(e.dbg),std::addressof(e.hist[0]),std::addressof(e.hist[1])};
 for(auto* r:rs)e.aposentar_(*r);
 constexpr auto UAV=D3D12_RESOURCE_STATE_UNORDERED_ACCESS,SRV=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
 const bool ok=textura(d,W,H,5,DXGI_FORMAT_R32_FLOAT,UAV,e.zmip,L"nr_ao_zmip")&&textura(d,Wa,Ha,1,DXGI_FORMAT_R16_FLOAT,UAV,e.ao[0],L"nr_ao_a")&&
  textura(d,Wa,Ha,1,DXGI_FORMAT_R16_FLOAT,UAV,e.ao[1],L"nr_ao_b")&&textura(d,Wa,Ha,1,DXGI_FORMAT_R16_FLOAT,UAV,e.ao[2],L"nr_ao_c")&&textura(d,Wa,Ha,1,DXGI_FORMAT_R8_UNORM,UAV,e.bordas,L"nr_ao_bordas")&&
  textura(d,Wa,Ha,1,DXGI_FORMAT_R8G8B8A8_UNORM,UAV,e.dbg,L"nr_ao_ver")&&textura(d,Wa,Ha,1,DXGI_FORMAT_R16G16B16A16_FLOAT,SRV,e.hist[0],L"nr_ao_hist0")&&
  textura(d,Wa,Ha,1,DXGI_FORMAT_R16G16B16A16_FLOAT,SRV,e.hist[1],L"nr_ao_hist1");
 if(!ok){log("FALHA recursos");return false;}
 e.W=W;e.H=H;e.Wa=Wa;e.Ha=Ha;e.S=S;e.hist_ok=false;e.trocas.feita(e.slot);
 char t[160];std::snprintf(t,sizeof t,"recursos render %ux%u ao %ux%u passo %u (%.1f MB)",W,H,Wa,Ha,S,(W*H*4.*1.34+Wa*Ha*(2.+2.+2.+1.+4.+16.))/1048576.);log(t);return true;}
inline D3D12_RESOURCE_STATES estado(unsigned ffx){return nrd3d::estado_ffx(ffx);}

inline bool pode(ID3D12GraphicsCommandList* lista){
 auto& en=nrao::entrada;auto& e=estado_();if(e.falhou||!en.valido||!en.prof||!lista)return false;
 auto ok=[&](void* r,unsigned s){if(!r)return true;const auto t=estado(s);if(t==nrd3d::estado_recusado)return false;
  return lista->GetType()!=D3D12_COMMAND_LIST_TYPE_COMPUTE||nrffx_estado::valido_em_compute(unsigned(t));};
 return ok(en.prof,en.prof_estado)&&ok(en.mov,en.mov_estado);}

inline bool calcular(ID3D12Device* d,ID3D12GraphicsCommandList* lista,const nrfiltros::Parametros& par,bool ao_ligada=true){
 auto& e=estado_();e.pronto=false;
 auto pular=[&](const char* m){++e.pulados;e.hist_ok=false;if(e.motivo!=m){e.motivo=m;char t[120];std::snprintf(t,sizeof t,"SEM motivo=%s feitos=%llu pulados=%llu",m,e.feitos,e.pulados);log(t);}return false;};
 if(!pode(lista))return pular("entrada");
 if(!preparar(e,d)){e.falhou=true;return pular("preparar");}
 const auto& en=nrao::entrada;const auto o=nrao::opcoes();
 auto* prof=static_cast<ID3D12Resource*>(en.prof);auto* mov=static_cast<ID3D12Resource*>(en.mov);
 const auto pd=prof->GetDesc();const DXGI_FORMAT pv=nrformato::vista_profundidade(pd.Format);if(!nrformato::entrada(prof,pv))return pular("profundidade_formato");
 DXGI_FORMAT mv_f=DXGI_FORMAT_UNKNOWN;if(mov){mv_f=nrformato::vista_movimento(mov->GetDesc().Format);if(!nrformato::entrada(mov,mv_f))mov=nullptr;}
 const unsigned W=(std::min)(en.render_w?en.render_w:unsigned(pd.Width),unsigned(pd.Width)),H=(std::min)(en.render_h?en.render_h:pd.Height,pd.Height);
 if(W<8u||H<8u||(std::max)(W,H)<16u)return pular("render_pequeno");
 const unsigned S=nrao::passo_resolucao(par.ao_res,W,H),Wa=(W+S-1u)/S,Ha=(H+S-1u)/S;
 ++e.slot;e.aposentados.soltar(e.slot);

 if(e.zmip&&(e.W!=W||e.H!=H||e.Wa!=Wa||e.Ha!=Ha||e.S!=S)&&!e.trocas.pode(e.slot,nrd3d::livres(e.aposentados),8u))return pular("render_mudando");
 if(!recursos(e,d,W,H,Wa,Ha,S)){e.falhou=true;return pular("recursos");}
 const auto cam=nrao::resolver_camera(en.near_,en.far_,en.fov,o.invertida>=0?o.invertida:en.invertida,o.infinita,o.fov_graus,o.metros);
 const auto lin=nrao::lin_fsr(cam.near_,cam.inf?cam.near_*1e6f:cam.far_,cam.inv,cam.inf);
 const bool reset=!e.hist_ok||en.reset||std::fabs(cam.fov-e.fov_ant)>2.f*float(nrao::pi)/180.f||!ao_ligada;e.fov_ant=cam.fov;
 const auto pr=nrao::predef(par.ao_qual);const unsigned nden=o.denoise>=0?unsigned(o.denoise):pr.denoise;
 K k{};k.W=W;k.H=H;k.Wa=Wa;k.Ha=Ha;k.S=S;k.modo=unsigned(o.modo);k.fatias=pr.fatias;k.passos=pr.passos;k.lc=lin.c;k.le=lin.e;k.metros=cam.metros;k.inv=cam.inv?1u:0u;
 const float ty=std::tan(cam.fov*.5f),tx=ty*float(W)/float(H);k.mulx=2.f*tx;k.muly=-2.f*ty;k.addx=-tx;k.addy=ty;
 k.raio=par.ao_raio;k.raio_mult=o.raio_mult;k.falloff=o.falloff;k.distrib=o.distribuicao;k.finos=o.finos;k.potencia=o.potencia;k.mip_off=o.mip_offset;k.raio_max_px=o.raio_max_px;
 k.esp=par.ao_esp;k.esp_lin=unsigned(o.esp_linear);k.quadro=o.temporal?e.quadro:0u;k.beta=o.denoise_beta;k.tol=o.tol_prof;k.margem=o.clamp_margem;
 k.qmax=float(o.quadros>0?unsigned(o.quadros):pr.quadros);k.reset=reset?1u:0u;k.temporal=unsigned(o.temporal);
 k.mvx=std::isfinite(en.mv_x)?en.mv_x:0.f;k.mvy=std::isfinite(en.mv_y)?en.mv_y:0.f;k.ver=par.ao_ver;

 {const float jx=std::isfinite(en.jitter_x)?en.jitter_x*o.desjitter:0.f,jy=std::isfinite(en.jitter_y)?en.jitter_y*o.desjitter:0.f;
  if(o.hist_jitter&&!reset){k.djx=jx-e.jx_ant;k.djy=jy-e.jy_ant;}
  e.jx_ant=jx;e.jy_ant=jy;}
 k.csig=o.clamp_sigma;k.pviz=(o.prof_viz?1u:0u)|(o.instavel?2u:0u)|(o.clamp_cru?4u:0u);k.qest=o.quadros>0?float(pr.quadros):0.f;e.kq=k;
 constexpr auto UAV=D3D12_RESOURCE_STATE_UNORDERED_ACCESS,SRV=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
 auto tr=[&](ID3D12Resource* r,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){nrd3d::transicao(lista,r,a,b);};

 const auto sp=estado(en.prof_estado),sm=mov?estado(en.mov_estado):SRV;
 auto para_leitura=[](D3D12_RESOURCE_STATES s){return (s&SRV)?s:SRV;};
 tr(prof,sp,para_leitura(sp));if(mov)tr(mov,sm,para_leitura(sm));
 ID3D12DescriptorHeap* hs[]{e.heap.Get()};lista->SetDescriptorHeaps(1,hs);lista->SetComputeRootSignature(e.raiz.Get());lista->SetComputeRoot32BitConstants(0,sizeof(K)/4,&k,0);
 const unsigned gx=(Wa+7u)/8u,gy=(Ha+7u)/8u;

 {Vista s[nSrv]{},u[nUav]{};s[0]={prof,pv};for(unsigned m=0;m<5;++m)u[m]={e.zmip.Get(),DXGI_FORMAT_R32_FLOAT,m};
  lista->SetPipelineState(e.pso[Prof].Get());lista->SetComputeRootDescriptorTable(1,bloco(e,d,s,u));lista->Dispatch((W+15u)/16u,(H+15u)/16u,1);
  tr(e.zmip.Get(),UAV,SRV);}
 const unsigned cur=e.atual^1u;unsigned fonte=0;
 if(ao_ligada){

 {Vista s[nSrv]{},u[nUav]{};s[1]={e.zmip.Get(),DXGI_FORMAT_R32_FLOAT,0,5};u[5]={e.ao[0].Get(),DXGI_FORMAT_R16_FLOAT};u[6]={e.bordas.Get(),DXGI_FORMAT_R8_UNORM};u[7]={e.dbg.Get(),DXGI_FORMAT_R8G8B8A8_UNORM};
  lista->SetPipelineState(e.pso[Principal].Get());lista->SetComputeRootDescriptorTable(1,bloco(e,d,s,u));lista->Dispatch(gx,gy,1);
  tr(e.ao[0].Get(),UAV,SRV);tr(e.bordas.Get(),UAV,SRV);nrd3d::barreira_uav(lista,e.dbg.Get());}

 for(unsigned i=0;i<nden;++i){k.final_=i+1u==nden?1u:0u;lista->SetComputeRoot32BitConstants(0,sizeof(K)/4,&k,0);const unsigned dst=fonte==1u?2u:1u;
  Vista s[nSrv]{},u[nUav]{};s[2]={e.ao[fonte].Get(),DXGI_FORMAT_R16_FLOAT};s[3]={e.bordas.Get(),DXGI_FORMAT_R8_UNORM};u[5]={e.ao[dst].Get(),DXGI_FORMAT_R16_FLOAT};
  lista->SetPipelineState(e.pso[Denoise].Get());lista->SetComputeRootDescriptorTable(1,bloco(e,d,s,u));lista->Dispatch(gx,gy,1);
  tr(e.ao[dst].Get(),UAV,SRV);if(fonte!=0u)tr(e.ao[fonte].Get(),SRV,UAV);fonte=dst;}

 {Vista s[nSrv]{},u[nUav]{};s[1]={e.zmip.Get(),DXGI_FORMAT_R32_FLOAT,0,5};s[2]={e.ao[fonte].Get(),DXGI_FORMAT_R16_FLOAT};if(mov)s[4]={mov,mv_f};s[12]={e.ao[0].Get(),DXGI_FORMAT_R16_FLOAT};
  s[3]={e.bordas.Get(),DXGI_FORMAT_R8_UNORM};
  s[5]={e.hist[e.atual].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT};u[7]={e.dbg.Get(),DXGI_FORMAT_R8G8B8A8_UNORM};u[8]={e.hist[cur].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT};
  tr(e.hist[cur].Get(),SRV,UAV);
  lista->SetPipelineState(e.pso[Temporal].Get());lista->SetComputeRootDescriptorTable(1,bloco(e,d,s,u));lista->Dispatch(gx,gy,1);
  tr(e.hist[cur].Get(),UAV,SRV);}

 tr(prof,para_leitura(sp),sp);if(mov)tr(mov,para_leitura(sm),sm);
 tr(e.zmip.Get(),SRV,UAV);tr(e.ao[fonte].Get(),SRV,UAV);if(fonte!=0u)tr(e.ao[0].Get(),SRV,UAV);tr(e.bordas.Get(),SRV,UAV);tr(e.dbg.Get(),UAV,SRV);
 e.atual=cur;e.hist_ok=true;++e.quadro;
 }else{tr(prof,para_leitura(sp),sp);if(mov)tr(mov,para_leitura(sm),sm);tr(e.zmip.Get(),SRV,UAV);tr(e.dbg.Get(),UAV,SRV);e.hist_ok=false;}
 e.pronto=true;e.motivo=nullptr;
 KA& a=e.ka;a={};a.rW=W;a.rH=H;a.aW=Wa;a.aH=Ha;a.aS=S;
 a.jx=std::isfinite(en.jitter_x)?en.jitter_x*o.desjitter:0.f;a.jy=std::isfinite(en.jitter_y)?en.jitter_y*o.desjitter:0.f;
 a.ups=unsigned(o.upsample);a.forca=o.forca;a.intens=par.ao_intens;a.d_fim=par.ao_alcance;a.d_ini=o.dist_ini>=0.f?(std::min)(o.dist_ini,par.ao_alcance*.999f):.6f*par.ao_alcance;
 a.d_min=o.dist_min;a.pele=par.ao_pele;a.pers=o.personagem;a.claro=par.ao_claro;a.c_ini=o.claro_ini;a.c_fim=o.claro_fim;a.mb=unsigned(o.multibounce);a.ver=par.ao_ver;a.pad1=o.instavel?1u:0u;
 if(!ao_ligada){a.forca=0.f;a.mb=0u;a.ver=0u;a.ups=2u;a.pele=0.f;a.pers=0.f;a.claro=0.f;a.pad1=0u;}
 e.cor_entrada=nullptr;
 if(ao_ligada&&o.upsample==1&&en.cor_entrada){auto* ci=static_cast<ID3D12Resource*>(en.cor_entrada);const auto vf=nrformato::vista_cor(ci->GetDesc().Format);const auto sc=estado(en.cor_estado);
  if(nrformato::entrada(ci,vf)&&sc!=nrd3d::estado_recusado&&(lista->GetType()!=D3D12_COMMAND_LIST_TYPE_COMPUTE||nrffx_estado::valido_em_compute(unsigned(sc)))){e.cor_entrada=ci;e.cor_entrada_vf=vf;e.cor_entrada_estado=en.cor_estado;}}
 if(++e.feitos==1||e.feitos%6000==0){char t[220];std::snprintf(t,sizeof t,"quadros=%llu render %ux%u ao %ux%u passo %u modo %d fov %.1f near %.3g inf %d inv %d metros %.3g fov_ini %d pulados %llu",
  e.feitos,W,H,Wa,Ha,S,o.modo,double(cam.fov)*180./nrao::pi,double(cam.near_),int(cam.inf),int(cam.inv),double(cam.metros),int(cam.fov_ini),e.pulados);log(t);}
 return true;}

inline void cancelar(ID3D12GraphicsCommandList* lista){auto& e=estado_();if(!e.pronto)return;constexpr auto SRV=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,UAV=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
 nrd3d::transicao(lista,e.dbg.Get(),SRV,UAV);if(e.il.tex){nrd3d::transicao(lista,e.il.tex,SRV,UAV);e.il={};}e.pronto=false;}

struct Mascara { ID3D12Resource* buf=nullptr;unsigned w=0,h=0,pitch=0,bits=0;unsigned long long bytes=0; };

inline void aplicar_cor(ID3D12Device* d,ID3D12GraphicsCommandList* lista,ID3D12Resource* src,ID3D12Resource* dst,DXGI_FORMAT vf,unsigned w,unsigned h,bool hdr,float expo,const Mascara& ms,bool no_lugar=false){
 auto& e=estado_();if(!e.pronto)return;KA a=e.ka;a.ow=w;a.oh=h;a.hdr=hdr?1u:0u;a.expo=expo>0.f&&expo<1e6f?expo:1.f;a.no_lugar=no_lugar?1u:0u;
 ID3D12Resource* il=e.il.tex;if(il)for(unsigned i=0;i<9;++i)a.il[i]=e.il.k[i];
 if(ms.buf&&(a.pers>0.f||(il&&e.il.pers>0.f))){a.ms_w=ms.w;a.ms_h=ms.h;a.ms_pitch=ms.pitch;a.ms_bits=ms.bits;}else a.pers=0.f;
 if(a.pele>0.f||(il&&e.il.pele>0.f))for(unsigned i=0;i<nrfiltros::n_cabecas&&i<4u;++i){const auto& c=nrfiltros::cabecas[i];if(!(c.r>1.f))continue;float* r=a.rost+4u*a.nrost++;r[0]=c.cx;r[1]=c.cy;r[2]=c.r;r[3]=c.ry>1.f?c.ry:0.f;}
 constexpr auto SRV=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
 ID3D12Resource* ci=a.ups==1u?e.cor_entrada:nullptr;const auto sc=ci?estado(e.cor_entrada_estado):SRV;auto para_leitura=[](D3D12_RESOURCE_STATES s){return (s&SRV)?s:SRV;};
 if(ci){nrd3d::transicao(lista,ci,sc,para_leitura(sc));a.tem_entrada=1u;}
 Vista s[nSrv]{},u[nUav]{};s[6]={src,vf};s[7]={e.hist[e.atual].Get(),DXGI_FORMAT_R16G16B16A16_FLOAT};
 if(a.ms_bits)s[8]={ms.buf,DXGI_FORMAT_UNKNOWN,0,1,unsigned(ms.bytes)};
 if(ci)s[9]={ci,e.cor_entrada_vf};s[10]={e.dbg.Get(),DXGI_FORMAT_R8G8B8A8_UNORM};u[9]={dst,vf};
 s[3]={e.bordas.Get(),DXGI_FORMAT_R8_UNORM};nrd3d::transicao(lista,e.bordas.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,SRV);
 if(il){s[11]={il,DXGI_FORMAT_R16G16B16A16_FLOAT};s[1]={e.zmip.Get(),DXGI_FORMAT_R32_FLOAT,0,5};nrd3d::transicao(lista,e.zmip.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,SRV);}
 ID3D12DescriptorHeap* hs[]{e.heap.Get()};lista->SetDescriptorHeaps(1,hs);lista->SetComputeRootSignature(e.raiz_a.Get());lista->SetComputeRoot32BitConstants(0,sizeof(KA)/4,&a,0);
 lista->SetPipelineState(e.pso[Aplica].Get());lista->SetComputeRootDescriptorTable(1,bloco(e,d,s,u));lista->Dispatch((w+7u)/8u,(h+7u)/8u,1);
 nrd3d::transicao(lista,e.bordas.Get(),SRV,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 if(ci)nrd3d::transicao(lista,ci,para_leitura(sc),sc);
 if(il){nrd3d::transicao(lista,il,SRV,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);nrd3d::transicao(lista,e.zmip.Get(),SRV,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);e.il={};}
 nrd3d::transicao(lista,e.dbg.Get(),SRV,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);e.pronto=false;}
}
