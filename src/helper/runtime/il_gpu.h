#pragma once

#include "il.h"
#include "ao_gpu.h"
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
#include "nr_il_il_rad.h"
#include "nr_il_il_principal.h"
#include "nr_il_il_temporal.h"
#include "nr_il_il_atrous.h"
#endif
namespace nrilgpu {
using Microsoft::WRL::ComPtr;
inline const char* hlsl=R"(
#define PI 3.14159265358979
#define PI2 1.5707963267949
static const float CEU=60000.0;
cbuffer K:register(b0){uint W,H,Wi,Hi,Si,modo,fatias,passos;float mulx,muly,addx,addy;float er,falloff,distrib,raio_max_px;float esp,mip_off,luzrel,mipb;
 uint esp_lin,quadro,R,fase;uint tudo,nemis,fonte,hdr;float costas,ceu_w,jx,jy;uint ow,oh,rebate,reset_;float rforca,rmed,mvx,mvy;float djx,djy,qmax,csig;
 float tol;uint passo,final_,ver;uint nm;uint emis,cont,plonge;float klonge;float capl;};
Texture2D<float> zmip:register(t0);Texture2D<float4> fonteT:register(t1);Texture2D<float4> mvTex:register(t2);Texture2D<float4> radT:register(t3);
Texture2D<float4> novoT:register(t4);Texture2D<float4> histT:register(t5);Texture2D<float> zT:register(t6);Texture2D<float2> momT:register(t7);Texture2D<float4> entT:register(t8);
RWTexture2D<float4> rad0:register(u0);RWTexture2D<float4> rad1:register(u1);RWTexture2D<float4> rad2:register(u2);RWTexture2D<float4> rad3:register(u3);RWTexture2D<float4> rad4:register(u4);
RWTexture2D<float4> novoOut:register(u5);RWTexture2D<float4> histOut:register(u6);RWTexture2D<float> zOut:register(u7);RWTexture2D<float2> momOut:register(u8);RWTexture2D<float4> filtOut:register(u9);
SamplerState sl:register(s0);
float lumi(float3 c){return dot(c,float3(0.2126,0.7152,0.0722));}
float zm(int2 q,uint m){int2 dm=int2(max(W>>m,1u),max(H>>m,1u));return zmip.Load(int3(clamp(q,int2(0,0),dm-1),m));}
float3 vpos(float2 uv,float z){return float3((float2(mulx,muly)*uv+float2(addx,addy))*z,z);}
// os mesmos da AO (ao_gpu.h): XeGTAO_FastACos, curva de Hilbert nossa, bordas/normais do XeGTAO (MIT), integral de cosseno, setores
float facos(float x){float a=min(abs(x),1.0);float r=(-0.156583*a+PI2)*sqrt(1.0-a);return x>=0.0?r:PI-r;}
uint hilbert(uint2 p){uint x=p.x&63u,y=p.y&63u,d=0u;[unroll]for(uint s=32u;s>0u;s>>=1){uint rx=(x&s)!=0u?1u:0u,ry=(y&s)!=0u?1u:0u;d+=s*s*((3u*rx)^ry);
 x&=s-1u;y&=s-1u;if(ry==0u){if(rx==1u){x=s-1u-x;y=s-1u-y;}uint t=x;x=y;y=t;}}return d;}
float4 bordas4(float zc,float zl,float zr,float zt,float zb){float4 e=float4(zl,zr,zt,zb)-zc;float slr=(e.y-e.x)*0.5,stb=(e.w-e.z)*0.5;
 float4 a=e+float4(slr,-slr,stb,-stb);e=min(abs(e),abs(a));return saturate(1.25-e/(zc*0.011));}
float3 normal4(float4 e,float3 c,float3 l,float3 r,float3 t,float3 b){float4 w=saturate(float4(e.x*e.z,e.z*e.y,e.y*e.w,e.w*e.x)+0.01);
 l=normalize(l-c);r=normalize(r-c);t=normalize(t-c);b=normalize(b-c);return normalize(w.x*cross(l,t)+w.y*cross(t,r)+w.z*cross(r,b)+w.w*cross(b,l));}
float iarc(float h,float n,float cn){return (cn+2.0*h*sin(n)-cos(2.0*h-n))/4.0;}
float Ah(float h,float n,float cn){float i=iarc(h,n,cn);return h>=0.0?i:-i;}
float umap(float h,float n,float cn){float lo=n-PI2;h=clamp(h,lo,n+PI2);if(modo==1u)return saturate((h-lo)/PI);float t=cn+n*sin(n);
 return t>1e-5?saturate((Ah(h,n,cn)-Ah(lo,n,cn))/t):saturate((h-lo)/PI);}
uint setores(float a,float b){if(b-a<0.5/32.0)return 0u;uint s0=min(uint(floor(a*32.0)),31u);uint c=min(uint(ceil((b-a)*32.0)),32u-s0);return c>=32u?0xFFFFFFFFu:(((1u<<c)-1u)<<s0);}
// rodizio (il.h mapa/novo): R = 2 xadrez, R = 4 um texel do bloco 2x2 na ordem de Bayer 0,3,1,2; tudo = o texel inteiro a cada R quadros
bool novoEm(int2 p){if(tudo!=0u)return fase==0u;if(R<=1u)return true;if(R==2u)return ((uint(p.x)+uint(p.y)+fase)&1u)==0u;
 uint2 b=fase==0u?uint2(0,0):fase==1u?uint2(1,1):fase==2u?uint2(1,0):uint2(0,1);return all((uint2(p)&1u)==b);}
// luz media local (Ybar): a radiancia no MIP pelo raio na tela (em texels da IL), entre 2 e 4
// uv de render -> uv da radiancia (a textura cobre Wi x Si >= W pixels de render)
float2 ur(float2 u){return u*float2(W,H)/float2(Wi*Si,Hi*Si);}
float ybar(float2 uv,float zc){float ssr=min(er/max(zc*mulx/float(W),1e-9),raio_max_px*float(H)/1080.0)/float(Si);
 float M=min(clamp(round(log2(max(ssr,1.0))),2.0,4.0),float(nm-1u));float4 r=radT.SampleLevel(sl,ur(uv),M);return r.a>1e-4?lumi(max(r.rgb,0.0)/r.a):0.0;}
// ---- 1 radiancia ----
float2 mvL(int2 r){uint mw,mh;mvTex.GetDimensions(mw,mh);float2 mv=float2(0.0,0.0);if(mw>0u&&mh>0u)mv=mvTex.Load(int3(min(r,int2(mw,mh)-1),0)).xy*float2(mvx,mvy);
 if(!all(mv==mv)||any(abs(mv)>1e5))mv=float2(0.0,0.0);return mv;}
// a IL do quadro anterior no pixel de render r (reprojetada, so as amostras com a profundidade da vizinhanca); n = 0 sem historico
float4 histRep(int2 r,float zc,float2 mv,float zmn,float zmx){float2 pp=(float2(r)+mv-float2(djx,djy))/float(Si);float2 b=floor(pp),f=pp-b;float tl=tol+0.01*length(mv);
 float4 s=0.0;float sw=0.0;
 [unroll]for(int k=0;k<4;++k){int2 o=int2(k&1,k>>1);int2 q=int2(b)+o;float wb=(o.x!=0?f.x:1.0-f.x)*(o.y!=0?f.y:1.0-f.y);
  if(q.x<0||q.y<0||q.x>=int(Wi)||q.y>=int(Hi)||!(wb>0.0))continue;float hz=zT.Load(int3(q,0));if(hz>=CEU)continue;
  float wz=saturate(2.0-max(max(zmn-hz,hz-zmx),0.0)/max(tl*zc,1e-6));if(wz>0.0){wb*=wz;s+=wb*histT.Load(int3(q,0));sw+=wb;}}
 return sw>1e-3?s/sw:float4(0.0,0.0,0.0,0.0);}
float3 lerFonte(int2 q,out bool ruim){float4 c;
 if(fonte==0u)c=fonteT.Load(int3(q,0));
 else{int2 o=clamp(int2(floor((float2(q)+0.5-float2(jx,jy))*float2(ow,oh)/float2(W,H))),int2(0,0),int2(ow,oh)-1);c=fonteT.Load(int3(o,0));}
 float3 x=c.rgb;ruim=!all(x==x)||any(abs(x)>3.0e38);if(ruim)return float3(0.0,0.0,0.0);
 x=max(x,0.0);if(hdr==0u){x=min(pow(x,2.2),1.0);x=x/(1.1-x);}return x;}   // UNORM: gama + pseudo-HDR (a mesma ida da AO)
float4 texelRad(int2 t){float4 s=0.0;int n=int(Si);
 for(int j=0;j<n;++j)for(int i=0;i<n;++i){int2 q=min(t*n+int2(i,j),int2(W,H)-1);float z=zm(q,0u);bool ruim;float3 c=lerFonte(q,ruim);
  bool ceu=ruim||z>=CEU;float w=ceu?ceu_w:1.0;if(ruim)c=0.0;
  if(rebate!=0u&&!ceu){float zmn=z,zmx=z;   // 2 rebatimentos: + RebateForca x albedo (rho_med x a cor) x a IL anterior reprojetada
   [unroll]for(int b=-1;b<=1;++b)[unroll]for(int a=-1;a<=1;++a){float zz=zm(q+int2(a,b),0u);if(zz<CEU){zmn=min(zmn,zz);zmx=max(zmx,zz);}}
   float4 h=histRep(q,z,mvL(q),zmn,zmx);float l=lumi(c);if(h.a>0.0&&l>0.0)c+=rforca*rmed*(c/l)*max(h.rgb,0.0);}
  s+=float4(w*c,w);}
 return s/float(n*n);}
groupshared float4 gr[8][8];
[numthreads(8,8,1)]void il_rad(uint3 id:SV_DispatchThreadID,uint3 gt:SV_GroupThreadID){
 int2 b=int2(id.xy),t0=b*2;float4 s=0.0;
 [unroll]for(int k=0;k<4;++k){int2 t=t0+int2(k&1,k>>1);float4 v=texelRad(min(t,int2(Wi,Hi)-1));rad0[t]=v;s+=v;}
 float4 m1=s*0.25;rad1[b]=m1;gr[gt.x][gt.y]=m1;GroupMemoryBarrierWithGroupSync();
 if(all((gt.xy%2u)==0u)){float4 m2=(gr[gt.x][gt.y]+gr[gt.x+1][gt.y]+gr[gt.x][gt.y+1]+gr[gt.x+1][gt.y+1])*0.25;rad2[b/2]=m2;gr[gt.x][gt.y]=m2;}
 GroupMemoryBarrierWithGroupSync();
 if(all((gt.xy%4u)==0u)){float4 m3=(gr[gt.x][gt.y]+gr[gt.x+2][gt.y]+gr[gt.x][gt.y+2]+gr[gt.x+2][gt.y+2])*0.25;rad3[b/4]=m3;gr[gt.x][gt.y]=m3;}
 GroupMemoryBarrierWithGroupSync();
 if(all((gt.xy%8u)==0u)){float4 m4=(gr[gt.x][gt.y]+gr[gt.x+4][gt.y]+gr[gt.x][gt.y+4]+gr[gt.x+4][gt.y+4])*0.25;rad4[b/8]=m4;}}
// ---- 2 principal ----
// normal do emissor pela profundidade no MIP m (NeuralFiltroIlNormalEmissor=1)
float3 normalEm(float2 su,uint m){int2 dm=int2(max(W>>m,1u),max(H>>m,1u));int2 t=clamp(int2(floor(su*float2(dm))),int2(0,0),dm-1);float2 pm=1.0/float2(dm);
 float2 u=(float2(t)+0.5)*pm;float zc=zmip.Load(int3(t,m)),zl=zmip.Load(int3(max(t-int2(1,0),0),m)),zr=zmip.Load(int3(min(t+int2(1,0),dm-1),m)),
 zt=zmip.Load(int3(max(t-int2(0,1),0),m)),zb=zmip.Load(int3(min(t+int2(0,1),dm-1),m));float4 e=bordas4(zc,zl,zr,zt,zb);
 return normal4(e,vpos(u,zc),vpos(u-float2(pm.x,0),zl),vpos(u+float2(pm.x,0),zr),vpos(u-float2(0,pm.y),zt),vpos(u+float2(0,pm.y),zb));}
[numthreads(8,8,1)]void il_principal(uint3 id:SV_DispatchThreadID){
 uint2 pu;if(R<=1u||tudo!=0u)pu=id.xy;else if(R==2u)pu=uint2(2u*id.x+((id.y+fase)&1u),id.y);
 else{uint2 bb=fase==0u?uint2(0,0):fase==1u?uint2(1,1):fase==2u?uint2(1,0):uint2(0,1);pu=2u*id.xy+bb;}
 if(pu.x>=Wi||pu.y>=Hi)return;int2 pa=int2(pu);int2 r=min(pa*int(Si),int2(W,H)-1);
 float zc=zm(r,0u);if(zc>=CEU){novoOut[pa]=float4(0.0,0.0,0.0,1.0);return;}
 int s=int(Si);float zl=zm(r-int2(s,0),0u),zr=zm(r+int2(s,0),0u),zt=zm(r-int2(0,s),0u),zb=zm(r+int2(0,s),0u);float4 e=bordas4(zc,zl,zr,zt,zb);
 float2 px=1.0/float2(W,H);float2 uv=(float2(r)+0.5)*px;
 float3 nrm=normal4(e,vpos(uv,zc),vpos(uv-float2(s,0)*px,zl),vpos(uv+float2(s,0)*px,zr),vpos(uv-float2(0,s)*px,zt),vpos(uv+float2(0,s)*px,zb));
 float yb=ybar(uv,zc);float lim=max(luzrel*yb,1e-30);
 zc*=0.99999;float3 c=vpos(uv,zc);float3 v=normalize(-c);
 float fr=max(falloff*er,1e-5),ff=er*(1.0-falloff);float fmul=-1.0/fr,fadd=ff/fr+1.0;
 float ssr=min(er/max(zc*mulx*px.x,1e-9),raio_max_px*float(H)/1080.0);
 // ssil-forte (26/10): anel longe (plonge passos depois dos de perto, ate klonge x o raio, teto capl px@1080p, MIPs grossos): as superficies
 // grandes (parede, chao, morro) tingem o entorno; a queda dele e pelo raio longe. Sem ganho (ssrL <= 1,1 ssr) = so o anel de perto.
 float erL=er*klonge,frL=max(falloff*erL,1e-5),ffL=erL*(1.0-falloff);float fmulL=-1.0/frL,faddL=ffL/frL+1.0;
 float ssrL=min(erL/max(zc*mulx*px.x,1e-9),capl*float(H)/1080.0);uint np=passos+(plonge>0u&&ssrL>ssr*1.1?plonge:0u);
 uint qi=R>1u?quadro/R:quadro;float2 nz=frac(0.5+float(hilbert(pu)+288u*(qi%64u))*float2(0.7548776662,0.5698402910));
 float minS=1.3/max(ssr,1e-3);float esp_v=esp_lin!=0u?esp*zc:esp;float3 IL=0.0,nsum=0.0;
 [loop]for(uint fi=0u;fi<fatias;++fi){
  float phi=(float(fi)+nz.x)/float(fatias)*PI;float cp=cos(phi),sp=sin(phi);float2 om=float2(cp,-sp)*ssr;
  float3 dv=float3(cp,sp,0.0);float3 odv=dv-dot(dv,v)*v;float3 ax=normalize(cross(odv,v));
  float3 pn=nrm-ax*dot(nrm,ax);float pl=length(pn);float cn=saturate(dot(pn,v)/max(pl,1e-6));float n=sign(dot(odv,pn))*facos(cn);
  float lo0=cos(n+PI2),lo1=cos(n-PI2);uint masc=0u;float3 Lf=0.0;float3 ant0=c,ant1=c;
  float ap0=-1.0,ap1=-1.0;float lant=0.0;   // ssil-forte: angulo da frente da amostra anterior de cada lado (-1 = nenhuma) e o passo anterior em px
  [loop]for(uint k=0u;k<np;++k){
   float sn=frac(nz.y+float(fi+k*passos)*0.6180339887498948);float2 off;float fm=fmul,fa=fadd;
   if(k<passos){float sv=pow((float(k)+sn)/float(passos),distrib)+minS;off=sv*om;}
   else{float t=(float(k-passos)+sn)/float(plonge);off=lerp(ssr,ssrL,t)*float2(cp,-sp);fm=fmulL;fa=faddL;}   // anel longe
   uint m=uint(clamp(log2(max(length(off),1e-6))-mip_off,0.0,4.0)+0.5);off=round(off);float lpx=length(off);
   int2 dm=int2(max(W>>m,1u),max(H>>m,1u));
   [unroll]for(int lado=0;lado<2;++lado){
    float2 su=uv+(lado==0?off:-off)*px;if(any(su<0.0)||any(su>1.0))continue;   // fora da tela: sem luz nem oclusao
    float sz=zmip.Load(int3(clamp(int2(floor(su*float2(dm))),int2(0,0),dm-1),m));
    float3 p3=vpos(su,sz);float3 dd=p3-c;float dl=length(dd);float3 hv=dd/max(dl,1e-9);float wgt=saturate(dl*fm+fa);
    float lowc=lado==0?lo0:lo1;float shc=lerp(lowc,dot(hv,v),wgt);
    float3 bd=p3+normalize(p3)*esp_v-c;float bc=lerp(lowc,dot(bd/max(length(bd),1e-9),v),wgt);
    float af=facos(shc),ab=facos(bc);float a0=min(af,ab),a1=max(af,ab);
    float3 ant=lado==0?ant0:ant1;float apv=lado==0?ap0:ap1;
    // ssil-forte (26/10): continuidade -- a amostra e a anterior do mesmo lado na mesma superficie (distancia 3D <= cont x o passo na tela em
    // metros, na profundidade da amostra): os setores entre as duas tambem sao dela (a espessura fixa deixava a parede "vazada" entre os passos)
    if(cont!=0u&&apv>=0.0&&wgt>0.0&&length(p3-ant)<=4.0*max(abs(lpx-lant),1.0)*sz*mulx*px.x){a0=min(a0,apv);a1=max(a1,apv);}
    if(lado==0)ap0=wgt>0.0?af:-1.0;else ap1=wgt>0.0?af:-1.0;   // amostra sem peso (fora do raio) nao ancora a continuidade (ultima de perto -> 1a do anel longe)
    uint bits=lado==0?setores(umap(a0,n,cn),umap(a1,n,cn)):setores(umap(-a1,n,cn),umap(-a0,n,cn));
    uint nov=bits&~masc;masc|=bits;uint kn=countbits(nov);   // SSRT3 ComputeOccludedBitfield: so os setores ocupados pela 1a vez
    if(kn>0u&&wgt>0.0){
     float lod=clamp(log2(max(length(off)/float(Si),1.0))-mipb,0.0,float(nm-1u));float4 rs=radT.SampleLevel(sl,ur(su),lod);
     if(rs.a>=0.25){
      float3 L=max(rs.rgb,0.0)/rs.a;L=L/(1.0+lumi(L)/lim);   // anti-vagalume relativo a media local
      float3 ns;if(nemis!=0u)ns=normalEm(su,m);
      else{float3 t=p3-ant;ns=(lado==0?-1.0:1.0)*cross(t/max(length(t),1e-9),ax);ns=ns/max(length(ns),1e-9);}   // SSRT3 Normal Approximation
      float g0=dot(ns,-hv);float g=costas>0.0&&dot(ns,v)>0.0?(g0<0.0?-g0*costas:g0):saturate(g0);   // SSRT3 backface
      if(emis!=0u)g=costas>0.0&&dot(ns,v)>0.0&&g0<0.0?-g0*costas:saturate(g0*4.0);   // ssil-forte: quase so o teste de frente/costas (o setor ja e o angulo solido; rasante = 0: o proprio plano nao se ilumina)
      float w=float(kn)/32.0*wgt*g;if(modo==1u)w*=saturate(dot(nrm,hv))*PI2;
      Lf+=w*L;nsum+=ns*float(kn);}}
    if(lado==0)ant0=p3;else ant1=p3;}
   lant=lpx;}
  IL+=modo==1u?Lf:lerp(pl,1.0,0.05)*(cn+n*sin(n))*Lf;}
 IL/=float(fatias);
 novoOut[pa]=float4(ver==7u?(dot(nsum,nsum)>0.0?normalize(nsum)*0.5+0.5:float3(0.5,0.5,0.5)):IL,1.0);}
)"
R"(// ---- 3 temporal (ssil-forte 26/10: a string vem em 2 pedacos: limite de literal do MSVC) ----
float3 ycocg(float3 c){return float3(dot(c,float3(0.25,0.5,0.25)),dot(c,float3(0.5,0.0,-0.5)),dot(c,float3(-0.25,0.5,-0.25)));}
float3 rgbDe(float3 y){return float3(y.x+y.y-y.z,y.x+y.z,y.x-y.y-y.z);}
float3 vista(int2 pa,float n){float4 r=radT.Load(int3(pa,0));float3 g=saturate(r.a).xxx;
 if(ver==2u||ver==7u)g=novoT.Load(int3(pa,0)).rgb;else if(ver==3u)g=r.a>1e-4?max(r.rgb,0.0)/r.a:float3(0.0,0.0,0.0);else if(ver==5u)g=saturate(n/max(qmax,1.0)).xxx;
 return g;}
