cbuffer Constants : register(b0) {
 uint largura,altura,fatiaAtual,fatiaFonte;
 uint idade,nx,ny,historico;
 float forca;
 float tau;
 float ridge;
 float escalaResiduo;
 uint celula;
 uint modo;
 uint guia;
};
static const uint FAIXAS=16;
static const uint GRUPO=128;
static const uint MAXPIX=8;
Texture2DArray<float4> historicoCor : register(t0);
Texture2DArray<float2> historicoMovimento : register(t1);
Texture2D<float4> residuoFonte : register(t2);
Texture2D<float4> featuresFonte : register(t3);
Texture2D<float> lumaTom : register(t4);
Texture3D<float4> coeficientes[5] : register(t5);
RWTexture2D<float4> featuresSaida : register(u0);
RWTexture3D<float4> coeficientesSaida[5] : register(u1);
RWTexture2D<float4> residuoAplicado : register(u6);
RWTexture2D<float> lumaSaida : register(u7);
Texture2DArray<float> historicoProfundidade : register(t10);
SamplerState linearClamp : register(s0);

float tmf(float v){v=max(v,0.0);v=v/(1.0+v);return v<=0.0031308?v*12.92:1.055*pow(v,1.0/2.4)-0.055;}
float3 tom(float4 amostra){
 const float e=isfinite(amostra.a)&&amostra.a>0?amostra.a:1.0;
 return float3(tmf(amostra.r*e),tmf(amostra.g*e),tmf(amostra.b*e));
}
float luma(float3 t){return dot(t,float3(0.299,0.587,0.114));}

float detalhe(uint2 p){
 const float k[5]={1,4,6,4,1};
 float soma=0;
 [unroll] for(int dy=-2;dy<=2;++dy)
  [unroll] for(int dx=-2;dx<=2;++dx){
   const int2 q=clamp(int2(p)+int2(dx,dy),int2(0,0),int2(int(largura)-1,int(altura)-1));
   soma+=k[dx+2]*k[dy+2]*lumaTom.Load(int3(q,0));
  }
 return lumaTom.Load(int3(p,0))-soma/256.0;
}
void gravarLuma(uint2 p,uint fatia){lumaSaida[p]=luma(tom(historicoCor.Load(int4(p,fatia,0))));}
[numthreads(8,8,1)]
void BilateralLumaFonte(uint3 id:SV_DispatchThreadID){if(id.x<largura&&id.y<altura)gravarLuma(id.xy,fatiaFonte);}
[numthreads(8,8,1)]
void BilateralLumaAtual(uint3 id:SV_DispatchThreadID){if(id.x<largura&&id.y<altura)gravarLuma(id.xy,fatiaAtual);}

[numthreads(8,8,1)]
void BilateralPrep(uint3 id:SV_DispatchThreadID){
 if(id.x>=largura||id.y>=altura)return;
 const float3 t=tom(historicoCor.Load(int4(id.xy,fatiaFonte,0)));
 featuresSaida[id.xy]=float4(t,detalhe(id.xy));
}

float pesoEixo(float g,uint vertice,uint n){
 const float f0=floor(g);const float fr=g-f0;
 const int i0=clamp(int(f0),0,int(n)-1),i1=clamp(int(f0)+1,0,int(n)-1);
 return (i0==int(vertice)?1.0-fr:0.0)+(i1==int(vertice)?fr:0.0);
}

groupshared float reducao[GRUPO][31];
groupshared uint faixaMin,faixaMax,profMin,profMax;

float faixaProfundidade(float d,float lo,float hi){
 const float largura_=hi-lo;
 if(!(largura_>max(1e-7,.02*abs(hi))))return 0;
 return saturate((d-lo)/largura_)*(FAIXAS-1);
}
float profundidadeValida(float d){return isfinite(d)&&d>=0?d:0;}

