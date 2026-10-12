Texture2D<float4> res : register(t0); Texture2D<float4> prev : register(t1); RWTexture2D<float4> outp : register(u0); RWByteAddressBuffer flags : register(u1);
#ifdef NR_GRADE_CG2R
cbuffer C : register(b0) { uint tone; uint w; uint h; float expo; uint estilo; float tomEstilo; float gradeEv, gradeContraste, gradeSaturacao; uint gp0, gp1, gp2;
#ifdef NR_GANHO_HISTORICO

    float ganhoProfRel, ganhoProfAbs; uint gp3, gp4;
#endif
#ifdef NR_LUZ

    float luzAlfa0, luzDelta, luzEps; uint luzBits;
#endif
}
#else
cbuffer C : register(b0) { uint tone; uint w; uint h; float expo; }
#endif

#ifndef NR_RESIDUAL_AT
#define NR_RESIDUAL_AT(p) res[p].rgb
#endif
float tmf(float v) { v = max(v, 0.0); v = v / (1.0 + v); return v <= 0.0031308 ? v * 12.92 : 1.055 * pow(v, 1.0 / 2.4) - 0.055; }
float tmi(float v) { v = saturate(v); v = v <= 0.04045 ? v / 12.92 : pow((v + 0.055) / 1.055, 2.4); v = min(v, 0.95); return v / (1.0 - v); }
float3 tmi3(float3 v) { return float3(tmi(v.r), tmi(v.g), tmi(v.b)); }
#ifdef NR_GRADE_CG2R

float3 gradeHsl(float3 rgb) {
    float high = max(max(rgb.r, rgb.g), rgb.b), low = min(min(rgb.r, rgb.g), rgb.b), sum = high + low, light = sum * 0.5;
    float hue = 0.0, sat = 0.0;
    if (high > low) {
        float delta = high - low;
        sat = light > 0.5 ? delta / ((2.0 - high) - low) : delta / sum;
        if (high == rgb.r) hue = ((rgb.g - rgb.b) / delta + (rgb.g < rgb.b ? 6.0 : 0.0)) / 6.0;
        else if (high == rgb.g) hue = ((rgb.b - rgb.r) / delta + 2.0) / 6.0;
        else hue = ((rgb.r - rgb.g) / delta + 4.0) / 6.0;
    }
    return float3(hue, sat, light);
}
float gradeHue(float p, float q, float h) {
    if (h < 0.0) h += 1.0;
    if (h > 1.0) h -= 1.0;
    if (h < 1.0 / 6.0) return p + (q - p) * 6.0 * h;
    if (h < 0.5) return q;
    if (h < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - h) * 6.0;
    return p;
}
float3 gradeRgb(float3 hsl) {
    if (hsl.y <= 0.0) return hsl.zzz;
    float q = hsl.z < 0.5 ? hsl.z * (hsl.y + 1.0) : hsl.z + hsl.y - hsl.z * hsl.y;
    float p = 2.0 * hsl.z - q;
    return float3(gradeHue(p, q, hsl.x + 1.0 / 3.0), gradeHue(p, q, hsl.x), gradeHue(p, q, hsl.x - 1.0 / 3.0));
}
float3 gradeCg2r(float3 v) {

    const float ev = gradeEv, contraste = gradeContraste, saturacao = gradeSaturacao;
    if (ev == 0.0 && contraste == 0.0 && saturacao == 0.0) return v;
    float3 c;
    [unroll] for (int k = 0; k < 3; ++k) {
        const float e = saturate(saturate(v[k]) * exp2(ev));
        const float curva = e * e * (3.0 - 2.0 * e) - e;
        c[k] = saturate(e + clamp(contraste, -1.0, 1.0) * curva);
    }
    const float3 hsl = gradeHsl(c);
    return saturate(gradeRgb(float3(hsl.x, saturate(hsl.y * (saturacao + 1.0)), hsl.z)));
}
#define NR_GRADE(v) gradeCg2r(v)
#else
#define NR_GRADE(v) (v)
#endif
#ifdef NR_GRADE_CG2R

static float2 ganhoLimite=float2(0.0,0.0);
static float ganhoLog=0.0;
#ifdef NR_GANHO_HISTORICO

static float3 luzPixel=float3(0.0,0.0,0.0);
static float luzGuia=0.0;
static float2 haloExc=float2(0.0,0.0);
static float3 haloTeto=float3(0.0,0.0,0.0);

float TanhLimitado(float x) { const float t=exp(-2.0*abs(x)); return (x<0.0 ? -1.0 : 1.0)*(1.0-t)/(1.0+t); }
#endif
float AjustarGanho(float lr,float detail,float guard,float bound) {
#ifdef NR_GANHO_HISTORICO
    if((gp0&2u)!=0u) {
        const float lnG=(1.0+max(detail-1.0,0.0))*log(max(lr,1e-6));
        const float L=luzPixel.x*luzGuia+luzPixel.y,H=lnG-L;
        const float gb=asfloat(gp3),ga=asfloat(gp4);

        const uint nivelCorte=(gp1>>3)&3u;
        float Lc=L,Hc=H;
        if(haloExc.x>0.0) {
            const float e=L-haloExc.y-(nivelCorte!=0u ? 0.005 : 0.02);Lc=L-haloExc.x*0.5*(e+sqrt(e*e+1e-4));
            if(nivelCorte!=0u)Hc=H*(1.0-haloExc.x);
        }
        const float lb=gb>1e-6 ? gb*TanhLimitado((Lc+luzPixel.z)/gb) : 0.0;
        float la=ga>1e-6 ? ga*TanhLimitado(Hc/ga) : 0.0;

        if(ganhoLimite.y>0.0)la=clamp(la,ganhoLimite.x-ganhoLimite.y,ganhoLimite.x+ganhoLimite.y);
        ganhoLog=la;
        float lnF=lb+la;

        if(haloTeto.x>0.0&&gb>1e-6) {
            const float refF=gb*TanhLimitado((haloTeto.y+luzPixel.z)/gb);
            const float eF=gb*TanhLimitado((haloTeto.y+haloTeto.z+luzPixel.z)/gb)-refF;
            if(eF>0.0)lnF=max(lnF-haloTeto.x*eF,min(lnF,refF));
        }
        return exp(lnF);
    }
#endif
    if((gp0&1u)!=0u) {
        const float g=log(guard);
        bound=g>1e-6 ? exp(g*tanh((1.0+max(detail-1.0,0.0))*log(max(lr,1e-6))/g)) : 1.0;
    }
    float lg=log(bound);
    if(ganhoLimite.y>0.0) {

        const float limitado=clamp(lg,ganhoLimite.x-ganhoLimite.y,ganhoLimite.x+ganhoLimite.y);
        if(limitado!=lg) {bound=exp(limitado);lg=limitado;}
    }
    ganhoLog=lg;
    return bound;
}
#endif
#ifdef NR_GANHO_HISTORICO

