cbuffer Constants : register(b0) {
 uint width,height,currentSlice,age;

 uint validMask,historyCount,flags,debugMotivo;
 float2 motionScaleUV,motionJitterUV;
 float2 inverseSize;
 float depthRelativeThreshold,depthAbsoluteThreshold;
 float colorThreshold,currentExposure,sourceExposure,residualExposureScale;
 float clampStrength,confidenceRate;

 float motionReference;

 float motionDivergence;

 float deformTolerance;
 float gateSmooth;
 float deformBlur;

 float detailMotion;

 float gateCoherent;
 float occlusionTolerance;
 float residualFloor;
 float confidenceFloor;
};
Texture2D<float4> inputColor : register(t0);
Texture2D<float2> inputMotion : register(t1);
Texture2D<float> inputDepth : register(t2);
Texture2DArray<float4> historyColor : register(t3);
Texture2DArray<float2> historyMotion : register(t4);
Texture2DArray<float> historyDepth : register(t5);
Texture2D<float4> sourceResidual : register(t6);
Texture2D<float> inputExposure : register(t7);
RWTexture2DArray<float4> outputColor : register(u0);
RWTexture2DArray<float2> outputMotion : register(u1);
RWTexture2DArray<float> outputDepth : register(u2);
RWTexture2D<float4> outputResidual : register(u3);
SamplerState linearClamp : register(s0);

bool finite2(float2 x) { return all(isfinite(x)); }
bool finite3(float3 x) { return all(isfinite(x)); }
#include "tom_comum.hlsli"
#include "prof_comum.hlsli"
#include "dir_comum.hlsli"

float profD(int2 p) { const float v=inputDepth.Load(int3(p,0)); return (flags&0x1000000u)!=0u ? 1.0-v : v; }
bool inside(float2 uv) {
 return finite2(uv) && all(uv >= inverseSize * 0.5) && all(uv <= 1.0 - inverseSize * 0.5);
}
[numthreads(8,8,1)]
void Capture(uint3 id : SV_DispatchThreadID) {
 if (id.x >= width || id.y >= height || currentSlice >= historyCount) return;
 float2 uv=(float2(id.xy)+0.5)*inverseSize;
 float4 c=inputColor.SampleLevel(linearClamp,uv,0);

 if((flags&16384u)!=0u) {
  if((flags&32768u)!=0u) { const float3 v=min(saturate(c.rgb),0.95); c.rgb=v/(1.0-v); }
  else c.rgb=tmi3(c.rgb);
 }
 c.a=(flags&2)!=0 ? inputExposure.Load(int3(0,0,0)) : 1.0;
 float2 m=inputMotion.SampleLevel(linearClamp,uv,0)*motionScaleUV+motionJitterUV;
 uint dw,dh; inputDepth.GetDimensions(dw,dh);
 float d=profD(min(uint2(uv*float2(dw,dh)),uint2(dw-1,dh-1)));
 if(age!=0u)d*=asfloat(age);

 outputColor[uint3(id.xy,currentSlice)]=c;
 outputMotion[uint3(id.xy,currentSlice)]=m;
 outputDepth[uint3(id.xy,currentSlice)]=d;
}
float depthAt(float2 uv,uint slice) {
 uint2 p=min(uint2(uv*float2(width,height)),uint2(width-1,height-1));
 return historyDepth.Load(int4(p,slice,0));
}