[numthreads(GRUPO,1,1)]
void BilateralFit(uint3 grupo:SV_GroupID,uint indice:SV_GroupIndex){
 const uint vx=grupo.x,vy=grupo.y;

 const float CELULA=float(celula);
 const int x0=max(0,int(floor((float(vx)-0.5)*CELULA-0.5))+1),x1=min(int(largura)-1,int(ceil((float(vx)+1.5)*CELULA-0.5))-1);
 const int y0=max(0,int(floor((float(vy)-0.5)*CELULA-0.5))+1),y1=min(int(altura)-1,int(ceil((float(vy)+1.5)*CELULA-0.5))-1);
 const int xa=vx==0?0:x0,xb=vx+1>=nx?int(largura)-1:x1,ya=vy==0?0:y0,yb=vy+1>=ny?int(altura)-1:y1;
 const uint w=uint(xb-xa+1),h=uint(yb-ya+1),total=w*h;
 if(indice==0){faixaMin=FAIXAS;faixaMax=0;profMin=0x7f7fffffu;profMax=0;}
 GroupMemoryBarrierWithGroupSync();

 float4 fe[MAXPIX];float3 re[MAXPIX];float pxy[MAXPIX];float gz[MAXPIX];float dz[MAXPIX];
 uint meuMin=FAIXAS,meuMax=0,meuPMin=0x7f7fffffu,meuPMax=0;
 [unroll] for(uint k=0;k<MAXPIX;++k){
  const uint i=indice+k*GRUPO;pxy[k]=0;fe[k]=0;re[k]=0;gz[k]=0;dz[k]=0;
  if(i<total){
   const int2 p=int2(xa+int(i%w),ya+int(i/w));
   const float4 f=featuresFonte.Load(int3(p,0));const float4 r=residuoFonte.Load(int3(p,0));
   if(all(isfinite(f))&&all(isfinite(r.rgb))){
    const float gx=(float(p.x)+0.5)/float(celula)-0.5,gy=(float(p.y)+0.5)/float(celula)-0.5;
    pxy[k]=pesoEixo(gx,vx,nx)*pesoEixo(gy,vy,ny);fe[k]=f;re[k]=r.rgb;gz[k]=saturate(luma(f.rgb))*(FAIXAS-1);
    if(guia!=0){dz[k]=profundidadeValida(historicoProfundidade.Load(int4(p,fatiaFonte,0)));
     if(pxy[k]>0){meuPMin=min(meuPMin,asuint(dz[k]));meuPMax=max(meuPMax,asuint(dz[k]));}}
   }
  }
 }

 float lo=0,hi=0;
 if(guia!=0){
  InterlockedMin(profMin,meuPMin);InterlockedMax(profMax,meuPMax);
  GroupMemoryBarrierWithGroupSync();
  if(profMin<=profMax){lo=asfloat(profMin);hi=asfloat(profMax);}
  [unroll] for(uint k4=0;k4<MAXPIX;++k4)gz[k4]=faixaProfundidade(dz[k4],lo,hi);
 }
 [unroll] for(uint k5=0;k5<MAXPIX;++k5)
  if(pxy[k5]>0){const uint b0=uint(floor(gz[k5]));meuMin=min(meuMin,b0);meuMax=max(meuMax,min(b0+1,FAIXAS-1));}
 InterlockedMin(faixaMin,meuMin);InterlockedMax(faixaMax,meuMax);
 GroupMemoryBarrierWithGroupSync();
 const uint fmin=faixaMin,fmax=faixaMax;

 if(indice<FAIXAS&&(fmin>fmax||indice<fmin||indice>fmax)){
  [unroll] for(uint o=0;o<5;++o)coeficientesSaida[o][uint3(vx,vy,indice)]=float4(0,0,0,o==0?lo:(o==1?hi:0));
 }
 if(fmin>fmax)return;
 [loop] for(uint faixa=fmin;faixa<=fmax;++faixa){
  float acc[31];[unroll] for(uint z=0;z<31;++z)acc[z]=0;
  [unroll] for(uint k2=0;k2<MAXPIX;++k2){
   const float peso=pxy[k2]*pesoEixo(gz[k2],faixa,FAIXAS);
   if(peso<=0)continue;
   const float f[5]={fe[k2].r,fe[k2].g,fe[k2].b,fe[k2].a,1.0};
   uint k=0;
   [unroll] for(uint a=0;a<5;++a)[unroll] for(uint b=a;b<5;++b){acc[k]+=peso*f[a]*f[b];++k;}
   [unroll] for(uint a2=0;a2<5;++a2){acc[15+a2*3]+=peso*f[a2]*re[k2].r;acc[16+a2*3]+=peso*f[a2]*re[k2].g;acc[17+a2*3]+=peso*f[a2]*re[k2].b;}
   acc[30]+=peso;
  }
  [unroll] for(uint z2=0;z2<31;++z2)reducao[indice][z2]=acc[z2];
  GroupMemoryBarrierWithGroupSync();
  [unroll] for(uint s=GRUPO/2;s>0;s>>=1){
   if(indice<s){[unroll] for(uint z3=0;z3<31;++z3)reducao[indice][z3]+=reducao[indice+s][z3];}
   GroupMemoryBarrierWithGroupSync();
  }
  if(indice==0){
   float A[5][5];uint k3=0;
   [unroll] for(uint a=0;a<5;++a)[unroll] for(uint b=a;b<5;++b){A[a][b]=reducao[0][k3];A[b][a]=reducao[0][k3];++k3;}
   const float reg=ridge*(reducao[0][30]+1e-6);
   [unroll] for(uint d=0;d<5;++d)A[d][d]+=reg;
   float L[5][5];[unroll] for(uint i2=0;i2<5;++i2)[unroll] for(uint j2=0;j2<5;++j2)L[i2][j2]=0;
   bool valido=true;
   [unroll] for(uint j=0;j<5;++j){
    float s2=A[j][j];[unroll] for(uint m=0;m<j;++m)s2-=L[j][m]*L[j][m];
    if(!(s2>1e-20)){valido=false;s2=1e-20;}
    L[j][j]=sqrt(s2);
    [unroll] for(uint i3=j+1;i3<5;++i3){float t=A[i3][j];[unroll] for(uint m2=0;m2<j;++m2)t-=L[i3][m2]*L[j][m2];L[i3][j]=t/L[j][j];}
   }
   float x[3][5];
   [unroll] for(uint c=0;c<3;++c){
    float y[5];
    [unroll] for(uint i4=0;i4<5;++i4){float t2=reducao[0][15+i4*3+c];[unroll] for(uint m3=0;m3<i4;++m3)t2-=L[i4][m3]*y[m3];y[i4]=t2/L[i4][i4];}
    [unroll] for(int i5=4;i5>=0;--i5){float t3=y[i5];[unroll] for(int m4=4;m4>i5;--m4)t3-=L[m4][i5]*x[c][m4];x[c][i5]=t3/L[i5][i5];}
   }
   [unroll] for(uint o=0;o<5;++o){
    const float a=o==0?lo:(o==1?hi:0);
    const float4 v=float4(x[0][o],x[1][o],x[2][o],a);
    coeficientesSaida[o][uint3(vx,vy,faixa)]=valido&&all(isfinite(v))?v:float4(0,0,0,a);
   }
  }
  GroupMemoryBarrierWithGroupSync();
 }
}