Texture2D<float> ganhoAnterior : register(t2);
Texture2DArray<float2> ganhoMovimento : register(t3);
Texture2DArray<float> ganhoProfundidade : register(t4);
RWTexture2D<float> ganhoAtual : register(u2);
SamplerState ganhoLinear : register(s0);

float2 LerGanhoAnterior(uint2 p) {
    if((gp1&3u)!=3u) return float2(0.0,0.0);
    const uint atual=(gp1>>8)&255u,anterior=(gp1>>16)&255u;
    const float2 uv=(float2(p)+0.5)/float2(w,h);
    const float2 mv=(gp1&4u)!=0u ? ganhoMovimento.Load(int4(p,atual,0)) : ganhoMovimento.SampleLevel(ganhoLinear,float3(uv,atual),0);
    const float2 uvp=uv+mv;
    if(!all(isfinite(uvp))||any(uvp<0.0)||any(uvp>1.0)) return float2(0.0,0.0);
    const float a=ganhoProfundidade.Load(int4(p,atual,0));
    const float b=ganhoProfundidade.Load(int4(min(uint2(uvp*float2(w,h)),uint2(w-1,h-1)),anterior,0));
    const float tol=max(ganhoProfAbs,ganhoProfRel*max(abs(a),abs(b)));
    if(!isfinite(a)||!isfinite(b)||!(tol>0.0)||abs(a-b)>tol) return float2(0.0,0.0);
    const float g=ganhoAnterior.SampleLevel(ganhoLinear,uvp,0);
    return isfinite(g) ? float2(g,asfloat(gp2)) : float2(0.0,0.0);
}

static float haloFator=1.0;
float HaloFator(uint2 p) {
    const float forca=float((gp0>>8)&255u)/255.0,degrau=float((gp0>>20)&255u)/1000.0;
    const int R=int(((gp0>>16)&15u)|((gp0>>2)&16u));
    const uint atual=(gp1>>8)&255u;
    const float z0=ganhoProfundidade.Load(int4(p,atual,0));
    if(R<=0||!(forca>0.0)||!(degrau>0.0)||!isfinite(z0))return 1.0;
    const int2 d8[8]={int2(1,0),int2(-1,0),int2(0,1),int2(0,-1),int2(1,1),int2(-1,1),int2(1,-1),int2(-1,-1)};
    float f=0.0;
    [unroll] for(int k=0;k<8;++k) {
        const float passo=k<4 ? 1.0 : 1.41421356;
        float zAnt=z0;
        [loop] for(int r=1;r<=R;++r) {
            const int2 q=int2(p)+d8[k]*r;
            if(any(q<0)||q.x>=int(w)||q.y>=int(h))break;
            const float zq=ganhoProfundidade.Load(int4(q,atual,0));
            if(!isfinite(zq))break;
            const float tol=degrau*max(abs(zAnt),abs(zq));
            if(zq-zAnt>tol){f=max(f,saturate((float(R)+1.0-float(r)*passo)/float(R)));break;}
            if(zAnt-zq>tol)break;
            zAnt=zq;
        }
    }
    return 1.0-forca*f;
}

float PesoProf(float za,float zb) {
    if(!isfinite(za)||!isfinite(zb))return 0.0;
    const float rel=abs(za-zb)/max(max(abs(za),abs(zb)),1e-6);
    return saturate(1.0-(rel-0.05)/0.10);
}
float ProfTexel(int2 q,uint fatia) {
    const uint2 p=min(uint2(q)*4u+2u,uint2(w-1,h-1));
    return ganhoProfundidade.Load(int4(p,fatia,0));
}

Texture2D<float4> luzCoef : register(t5);
float3 LerLuz(uint2 p) {
    const int2 m=int2((w+3u)/4u,(h+3u)/4u)-1;
    const float2 s=clamp((float2(p)+0.5)/4.0-0.5,float2(0.0,0.0),float2(m));
    const int2 b=min(int2(floor(s)),m);const float2 f=s-float2(b);const int2 e=min(b+1,m);
    const float4 c00=luzCoef.Load(int3(b,0)),c10=luzCoef.Load(int3(e.x,b.y,0)),c01=luzCoef.Load(int3(b.x,e.y,0)),c11=luzCoef.Load(int3(e,0));
    if((gp0&32u)!=0u) {
        const float zp=ganhoProfundidade.Load(int4(p,(gp1>>8)&255u,0));
        if(isfinite(zp)) {
            const float4 wt=float4((1.0-f.x)*(1.0-f.y),f.x*(1.0-f.y),(1.0-f.x)*f.y,f.x*f.y)*
                            float4(PesoProf(zp,c00.w),PesoProf(zp,c10.w),PesoProf(zp,c01.w),PesoProf(zp,c11.w));
            const float soma=wt.x+wt.y+wt.z+wt.w;
            if(soma>1e-4)return (c00.xyz*wt.x+c10.xyz*wt.y+c01.xyz*wt.z+c11.xyz*wt.w)/soma;
            const float4 dz=abs(float4(c00.w,c10.w,c01.w,c11.w)-zp);
            float3 r=c00.xyz;float melhor=dz.x;
            if(dz.y<melhor){melhor=dz.y;r=c10.xyz;}
            if(dz.z<melhor){melhor=dz.z;r=c01.xyz;}
            if(dz.w<melhor)r=c11.xyz;
            return r;
        }
    }
    return lerp(lerp(c00,c10,f.x),lerp(c01,c11,f.x),f.y).xyz;
}

static const int2 haloD8[8]={int2(1,0),int2(-1,0),int2(0,1),int2(0,-1),int2(1,1),int2(-1,1),int2(1,-1),int2(-1,-1)};
static const float haloRaios[9]={1.0,2.0,3.0,4.0,5.0,6.0,8.0,10.0,12.0};