[numthreads(8,8,1)]void il_temporal(uint3 id:SV_DispatchThreadID){
 if(id.x>=Wi||id.y>=Hi)return;int2 pa=int2(id.xy);int2 r=min(pa*int(Si),int2(W,H)-1);
 float zc=zm(r,0u);bool dbg=ver==2u||ver==3u||ver>=5u;
 if(zc>=CEU){histOut[pa]=float4(0.0,0.0,0.0,0.0);zOut[pa]=CEU;momOut[pa]=float2(0.0,0.0);if(dbg)filtOut[pa]=float4(ver==6u?vista(pa,0.0):float3(0.0,0.0,0.0),1.0);return;}
 bool nv=novoEm(pa);float3 cur=novoT.Load(int3(pa,0)).rgb;
 float2 mv=mvL(r);float4 h=float4(0.0,0.0,0.0,0.0);float2 hm=float2(0.0,0.0);bool hv=false;
 if(reset_==0u){float zmn=zc,zmx=zc;int s=int(Si);
  [unroll]for(int j=-1;j<=1;++j)[unroll]for(int i=-1;i<=1;++i){float z=zm(r+int2(i,j)*s,0u);if(z<CEU){zmn=min(zmn,z);zmx=max(zmx,z);}}
  float2 pp=(float2(r)+mv-float2(djx,djy))/float(Si);float2 b=floor(pp),f=pp-b;float tl=tol+0.01*length(mv);float sw=0.0;
  [unroll]for(int k=0;k<4;++k){int2 o=int2(k&1,k>>1);int2 q=int2(b)+o;float wb=(o.x!=0?f.x:1.0-f.x)*(o.y!=0?f.y:1.0-f.y);
   if(q.x<0||q.y<0||q.x>=int(Wi)||q.y>=int(Hi)||!(wb>0.0))continue;float hz=zT.Load(int3(q,0));if(hz>=CEU)continue;
   float wz=saturate(2.0-max(max(zmn-hz,hz-zmx),0.0)/max(tl*zc,1e-6));if(wz>0.0){wb*=wz;h+=wb*histT.Load(int3(q,0));hm+=wb*momT.Load(int3(q,0));sw+=wb;}}
  if(sw>1e-3){h/=sw;hm/=sw;hv=h.a>0.5;}}
 // vizinhanca: so os texels novos deste quadro (3x3; 5x5 no rodizio 2 e 4), em YCoCg; e a media bilateral pela profundidade (preenchimento)
 int rv=R>=2u&&tudo==0u?2:1;float3 s1=0.0,s2=0.0,fill=0.0;float cnt=0.0,fw=0.0;
 for(int j=-rv;j<=rv;++j)for(int i=-rv;i<=rv;++i){int2 q=pa+int2(i,j);if(q.x<0||q.y<0||q.x>=int(Wi)||q.y>=int(Hi)||!novoEm(q))continue;
  float zq=zm(min(q*int(Si),int2(W,H)-1),0u);if(zq>=CEU)continue;float3 t=max(novoT.Load(int3(q,0)).rgb,0.0);float3 y=ycocg(t);s1+=y;s2+=y*y;cnt+=1.0;
  float wq=exp(-abs(zq-zc)/max(tol*zc,1e-6))+1e-6;fill+=wq*t;fw+=wq;}
 // a caixa do clamp: media +- (ClampSigma x desvio dos novos da janela + 0,5 x a luma media deles). A folga relativa (escala da imagem: vale
 // no RDR2 e no RE) segura o pico da luz junto da quina (vizinhos de linhas com menos luz: o clamp so por desvio puxava o historico para
 // baixo, -20% todo quadro e -40% no rodizio 4) e continua cortando a luz velha que sumiu (media ~0 => folga ~0)
 // texel sem amostra nova neste quadro (rodizio): no minimo o desvio temporal dele (o pico nao e cortado a cada quadro de espera)
 if(hv&&cnt>0.0){float3 mu=s1/cnt,sg=sqrt(max(s2/cnt-mu*mu,0.0));float3 fo=csig*sg+0.5*max(mu.x,0.0);if(!nv)fo=max(fo,sqrt(max(hm.y-hm.x*hm.x,0.0)).xxx);h.rgb=max(rgbDe(clamp(ycocg(h.rgb),mu-fo,mu+fo)),0.0);}
 float qe=max(lerp(qmax,4.0,saturate(length(mv)/32.0)),1.0);float3 o;float n;float2 mo;float lc=lumi(max(cur,0.0));
 if(nv){if(hv){n=min(h.a+1.0,qe);o=lerp(h.rgb,max(cur,0.0),1.0/n);mo=lerp(hm,float2(lc,lc*lc),1.0/n);}else{n=1.0;o=max(cur,0.0);mo=float2(lc,lc*lc);}}
 else{if(hv){o=h.rgb;n=min(h.a,qe);mo=hm;}else{o=fw>0.0?fill/fw:float3(0.0,0.0,0.0);n=1.0;float l=lumi(o);mo=float2(l,l*l);}}
 histOut[pa]=float4(o,n);zOut[pa]=zc;momOut[pa]=mo;if(dbg)filtOut[pa]=float4(vista(pa,n),1.0);}