float3 modeloProfundidade(float2 posicaoFonte,float3 t,float d,float profundidade){
 const float gx=posicaoFonte.x/float(celula)-0.5,gy=posicaoFonte.y/float(celula)-0.5;
 const float fx=floor(gx),fy=floor(gy),rx=gx-fx,ry=gy-fy;
 const float f[5]={t.r,t.g,t.b,d,1.0};
 float3 soma=0;
 [unroll] for(uint b=0;b<2;++b)[unroll] for(uint a=0;a<2;++a){
  const int ix=clamp(int(fx)+int(a),0,int(nx)-1),iy=clamp(int(fy)+int(b),0,int(ny)-1);
  const float peso=(a?rx:1.0-rx)*(b?ry:1.0-ry);
  const float lo=coeficientes[0].Load(int4(ix,iy,0,0)).a,hi=coeficientes[1].Load(int4(ix,iy,0,0)).a;
  const float3 uvw=float3((float(ix)+0.5)/float(nx),(float(iy)+0.5)/float(ny),(faixaProfundidade(profundidade,lo,hi)+0.5)/FAIXAS);
  [unroll] for(uint o=0;o<5;++o)soma+=peso*f[o]*coeficientes[o].SampleLevel(linearClamp,uvw,0).rgb;
 }
 return soma;
}
float3 modelo(float2 posicaoFonte,float3 t,float d){

 const float3 uvw=float3((posicaoFonte.x/float(celula))/float(nx),(posicaoFonte.y/float(celula))/float(ny),(saturate(luma(t))*(FAIXAS-1)+0.5)/FAIXAS);
 const float f[5]={t.r,t.g,t.b,d,1.0};
 float3 soma=0;
 [unroll] for(uint o=0;o<5;++o)soma+=f[o]*coeficientes[o].SampleLevel(linearClamp,uvw,0).rgb;
 return soma;
}

[numthreads(8,8,1)]
void BilateralApply(uint3 id:SV_DispatchThreadID){
 if(id.x>=largura||id.y>=altura||forca<=0)return;
 const float4 atual=historicoCor.Load(int4(id.xy,fatiaAtual,0));
 if(!all(isfinite(atual)))return;
 const float3 t=tom(atual);
 const float d=detalhe(id.xy);

 const float2 mv=historicoMovimento.Load(int4(id.xy,fatiaAtual,0));
 const float2 mvPx=all(isfinite(mv))?mv*float2(largura,altura):float2(0,0);
 const float2 posicao=float2(id.xy)+0.5+mvPx*float(idade);
 const float3 m=(guia!=0?modeloProfundidade(posicao,t,d,profundidadeValida(historicoProfundidade.Load(int4(id.xy,fatiaAtual,0))))
                         :modelo(posicao,t,d))*escalaResiduo;
 if(!all(isfinite(m)))return;

 const bool soModelo=(modo&1u)!=0;
 const float4 arrastado=soModelo?float4(0,0,0,1):residuoAplicado[id.xy];
 const float confianca=soModelo?0.0:(isfinite(arrastado.a)?saturate(arrastado.a):0.0);
 const float3 r=all(isfinite(arrastado.rgb))?arrastado.rgb:float3(0,0,0);
 const float3 transferido=m+confianca*clamp(r-m,-tau,tau);
 residuoAplicado[id.xy]=float4(lerp(r,transferido,forca),arrastado.a);
}
