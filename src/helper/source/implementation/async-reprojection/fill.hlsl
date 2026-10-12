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
  const int2 d8[8]={int2(1,0),int2(-1,0),int2(0,1),int2(0,-1),int2(1,1),int2(-1,1),int2(1,-1),int2(-1,-1)};
  [unroll] for(int r=1;r<=3;++r) [unroll] for(int k=0;k<8;++k) {
   const int2 q=clamp(int2(id.xy)+d8[k]*r,int2(0,0),int2(int(largura)-1,int(altura)-1));
   if(validade(instantaneo.Load(int3(q,0)).a)<1.0)perto=min(perto,float(r));
  }
  v=saturate((perto-1.0)/3.0);
 }

 if(v>=1.0)return;
 const float zBuraco=profundidade.Load(int4(id.xy,fatiaAtual,0));
 if(!isfinite(zBuraco))return;

 if(modo==1) {
  const int2 dirs8[8]={int2(1,0),int2(-1,0),int2(0,1),int2(0,-1),int2(1,1),int2(-1,1),int2(1,-1),int2(-1,-1)};
  int melhor=-1;int df=0;
  [loop] for(int r=1;r<=int(min(raioMaximo,96u));++r) {
   [unroll] for(int d=0;d<8;++d) {
    if(melhor>=0)continue;
    const int2 q=int2(id.xy)+dirs8[d]*r;
    if(any(q<0)||q.x>=int(largura)||q.y>=int(altura))continue;
    const float4 dq=instantaneo.Load(int3(q,0));
    if(!(validade(dq.a)>=1.0))continue;
    const float zq=profundidade.Load(int4(q,fatiaAtual,0));
    const float tq=max(toleranciaAbsoluta,toleranciaRelativa*max(abs(zBuraco),abs(zq)));
    if(!isfinite(zq)||abs(zq-zBuraco)>tq)continue;
    melhor=d;df=r;
   }
   if(melhor>=0)break;
  }
  if(melhor<0)return;

  int dn=1;
  [loop] for(;dn<96;++dn) {
   const int2 q=int2(id.xy)-dirs8[melhor]*dn;
   if(any(q<0)||q.x>=int(largura)||q.y>=int(altura))break;
   if(validade(instantaneo.Load(int3(q,0)).a)>=1.0)break;
   const float zq=profundidade.Load(int4(q,fatiaAtual,0));
   const float tq=max(toleranciaAbsoluta,toleranciaRelativa*max(abs(zBuraco),abs(zq)));
   if(!isfinite(zq)||abs(zq-zBuraco)>tq)break;
  }
  const int2 alvo=int2(id.xy)+dirs8[melhor]*(df+dn);
  int2 fonte=int2(id.xy)+dirs8[melhor]*df;
  if(all(alvo>=0)&&alvo.x<int(largura)&&alvo.y<int(altura)) {
   const float4 da=instantaneo.Load(int3(alvo,0));
   const float za=profundidade.Load(int4(alvo,fatiaAtual,0));
   const float ta=max(toleranciaAbsoluta,toleranciaRelativa*max(abs(zBuraco),abs(za)));
   if(validade(da.a)>=1.0&&all(isfinite(da.rgb))&&isfinite(za)&&abs(za-zBuraco)<=ta)fonte=alvo;
  }
  const float3 copia=instantaneo.Load(int3(fonte,0)).rgb;
  if(!all(isfinite(copia)))return;
  const float3 baseC=all(isfinite(atual.rgb))?atual.rgb:float3(0,0,0);
  residuo[id.xy]=float4(baseC*v+(1.0-v)*copia,atual.a);
  return;
 }

 const float2 direcoes[8]={float2(1,0),float2(-1,0),float2(0,1),float2(0,-1),
                          float2(.70710678,.70710678),float2(-.70710678,.70710678),
                          float2(.70710678,-.70710678),float2(-.70710678,-.70710678)};
 float3 soma=0;float peso=0;
 [loop] for(uint raio=(modo==2?4u:2u);raio<=raioMaximo;raio*=2){
  [unroll] for(uint d=0;d<8;++d){
   const int2 p=int2(id.xy)+int2(round(direcoes[d]*float(raio)));
   if(any(p<0)||p.x>=int(largura)||p.y>=int(altura))continue;
   const float4 doador=instantaneo.Load(int3(p,0));
   if(!(validade(doador.a)>=1.0)||!all(isfinite(doador.rgb)))continue;
   const float zDoador=profundidade.Load(int4(p,fatiaAtual,0));
   const float tolerancia=max(toleranciaAbsoluta,toleranciaRelativa*max(abs(zBuraco),abs(zDoador)));
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