// ---- 4 a-trous (SVGF simplificado): kernel (1/4,1/2,1/4)^2 x profundidade x luma (so com n >= 4: com poucos quadros filtra mais) ----
[numthreads(8,8,1)]void il_atrous(uint3 id:SV_DispatchThreadID){
 if(id.x>=Wi||id.y>=Hi)return;int2 pa=int2(id.xy);int2 r=min(pa*int(Si),int2(W,H)-1);float zc=zm(r,0u);
 float yb=final_!=0u&&zc<CEU?ybar((float2(r)+0.5)/float2(W,H),zc):0.0;
 float4 c=entT.Load(int3(pa,0));if(zc>=CEU){filtOut[pa]=float4(0.0,0.0,0.0,0.0);return;}
 if(passo==0u){filtOut[pa]=float4(c.rgb,yb);return;}
 float n=histT.Load(int3(pa,0)).a;float2 mo=momT.Load(int3(pa,0));float sl2=sqrt(max(mo.y-mo.x*mo.x,0.0));float lp=lumi(c.rgb);
 float3 s=0.0;float sw=0.0;int ps=int(passo);
 [unroll]for(int j=-1;j<=1;++j)[unroll]for(int i=-1;i<=1;++i){int2 q=pa+int2(i,j)*ps;if(q.x<0||q.y<0||q.x>=int(Wi)||q.y>=int(Hi))continue;
  float zq=zm(min(q*int(Si),int2(W,H)-1),0u);if(zq>=CEU)continue;float3 cq=entT.Load(int3(q,0)).rgb;
  float k=(i==0?0.5:0.25)*(j==0?0.5:0.25);float wz=exp(-abs(zq-zc)/(tol*zc*float(ps)+1e-6));
  float wl=n>=4.0?exp(-abs(lumi(cq)-lp)/(4.0*sl2+1e-3*lp+1e-12)):1.0;float w=k*wz*wl;s+=w*cq;sw+=w;}
 filtOut[pa]=float4(sw>0.0?s/sw:c.rgb,yb);}
)";
struct K { std::uint32_t W,H,Wi,Hi,Si,modo,fatias,passos;float mulx,muly,addx,addy;float er,falloff,distrib,raio_max_px;float esp,mip_off,luzrel,mipb;
 std::uint32_t esp_lin,quadro,R,fase,tudo,nemis,fonte,hdr;float costas,ceu_w,jx,jy;std::uint32_t ow,oh,rebate,reset;float rforca,rmed,mvx,mvy;float djx,djy,qmax,csig;
 float tol;std::uint32_t passo,final_,ver,nm;std::uint32_t emis,cont,plonge;float klonge,capl; };