float2 HaloGrad(int2 p,uint fatia,float z0) {
    const int2 m=int2(int(w)-1,int(h)-1);
    const float xp=ganhoProfundidade.Load(int4(min(p+int2(1,0),m),fatia,0)),xm=ganhoProfundidade.Load(int4(max(p-int2(1,0),int2(0,0)),fatia,0));
    const float yp=ganhoProfundidade.Load(int4(min(p+int2(0,1),m),fatia,0)),ym=ganhoProfundidade.Load(int4(max(p-int2(0,1),int2(0,0)),fatia,0));
    const float ax=xp-z0,bx=z0-xm,ay=yp-z0,by=z0-ym;

    float gx=p.x<=0 ? ax : p.x>=m.x ? bx : (abs(ax)<abs(bx) ? ax : bx),gy=p.y<=0 ? ay : p.y>=m.y ? by : (abs(ay)<abs(by) ? ay : by);
    if(!isfinite(gx))gx=0.0;
    if(!isfinite(gy))gy=0.0;
    return float2(gx,gy);
}
float2 HaloExcesso(uint2 p) {
    const uint atual=(gp1>>8)&255u;
    const float R=float(((gp0>>28)&15u)*2u);
    const float z0=ganhoProfundidade.Load(int4(p,atual,0));
    if(!(R>0.0)||!isfinite(z0))return float2(0.0,0.0);
    const float2 g=HaloGrad(int2(p),atual,z0);
    float f=0.0,wOc=0.0;float2 mvOc=float2(0.0,0.0);
    [unroll] for(int k=0;k<8;++k) {
        const float passo=k<4 ? 1.0 : 1.41421356;
        [loop] for(int i=0;i<9;++i) {
            const int r=max(1,int(round(haloRaios[i]*R/12.0)));
            const float d=float(r)*passo;
            if(d>R+0.5)break;
            const int2 o=haloD8[k]*r,q=int2(p)+o;
            if(any(q<0)||q.x>=int(w)||q.y>=int(h))break;
            const float zq=ganhoProfundidade.Load(int4(q,atual,0));
            if(!isfinite(zq))continue;
            const float zp=z0+dot(g,float2(o));
            const float s=(zq-zp)/max(max(abs(zq),abs(zp)),1e-9);
            const float wt=smoothstep(0.02,0.08,s)*saturate((R+1.0-d)/R);
            if(wt>0.0) {
                f=max(f,wt);
                const float2 mq=ganhoMovimento.Load(int4(q,atual,0));
                if(all(isfinite(mq))){mvOc+=wt*mq;wOc+=wt;}
            }
        }
    }
    if(!(f>0.0)||!(wOc>1e-4))return float2(0.0,0.0);

    const int2 m=int2((w+3u)/4u,(h+3u)/4u)-1;
    const float2 s=clamp((float2(p)+0.5)/4.0-0.5,float2(0.0,0.0),float2(m));
    const int2 b=min(int2(floor(s)),m);const float2 fr=s-float2(b);const int2 e=min(b+1,m);
    const int2 t[4]={b,int2(e.x,b.y),int2(b.x,e.y),e};
    const float wb[4]={(1.0-fr.x)*(1.0-fr.y),fr.x*(1.0-fr.y),(1.0-fr.x)*fr.y,fr.x*fr.y};
    float4 acc=float4(0.0,0.0,0.0,0.0);float soma=0.0;
    [unroll] for(int j=0;j<4;++j) {
        const float wj=wb[j]*PesoProf(z0,ProfTexel(t[j],atual));
        if(wj>0.0){const float4 v=prev.Load(int3(t[j],0));if(all(isfinite(v))){acc+=wj*v;soma+=wj;}}
    }
    if(!(soma>1e-4))return float2(0.0,0.0);
    acc/=soma;
    const float mascara=smoothstep(0.25,1.0,length((mvOc/wOc-acc.zw)*float2(w,h))*720.0/float(h));
    const uint nivelCorte=(gp1>>3)&3u;
    if(nivelCorte!=0u&&mascara>0.0) {

        const float idade=float((gp1>>24)&15u);
        const float Reff=clamp(0.5*R+length((mvOc/wOc-acc.zw)*float2(w,h))*idade,0.5*R,R);
        float f2=0.0;
        [loop] for(int k2=0;k2<8;++k2) {
            const float passo=k2<4 ? 1.0 : 1.41421356;
            [loop] for(int i2=0;i2<9;++i2) {
                const int r=max(1,int(round(haloRaios[i2]*R/12.0)));
                const float d=float(r)*passo;
                if(d>R+0.5)break;
                const int2 o=haloD8[k2]*r,q=int2(p)+o;
                if(any(q<0)||q.x>=int(w)||q.y>=int(h))break;
                const float zq=ganhoProfundidade.Load(int4(q,atual,0));
                if(!isfinite(zq))continue;
                const float zp=z0+dot(g,float2(o));
                const float s=(zq-zp)/max(max(abs(zq),abs(zp)),1e-9);
                f2=max(f2,smoothstep(0.02,0.08,s)*(1.0-smoothstep(0.75*Reff,Reff+0.5,d)));
            }
        }
        const float kc=min(acc.y*f2*mascara,1.0);
        return kc>0.0 ? float2(kc,acc.x) : float2(0.0,0.0);
    }
    const float kk=min(acc.y*f*mascara,1.0);
    return kk>0.0 ? float2(kk,acc.x) : float2(0.0,0.0);
}