#define SAIR(c) { outputResidual[id.xy]=diag?float4(0,(c),0,0):float4(0,0,0,0); return; }
[numthreads(8,8,1)]
void Warp(uint3 id : SV_DispatchThreadID) {
 if(id.x>=width || id.y>=height) return;
 const bool diag=(debugMotivo&1u)!=0u;
 float diagPior=1.0,diagCod=0.0;
 if(historyCount==0 || historyCount>16 || currentSlice>=historyCount || age>=historyCount || (flags&1)==0) SAIR(1)
 if(!isfinite(currentExposure)||!isfinite(sourceExposure)||currentExposure<=0||sourceExposure<=0||!isfinite(residualExposureScale)) SAIR(2)
 float2 uv=(float2(id.xy)+0.5)*inverseSize;

 float2 currentUv=uv;
 float4 currentSample=historyColor.Load(int4(id.xy,currentSlice,0));
 float3 current=currentSample.rgb;
 if(!finite3(current)) SAIR(3)
 uint slice=currentSlice;
 float confidence=1;
 if((validMask&(1u<<slice))==0) SAIR(4)
 [loop] for(uint step=0;step<age;++step) {

  float2 mv;
  if((flags&512u)!=0) {
   const int2 at=clamp(int2(uv*float2(width,height)),int2(0,0),int2(int(width)-1,int(height)-1));
   mv=historyMotion.Load(int4(at,slice,0));
  } else {
   mv=historyMotion.SampleLevel(linearClamp,float3(uv,slice),0);
  }
  if(!finite2(mv)) SAIR(5)
  float2 prior=uv+mv;
  if(!inside(prior)) SAIR(6)
  uint previous=(slice+historyCount-1)%historyCount;
  if((validMask&(1u<<previous))==0) SAIR(7)

  const uint rampaBruta=(debugMotivo>>16)&3u;
  if(rampaBruta>0) {
   const float L=rampaBruta==1u?20.0:rampaBruta==2u?8.0:4.0;
   const int2 pa=clamp(int2(prior*float2(width,height)),int2(0,0),int2(int(width)-1,int(height)-1));
   const float2 mvA=historyMotion.Load(int4(pa,previous,0));
   const float lenD=length(mv*float2(width,height));
   if(lenD>1.0&&all(isfinite(mvA))) {
    const float razao=saturate(length(mvA*float2(width,height))/max(lenD,1e-4));
    const float rampa=saturate(pow(lenD/L,3.0));

    const float fRaw=1.0-(1.0-razao)*rampa;
    confidence*=fRaw;
    if(fRaw<diagPior){diagPior=fRaw;diagCod=20.0;}
   }
  }
  float a=depthAt(uv,slice), b=depthAt(prior,previous);

  if(!isfinite(a)||!isfinite(b)) SAIR(8)

  float tolerance=NR_TOL_PROF(depthAbsoluteThreshold,depthRelativeThreshold,a,b);
  if(tolerance<=0) SAIR(9)
  if(abs(a-b)>tolerance) SAIR(10.0+float(step)/100.0)
  {const float fProf=saturate(2.0-2.0*abs(a-b)/tolerance);confidence=min(confidence,fProf);if(fProf<diagPior){diagPior=fProf;diagCod=21.0;}}
  if(motionDivergence>0) {

   int2 priorTexel=clamp(int2(prior*float2(width,height)),int2(0,0),int2(int(width)-1,int(height)-1));
   float2 priorMotion=historyMotion.Load(int4(priorTexel,previous,0));
   if(!finite2(priorMotion))SAIR(11)
   float divergence=length(priorMotion-mv);

   float coherent=divergence;

   [unroll] for(int tap=0;tap<4;++tap) {
    const int2 offset=tap<2 ? int2(4,0) : int2(0,4);
    const int2 at=clamp(priorTexel+((tap&1)!=0 ? -offset : offset),int2(0,0),int2(int(width)-1,int(height)-1));
    float2 neighbourMotion=historyMotion.Load(int4(at,previous,0));
    if(!finite2(neighbourMotion))continue;
    coherent=min(coherent,length(neighbourMotion-mv));
   }
   float excess=max(divergence-coherent,0.0);
   float tolerance=motionDivergence*max(length(mv),inverseSize.x);
   if(tolerance<=0)SAIR(12)
   {const float fDiv=saturate(1.0-excess/tolerance);confidence=min(confidence,fDiv);if(fDiv<diagPior){diagPior=fDiv;diagCod=22.0;}}
  }

  uv=prior; slice=previous;
 }
 float4 originalSample=historyColor.SampleLevel(linearClamp,float3(uv,slice),0);
 if(!isfinite(currentSample.a)||!isfinite(originalSample.a)||currentSample.a<=0||originalSample.a<=0) SAIR(14)

 float exposureError=abs(currentSample.a-originalSample.a)/max(currentSample.a,originalSample.a);
 {const float fExpo=saturate(1.0-exposureError/0.1);confidence*=fExpo;if(fExpo<diagPior){diagPior=fExpo;diagCod=23.0;}}
 float3 original=originalSample.rgb;
 original*=currentExposure/sourceExposure;
 if(!finite3(original)) SAIR(15)
 float3 scale=max(max(abs(original),abs(current)),0.05);
 float error=max(max(abs(original.r-current.r)/scale.r,abs(original.g-current.g)/scale.g),abs(original.b-current.b)/scale.b);
 if(!isfinite(error)||colorThreshold<=0) SAIR(16)
 {const float fCor=saturate(1.0-error/colorThreshold);confidence*=fCor;if(fCor<diagPior){diagPior=fCor;diagCod=24.0;}}

 uv+=motionJitterUV;
 float4 residual;
 {
  residual=sourceResidual.SampleLevel(linearClamp,uv,0);
 }
 if(!all(isfinite(residual))) SAIR(17)
 const float3 diagLw=float3(0.2126,0.7152,0.0722);
 float diagPos=1.0,diagPosCod=0.0;
 const float diagRes0=abs(dot(residual.rgb,diagLw));
 if(detailMotion>0) {

  float2 movimentoAqui=historyMotion.SampleLevel(linearClamp,float3(currentUv,currentSlice),0);
  const float pixelsAqui=finite2(movimentoAqui)?length(movimentoAqui*float2(width,height)):0;
  const float manterDetalhe=saturate(1.0-pixelsAqui/detailMotion);
  if(manterDetalhe<1.0) {
   float4 baixa=residual;float pesoB=1;
   [unroll] for(int tb=1;tb<=2;++tb) {
    const float2 passoB=inverseSize*float(tb);
    const float2 pontosB[4]={uv+float2(passoB.x,0),uv-float2(passoB.x,0),
                             uv+float2(0,passoB.y),uv-float2(0,passoB.y)};
    [unroll] for(int kb=0;kb<4;++kb) {
     if(!inside(pontosB[kb]))continue;
     float4 vb=sourceResidual.SampleLevel(linearClamp,pontosB[kb],0);
     if(!all(isfinite(vb)))continue;
     baixa+=vb;pesoB+=1;
    }
   }
   baixa/=pesoB;
   residual=baixa+(residual-baixa)*manterDetalhe;
   if(diag){const float f=abs(dot(residual.rgb,diagLw))/max(diagRes0,1e-6);if(f<diagPos){diagPos=f;diagPosCod=30.0;}}
  }
 }

 float3 applied=residual.rgb*(confidence*residualExposureScale);
 {

  const uint iForca=(debugMotivo>>20)&7u,iRef=(debugMotivo>>24)&3u;
  if(iForca!=0u) {
   const float fracao=iForca==1u?0.75:iForca==2u?0.6:iForca==3u?0.5:iForca==4u?0.4:0.3;
   const float referencia=iRef==1u?4.0:iRef==2u?8.0:iRef==3u?16.0:2.0;
   const float2 mvForca=historyMotion.SampleLevel(linearClamp,float3(currentUv,currentSlice),0);
   const float pxForca=finite2(mvForca)?length(mvForca*float2(width,height)):0;

   const float fForca=lerp(1.0,fracao,saturate(pxForca/referencia));
   applied*=fForca;
   if(fForca<diagPos){diagPos=fForca;diagPosCod=31.0;}
  }
 }

 if(clampStrength>0) {

  float2 currentMotion=historyMotion.SampleLevel(linearClamp,float3(currentUv,currentSlice),0);
  float motionFactor=finite2(currentMotion)&&motionReference>0 ? saturate(length(currentMotion)/motionReference) : 0;
  float effectiveClampStrength=clampStrength*motionFactor;

  float3 low=current,high=current;
  [unroll] for(int dy=-1;dy<=1;++dy)
   [unroll] for(int dx=-1;dx<=1;++dx) {
    int2 at=clamp(int2(id.xy)+int2(dx,dy),int2(0,0),int2(width-1,height-1));
    float3 neighbour=historyColor.Load(int4(at,currentSlice,0)).rgb;
    if(!finite3(neighbour))continue;
    low=min(low,neighbour); high=max(high,neighbour);
   }

  float3 slack=max((high-low)*0.5,abs(current)*0.1*(1.0-motionFactor));
  float3 limited=clamp(current+applied,low-slack,high+slack)-current;
  const float diagAntes=abs(dot(applied,diagLw));
  applied=lerp(applied,limited,saturate(effectiveClampStrength));
  if(diag){const float f=abs(dot(applied,diagLw))/max(diagAntes,1e-6);if(f<diagPos){diagPos=f;diagPosCod=32.0;}}
 }
 if(diag){outputResidual[id.xy]=float4(saturate(confidence),diagPior<1.0?diagCod:0.0,saturate(diagPos),diagPosCod);return;}
 outputResidual[id.xy]=float4(applied,residual.a*confidence);
}

[numthreads(8,8,1)]
void Reduzir(uint3 id : SV_DispatchThreadID) {
 if (id.x >= width || id.y >= height) return;
 outputResidual[id.xy]=sourceResidual.SampleLevel(linearClamp,(float2(id.xy)+0.5)*inverseSize,0);
}