static_assert(sizeof(K)==54*4,"54 constantes em b0 (ssil-forte: + 5)");
enum Pso : unsigned { Rad, Principal, Temporal, Atrous, PsoN };
inline constexpr unsigned nSrv=9,nUav=10,nDesc=nSrv+nUav,blocos=8u*8u;
struct Estado {
 bool falhou=false;ComPtr<ID3D12Device> dev;ComPtr<ID3D12RootSignature> raiz;ComPtr<ID3D12PipelineState> pso[PsoN];ComPtr<ID3D12DescriptorHeap> heap;unsigned bloco=0;
 ComPtr<ID3D12Resource> rad,novo,hist[2],z[2],mom[2],filt[2];unsigned W=0,H=0,Wi=0,Hi=0,Si=0,nm=0;unsigned atual=0;bool hist_ok=false;float fov_ant=0.f;unsigned quadro=0,ver_ant=0;
 float jx_ant=0.f,jy_ant=0.f;unsigned long long feitos=0,pulados=0,fonte1=0;const char* motivo=nullptr;int fonte_ult=-1;nrd3d::Aposentados<64> aposentados;unsigned slot=0;
 nrd3d::Trocas trocas;
 void aposentar_(ComPtr<ID3D12Resource>& r);
};
inline void log(const char* t){
 (void)t;
}
inline void Estado::aposentar_(ComPtr<ID3D12Resource>& r){if(aposentados.aposentar(r,slot))log("APOSENTADOS cheios: recurso trocado fica vivo");}
inline Estado& estado_(){static Estado* e=new Estado;return *e;}
inline std::atomic<bool> teste_compilar{false};
inline void teste_recriar(){auto& e=estado_();for(auto& p:e.pso)p.Reset();e.raiz.Reset();e.dev.Reset();}
inline void teste_reiniciar(){auto& e=estado_();e.hist_ok=false;e.quadro=0;e.trocas.seguidas=0;}
inline bool preparar(Estado& e,ID3D12Device* d){
 if(e.pso[Atrous]&&e.dev.Get()==d)return true;
 e.dev=d;const char* entradas[PsoN]{"il_rad","il_principal","il_temporal","il_atrous"};ComPtr<ID3DBlob> cs[PsoN];const void* codigo[PsoN]{};SIZE_T tam[PsoN]{};bool embutido=false;
#if defined(NR_FILTROS_CS) && NR_FILTROS_CS
 if(!teste_compilar.load()){embutido=true;codigo[0]=g_nr_il_il_rad;tam[0]=sizeof g_nr_il_il_rad;codigo[1]=g_nr_il_il_principal;tam[1]=sizeof g_nr_il_il_principal;
  codigo[2]=g_nr_il_il_temporal;tam[2]=sizeof g_nr_il_il_temporal;codigo[3]=g_nr_il_il_atrous;tam[3]=sizeof g_nr_il_il_atrous;}
#endif
 if(!embutido){HMODULE c=LoadLibraryExW(L"d3dcompiler_47.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!c){log("FALHA d3dcompiler_47 ausente");return false;}
  auto compila=reinterpret_cast<decltype(&D3DCompile)>(GetProcAddress(c,"D3DCompile"));if(!compila){log("FALHA D3DCompile");return false;}
  for(unsigned i=0;i<PsoN;++i){ComPtr<ID3DBlob> er;
   if(FAILED(compila(hlsl,std::strlen(hlsl),"nr_il",nullptr,nullptr,entradas[i],"cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&cs[i],&er))){
    char s[900];std::snprintf(s,sizeof s,"FALHA compilacao %s: %s",entradas[i],er?static_cast<const char*>(er->GetBufferPointer()):"?");log(s);return false;}
   codigo[i]=cs[i]->GetBufferPointer();tam[i]=cs[i]->GetBufferSize();}}
 {D3D12_DESCRIPTOR_RANGE rg[2]{};rg[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,nSrv,0,0,0};rg[1]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,nUav,0,0,nSrv};
  D3D12_ROOT_PARAMETER p[2]{};p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[0].Constants={0,0,sizeof(K)/4};
  p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;p[1].DescriptorTable={2,rg};
  D3D12_STATIC_SAMPLER_DESC sm{};sm.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;sm.AddressU=sm.AddressV=sm.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;sm.MaxLOD=D3D12_FLOAT32_MAX;
  sm.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;sm.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
  D3D12_ROOT_SIGNATURE_DESC rd{2,p,1,&sm,D3D12_ROOT_SIGNATURE_FLAG_NONE};ComPtr<ID3DBlob> b,er;
  if(FAILED(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&b,&er))||FAILED(d->CreateRootSignature(0,b->GetBufferPointer(),b->GetBufferSize(),IID_PPV_ARGS(e.raiz.ReleaseAndGetAddressOf())))){log("FALHA raiz");return false;}}
 for(unsigned i=0;i<PsoN;++i){D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=e.raiz.Get();pd.CS={codigo[i],tam[i]};
  if(FAILED(d->CreateComputePipelineState(&pd,IID_PPV_ARGS(e.pso[i].ReleaseAndGetAddressOf())))){log("FALHA pso");return false;}}
 if(!e.heap){D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,nDesc*blocos,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
  if(FAILED(d->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&e.heap)))){log("FALHA heap");return false;}}
 log(embutido?"pronta (bytecode do build)":"pronta (HLSL compilado agora)");return true;}