float3 HaloTeto(uint2 p) {
    const uint atual=(gp1>>8)&255u;
    const float R=float(((gp0>>28)&15u)*2u),forca=float((gp1>>28)&15u)/15.0;
    const float z0=ganhoProfundidade.Load(int4(p,atual,0));
    if(!(R>0.0)||!(forca>0.0)||!isfinite(z0))return float3(0.0,0.0,0.0);
    const float2 g=HaloGrad(int2(p),atual,z0);
    float f=0.0;
    [loop] for(int k=0;k<8;++k) {
        const float passo=k<4 ? 1.0 : 1.41421356;
        [loop] for(int i=0;i<9;++i) {
            const int r=max(1,int(round(haloRaios[i]*R/12.0)));
            const float d=float(r)*passo;
            if(d>R+0.5)break;
            const int2 o=haloD8[k]*r,q=int2(p)+o;
            if(any(q<0)||q.x>=int(w)||q.y>=int(h))break;
            const float zq=ganhoProfundidade.Load(int4(q,atual,0));
            if(!isfinite(zq))continue;
            const float zp=z0+dot(g,float2(o));
            const float s=(zq-zp)/max(max(abs(zq),abs(zp)),1e-9);
            f=max(f,smoothstep(0.02,0.08,s)*saturate((R+1.0-d)/R));
        }
    }
    if(!(f>0.0))return float3(0.0,0.0,0.0);
    const int2 m=int2((w+3u)/4u,(h+3u)/4u)-1;
    const float2 s=clamp((float2(p)+0.5)/4.0-0.5,float2(0.0,0.0),float2(m));
    const int2 b=min(int2(floor(s)),m);const float2 fr=s-float2(b);const int2 e=min(b+1,m);
    const int2 t[4]={b,int2(e.x,b.y),int2(b.x,e.y),e};
    const float wb[4]={(1.0-fr.x)*(1.0-fr.y),fr.x*(1.0-fr.y),(1.0-fr.x)*fr.y,fr.x*fr.y};
    float3 acc=float3(0.0,0.0,0.0);float soma=0.0;
    [unroll] for(int j=0;j<4;++j) {
        const float wj=wb[j]*PesoProf(z0,ProfTexel(t[j],atual));
        if(wj>0.0){const float3 v=prev.Load(int3(t[j],0)).xyz;if(all(isfinite(v))){acc+=wj*v;soma+=wj;}}
    }
    if(!(soma>1e-4))return float3(0.0,0.0,0.0);
    acc/=soma;
    const float exc=max(acc.y-acc.x,0.0),kt=forca*f*saturate(acc.z);
    return kt>0.0&&exc>0.0 ? float3(kt,acc.x,exc) : float3(0.0,0.0,0.0);
}
#endif

float SanitizeFinite(float v, float fallback) { return isfinite(v) ? v : fallback; }

float3 SanitizeFinite3(float3 v, float3 fallback)
{
    return float3(SanitizeFinite(v.x, fallback.x), SanitizeFinite(v.y, fallback.y),
                  SanitizeFinite(v.z, fallback.z));
}

float SafeDivide(float numerator, float denominator, float fallback)
{
    return abs(denominator) > 1e-8 ? numerator / denominator : fallback;
}

float3 LMSToBT709(float3 color)
{
    const float3x3 m = { 5.62059812, -4.57145756, 0.15577924,
                         -1.15555585, 2.25800438, -0.15415806,
                         0.03059913, -0.19018011, 1.06820532 };
    return mul(m, color);
}

float3 BT709ToLMS(float3 color)
{
    const float3x3 m = { 0.30569589, 0.62271286, 0.04528636,
                         0.15776262, 0.76968599, 0.08807030,
                         0.01933082, 0.11919478, 0.95053215 };
    return mul(m, color);
}

float3 D65NeutralBT709(float3 adaptiveStateLms, float luminance)
{
    float3 d65 = LMSToBT709(max(adaptiveStateLms, 1e-8));
    float d65Y = max(dot(d65, float3(0.2126, 0.7152, 0.0722)), 1e-8);
    return d65 * (luminance / d65Y);
}

float GamutCompressionScale(float3 color, float3 adaptiveStateLms)
{
    color = SanitizeFinite3(color, float3(0.0, 0.0, 0.0));

    const float y = dot(color, float3(0.2126, 0.7152, 0.0722));

    if (!(y > 1e-8))
        return 1.0;

    const float3 neutral = D65NeutralBT709(adaptiveStateLms, y);
    float scale = 1.0;

    if (color.r < 0.0 && neutral.r > color.r)
        scale = min(scale, SafeDivide(neutral.r, neutral.r - color.r, 1.0));

    if (color.g < 0.0 && neutral.g > color.g)
        scale = min(scale, SafeDivide(neutral.g, neutral.g - color.g, 1.0));

    if (color.b < 0.0 && neutral.b > color.b)
        scale = min(scale, SafeDivide(neutral.b, neutral.b - color.b, 1.0));

    return saturate(SanitizeFinite(scale, 1.0));
}

float3 ClampAp1(float3 color)
{
    const float3 adaptiveStateLms = BT709ToLMS(float3(0.18, 0.18, 0.18));
    const float scale = GamutCompressionScale(color, adaptiveStateLms);

    if (scale >= 1.0)
        return color;

    const float y = dot(color, float3(0.2126, 0.7152, 0.0722));
    const float3 neutral = D65NeutralBT709(adaptiveStateLms, y);

    return SanitizeFinite3(neutral + (color - neutral) * scale, max(neutral, 0.0));
}

float3 CbrtSigned(float3 v) { return sign(v) * pow(abs(v), 1.0 / 3.0); }

float3 ToOkLab(float3 color)
{
    const float3x3 rgb_to_lms = { 0.4122214708, 0.5363325363, 0.0514459929,
                                  0.2119034982, 0.6806995451, 0.1073969566,
                                  0.0883024619, 0.2817188376, 0.6299787005 };
    const float3x3 lms_to_lab = { 0.2104542553, 0.7936177850, -0.0040720468,
                                  1.9779984951, -2.4285922050, 0.4505937099,
                                  0.0259040371, 0.7827717662, -0.8086757660 };
    return mul(lms_to_lab, CbrtSigned(mul(rgb_to_lms, color)));
}

float3 FromOkLab(float3 lab)
{
    const float3x3 lab_to_lms = { 1.0, 0.3963377774, 0.2158037573,
                                  1.0, -0.1055613458, -0.0638541728,
                                  1.0, -0.0894841775, -1.2914855480 };
    const float3x3 lms_to_rgb = { 4.0767416621, -3.3077115913, 0.2309699292,
                                  -1.2684380046, 2.6097574011, -0.3413193965,
                                  -0.0041960863, -0.7034186147, 1.7076147010 };
    float3 lms = mul(lab_to_lms, lab);
    return mul(lms_to_rgb, lms * lms * lms);
}

float3 HueOkLab(float3 incorrect, float3 correct)
{
    float3 incorrectLab = ToOkLab(incorrect);
    const float3 correctLab = ToOkLab(correct);
    const float incorrectChroma = length(incorrectLab.yz);
    const float correctChroma = length(correctLab.yz);

    const float2 hueDirection = correctChroma > 1e-5 ? correctLab.yz / correctChroma : float2(0.0, 0.0);

    incorrectLab.yz = hueDirection * incorrectChroma;

    return ClampAp1(FromOkLab(incorrectLab));
}

