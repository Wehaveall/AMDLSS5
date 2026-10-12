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

float tomRuido(float v) { v = max(v, 0.0); v = v / (1.0 + v); return v <= 0.0031308 ? v * 12.92 : 1.055 * pow(v, 1.0 / 2.4) - 0.055; }
float3 tomRuido3(float3 v) { return float3(tomRuido(v.r), tomRuido(v.g), tomRuido(v.b)); }

float tomInv(float v) { v = saturate(v); v = v <= 0.04045 ? v / 12.92 : pow((v + 0.055) / 1.055, 2.4); v = min(v, 0.95); return v / (1.0 - v); }
float3 tomInv3(float3 v) { return float3(tomInv(v.r), tomInv(v.g), tomInv(v.b)); }
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
  else c.rgb=tomInv3(c.rgb);
 }
 c.a=(flags&2)!=0 ? inputExposure.Load(int3(0,0,0)) : 1.0;
 float2 m=inputMotion.SampleLevel(linearClamp,uv,0)*motionScaleUV+motionJitterUV;
 uint dw,dh; inputDepth.GetDimensions(dw,dh);
 float d=inputDepth.Load(int3(min(uint2(uv*float2(dw,dh)),uint2(dw-1,dh-1)),0));

 const uint simula=(debugMotivo>>12)&3u, reparo=(debugMotivo>>10)&3u;
 if(simula>0||reparo>0) {
  const int2 p0=int2(min(uint2(uv*float2(dw,dh)),uint2(dw-1,dh-1)));
  const int2 d8[8]={int2(1,0),int2(-1,0),int2(0,1),int2(0,-1),int2(1,1),int2(-1,1),int2(1,-1),int2(-1,-1)};
  const float2 escalaD=float2(dw,dh);
  if(simula>0) {
   [loop] for(int r=1;r<=int(simula);++r) [unroll] for(int k=0;k<8;++k) {
    const int2 q=clamp(p0+d8[k]*r,int2(0,0),int2(int(dw)-1,int(dh)-1));
    const float dq=inputDepth.Load(int3(q,0));
    const float tq=max(depthAbsoluteThreshold,depthRelativeThreshold*max(abs(d),abs(dq)));
    if(isfinite(dq)&&dq>d+tq) m=inputMotion.SampleLevel(linearClamp,(float2(q)+0.5)/escalaD,0)*motionScaleUV+motionJitterUV;
   }
  }
  if(reparo>0) {
   const int R=int(reparo)+1;
   float2 ladoDiferente=0;bool borda=false;
   [loop] for(int r=1;r<=R;++r) [unroll] for(int k=0;k<8;++k) {
    const int2 q=clamp(p0+d8[k]*r,int2(0,0),int2(int(dw)-1,int(dh)-1));
    const float dq=inputDepth.Load(int3(q,0));
    const float tq=max(depthAbsoluteThreshold,depthRelativeThreshold*max(abs(d),abs(dq)));
    if(isfinite(dq)&&abs(dq-d)>tq){borda=true;ladoDiferente+=float2(d8[k]);}
   }
   if(borda&&isfinite(d)) {
    float2 soma=0;float peso=0;
    [loop] for(int r2=R+1;r2<=R+3;++r2) [unroll] for(int k2=0;k2<8;++k2) {
     if(dot(float2(d8[k2]),ladoDiferente)>0)continue;
     const int2 q=p0+d8[k2]*r2;
     if(any(q<0)||q.x>=int(dw)||q.y>=int(dh))continue;
     const float dq=inputDepth.Load(int3(q,0));
     const float tq=max(depthAbsoluteThreshold,depthRelativeThreshold*max(abs(d),abs(dq)));
     if(!isfinite(dq)||abs(dq-d)>tq)continue;
     const float2 mq=inputMotion.SampleLevel(linearClamp,(float2(q)+0.5)/escalaD,0);
     if(!all(isfinite(mq)))continue;
     soma+=mq;peso+=1;
    }
    if(peso>0)m=(soma/peso)*motionScaleUV+motionJitterUV;
   }
  }
 }

 const uint raioBanda=(debugMotivo>>1)&127u;
 if((flags&2048u)!=0u&&raioBanda>0u&&isfinite(d)&&finite2(m)) {
  const int2 p0=int2(min(uint2(uv*float2(dw,dh)),uint2(dw-1,dh-1)));
  const int2 limD=int2(int(dw)-1,int(dh)-1);
  const float2 escalaD=float2(dw,dh);

  const float xp=inputDepth.Load(int3(min(p0+int2(1,0),limD),0)),xm=inputDepth.Load(int3(max(p0-int2(1,0),int2(0,0)),0));
  const float yp=inputDepth.Load(int3(min(p0+int2(0,1),limD),0)),ym=inputDepth.Load(int3(max(p0-int2(0,1),int2(0,0)),0));

  float gx=p0.x<=0 ? xp-d : p0.x>=limD.x ? d-xm : (abs(xp-d)<abs(d-xm) ? xp-d : d-xm);
  float gy=p0.y<=0 ? yp-d : p0.y>=limD.y ? d-ym : (abs(yp-d)<abs(d-ym) ? yp-d : d-ym);
  if(!isfinite(gx))gx=0;
  if(!isfinite(gy))gy=0;

  const float R=float(raioBanda)*float(dh)/float(height);
  const int2 d8b[8]={int2(1,0),int2(-1,0),int2(0,1),int2(0,-1),int2(1,1),int2(-1,1),int2(1,-1),int2(-1,-1)};
  const float raios[8]={1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0};
  float melhor=0;float2 mvF=m;
  [unroll] for(int k=0;k<8;++k) {
   const float passo=k<4 ? 1.0 : 1.41421356;
   [loop] for(int i=0;i<8;++i) {
    const int r=max(1,int(round(raios[i]*R/8.0)));
    const float dist=float(r)*passo;
    if(dist>R+0.5)break;
    const int2 o=d8b[k]*r,q=p0+o;
    if(any(q<0)||q.x>int(limD.x)||q.y>int(limD.y))break;
    const float dq=inputDepth.Load(int3(q,0));
    if(!isfinite(dq))continue;
    const float zp=d+gx*float(o.x)+gy*float(o.y);
    const float s=(dq-zp)/max(max(abs(dq),abs(zp)),1e-9);
    const float wt=smoothstep(0.02,0.08,s)*(1.0-smoothstep(0.5*R,R+0.5,dist));
    if(wt>melhor) {
     const float2 mq=inputMotion.SampleLevel(linearClamp,(float2(q)+0.5)/escalaD,0)*motionScaleUV+motionJitterUV;
     if(finite2(mq)){melhor=wt;mvF=mq;}
    }
   }
  }
  if(melhor>0) {
   const float rel=length((mvF-m)*float2(width,height))*720.0/float(height);
   const float t=melhor*smoothstep(0.25,1.0,rel);
   if(t>0)m=lerp(m,mvF,t);
  }
 }
 outputColor[uint3(id.xy,currentSlice)]=c;
 outputMotion[uint3(id.xy,currentSlice)]=m;
 outputDepth[uint3(id.xy,currentSlice)]=d;
}
float depthAt(float2 uv,uint slice) {
 uint2 p=min(uint2(uv*float2(width,height)),uint2(width-1,height-1));
 return historyDepth.Load(int4(p,slice,0));
}