using Vista=nraogpu::Vista;

inline D3D12_GPU_DESCRIPTOR_HANDLE bloco(Estado& e,ID3D12Device* d,const Vista (&s)[nSrv],const Vista (&u)[nUav]){
 constexpr auto F4=DXGI_FORMAT_R16G16B16A16_FLOAT;
 static constexpr DXGI_FORMAT fs[nSrv]{DXGI_FORMAT_R32_FLOAT,F4,DXGI_FORMAT_R16G16_FLOAT,F4,F4,F4,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R32G32_FLOAT,F4};
 static constexpr DXGI_FORMAT fu[nUav]{F4,F4,F4,F4,F4,F4,F4,DXGI_FORMAT_R32_FLOAT,DXGI_FORMAT_R32G32_FLOAT,F4};
 const auto inc=d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);const unsigned base=(e.bloco++%blocos)*nDesc;
 auto hc=[&](unsigned i){auto c=e.heap->GetCPUDescriptorHandleForHeapStart();c.ptr+=SIZE_T(base+i)*inc;return c;};
 for(unsigned i=0;i<nSrv;++i){D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  v.Format=s[i].r?s[i].f:fs[i];v.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;v.Texture2D.MipLevels=s[i].mips;d->CreateShaderResourceView(s[i].r,&v,hc(i));}
 for(unsigned i=0;i<nUav;++i){D3D12_UNORDERED_ACCESS_VIEW_DESC v{};v.Format=u[i].r?u[i].f:fu[i];v.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;v.Texture2D.MipSlice=u[i].mip;
  d->CreateUnorderedAccessView(u[i].r,nullptr,&v,hc(nSrv+i));}
 auto g=e.heap->GetGPUDescriptorHandleForHeapStart();g.ptr+=UINT64(base)*inc;return g;}