#ifdef NR_GRADE_CG2R
float PisoRazao() { const uint i=(gp0>>3)&3u; return i==1u ? 0.005 : i==2u ? 0.01 : 1.0/512.0; }
#else
float PisoRazao() { return 1.0/512.0; }
#endif
float3 ComposeControls(float3 original,float3 proxy,float3 model,float detail,float color,float guard) {
    const float3 luma=float3(.2126,.7152,.0722);
    float ol=dot(original,luma),pl=dot(proxy,luma),ml=dot(model,luma);
    float3 upgraded=original;
    if(ml>1e-5) {
        float ratio=ol<pl ? ol/max(pl,1e-6) : (ml+max(0.0,ol-pl))/ml;
        upgraded=lerp(original,HueOkLab(model*ratio,model),saturate(detail));
    }
    const float piso=PisoRazao();float lr=(dot(upgraded,luma)+piso)/(ol+piso);
    float bound=clamp(pow(max(lr,1e-6),1.0+max(detail-1.0,0.0)),1.0/guard,guard);
#ifdef NR_GRADE_CG2R
    bound=AjustarGanho(lr,detail,guard,bound);
#endif
#ifdef NR_GANHO_HISTORICO
    if(haloFator<1.0&&bound>1.0)bound=exp(log(bound)*max(haloFator,0.0));
#endif
    upgraded*=bound/max(lr,1e-6);
    float3 result=lerp(original*bound,upgraded,min(color,1.0));
    if(color>1.0)result=ClampAp1(FromOkLab(ToOkLab(max(result,0.0))*float3(1,color,color)));
    return result;
}
[numthreads(8,8,1)] void main(uint3 id:SV_DispatchThreadID) {
    if(id.x>=w || id.y>=h)return;
#ifdef NR_GANHO_HISTORICO
    ganhoLimite=LerGanhoAnterior(id.xy);
    if((gp0&4u)!=0u)haloFator=HaloFator(id.xy);
    if((gp0&130u)==130u)haloExc=HaloExcesso(id.xy);
    if((gp1&32u)!=0u&&(gp0&2u)!=0u)haloTeto=HaloTeto(id.xy);
#endif
    float4 c=outp[id.xy];float3 d=NR_RESIDUAL_AT(id.xy);float3 e;
#ifdef NR_GANHO_HISTORICO
    if((gp0&2u)!=0u) {
        const float3 cg=(tone&1) ? float3(tmf(c.r*expo),tmf(c.g*expo),tmf(c.b*expo)) : saturate(c.rgb);
        luzGuia=dot(cg,float3(.2126,.7152,.0722));luzPixel=LerLuz(id.xy);
    }
#endif
    if(tone&4) {if(flags.Load(12)!=0)d=(tone&2)?prev[id.xy].rgb:float3(0,0,0);}
    uint di=(tone>>8)&63,co=(tone>>14)&127,gu=(tone>>21)&255;
    bool bridge=(tone&0x80000000)!=0 && !(di==20 && co==20 && gu==40);
    if(!bridge) {
        if(tone&1) {float3 s=c.rgb*expo;float3 b=float3(tmf(s.r),tmf(s.g),tmf(s.b));
            e=max(s+tmi3(NR_GRADE(b+d))-float3(tmi(b.r),tmi(b.g),tmi(b.b)),0.0)/expo;
        }else e=NR_GRADE(saturate(c.rgb+d));
    }else if(all(d==float3(0,0,0))) {

        e=c.rgb;
    }else {
        float detail=min(di,40u)/20.0,color=min(co,80u)/20.0,guard=clamp(gu,20u,160u)/20.0;
        if(tone&1) {
            float3 s=c.rgb*expo,b=float3(tmf(s.r),tmf(s.g),tmf(s.b));
            float3 proxy=float3(tmi(b.r),tmi(b.g),tmi(b.b));
            float3 model=tmi3(NR_GRADE(b+d));
            e=max(ComposeControls(s,proxy,model,detail,color,guard),0.0)/expo;
        }else e=saturate(ComposeControls(c.rgb,c.rgb,NR_GRADE(saturate(c.rgb+d)),detail,color,guard));
    }
    outp[id.xy]=float4(e,c.a);
#ifdef NR_GANHO_HISTORICO
    if((gp1&1u)!=0u)ganhoAtual[id.xy]=ganhoLog;
#endif
}
#ifdef NR_LUZ

#define LUZ_RAIO int((luzBits>>8)&7u)
Texture2D<float4> luzMomentos : register(t6);
Texture2D<float> luzBase : register(t7);
Texture2D<float2> luzAB : register(t8);
Texture2D<float2> luzHistAnterior : register(t9);
RWTexture2D<float4> luzMomentosOut : register(u3);
RWTexture2D<float> luzBaseOut : register(u4);
RWTexture2D<float2> luzABOut : register(u5);
RWTexture2D<float4> luzCoefOut : register(u6);
RWTexture2D<float2> luzHistAtual : register(u7);