void LerResiduoProf(float2 uv,uint fatia,float ref,inout float4 soma,inout float peso) {
 const float2 p=uv*float2(width,height)-0.5;
 const float2 f0=floor(p),fr=p-f0;
 const int2 i0=int2(f0),limite=int2(int(width)-1,int(height)-1);
 [unroll] for(int k=0;k<4;++k) {
  const int2 o=int2(k&1,k>>1);
  const int2 t=clamp(i0+o,int2(0,0),limite);
  const float wb=(o.x!=0?fr.x:1.0-fr.x)*(o.y!=0?fr.y:1.0-fr.y);
  const float dt=historyDepth.Load(int4(t,fatia,0));
  const float tol=max(depthAbsoluteThreshold,depthRelativeThreshold*max(abs(ref),abs(dt)));
  const float ws=(isfinite(dt)&&tol>0)?saturate(2.0-2.0*abs(dt-ref)/tol):0.0;
  const float4 r=sourceResidual.Load(int3(t,0));
  if(wb*ws>0&&all(isfinite(r))){soma+=r*(wb*ws);peso+=wb*ws;}
 }
}

static const int2 corteD8[8]={int2(1,0),int2(-1,0),int2(0,1),int2(0,-1),int2(1,1),int2(-1,1),int2(1,-1),int2(-1,-1)};
bool CorteReferencia(int j,float2 fora,int2 pj,int2 limC,float z0,float2 grad,float raio,uint fatia,out float wc,out float3 rr,out float2 mt) {
 wc=0;rr=0;mt=0;
 const float cr=j<2 ? 1.0 : 0.76604444,sr=j<2 ? 0.0 : (j==2 ? 0.64278761 : -0.64278761);
 const float2 o=float2(fora.x*cr-fora.y*sr,fora.x*sr+fora.y*cr)*((j==1 ? 1.5 : 1.0)*raio+3.0);
 const int2 t=pj+int2(round(o));
 if(any(t<0)||any(t>limC))return false;
 const float zt=historyDepth.Load(int4(t,fatia,0));
 const float zpt=z0+dot(grad,float2(t-pj));
 if(!isfinite(zt))return false;
 wc=1.0-smoothstep(0.02,0.08,abs(zt-zpt)/max(max(abs(zt),abs(zpt)),1e-9));
 if(!(wc>0.0))return false;
 const int2 tr=clamp(int2(((float2(t)+0.5)*inverseSize+motionJitterUV)*float2(width,height)),int2(0,0),limC);
 const float4 r4=sourceResidual.Load(int3(tr,0));
 mt=historyMotion.Load(int4(t,fatia,0));
 rr=r4.rgb;
 return all(isfinite(r4))&&finite2(mt);
}