inline bool recursos(Estado& e,ID3D12Device* d,unsigned W,unsigned H,unsigned Wi,unsigned Hi,unsigned Si){
 if(e.rad&&e.W==W&&e.H==H&&e.Wi==Wi&&e.Hi==Hi&&e.Si==Si)return true;
 ComPtr<ID3D12Resource>* rs[]{&e.rad,&e.novo,&e.hist[0],&e.hist[1],&e.z[0],&e.z[1],&e.mom[0],&e.mom[1],&e.filt[0],&e.filt[1]};
 for(auto* r:rs)e.aposentar_(*r);
 unsigned nm=1;while(nm<5u&&((std::max)(Wi,Hi)>>nm)>0u)++nm;
 constexpr auto UAV=D3D12_RESOURCE_STATE_UNORDERED_ACCESS,SRV=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;constexpr auto F4=DXGI_FORMAT_R16G16B16A16_FLOAT;
 using nraogpu::textura;
 const bool ok=textura(d,Wi,Hi,nm,F4,UAV,e.rad,L"nr_il_rad")&&textura(d,Wi,Hi,1,F4,UAV,e.novo,L"nr_il_novo")&&
  textura(d,Wi,Hi,1,F4,SRV,e.hist[0],L"nr_il_hist0")&&textura(d,Wi,Hi,1,F4,SRV,e.hist[1],L"nr_il_hist1")&&
  textura(d,Wi,Hi,1,DXGI_FORMAT_R32_FLOAT,SRV,e.z[0],L"nr_il_z0")&&textura(d,Wi,Hi,1,DXGI_FORMAT_R32_FLOAT,SRV,e.z[1],L"nr_il_z1")&&
  textura(d,Wi,Hi,1,DXGI_FORMAT_R32G32_FLOAT,SRV,e.mom[0],L"nr_il_mom0")&&textura(d,Wi,Hi,1,DXGI_FORMAT_R32G32_FLOAT,SRV,e.mom[1],L"nr_il_mom1")&&
  textura(d,Wi,Hi,1,F4,UAV,e.filt[0],L"nr_il_filt0")&&textura(d,Wi,Hi,1,F4,UAV,e.filt[1],L"nr_il_filt1");
 if(!ok){log("FALHA recursos");return false;}
 e.W=W;e.H=H;e.Wi=Wi;e.Hi=Hi;e.Si=Si;e.nm=nm;e.hist_ok=false;e.trocas.feita(e.slot);
 char t[180];std::snprintf(t,sizeof t,"recursos render %ux%u il %ux%u passo %u mips %u (%.1f MB)",W,H,Wi,Hi,Si,nm,Wi*Hi*(8.*1.34+8.+16.+8.+16.+16.)/1048576.);log(t);return true;}

