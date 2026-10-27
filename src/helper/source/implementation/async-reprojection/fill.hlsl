#include "prof_comum.hlsli"
#include "dir_comum.hlsli"
cbuffer Constants : register(b0) {
 uint largura,altura,fatiaAtual;

 float limiar;
 float toleranciaRelativa,toleranciaAbsoluta;

 uint raioMaximo,modo;
};
Texture2DArray<float> profundidade : register(t0);

Texture2D<float4> instantaneo : register(t1);
RWTexture2D<float4> residuo : register(u0);
float validade(float alfa){return isfinite(alfa)?smoothstep(0.0,limiar,alfa):0.0;}
[numthreads(8,8,1)]
void Fill(uint3 id : SV_DispatchThreadID) {
 if(id.x>=largura||id.y>=altura)return;
 const float4 atual=instantaneo.Load(int3(id.xy,0));
 float v=validade(atual.a);

 if(modo==2&&v>=1.0) {
  float perto=4.0;
  const int2 d8[8]=NR_D8_INT;
  [unroll] for(int r=1;r<=3;++r) [unroll] for(int k=0;k<8;++k) {
   const int2 q=clamp(int2(id.xy)+d8[k]*r,int2(0,0),int2(int(largura)-1,int(altura)-1));
   if(validade(instantaneo.Load(int3(q,0)).a)<1.0)perto=min(perto,float(r));
  }
  v=saturate((perto-1.0)/3.0);
 }

 if(v>=1.0)return;
 const float zBuraco=profundidade.Load(int4(id.xy,fatiaAtual,0));
 if(!isfinite(zBuraco))return;

 const float2 direcoes[8]=NR_D8_UNIT;
 float3 soma=0;float peso=0;
 [loop] for(uint raio=(modo==2?4u:2u);raio<=raioMaximo;raio*=2){
  [unroll] for(uint d=0;d<8;++d){
   const int2 p=int2(id.xy)+int2(round(direcoes[d]*float(raio)));
   if(any(p<0)||p.x>=int(largura)||p.y>=int(altura))continue;
   const float4 doador=instantaneo.Load(int3(p,0));
   if(!(validade(doador.a)>=1.0)||!all(isfinite(doador.rgb)))continue;
   const float zDoador=profundidade.Load(int4(p,fatiaAtual,0));
   const float tolerancia=NR_TOL_PROF(toleranciaAbsoluta,toleranciaRelativa,zBuraco,zDoador);
   if(!isfinite(zDoador)||abs(zDoador-zBuraco)>tolerancia)continue;

   const float w=1.0/float(raio);
   soma+=doador.rgb*w;peso+=w;
  }
 }
 if(!(peso>0))return;
 const float3 base=all(isfinite(atual.rgb))?atual.rgb:float3(0,0,0);
 if(modo==2){residuo[id.xy]=float4(lerp(soma/peso,base,v),atual.a);return;}
 residuo[id.xy]=float4(base+(1.0-v)*(soma/peso),atual.a);
}

#define FILL_TELHA 14
groupshared float telhaAlfa[FILL_TELHA*FILL_TELHA];
[numthreads(8,8,1)]
void FillRapido(uint3 id : SV_DispatchThreadID, uint3 grupo : SV_GroupID, uint gi : SV_GroupIndex) {
 const int2 origem=int2(grupo.xy)*8-3;
 for(uint e=gi;e<FILL_TELHA*FILL_TELHA;e+=64u){
  const int2 q=clamp(origem+int2(e%FILL_TELHA,e/FILL_TELHA),int2(0,0),int2(int(largura)-1,int(altura)-1));
  telhaAlfa[e]=instantaneo.Load(int3(q,0)).a;
 }
 GroupMemoryBarrierWithGroupSync();
 if(id.x>=largura||id.y>=altura)return;
 const float4 atual=instantaneo.Load(int3(id.xy,0));
 float v=validade(atual.a);
 if(modo==2&&v>=1.0) {
  float perto=4.0;
  const int2 d8[8]=NR_D8_INT;
  [unroll] for(int r=1;r<=3;++r) [unroll] for(int k=0;k<8;++k) {
   const int2 t=int2(id.xy)+d8[k]*r-origem;
   if(validade(telhaAlfa[t.y*FILL_TELHA+t.x])<1.0)perto=min(perto,float(r));
  }
  v=saturate((perto-1.0)/3.0);
 }
 if(v>=1.0){residuo[id.xy]=atual;return;}
 const float zBuraco=profundidade.Load(int4(id.xy,fatiaAtual,0));
 if(!isfinite(zBuraco)){residuo[id.xy]=atual;return;}

 const float2 direcoes[8]=NR_D8_UNIT;
 float3 soma=0;float peso=0;
 [loop] for(uint raio=(modo==2?4u:2u);raio<=raioMaximo;raio*=2){
  [unroll] for(uint d=0;d<8;++d){
   const int2 p=int2(id.xy)+int2(round(direcoes[d]*float(raio)));
   if(any(p<0)||p.x>=int(largura)||p.y>=int(altura))continue;
   const float4 doador=instantaneo.Load(int3(p,0));
   if(!(validade(doador.a)>=1.0)||!all(isfinite(doador.rgb)))continue;
   const float zDoador=profundidade.Load(int4(p,fatiaAtual,0));
   const float tolerancia=NR_TOL_PROF(toleranciaAbsoluta,toleranciaRelativa,zBuraco,zDoador);
   if(!isfinite(zDoador)||abs(zDoador-zBuraco)>tolerancia)continue;
   const float w=1.0/float(raio);
   soma+=doador.rgb*w;peso+=w;
  }
 }
 if(!(peso>0)){residuo[id.xy]=atual;return;}
 const float3 base=all(isfinite(atual.rgb))?atual.rgb:float3(0,0,0);
 if(modo==2){residuo[id.xy]=float4(lerp(soma/peso,base,v),atual.a);return;}
 residuo[id.xy]=float4(base+(1.0-v)*(soma/peso),atual.a);
}