float LuzAlvo(float3 original,float3 proxy,float3 model,float detail) {
    const float3 luma=float3(.2126,.7152,.0722);
    float ol=dot(original,luma),pl=dot(proxy,luma),ml=dot(model,luma);
    float3 upgraded=original;
    if(ml>1e-5) {
        float ratio=ol<pl ? ol/max(pl,1e-6) : (ml+max(0.0,ol-pl))/ml;
        upgraded=lerp(original,HueOkLab(model*ratio,model),saturate(detail));
    }
    const float piso=PisoRazao();float lr=(dot(upgraded,luma)+piso)/(ol+piso);
    return (1.0+max(detail-1.0,0.0))*log(max(lr,1e-6));
}
[numthreads(8,8,1)] void LuzMomentos(uint3 id:SV_DispatchThreadID) {
    const uint qw=(w+3u)/4u,qh=(h+3u)/4u;
    if(id.x>=qw||id.y>=qh)return;
    const float3 luma=float3(.2126,.7152,.0722);
    const uint di=(tone>>8)&63,co=(tone>>14)&127,gu=(tone>>21)&255;
    const bool bridge=(tone&0x80000000)!=0 && !(di==20 && co==20 && gu==40);
    const float detail=min(di,40u)/20.0;

    const uint atual=(gp1>>8)&255u;const float zc=(luzBits&2u)!=0u ? ProfTexel(int2(id.xy),atual) : 0.0;
    const bool camada=(luzBits&2u)!=0u&&isfinite(zc);
    float4 m=float4(0.0,0.0,0.0,0.0);float sb=0.0,n=0.0;
    for(uint y=0;y<4u;++y)for(uint x=0;x<4u;++x) {
        const uint2 p=id.xy*4u+uint2(x,y);
        if(p.x>=w||p.y>=h)continue;
        const float wp=camada ? PesoProf(zc,ganhoProfundidade.Load(int4(p,atual,0))) : 1.0;
        if(!(wp>0.0))continue;
        const float3 c=outp[p].rgb,d=res[p].rgb;
        const bool temRealce=bridge&&any(d!=float3(0,0,0));
        float I,B,g=0.0;
        if(tone&1) {
            const float3 s=c*expo,b=float3(tmf(s.r),tmf(s.g),tmf(s.b)),proxy=float3(tmi(b.r),tmi(b.g),tmi(b.b));
            I=dot(b,luma);B=log(dot(proxy,luma)+1.0/512.0);
            if(temRealce)g=LuzAlvo(s,proxy,tmi3(NR_GRADE(b+d)),detail);
        }else {
            I=dot(saturate(c),luma);B=log(max(dot(c,luma),0.0)+1.0/512.0);
            if(temRealce)g=LuzAlvo(c,c,NR_GRADE(saturate(c+d)),detail);
        }
        if(!isfinite(g))g=0.0;
        if(camada){m+=wp*float4(I,g,I*I,I*g);sb+=wp*B;n+=wp;}
        else {m+=float4(I,g,I*I,I*g);sb+=B;n+=1.0;}
    }
    luzMomentosOut[id.xy]=m/n;luzBaseOut[id.xy]=sb/n;
}
[numthreads(8,8,1)] void LuzCoeficientes(uint3 id:SV_DispatchThreadID) {
    const int2 q=int2((w+3u)/4u,(h+3u)/4u);
    if(any(int2(id.xy)>=q))return;
    const uint atual=(gp1>>8)&255u;const float zc=(luzBits&2u)!=0u ? ProfTexel(int2(id.xy),atual) : 0.0;
    const bool camada=(luzBits&2u)!=0u&&isfinite(zc);
    float4 m=float4(0.0,0.0,0.0,0.0);float n=0.0;
    for(int y=-LUZ_RAIO;y<=LUZ_RAIO;++y)for(int x=-LUZ_RAIO;x<=LUZ_RAIO;++x) {
        const int2 p=int2(id.xy)+int2(x,y);
        if(any(p<0)||any(p>=q))continue;
        if(camada){const float wp=PesoProf(zc,ProfTexel(p,atual));m+=wp*luzMomentos.Load(int3(p,0));n+=wp;}
        else {m+=luzMomentos.Load(int3(p,0));n+=1.0;}
    }
    m/=n;
    const float var=max(m.z-m.x*m.x,0.0),cov=m.w-m.x*m.y,a=cov/(var+luzEps);
    luzABOut[id.xy]=float2(a,m.y-a*m.x);
}

bool LerLuzAnterior(uint2 q,bool camada,out float2 anterior) {
    anterior=float2(0.0,0.0);
    if((gp1&2u)==0u)return false;
    const uint atual=(gp1>>8)&255u,fatia=(gp1>>16)&255u;
    const uint2 p=min(q*4u+2u,uint2(w-1,h-1));
    const float2 uv=(float2(q)*4.0+2.0)/float2(w,h);
    const float2 mv=(gp1&4u)!=0u ? ganhoMovimento.Load(int4(p,atual,0)) : ganhoMovimento.SampleLevel(ganhoLinear,float3(uv,atual),0);
    const float2 uvp=uv+mv;
    if(!all(isfinite(uvp))||any(uvp<0.0)||any(uvp>1.0))return false;
    const float a=ganhoProfundidade.Load(int4(p,atual,0));
    const float b=ganhoProfundidade.Load(int4(min(uint2(uvp*float2(w,h)),uint2(w-1,h-1)),fatia,0));
    const float tol=max(ganhoProfAbs,ganhoProfRel*max(abs(a),abs(b)));
    if(!isfinite(a)||!isfinite(b)||!(tol>0.0)||abs(a-b)>tol)return false;
    const int2 m=int2((w+3u)/4u,(h+3u)/4u)-1;
    const float2 s=clamp(uvp*float2(w,h)/4.0-0.5,float2(0.0,0.0),float2(m));
    const int2 i=min(int2(floor(s)),m);const float2 f=s-float2(i);const int2 e=min(i+1,m);
    if(camada) {
        const float4 wt=float4((1.0-f.x)*(1.0-f.y),f.x*(1.0-f.y),(1.0-f.x)*f.y,f.x*f.y)*
                        float4(PesoProf(a,ProfTexel(i,fatia)),PesoProf(a,ProfTexel(int2(e.x,i.y),fatia)),
                               PesoProf(a,ProfTexel(int2(i.x,e.y),fatia)),PesoProf(a,ProfTexel(e,fatia)));
        const float soma=wt.x+wt.y+wt.z+wt.w;
        if(!(soma>1e-4))return false;
        anterior=(luzHistAnterior.Load(int3(i,0))*wt.x+luzHistAnterior.Load(int3(e.x,i.y,0))*wt.y+
                  luzHistAnterior.Load(int3(i.x,e.y,0))*wt.z+luzHistAnterior.Load(int3(e,0))*wt.w)/soma;
        return all(isfinite(anterior));
    }
    anterior=lerp(lerp(luzHistAnterior.Load(int3(i,0)),luzHistAnterior.Load(int3(e.x,i.y,0)),f.x),
                  lerp(luzHistAnterior.Load(int3(i.x,e.y,0)),luzHistAnterior.Load(int3(e,0)),f.x),f.y);
    return all(isfinite(anterior));
}
[numthreads(8,8,1)] void LuzTemporal(uint3 id:SV_DispatchThreadID) {
    const int2 q=int2((w+3u)/4u,(h+3u)/4u);
    if(any(int2(id.xy)>=q))return;
    const uint atual=(gp1>>8)&255u;const float zc=(luzBits&2u)!=0u ? ProfTexel(int2(id.xy),atual) : 0.0;
    const bool camada=(luzBits&2u)!=0u&&isfinite(zc);
    float2 ab=float2(0.0,0.0);float n=0.0;
    for(int y=-LUZ_RAIO;y<=LUZ_RAIO;++y)for(int x=-LUZ_RAIO;x<=LUZ_RAIO;++x) {
        const int2 p=int2(id.xy)+int2(x,y);
        if(any(p<0)||any(p>=q))continue;
        if(camada){const float wp=PesoProf(zc,ProfTexel(p,atual));ab+=wp*luzAB.Load(int3(p,0));n+=wp;}
        else {ab+=luzAB.Load(int3(p,0));n+=1.0;}
    }
    ab/=n;
    const float novo=ab.x*luzMomentos.Load(int3(id.xy,0)).x+ab.y,base=luzBase.Load(int3(id.xy,0));
    float filtrado=novo;
    float2 anterior;
    if((luzBits&1u)!=0u&&LerLuzAnterior(id.xy,camada,anterior)) {
        const float lambda=saturate(abs(base-anterior.y)/max(luzDelta,1e-6));
        const float alfa=lerp(luzAlfa0,1.0,lambda);
        filtrado=lambda>=1.0 ? novo : anterior.x+alfa*(novo-anterior.x);
    }
    luzCoefOut[id.xy]=float4(ab.x,ab.y,filtrado-novo,camada ? zc : 0.0);
    if((luzBits&1u)!=0u)luzHistAtual[id.xy]=float2(filtrado,base);
}