inline bool calcular(ID3D12Device* d,ID3D12GraphicsCommandList* lista,const nrfiltros::Parametros& par,ID3D12Resource* out,D3D12_RESOURCE_STATES s0,DXGI_FORMAT vf,bool hdr,unsigned ow,unsigned oh){
 auto& e=estado_();auto& ea=nraogpu::estado_();
 constexpr auto UAV=D3D12_RESOURCE_STATE_UNORDERED_ACCESS,SRV=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
 auto tr=[&](ID3D12Resource* r,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){nrd3d::transicao(lista,r,a,b);};
 if(ea.il.tex){tr(ea.il.tex,SRV,UAV);ea.il={};}
 auto pular=[&](const char* m){++e.pulados;e.hist_ok=false;if(e.motivo!=m){e.motivo=m;char t[120];std::snprintf(t,sizeof t,"SEM motivo=%s feitos=%llu pulados=%llu",m,e.feitos,e.pulados);log(t);}return false;};
 if(e.falhou)return pular("falhou");
 if(!ea.pronto||!ea.zmip)return pular("sem_profundidade");
 if(!preparar(e,d)){e.falhou=true;return pular("preparar");}
 const auto o=nril::opcoes();const auto oa=nrao::opcoes();const auto& en=nrao::entrada;const auto& ka=ea.kq;
 const unsigned W=ka.W,H=ka.H,Si=nril::passo_resolucao(o.resolucao),Wi=(W+Si-1u)/Si,Hi=(H+Si-1u)/Si;
 ++e.slot;e.aposentados.soltar(e.slot);

 if(e.rad&&(e.W!=W||e.H!=H||e.Wi!=Wi||e.Hi!=Hi||e.Si!=Si)&&!e.trocas.pode(e.slot,nrd3d::livres(e.aposentados),10u))return pular("render_mudando");
 if(!recursos(e,d,W,H,Wi,Hi,Si)){e.falhou=true;return pular("recursos");}

 ID3D12Resource* fonte=nullptr;DXGI_FORMAT fvf=DXGI_FORMAT_UNKNOWN;D3D12_RESOURCE_STATES fs=SRV;unsigned fef=1;
 if(o.fonte!=1&&en.cor_entrada){auto* ci=static_cast<ID3D12Resource*>(en.cor_entrada);const auto cd=ci->GetDesc();const auto cv=nrformato::vista_cor(cd.Format);const auto sc=nrao::entrada.cor_estado;
  const auto st=nraogpu::estado(sc);
  if(nrformato::entrada(ci,cv)&&st!=nrd3d::estado_recusado&&(lista->GetType()!=D3D12_COMMAND_LIST_TYPE_COMPUTE||nrffx_estado::valido_em_compute(unsigned(st)))&&cd.Width>=W&&cd.Height>=H&&ci!=out){
   fonte=ci;fvf=cv;fs=st;fef=0;}}
 if(!fonte){fonte=out;fvf=vf;fs=s0;fef=1;++e.fonte1;}
 if(int(fef)!=e.fonte_ult){e.fonte_ult=int(fef);char t[160];std::snprintf(t,sizeof t,"fonte=%u%s",fef,fef==1u&&o.fonte!=1?" (a entrada do FSR nao serviu neste quadro: a saida atual)":"");log(t);}
 const auto cam=nrao::resolver_camera(en.near_,en.far_,en.fov,oa.invertida>=0?oa.invertida:en.invertida,oa.infinita,oa.fov_graus,oa.metros);
 const bool reset=!e.hist_ok||en.reset||std::fabs(cam.fov-e.fov_ant)>2.f*float(nrao::pi)/180.f||par.il_ver!=e.ver_ant;e.fov_ant=cam.fov;e.ver_ant=par.il_ver;
 const auto pr=nril::predef(par.il_qual);const unsigned R=nril::rodizio(par.il_atual),fase=e.quadro%R;const bool tudo=o.tudo_cada_n!=0&&R>1u;
 K k{};k.W=W;k.H=H;k.Wi=Wi;k.Hi=Hi;k.Si=Si;k.modo=unsigned(o.modo);k.fatias=pr.fatias;k.passos=pr.passos;k.mulx=ka.mulx;k.muly=ka.muly;k.addx=ka.addx;k.addy=ka.addy;
 const auto ef=nril::efetivas(o,par.il_estilo);
 k.er=(ef.raio>0.f?ef.raio:par.ao_raio)*oa.raio_mult;k.falloff=oa.falloff;k.distrib=oa.distribuicao;k.raio_max_px=ef.teto_px;k.esp=ef.espessura>0.f?ef.espessura:par.ao_esp;
 k.mip_off=oa.mip_offset;k.luzrel=o.luz_max_rel;k.mipb=o.mip_bias;k.esp_lin=unsigned(oa.esp_linear);k.quadro=e.quadro;k.R=R;k.fase=fase;k.tudo=tudo?1u:0u;
 k.nemis=unsigned(o.normal_emissor);k.fonte=fef;k.hdr=hdr?1u:0u;k.costas=o.costas;k.ceu_w=o.ceu?1.f:0.f;
 k.jx=std::isfinite(en.jitter_x)?en.jitter_x*oa.desjitter:0.f;k.jy=std::isfinite(en.jitter_y)?en.jitter_y*oa.desjitter:0.f;k.ow=ow;k.oh=oh;
 const bool rebate=(o.rebatimentos==2||o.fonte==2)&&!reset;k.rebate=rebate?1u:0u;k.reset=reset?1u:0u;
 k.rmed=o.albedo_medio;k.rforca=(std::min)(o.rebate_forca,.95f/(std::max)(o.albedo_max,.5f));
 k.mvx=ka.mvx;k.mvy=ka.mvy;
 if(oa.hist_jitter&&!reset){k.djx=k.jx-e.jx_ant;k.djy=k.jy-e.jy_ant;}
 e.jx_ant=k.jx;e.jy_ant=k.jy;
 k.qmax=float(o.quadros>0?unsigned(o.quadros):nril::quadros_padrao(R));k.csig=o.clamp_sigma;k.tol=o.tol_prof;k.ver=par.il_ver;k.nm=e.nm;
 k.emis=unsigned(o.emissor);k.cont=unsigned(o.continuo);k.plonge=ef.passos_longe;k.klonge=ef.raio_longe;k.capl=o.teto_longe_px;
 auto para_leitura=[](D3D12_RESOURCE_STATES s){return (s&SRV)?s:SRV;};
 auto* mov=static_cast<ID3D12Resource*>(en.mov);DXGI_FORMAT mv_f=DXGI_FORMAT_UNKNOWN;D3D12_RESOURCE_STATES sm=SRV;
 if(mov){mv_f=nrformato::vista_movimento(mov->GetDesc().Format);if(!nrformato::entrada(mov,mv_f))mov=nullptr;else sm=nraogpu::estado(en.mov_estado);}
 if(mov&&(sm==nrd3d::estado_recusado||(lista->GetType()==D3D12_COMMAND_LIST_TYPE_COMPUTE&&!nrffx_estado::valido_em_compute(unsigned(sm)))))mov=nullptr;
 tr(ea.zmip.Get(),UAV,SRV);tr(fonte,fs,para_leitura(fs));if(mov)tr(mov,sm,para_leitura(sm));
 ID3D12DescriptorHeap* hs[]{e.heap.Get()};lista->SetDescriptorHeaps(1,hs);lista->SetComputeRootSignature(e.raiz.Get());lista->SetComputeRoot32BitConstants(0,sizeof(K)/4,&k,0);
 constexpr auto F4=DXGI_FORMAT_R16G16B16A16_FLOAT;const Vista vz{ea.zmip.Get(),DXGI_FORMAT_R32_FLOAT,0,5},vr{e.rad.Get(),F4,0,e.nm};
 const unsigned cur=e.atual^1u;

 {Vista s[nSrv]{},u[nUav]{};s[0]=vz;s[1]={fonte,fvf};if(mov)s[2]={mov,mv_f};s[5]={e.hist[e.atual].Get(),F4};s[6]={e.z[e.atual].Get(),DXGI_FORMAT_R32_FLOAT};
  for(unsigned m=0;m<e.nm;++m)u[m]={e.rad.Get(),F4,m};
  lista->SetPipelineState(e.pso[Rad].Get());lista->SetComputeRootDescriptorTable(1,bloco(e,d,s,u));lista->Dispatch((Wi+15u)/16u,(Hi+15u)/16u,1);
  tr(e.rad.Get(),UAV,SRV);}

 {Vista s[nSrv]{},u[nUav]{};s[0]=vz;s[3]=vr;u[5]={e.novo.Get(),F4};
  if(!tudo||fase==0u){unsigned tx=0,ty=0;nril::despacho(Wi,Hi,tudo?1u:R,tx,ty);
   lista->SetPipelineState(e.pso[Principal].Get());lista->SetComputeRootDescriptorTable(1,bloco(e,d,s,u));lista->Dispatch((tx+7u)/8u,(ty+7u)/8u,1);}
  tr(e.novo.Get(),UAV,SRV);}

 const bool dbg=par.il_ver==2u||par.il_ver==3u||par.il_ver>=5u;
 {Vista s[nSrv]{},u[nUav]{};s[0]=vz;if(mov)s[2]={mov,mv_f};s[3]=vr;s[4]={e.novo.Get(),F4};s[5]={e.hist[e.atual].Get(),F4};s[6]={e.z[e.atual].Get(),DXGI_FORMAT_R32_FLOAT};
  s[7]={e.mom[e.atual].Get(),DXGI_FORMAT_R32G32_FLOAT};u[6]={e.hist[cur].Get(),F4};u[7]={e.z[cur].Get(),DXGI_FORMAT_R32_FLOAT};u[8]={e.mom[cur].Get(),DXGI_FORMAT_R32G32_FLOAT};
  u[9]={e.filt[0].Get(),F4};
  tr(e.hist[cur].Get(),SRV,UAV);tr(e.z[cur].Get(),SRV,UAV);tr(e.mom[cur].Get(),SRV,UAV);
  lista->SetPipelineState(e.pso[Temporal].Get());lista->SetComputeRootDescriptorTable(1,bloco(e,d,s,u));lista->Dispatch((Wi+7u)/8u,(Hi+7u)/8u,1);
  tr(e.hist[cur].Get(),UAV,SRV);tr(e.z[cur].Get(),UAV,SRV);tr(e.mom[cur].Get(),UAV,SRV);}

 unsigned saida=0;
 if(dbg)tr(e.filt[0].Get(),UAV,SRV);
 else{const unsigned np=(std::max)(1,o.atrous);ID3D12Resource* ent=e.hist[cur].Get();
  for(unsigned i=0;i<np;++i){k.passo=o.atrous==0?0u:(1u<<i);k.final_=i+1u==np?1u:0u;lista->SetComputeRoot32BitConstants(0,sizeof(K)/4,&k,0);
   Vista s[nSrv]{},u[nUav]{};s[0]=vz;s[3]=vr;s[5]={e.hist[cur].Get(),F4};s[7]={e.mom[cur].Get(),DXGI_FORMAT_R32G32_FLOAT};s[8]={ent,F4};u[9]={e.filt[i&1u].Get(),F4};
   lista->SetPipelineState(e.pso[Atrous].Get());lista->SetComputeRootDescriptorTable(1,bloco(e,d,s,u));lista->Dispatch((Wi+7u)/8u,(Hi+7u)/8u,1);
   tr(e.filt[i&1u].Get(),UAV,SRV);if(i>0u)tr(e.filt[(i-1u)&1u].Get(),SRV,UAV);ent=e.filt[i&1u].Get();saida=i&1u;}}

 tr(fonte,para_leitura(fs),fs);if(mov)tr(mov,para_leitura(sm),sm);tr(ea.zmip.Get(),SRV,UAV);tr(e.rad.Get(),SRV,UAV);tr(e.novo.Get(),SRV,UAV);
 e.atual=cur;e.hist_ok=true;++e.quadro;e.motivo=nullptr;
 auto& il=ea.il;il={};il.tex=e.filt[saida].Get();il.pele=par.il_pele;il.pers=o.personagem;
 il.k[0]=1u|((par.il_ver&7u)<<2)|((unsigned(o.albedo)&3u)<<5)|((Si&7u)<<8);il.k[1]=Wi|(Hi<<16);
 il.k[2]=nril::meia2(par.il_intens*ef.ganho,par.il_sat);il.k[3]=nril::meia2(par.il_brilho,par.il_pele);il.k[4]=nril::meia2(o.personagem,o.claro);
 il.k[5]=nril::meia2(o.claro_ini,o.claro_fim);il.k[6]=nril::meia2(o.claro_max,ef.albedo_viz);il.k[7]=nril::meia2(o.albedo_medio,o.albedo_max);il.k[8]=nril::meia2(o.tol_prof,0.f);
 if(++e.feitos==1||e.feitos%6000==0){char t[300];std::snprintf(t,sizeof t,"quadros=%llu fonte=%u render %ux%u il %ux%u passo %u rodizio %u qualidade %u (%ux%u) raio %.2f m modo %d estilo %u anel %u fonte1=%llu pulados=%llu",
  e.feitos,fef,W,H,Wi,Hi,Si,R,par.il_qual,pr.fatias,pr.passos,double(k.er/oa.raio_mult),o.modo,par.il_estilo,k.plonge,e.fonte1,e.pulados);log(t);}
 return true;}
}