#define SAIR(c) { if((debugMotivo&1u)!=0) outputResidual[id.xy]=float4(0,0,0,-(c)); return; }
[numthreads(8,8,1)]
void Warp(uint3 id : SV_DispatchThreadID) {
 if(id.x>=width || id.y>=height) return;
 outputResidual[id.xy]=0;
 if(historyCount==0 || historyCount>16 || currentSlice>=historyCount || age>=historyCount || (flags&1)==0) SAIR(1)
 if(!isfinite(currentExposure)||!isfinite(sourceExposure)||currentExposure<=0||sourceExposure<=0||!isfinite(residualExposureScale)) SAIR(2)
 float2 uv=(float2(id.xy)+0.5)*inverseSize;

 float2 currentUv=uv;
 float4 currentSample=historyColor.Load(int4(id.xy,currentSlice,0));
 float3 current=currentSample.rgb;
 if(!finite3(current)) SAIR(3)
 uint slice=currentSlice;
 float confidence=1; uint motivoBrando=0;

 float deformacaoVista=0;
 if((validMask&(1u<<slice))==0) SAIR(4)

 float refProf=depthAt(uv,slice);
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

  const uint iteracoesDual=(flags>>2)&7u;
  if(iteracoesDual>0) {

   const float limiarDual=float((flags>>5)&15u)*0.25;
   [loop] for(uint iteracao=0;iteracao<iteracoesDual;++iteracao) {
    const int2 chegada=clamp(int2(prior*float2(width,height)),int2(0,0),int2(int(width)-1,int(height)-1));
    const float2 mvChegada=historyMotion.Load(int4(chegada,previous,0));
    if(!finite2(mvChegada)) break;
    const float2 refinado=uv+mvChegada;

    if(!inside(refinado)) break;
    float peso=1.0;
    if(limiarDual>0) {

     const float degrau=length((mvChegada-mv)*float2(width,height));
     peso=smoothstep(0.5*limiarDual,1.5*limiarDual,degrau);
    }
    prior=lerp(prior,refinado,peso);
   }
  }

  const uint rampaBruta=(debugMotivo>>16)&3u;
  if(rampaBruta>0) {
   const float L=rampaBruta==1u?20.0:rampaBruta==2u?8.0:4.0;
   const int2 pa=clamp(int2(prior*float2(width,height)),int2(0,0),int2(int(width)-1,int(height)-1));
   const float2 mvA=historyMotion.Load(int4(pa,previous,0));
   const float lenD=length(mv*float2(width,height));
   if(lenD>1.0&&all(isfinite(mvA))) {
    const float razao=saturate(length(mvA*float2(width,height))/max(lenD,1e-4));
    const float rampa=saturate(pow(lenD/L,3.0));

    {const float antesM=confidence;confidence*=1.0-(1.0-razao)*rampa;if(confidence<antesM-1e-4)motivoBrando=28;}
   }
  }
  float a=depthAt(uv,slice), b=depthAt(prior,previous);
  refProf=b;

  const int raioBorda=int((debugMotivo>>8)&3u);
  if(raioBorda>0&&isfinite(a)) {
   const int2 pc=clamp(int2(prior*float2(width,height)),int2(0,0),int2(int(width)-1,int(height)-1));
   const int2 ua=clamp(int2(uv*float2(width,height)),int2(0,0),int2(int(width)-1,int(height)-1));
   const float tolB=max(depthAbsoluteThreshold,depthRelativeThreshold*abs(a));
   [loop] for(int ey=-raioBorda;ey<=raioBorda;++ey) [loop] for(int ex=-raioBorda;ex<=raioBorda;++ex) {
    const float bp=historyDepth.Load(int4(clamp(pc+int2(ex,ey),int2(0,0),int2(int(width)-1,int(height)-1)),previous,0));
    const float bc=historyDepth.Load(int4(clamp(ua+int2(ex,ey),int2(0,0),int2(int(width)-1,int(height)-1)),slice,0));
    if(isfinite(bp)&&isfinite(bc)&&abs(bp-a)>tolB&&abs(bc-bp)>tolB)b=bp;
   }
  }
  if(!isfinite(a)||!isfinite(b)) SAIR(8)

  float tolerance=max(depthAbsoluteThreshold,depthRelativeThreshold*max(abs(a),abs(b)));
  if(tolerance<=0) SAIR(9)
  if(abs(a-b)>tolerance) SAIR(10.0+float(step)/100.0)
  {const float antesM=confidence;confidence=min(confidence,saturate(2.0-2.0*abs(a-b)/tolerance));if(confidence<antesM-1e-4)motivoBrando=20;}
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

   if(gateSmooth>0) {
    const int raioE=int(min(gateSmooth,4.0));
    const int2 baseE=clamp(int2(uv*float2(width,height)),int2(0,0),int2(width-1,height-1));
    float somaE=excess;float pesoE=1;
    [loop] for(int te=1;te<=raioE;++te) {
     const int2 vizinhos[4]={int2(min(baseE.x+te,int(width)-1),baseE.y),
                             int2(max(baseE.x-te,0),baseE.y),
                             int2(baseE.x,min(baseE.y+te,int(height)-1)),
                             int2(baseE.x,max(baseE.y-te,0))};
     [unroll] for(int ke=0;ke<4;++ke) {
      float2 mvE=historyMotion.Load(int4(vizinhos[ke],slice,0));
      if(!finite2(mvE))continue;
      float2 chegadaE=(float2(vizinhos[ke])+0.5)*inverseSize+mvE;
      if(!inside(chegadaE))continue;
      float2 priorE=historyMotion.SampleLevel(linearClamp,float3(chegadaE,previous),0);
      if(!finite2(priorE))continue;
      somaE+=max(length(priorE-mvE)-coherent,0.0);
      pesoE+=1;
     }
    }
    excess=somaE/pesoE;
   }
   float tolerance=motionDivergence*max(length(mv),inverseSize.x);
   if(tolerance<=0)SAIR(12)
   {const float antesM=confidence;confidence=min(confidence,saturate(1.0-excess/tolerance));if(confidence<antesM-1e-4)motivoBrando=21;}
  }
  if(deformTolerance>0) {

   const int2 base=clamp(int2(uv*float2(width,height)),int2(0,0),int2(width-1,height-1));
   float2 dx=historyMotion.Load(int4(min(base.x+1,int(width)-1),base.y,slice,0))
           -historyMotion.Load(int4(max(base.x-1,0),base.y,slice,0));
   float2 dy=historyMotion.Load(int4(base.x,min(base.y+1,int(height)-1),slice,0))
           -historyMotion.Load(int4(base.x,max(base.y-1,0),slice,0));
   if(finite2(dx)&&finite2(dy)) {

    float porPixel=(length(dx*float2(width,height))+length(dy*float2(width,height)))*.5;

    if(gateSmooth>0) {
     const int raio=int(min(gateSmooth,4.0));
     float soma=porPixel;float peso=1;
     [loop] for(int t=1;t<=raio;++t) {
      const int2 ex=int2(min(base.x+t,int(width)-1),base.y), ox=int2(max(base.x-t,0),base.y);
      const int2 ey=int2(base.x,min(base.y+t,int(height)-1)), oy=int2(base.x,max(base.y-t,0));
      const int2 pontos[4]={ex,ox,ey,oy};
      [unroll] for(int k=0;k<4;++k) {
       float2 gx=historyMotion.Load(int4(min(pontos[k].x+1,int(width)-1),pontos[k].y,slice,0))
                -historyMotion.Load(int4(max(pontos[k].x-1,0),pontos[k].y,slice,0));
       float2 gy=historyMotion.Load(int4(pontos[k].x,min(pontos[k].y+1,int(height)-1),slice,0))
                -historyMotion.Load(int4(pontos[k].x,max(pontos[k].y-1,0),slice,0));
       if(!finite2(gx)||!finite2(gy))continue;
       soma+=(length(gx*float2(width,height))+length(gy*float2(width,height)))*.5;
       peso+=1;
      }
     }
     porPixel=soma/peso;
    }
    const float acumulada=porPixel*float(age);
    deformacaoVista=max(deformacaoVista,acumulada);
    {const float antesM=confidence;confidence=min(confidence,saturate(1.0-acumulada/deformTolerance));if(confidence<antesM-1e-4)motivoBrando=22;}
   }
  }
  if(occlusionTolerance>0) {

   const int2 pt=clamp(int2(prior*float2(width,height)),int2(0,0),int2(int(width)-1,int(height)-1));
   const float2 mvPrior=historyMotion.Load(int4(pt,previous,0));
   if(!finite2(mvPrior))SAIR(13)

   const float2 volta=(float2(pt)+0.5)*inverseSize-mvPrior;
   const float erroPx=length((volta-uv)*float2(width,height));
   {const float antesM=confidence;confidence=min(confidence,saturate(1.0-erroPx/occlusionTolerance));if(confidence<antesM-1e-4)motivoBrando=23;}
  }
  uv=prior; slice=previous;
 }
 float4 originalSample=historyColor.SampleLevel(linearClamp,float3(uv,slice),0);
 if(!isfinite(currentSample.a)||!isfinite(originalSample.a)||currentSample.a<=0||originalSample.a<=0) SAIR(14)

 float exposureError=abs(currentSample.a-originalSample.a)/max(currentSample.a,originalSample.a);
 {const float antesM=confidence;confidence*=saturate(1.0-exposureError/0.1);if(confidence<antesM-1e-4)motivoBrando=24;}
 float3 original=originalSample.rgb;
 original*=currentExposure/sourceExposure;
 if(!finite3(original)) SAIR(15)
 float3 scale=max(max(abs(original),abs(current)),0.05);
 float error=max(max(abs(original.r-current.r)/scale.r,abs(original.g-current.g)/scale.g),abs(original.b-current.b)/scale.b);
 if(!isfinite(error)||colorThreshold<=0) SAIR(16)
 {const float antesM=confidence;confidence*=saturate(1.0-error/colorThreshold);if(confidence<antesM-1e-4)motivoBrando=25;}
 if(gateCoherent>0&&confidence<1.0) {

  const int raioC=int(min(gateCoherent,4.0));
  const int2 baseC=clamp(int2(currentUv*float2(width,height)),int2(0,0),int2(width-1,height-1));
  float menor=confidence;
  [loop] for(int tc=1;tc<=raioC;++tc) {
   const int2 vizC[4]={int2(min(baseC.x+tc,int(width)-1),baseC.y),int2(max(baseC.x-tc,0),baseC.y),
                       int2(baseC.x,min(baseC.y+tc,int(height)-1)),int2(baseC.x,max(baseC.y-tc,0))};
   [unroll] for(int kc=0;kc<4;++kc) {

    float vizinha=sourceResidual.Load(int3(vizC[kc],0)).a;
    if(isfinite(vizinha))menor=min(menor,saturate(vizinha));
   }
  }
  {const float antesM=confidence;confidence=menor;if(confidence<antesM-1e-4)motivoBrando=26;}
 }

 const float2 uvJob=uv;

 uv+=motionJitterUV;
 const bool lerProf=(flags&1024u)!=0;
 float4 residual;
 float4 somaCentro=0;float pesoCentro=0;
 if(lerProf) {

  LerResiduoProf(uv,slice,refProf,somaCentro,pesoCentro);
  if(!(pesoCentro>0)) SAIR(18)
  residual=somaCentro/pesoCentro;
 } else {
  residual=sourceResidual.SampleLevel(linearClamp,uv,0);
 }
 if(!all(isfinite(residual))) SAIR(17)
 if(detailMotion>0&&lerProf) {

  float2 movimentoAqui=historyMotion.SampleLevel(linearClamp,float3(currentUv,currentSlice),0);
  const float pixelsAqui=finite2(movimentoAqui)?length(movimentoAqui*float2(width,height)):0;
  const float manterDetalhe=saturate(1.0-pixelsAqui/detailMotion);
  if(manterDetalhe<1.0) {
   float4 somaB=somaCentro;float pesoB=pesoCentro;
   [unroll] for(int tb=1;tb<=2;++tb) {
    const float2 passoB=inverseSize*float(tb);
    const float2 pontosB[4]={uv+float2(passoB.x,0),uv-float2(passoB.x,0),
                             uv+float2(0,passoB.y),uv-float2(0,passoB.y)};
    [unroll] for(int kb=0;kb<4;++kb) {
     if(!inside(pontosB[kb]))continue;
     LerResiduoProf(pontosB[kb],slice,refProf,somaB,pesoB);
    }
   }
   const float4 baixa=somaB/pesoB;
   residual=baixa+(residual-baixa)*manterDetalhe;
  }
 } else if(detailMotion>0) {

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
  }
 }
 if(deformBlur>0&&deformacaoVista>0) {

  const float mistura=saturate(deformacaoVista/max(deformTolerance,1e-4));
  const float raioF=min(deformBlur,4.0);
  if(raioF>=1.0&&mistura>0) {
   const int passos=int(raioF);
   float4 soma=residual;float pesoF=1;
   [loop] for(int tf=1;tf<=passos;++tf) {
    const float2 passo=inverseSize*float(tf);
    const float2 pontosF[4]={uv+float2(passo.x,0),uv-float2(passo.x,0),
                             uv+float2(0,passo.y),uv-float2(0,passo.y)};
    [unroll] for(int kf=0;kf<4;++kf) {
     if(!inside(pontosF[kf]))continue;
     float4 vizinhoF=sourceResidual.SampleLevel(linearClamp,pontosF[kf],0);
     if(!all(isfinite(vizinhoF)))continue;
     soma+=vizinhoF;pesoF+=1;
    }
   }
   residual=lerp(residual,soma/pesoF,mistura);
  }
 }

 float corte=0.0;
 {
  const uint nivelCorte=(flags>>12)&3u;
  const float raioCorte=float((flags>>16)&63u);
  if(nivelCorte!=0u&&age>0u&&raioCorte>0.0) {
   const float2 tam=float2(width,height);
   const int2 limC=int2(int(width)-1,int(height)-1);
   const int2 pj=clamp(int2(uvJob*tam),int2(0,0),limC);
   const float z0=historyDepth.Load(int4(pj,slice,0));
   if(isfinite(z0)) {
    const float xp=historyDepth.Load(int4(min(pj+int2(1,0),limC),slice,0)),xm=historyDepth.Load(int4(max(pj-int2(1,0),int2(0,0)),slice,0));
    const float yp=historyDepth.Load(int4(min(pj+int2(0,1),limC),slice,0)),ym=historyDepth.Load(int4(max(pj-int2(0,1),int2(0,0)),slice,0));
    float gx=pj.x<=0 ? xp-z0 : pj.x>=limC.x ? z0-xm : (abs(xp-z0)<abs(z0-xm) ? xp-z0 : z0-xm);
    float gy=pj.y<=0 ? yp-z0 : pj.y>=limC.y ? z0-ym : (abs(yp-z0)<abs(z0-ym) ? yp-z0 : z0-ym);
    if(!isfinite(gx))gx=0;
    if(!isfinite(gy))gy=0;
    const float2 grad=float2(gx,gy);

    const int passoR=max(1,int(ceil(raioCorte/10.0)));
    float fJob=0,wOc=0;float2 mvOc=0,dirOc=0;
    [loop] for(int kc=0;kc<8;++kc) {
     const int2 dk=corteD8[kc];
     const float passoD=kc<4 ? 1.0 : 1.41421356;
     [loop] for(int rc=1;float(rc)*passoD<=raioCorte+0.5;rc+=passoR) {
      const int2 o=dk*rc,q=pj+o;
      if(any(q<0)||any(q>limC))break;
      const float zq=historyDepth.Load(int4(q,slice,0));
      if(!isfinite(zq))continue;
      const float zp=z0+dot(grad,float2(o));
      const float s=(zq-zp)/max(max(abs(zq),abs(zp)),1e-9);
      const float wt=smoothstep(0.02,0.08,s)*(1.0-smoothstep(0.75*raioCorte,raioCorte+1.5,float(rc)*passoD));
      if(wt>0.0) {
       fJob=max(fJob,wt);
       const float2 mq=historyMotion.Load(int4(q,slice,0));
       if(finite2(mq)){mvOc+=wt*mq;wOc+=wt;dirOc+=wt*float2(dk)/passoD;}
      }
     }
    }
    const float nd=length(dirOc);
    if(fJob>0.0&&wOc>1e-4&&nd>1e-4) {
     mvOc/=wOc;
     const float2 fora=-dirOc/nd;

     float pesoR=0,menorY=1e30;float2 mvR=0;
     [loop] for(int jr=0;jr<4;++jr) {
      float wc;float3 rr;float2 mt;
      if(!CorteReferencia(jr,fora,pj,limC,z0,grad,raioCorte,slice,wc,rr,mt))continue;
      pesoR+=wc;mvR+=wc*mt;menorY=min(menorY,dot(rr,float3(.2126,.7152,.0722)));
     }
     if(pesoR>0.25) {
      mvR/=pesoR;
      const float tauR=0.1*abs(menorY)+0.002;
      float3 somaR=0;float pesoM=0;
      [loop] for(int jm=0;jm<4;++jm) {
       float wc;float3 rr;float2 mt;
       if(!CorteReferencia(jm,fora,pj,limC,z0,grad,raioCorte,slice,wc,rr,mt))continue;
       const float wm=wc*exp(-(dot(rr,float3(.2126,.7152,.0722))-menorY)/tauR);
       if(wm>0.0){somaR+=wm*rr;pesoM+=wm;}
      }
      const float px720=720.0/float(height);
      float v=length((mvOc-mvR)*tam)*px720;
      if(nivelCorte==1u)v=min(v,length((mvOc-(uvJob-currentUv)/float(age))*tam)*px720);
      if(pesoM>0.0)corte=fJob*smoothstep(0.25,1.0,v)*saturate(pesoR);
      if(corte>0.0)residual.rgb=lerp(residual.rgb,somaR/pesoM,corte);
     }
    }
   }
  }
 }
 if(confidenceRate>0) {

  float previous=saturate(sourceResidual.SampleLevel(linearClamp,uv,0).a);
  {const float antesM=confidence;confidence=clamp(confidence,previous-confidenceRate,previous+confidenceRate);if(confidence<antesM-1e-4)motivoBrando=27;}
 }
 if(confidenceFloor>0) {

  confidence=max(confidence,min(confidenceFloor,1.0));
 }

 if(corte>0.0)confidence=lerp(confidence,1.0,corte);
 float3 applied=residual.rgb*(confidence*residualExposureScale);
 {

  const uint iForca=(debugMotivo>>20)&7u,iRef=(debugMotivo>>24)&3u;
  if(iForca!=0u) {
   const float fracao=iForca==1u?0.75:iForca==2u?0.6:iForca==3u?0.5:iForca==4u?0.4:0.3;
   const float referencia=iRef==1u?4.0:iRef==2u?8.0:iRef==3u?16.0:2.0;
   const float2 mvForca=historyMotion.SampleLevel(linearClamp,float3(currentUv,currentSlice),0);
   const float pxForca=finite2(mvForca)?length(mvForca*float2(width,height)):0;

   applied*=lerp(1.0,fracao,saturate(pxForca/referencia));
  }
 }
 {

  const uint iRuido=(debugMotivo>>26)&7u;
  if(iRuido!=0u) {
   const float limiar=iRuido==1u?0.02:iRuido==2u?0.05:iRuido==3u?0.1:0.2;
   const bool tom=((debugMotivo>>29)&1u)!=0u;
   const float3 dRuido=tom?tomRuido3(original*currentExposure)-tomRuido3(current*currentExposure):original-current;
   const float amplitude=max(max(abs(dRuido.r),abs(dRuido.g)),abs(dRuido.b));
   const float2 mvRuido=historyMotion.SampleLevel(linearClamp,float3(currentUv,currentSlice),0);
   const float pxRuido=finite2(mvRuido)?length(mvRuido*float2(width,height)):0;
   const float peso=(1.0-saturate(amplitude/limiar))*(1.0-saturate(pxRuido/2.0))*saturate(confidence);
   if(finite3(dRuido)&&peso>0)applied+=dRuido*peso;
  }
 }
 if(((debugMotivo>>30)&1u)!=0u&&((debugMotivo>>29)&1u)!=0u) {

  const float3 bJob=tomRuido3(original*currentExposure),bAgora=tomRuido3(current*currentExposure);
  const float3 razao=(tomInv3(bJob+applied)+1e-4)/(tomInv3(bJob)+1e-4);
  const float3 novo=tomRuido3(tomInv3(bAgora)*razao)-bAgora;
  if(finite3(novo)&&finite3(bJob)&&finite3(bAgora))applied=novo;
 }
 if(residualFloor>0) {

  const float magnitude=max(max(abs(residual.r),abs(residual.g)),abs(residual.b));

  applied*=smoothstep(0.0,residualFloor,magnitude);
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
  applied=lerp(applied,limited,saturate(effectiveClampStrength));
 }

 if((debugMotivo&1u)!=0){outputResidual[id.xy]=float4(applied,float(motivoBrando)+saturate(confidence)*0.99);return;}
 outputResidual[id.xy]=float4(applied,residual.a*confidence);
}

[numthreads(8,8,1)]
void Reduzir(uint3 id : SV_DispatchThreadID) {
 if (id.x >= width || id.y >= height) return;
 outputResidual[id.xy]=sourceResidual.SampleLevel(linearClamp,(float2(id.xy)+0.5)*inverseSize,0);
}