RWTexture2D<float4> luzHaloOut : register(u8);
#define HALO_RAIO float((luzBits>>16)&63u)
#define HALO_FORCA (float((luzBits>>24)&255u)/64.0)
float2 HaloGradTexel(int2 q,uint fatia,float z0) {
    const int2 m=int2((w+3u)/4u,(h+3u)/4u)-1;
    const float xp=ProfTexel(min(q+int2(1,0),m),fatia),xm=ProfTexel(max(q-int2(1,0),int2(0,0)),fatia);
    const float yp=ProfTexel(min(q+int2(0,1),m),fatia),ym=ProfTexel(max(q-int2(0,1),int2(0,0)),fatia);
    const float ax=xp-z0,bx=z0-xm,ay=yp-z0,by=z0-ym;
    float gx=q.x<=0 ? ax : q.x>=m.x ? bx : (abs(ax)<abs(bx) ? ax : bx),gy=q.y<=0 ? ay : q.y>=m.y ? by : (abs(ay)<abs(by) ? ay : by);
    if(!isfinite(gx))gx=0.0;
    if(!isfinite(gy))gy=0.0;
    return float2(gx,gy);
}

float PesoCamada(float zc,float2 g,float2 o,float zt) {
    const float zp=zc+dot(g,o);
    if(!isfinite(zt)||!isfinite(zp))return 0.0;
    return 1.0-smoothstep(0.02,0.08,abs(zt-zp)/max(max(abs(zt),abs(zp)),1e-9));
}
[numthreads(8,8,1)] void LuzProx(uint3 id:SV_DispatchThreadID) {
    const int2 qn=int2((w+3u)/4u,(h+3u)/4u);
    if(any(int2(id.xy)>=qn))return;
    const uint atual=(gp1>>8)&255u;
    const float Rt=HALO_RAIO/4.0,zc=ProfTexel(int2(id.xy),atual);
    float f=0.0;
    if(Rt>0.0&&isfinite(zc)) {
        const float2 g=HaloGradTexel(int2(id.xy),atual,zc);
        const int2 d8[8]={int2(1,0),int2(-1,0),int2(0,1),int2(0,-1),int2(1,1),int2(-1,1),int2(1,-1),int2(-1,-1)};
        [unroll] for(int k=0;k<8;++k) {
            const float passo=k<4 ? 1.0 : 1.41421356;
            [loop] for(int r=1;float(r)*passo<=Rt+0.5;++r) {
                const int2 o=d8[k]*r,q=int2(id.xy)+o;
                if(any(q<0)||any(q>=qn))break;
                const float zq=ProfTexel(q,atual);
                if(!isfinite(zq))continue;
                const float zp=zc+dot(g,float2(o));
                const float s=(zq-zp)/max(max(abs(zq),abs(zp)),1e-9);
                f=max(f,smoothstep(0.02,0.08,s)*saturate((Rt+1.0-float(r)*passo)/max(Rt,1e-4)));
            }
        }
    }
    ganhoAtual[id.xy]=f;
}

float4 HaloTetoTexel(int2 q,uint atual,float zc) {
    const int2 qn=int2((w+3u)/4u,(h+3u)/4u);
    if(!isfinite(zc)||!(HALO_RAIO>0.0))return float4(0.0,0.0,0.0,0.0);
    const float3 luma=float3(.2126,.7152,.0722);
    const uint di=(tone>>8)&63,co=(tone>>14)&127,gu=(tone>>21)&255;
    const bool bridge=(tone&0x80000000)!=0 && !(di==20 && co==20 && gu==40);
    const float detail=min(di,40u)/20.0;
    float sg=0.0,sw=0.0;
    for(uint y=0;y<4u;++y)for(uint x=0;x<4u;++x) {
        const uint2 p=uint2(q)*4u+uint2(x,y);
        if(p.x>=w||p.y>=h)continue;
        const float wp=PesoProf(zc,ganhoProfundidade.Load(int4(p,atual,0)));
        if(!(wp>0.0))continue;
        const float3 c=outp[p].rgb,d=res[p].rgb;
        float g=0.0;
        if(bridge&&any(d!=float3(0,0,0))) {
            if(tone&1) {
                const float3 s=c*expo,b=float3(tmf(s.r),tmf(s.g),tmf(s.b)),proxy=float3(tmi(b.r),tmi(b.g),tmi(b.b));
                g=LuzAlvo(s,proxy,tmi3(NR_GRADE(b+d)),detail);
            }else g=LuzAlvo(c,c,NR_GRADE(saturate(c+d)),detail);
        }
        if(!isfinite(g))g=0.0;
        sg+=wp*g;sw+=wp;
    }
    const float proprio=sw>0.0 ? sg/sw : luzMomentos.Load(int3(q,0)).y;
    const float2 g2=HaloGradTexel(q,atual,zc);
    const float2 D8[8]={float2(1,0),float2(-1,0),float2(0,1),float2(0,-1),float2(0.70710678,0.70710678),float2(-0.70710678,0.70710678),
                        float2(0.70710678,-0.70710678),float2(-0.70710678,-0.70710678)};
    float v[32],pv[32];float W=0.0;
    [unroll] for(int k=0;k<8;++k) {
        [unroll] for(int j=0;j<4;++j) {
            const int n=k*4+j;v[n]=0.0;pv[n]=0.0;
            const int2 t=q+int2(round(D8[k]*((1.5+0.5*float(j))*HALO_RAIO/4.0)));
            if(any(t<0)||any(t>=qn))continue;
            const float wc=PesoCamada(zc,g2,float2(t-q),ProfTexel(t,atual));
            const float wl=wc*(1.0-saturate(ganhoAnterior.Load(int3(t,0))));
            const float vt=luzMomentos.Load(int3(t,0)).y;
            if(wl>0.0&&isfinite(vt)){v[n]=vt;pv[n]=wl;W+=wl;}
        }
    }
    if(!(W>1e-4))return float4(0.0,proprio,0.0,0.0);
    float acc=0.0,accW=0.0;
    [loop] for(int i=0;i<32;++i) {
        if(!(pv[i]>0.0))continue;
        float abaixo=0.0;
        [unroll] for(int j2=0;j2<32;++j2)abaixo+=pv[j2]*(v[j2]<v[i] ? 1.0 : (v[j2]==v[i] ? 0.5 : 0.0));
        const float posto=abaixo/W;
        const float wi=pv[i]*smoothstep(0.1,0.3,posto)*(1.0-smoothstep(0.7,0.9,posto));
        acc+=wi*v[i];accW+=wi;
    }
    const float ref=accW>1e-6 ? acc/accW : proprio;
    return float4(isfinite(ref) ? ref : proprio,proprio,accW>1e-6 ? saturate(W/4.0) : 0.0,0.0);
}
[numthreads(8,8,1)] void LuzHalo(uint3 id:SV_DispatchThreadID) {
    const int2 qn=int2((w+3u)/4u,(h+3u)/4u);
    if(any(int2(id.xy)>=qn))return;
    const uint atual=(gp1>>8)&255u;
    const int2 q=int2(id.xy);
    const float Lq=luzCoef.Load(int3(q,0)).x*luzMomentos.Load(int3(q,0)).x+luzCoef.Load(int3(q,0)).y;
    const float zc=ProfTexel(q,atual);
    const uint2 pc=min(uint2(q)*4u+2u,uint2(w-1,h-1));
    const float2 mvCentro=ganhoMovimento.Load(int4(pc,atual,0));
    if((luzBits&8u)!=0u){luzHaloOut[q]=HaloTetoTexel(q,atual,zc);return;}
    if(!isfinite(zc)||!(HALO_RAIO>0.0)){luzHaloOut[q]=float4(Lq,0.0,all(isfinite(mvCentro))?mvCentro:float2(0.0,0.0));return;}
    const float2 g=HaloGradTexel(q,atual,zc);

    const bool corteRef=(luzBits&4u)!=0u;
    float2 fora=float2(0.0,0.0);
    if(corteRef) {

        const float fq=ganhoAnterior.Load(int3(q,0));
        const int2 qxp=min(q+int2(1,0),qn-1),qxm=max(q-int2(1,0),int2(0,0)),qyp=min(q+int2(0,1),qn-1),qym=max(q-int2(0,1),int2(0,0));
        const float fxp=lerp(fq,ganhoAnterior.Load(int3(qxp,0)),PesoCamada(zc,g,float2(qxp-q),ProfTexel(qxp,atual)));
        const float fxm=lerp(fq,ganhoAnterior.Load(int3(qxm,0)),PesoCamada(zc,g,float2(qxm-q),ProfTexel(qxm,atual)));
        const float fyp=lerp(fq,ganhoAnterior.Load(int3(qyp,0)),PesoCamada(zc,g,float2(qyp-q),ProfTexel(qyp,atual)));
        const float fym=lerp(fq,ganhoAnterior.Load(int3(qym,0)),PesoCamada(zc,g,float2(qym-q),ProfTexel(qym,atual)));
        const float2 gf=float2(fxp-fxm,fyp-fym);
        if(all(isfinite(gf))&&length(gf)>1e-3)fora=-normalize(gf);
    }
    const float2 D8[8]={float2(1,0),float2(-1,0),float2(0,1),float2(0,-1),float2(0.70710678,0.70710678),float2(-0.70710678,0.70710678),
                        float2(0.70710678,-0.70710678),float2(-0.70710678,-0.70710678)};
    float med[8],wd[8];float media=0.0,ws=0.0,wmv=0.0;float2 mv=float2(0.0,0.0);
    [unroll] for(int k=0;k<8;++k) {
        med[k]=0.0;wd[k]=0.0;
        [unroll] for(int j=0;j<2;++j) {
            const float2 o=D8[k]*((j==0 ? 1.5 : 3.0)*HALO_RAIO/4.0);
            const int2 t=q+int2(round(o));
            if(any(t<0)||any(t>=qn))continue;
            const float wc=PesoCamada(zc,g,float2(t-q),ProfTexel(t,atual));
            if(!(wc>0.0))continue;
            const float4 cf=luzCoef.Load(int3(t,0));
            const float Lt=cf.x*luzMomentos.Load(int3(t,0)).x+cf.y;
            const float wl=wc*(1.0-saturate(ganhoAnterior.Load(int3(t,0))));
            if(wl>0.0&&isfinite(Lt)&&!(corteRef&&j==1)){med[k]+=wl*Lt;wd[k]+=wl;}
            const float2 mt=ganhoMovimento.Load(int4(min(uint2(t)*4u+2u,uint2(w-1,h-1)),atual,0));
            if(all(isfinite(mt))){mv+=wc*mt;wmv+=wc;}
        }
        media+=med[k];ws+=wd[k];
    }
    media/=max(ws,1e-4);

    float mx=-1e30,cs=0.0;
    [unroll] for(int k2=0;k2<8;++k2) {
        const float mk=wd[k2]>1e-4 ? med[k2]/wd[k2] : media;
        float c2=saturate(wd[k2]/2.0);
        if(corteRef&&any(fora!=0.0))c2*=smoothstep(0.2,0.7,dot(D8[k2],fora));
        if(c2>0.0)mx=max(mx,mk);
    }
    float acc=0.0,accMedia=0.0;
    [unroll] for(int k3=0;k3<8;++k3) {
        float c=saturate(wd[k3]/2.0);const float mk=wd[k3]>1e-4 ? med[k3]/wd[k3] : media;
        if(corteRef&&any(fora!=0.0))c*=smoothstep(0.2,0.7,dot(D8[k3],fora));
        if(c>0.0){acc+=c*exp((mk-mx)/0.03);cs+=c;accMedia+=c*mk;}
    }
    const float ref=cs>1e-4 ? (corteRef ? accMedia/cs : mx+0.03*log(max(acc,1e-30)/cs)) : Lq;
    const float conf=cs>1e-4 ? saturate(ws/3.0) : 0.0;
    const float2 mvFundo=wmv>1e-4 ? mv/wmv : (all(isfinite(mvCentro)) ? mvCentro : float2(0.0,0.0));
    luzHaloOut[q]=float4(isfinite(ref) ? ref : Lq,HALO_FORCA*conf,mvFundo);
}
#endif
