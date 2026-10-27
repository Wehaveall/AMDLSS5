#ifndef NR_MASC_PASSO
#define NR_MASC_PASSO 0
#endif
#define MC_S 8
cbuffer K : register(b0) {
    uint largura, altura;
    uint celW, celH;
    uint mLarg, mAlt, mPitch, mBits;

    uint fatia;
    float r0, r1;
    uint kd, kf;
    float lnPerto;
    float lnFrente;
    float expoTom;
    uint suave;
};

#if NR_MASC_PASSO == 1 || NR_MASC_PASSO == 4 || NR_MASC_PASSO == 7 || NR_MASC_PASSO == 9
ByteAddressBuffer mascara : register(t1);
uint MByte(int2 q) {
    q = clamp(q, int2(0, 0), int2(int(mLarg) - 1, int(mAlt) - 1));
    const uint a = uint(q.y) * mPitch + uint(q.x);
    return (mascara.Load(a & ~3u) >> ((a & 3u) * 8u)) & 255u;
}
bool MBit(int2 q) { return (MByte(q) & (mBits & 255u)) != 0u; }
bool Cru(float2 uv) { return MBit(int2(floor(uv * float2(mLarg, mAlt)))); }
bool Nucleo(float2 uv) {
    if ((mBits & 256u) != 0u) return Cru(uv);
    const int2 b = int2(floor(uv * float2(mLarg, mAlt) - 0.5));
    return MBit(b) && MBit(b + int2(1, 0)) && MBit(b + int2(0, 1)) && MBit(b + int2(1, 1));
}
#endif

#if NR_MASC_PASSO == 1
Texture2D<float4> realce : register(t0);
Texture2DArray<float> profundidade : register(t2);
RWTexture2D<float4> celula : register(u0);
RWTexture2D<float4> aux : register(u1);
groupshared float4 gsD[64];
groupshared float2 gsZ[64];
groupshared float2 gsM[64];
[numthreads(MC_S, MC_S, 1)]
void MascaraCelulas(uint3 g : SV_GroupID, uint3 t : SV_GroupThreadID, uint gi : SV_GroupIndex) {
    const uint2 p = g.xy * MC_S + t.xy;
    float4 d = float4(0.0, 0.0, 0.0, 0.0);
    float2 z2 = float2(0.0, 0.0);
    float2 mm = float2(1e9, -1e9);
    if (p.x < largura && p.y < altura) {
        const float2 uv = (float2(p) + 0.5) / float2(largura, altura);
        const bool cru = Cru(uv);
        const float z = profundidade.Load(int4(p, fatia, 0));
        const float3 v = realce.Load(int3(p, 0)).rgb;
        if (!cru && z > 0.0 && isfinite(z) && all(isfinite(v))) { d = float4(v, 1.0); z2.x = log(z); }
        if (cru) z2.y = 1.0;
        if (z > 0.0 && isfinite(z)) { const float lz = log(z); mm.y = lz; if (!cru) mm.x = lz; }
    }
    gsD[gi] = d; gsZ[gi] = z2; gsM[gi] = mm;
    GroupMemoryBarrierWithGroupSync();
    [unroll] for (uint s = 32u; s > 0u; s >>= 1) {
        if (gi < s) { gsD[gi] += gsD[gi + s]; gsZ[gi] += gsZ[gi + s]; gsM[gi] = float2(min(gsM[gi].x, gsM[gi + s].x), max(gsM[gi].y, gsM[gi + s].y)); }
        GroupMemoryBarrierWithGroupSync();
    }
    if (gi != 0u) return;
    const float n = gsD[0].w;
    celula[g.xy] = float4(gsD[0].rgb / max(n, 1.0), n);
    aux[g.xy] = float4(n > 0.0 ? gsZ[0].x / n : 0.0, gsZ[0].y, gsM[0].x < 1e8 ? gsM[0].x : 0.0, gsM[0].y > -1e8 ? gsM[0].y : -99.0);
}
#endif

#if NR_MASC_PASSO == 2
Texture2D<float4> aux : register(t0);
RWTexture2D<float> distancia : register(u0);
[numthreads(8, 8, 1)]
void MascaraDistancia(uint3 id : SV_DispatchThreadID) {
    if (id.x >= celW || id.y >= celH) return;
    float dist = 1e9;
    const int k = int(kd);
    [loop] for (int dy = -k; dy <= k; ++dy) {
        const int y = int(id.y) + dy;
        if (y < 0 || y >= int(celH)) continue;
        [loop] for (int dx = -k; dx <= k; ++dx) {
            const int x = int(id.x) + dx;
            if (x < 0 || x >= int(celW)) continue;
            if (!(aux.Load(int3(x, y, 0)).y > 0.0)) continue;
            dist = min(dist, max(sqrt(float(dx * dx + dy * dy)) * MC_S - 0.5 * MC_S, 0.0));
        }
    }
    distancia[id.xy] = min(dist, 1e4);
}
#endif

#if NR_MASC_PASSO == 3
Texture2D<float4> celula : register(t0);
Texture2D<float4> aux : register(t1);
Texture2D<float> distancia : register(t2);
RWTexture2D<float4> preenche : register(u0);
[numthreads(8, 8, 1)]
void MascaraPreenche(uint3 id : SV_DispatchThreadID) {
    if (id.x >= celW || id.y >= celH) return;

    if (!(distancia.Load(int3(id.xy, 0)) < r1 + 12.0)) { preenche[id.xy] = float4(0.0, 0.0, 0.0, 0.0); return; }
    const float nc = celula.Load(int3(id.xy, 0)).w, zc = aux.Load(int3(id.xy, 0)).x;
    float3 acc = float3(0.0, 0.0, 0.0);
    float ws = 0.0, conf = 0.0;
    const int k = int(kf);
    const float minimo = MC_S * MC_S / 4.0;
    [loop] for (int dy = -k; dy <= k; ++dy) {
        const int y = int(id.y) + dy;
        if (y < 0 || y >= int(celH)) continue;
        [loop] for (int dx = -k; dx <= k; ++dx) {
            const int x = int(id.x) + dx;
            if (x < 0 || x >= int(celW)) continue;
            const float r = sqrt(float(dx * dx + dy * dy));
            if (r > float(k) || r == 0.0) continue;
            const float4 q = celula.Load(int3(x, y, 0));
            if (!(q.w >= minimo) || !(distancia.Load(int3(x, y, 0)) >= r1)) continue;

            const float dz = abs(aux.Load(int3(x, y, 0)).x - zc);
            const float g = nc > 0.0 ? (suave ? saturate(2.0 - dz / lnPerto) : (dz < lnPerto ? 1.0 : 0.0)) : 1.0;
            if (!(g > 0.0)) continue;
            const float rs = r * MC_S;
            const float w = q.w * g / (rs * rs * rs);
            acc += w * q.rgb; ws += w; conf = max(conf, g);
        }
    }
    preenche[id.xy] = ws > 0.0 ? float4(acc / ws, suave ? conf : 1.0) : float4(0.0, 0.0, 0.0, 0.0);
}
#endif

#if NR_MASC_PASSO == 4
Texture2D<float4> celula : register(t0);
Texture2D<float4> aux : register(t2);
Texture2D<float> distancia : register(t3);
Texture2D<float4> preenche : register(t4);
Texture2DArray<float> profundidade : register(t5);
RWTexture2D<float4> realce : register(u0);
[numthreads(8, 8, 1)]
void MascaraAplica(uint3 id : SV_DispatchThreadID) {
    if (id.x >= largura || id.y >= altura) return;
    if (!(distancia.Load(int3(min(id.xy / MC_S, uint2(celW - 1u, celH - 1u)), 0)) < r1 + 12.0)) return;
    const float2 uv = (float2(id.xy) + 0.5) / float2(largura, altura);
    if (Nucleo(uv)) return;
    const float z = profundidade.Load(int4(id.xy, fatia, 0));
    if (!(z > 0.0) || !isfinite(z)) return;
    const float2 c = (float2(id.xy) + 0.5) / MC_S - 0.5;
    const float2 fc = floor(c);
    const int2 m = int2(int(celW) - 1, int(celH) - 1);
    const int2 c0 = clamp(int2(fc), int2(0, 0), m), c1 = clamp(int2(fc) + 1, int2(0, 0), m);
    const float2 f = saturate(c - fc);
    float3 sf = float3(0.0, 0.0, 0.0);
    float sw = 0.0, sd = 0.0, sz = 0.0, szw = 0.0;
    [unroll] for (int j = 0; j < 2; ++j) [unroll] for (int i = 0; i < 2; ++i) {
        const int2 q = int2(i ? c1.x : c0.x, j ? c1.y : c0.y);
        const float wq = (i ? f.x : 1.0 - f.x) * (j ? f.y : 1.0 - f.y);
        const float4 pr = preenche.Load(int3(q, 0));
        if (pr.w > 0.0) { sf += wq * pr.w * pr.rgb; sw += wq * pr.w; }
        sd += wq * distancia.Load(int3(q, 0));
        if (celula.Load(int3(q, 0)).w > 0.0) { sz += wq * aux.Load(int3(q, 0)).x; szw += wq; }
    }
    if (!(sw > 0.0) || !(szw > 0.0)) return;

    const float peso = (1.0 - smoothstep(r0, r1, sd)) * (suave ? saturate(sw) : 1.0);
    if (!(peso > 0.0)) return;
    if (!(abs(log(z) - sz / szw) < lnPerto) && !Cru(uv)) return;
    const float4 v = realce[id.xy];
    if (!all(isfinite(v.rgb))) return;
    const float3 fp = sf / sw;
    realce[id.xy] = float4(v.rgb + peso * (fp - v.rgb), v.a);
}
#endif

#define MC_NB 20
#define MC_PARCIAL 120
#if NR_MASC_PASSO == 7 || NR_MASC_PASSO == 9

float DistBil(uint2 p, Texture2D<float> dz, Texture2D<float4> ax, float lz) {
    const float2 c = (float2(p) + 0.5) / MC_S - 0.5;
    const float2 fc = floor(c);
    const int2 m = int2(int(celW) - 1, int(celH) - 1);
    const int2 c0 = clamp(int2(fc), int2(0, 0), m), c1 = clamp(int2(fc) + 1, int2(0, 0), m);
    const float2 f = saturate(c - fc);
    float s = 0.0, w = 0.0;
    [unroll] for (int j = 0; j < 2; ++j) [unroll] for (int i = 0; i < 2; ++i) {
        const int2 q = int2(i ? c1.x : c0.x, j ? c1.y : c0.y);
        const float wq = (i ? f.x : 1.0 - f.x) * (j ? f.y : 1.0 - f.y) * (abs(lz - ax.Load(int3(q, 0)).z) < lnFrente ? 1.0 : 0.0);
        s += wq * dz.Load(int3(q, 0)); w += wq;
    }
    return w > 1e-6 ? s / w : dz.Load(int3(p / MC_S, 0));
}
float TomBase(float4 c) { const float v = max(dot(max(c.rgb, 0.0), float3(0.2126, 0.7152, 0.0722)) * expoTom, 0.0); const float r = v / (1.0 + v);
    return r <= 0.0031308 ? r * 12.92 : 1.055 * pow(r, 1.0 / 2.4) - 0.055; }
#endif

#if NR_MASC_PASSO == 6
Texture2D<float4> aux : register(t0);
RWTexture2D<float> distz : register(u0);
[numthreads(8, 8, 1)]
void MascaraDistZ(uint3 id : SV_DispatchThreadID) {
    if (id.x >= celW || id.y >= celH) return;
    const float zc = aux.Load(int3(id.xy, 0)).z;
    float dist = 1e9;
    const int k = int(kd);
    [loop] for (int dy = -k; dy <= k; ++dy) {
        const int y = int(id.y) + dy;
        if (y < 0 || y >= int(celH)) continue;
        [loop] for (int dx = -k; dx <= k; ++dx) {
            const int x = int(id.x) + dx;
            if (x < 0 || x >= int(celW)) continue;
            const float4 q = aux.Load(int3(x, y, 0));
            if (!(q.y > 0.0) && !(q.w > zc + lnFrente)) continue;
            dist = min(dist, max(sqrt(float(dx * dx + dy * dy)) * MC_S - 0.5 * MC_S, 0.0));
        }
    }
    distz[id.xy] = min(dist, 1e4);
}
#endif

#if NR_MASC_PASSO == 7
Texture2D<float4> aux : register(t0);
Texture2DArray<float> profundidade : register(t2);
Texture2D<float> distz : register(t3);
Texture2DArray<float4> cor : register(t4);
RWByteAddressBuffer parcial : register(u0);
RWTexture2D<float4> realce : register(u1);

groupshared int gsI[MC_PARCIAL];
[numthreads(16, 16, 1)]
void MascaraCurva(uint3 g : SV_GroupID, uint3 t : SV_GroupThreadID, uint gi : SV_GroupIndex) {
    const uint2 p = g.xy * 16u + t.xy;
    float4 c = float4(-1.0, 0.0, 0.0, 0.0); float4 dd = float4(0.0, 0.0, 0.0, 0.0);
    if (p.x < largura && p.y < altura) {
        const float2 uv = (float2(p) + 0.5) / float2(largura, altura);
        const float z = profundidade.Load(int4(p, fatia, 0));
        if (z > 0.0 && isfinite(z) && !Cru(uv)) {
            const float lz = log(z);
            const float zmin = aux.Load(int3(p / MC_S, 0)).z;
            if (lz < zmin + lnFrente && DistBil(p, distz, aux, lz) >= r1) {
                const float3 d = realce[p].rgb;
                const float b = TomBase(cor.Load(int4(p, fatia, 0)));
                if (all(isfinite(d)) && isfinite(b)) { c = float4(float(clamp(int(b * MC_NB), 0, MC_NB - 1)), lz, 0.0, 0.0); dd = float4(d, 0.0); }
            }
        }
    }
    if (gi < MC_PARCIAL) gsI[gi] = 0;
    GroupMemoryBarrierWithGroupSync();
    if (c.x >= 0.0) {
        const uint o = uint(c.x) * 6u;
        InterlockedAdd(gsI[o], 1);
        InterlockedAdd(gsI[o + 1u], int(round(dd.r * 1048576.0))); InterlockedAdd(gsI[o + 2u], int(round(dd.g * 1048576.0)));
        InterlockedAdd(gsI[o + 3u], int(round(dd.b * 1048576.0)));
        InterlockedAdd(gsI[o + 4u], int(round(c.y * 65536.0))); InterlockedAdd(gsI[o + 5u], int(round(c.y * c.y * 4096.0)));
    }
    GroupMemoryBarrierWithGroupSync();
    if (gi >= MC_PARCIAL) return;
    const uint campo = gi % 6u;
    const float esc = campo == 0u ? 1.0 : campo <= 3u ? 1.0 / 1048576.0 : campo == 4u ? 1.0 / 65536.0 : 1.0 / 4096.0;
    parcial.Store(((g.y * ((largura + 15u) / 16u) + g.x) * MC_PARCIAL + gi) * 4u, asuint(float(gsI[gi]) * esc));
}
#endif

#define MC_BLOCO 64
#if NR_MASC_PASSO == 10

RWByteAddressBuffer parcial : register(u0);
[numthreads(128, 1, 1)]
void MascaraCurvaSoma(uint3 g : SV_GroupID, uint gi : SV_GroupIndex) {
    if (gi >= MC_PARCIAL) return;
    const uint ng = ((largura + 15u) / 16u) * ((altura + 15u) / 16u);
    const uint ini = g.x * MC_BLOCO, fim = min(ini + MC_BLOCO, ng);
    float s = 0.0;
    [loop] for (uint k = ini; k < fim; ++k) s += asfloat(parcial.Load((k * MC_PARCIAL + gi) * 4u));
    parcial.Store(((ng + g.x) * MC_PARCIAL + gi) * 4u, asuint(s));
}
#endif
#if NR_MASC_PASSO == 8
RWByteAddressBuffer parcial : register(u0);
RWByteAddressBuffer curva : register(u1);
groupshared float gsS[MC_PARCIAL];
[numthreads(128, 1, 1)]
void MascaraCurvaFim(uint gi : SV_GroupIndex) {
    const uint ng = ((largura + 15u) / 16u) * ((altura + 15u) / 16u);
    const uint nb = (ng + MC_BLOCO - 1u) / MC_BLOCO;
    if (gi < MC_PARCIAL) {
        float s = 0.0;
        [loop] for (uint g = 0u; g < nb; ++g) s += asfloat(parcial.Load(((ng + g) * MC_PARCIAL + gi) * 4u));
        gsS[gi] = s;
    }
    GroupMemoryBarrierWithGroupSync();
    if (gi != 0u) return;
    float n = 0.0, slz = 0.0, slz2 = 0.0;
    float4 bins[MC_NB];
    [unroll] for (uint j = 0u; j < MC_NB; ++j) {
        const float nj = gsS[j * 6u];
        n += nj; slz += gsS[j * 6u + 4u]; slz2 += gsS[j * 6u + 5u];
        bins[j] = nj > 200.0 ? float4(gsS[j * 6u + 1u] / nj, gsS[j * 6u + 2u] / nj, gsS[j * 6u + 3u] / nj, 1.0) : float4(0.0, 0.0, 0.0, 0.0);
    }

    bool alguma = false;
    [unroll] for (uint a = 0u; a < MC_NB; ++a) alguma = alguma || bins[a].w > 0.0;
    float4 saida[MC_NB];
    [unroll] for (uint j2 = 0u; j2 < MC_NB; ++j2) {
        if (bins[j2].w > 0.0) { saida[j2] = bins[j2]; continue; }
        int esq = -1, dir = -1;
        [unroll] for (int k = 0; k < MC_NB; ++k) { if (bins[k].w > 0.0) { if (k < int(j2)) esq = k; if (k > int(j2) && dir < 0) dir = k; } }
        float3 v = float3(0.0, 0.0, 0.0);
        if (esq >= 0 && dir >= 0) v = lerp(bins[esq].rgb, bins[dir].rgb, float(int(j2) - esq) / float(dir - esq));
        else if (esq >= 0) v = bins[esq].rgb; else if (dir >= 0) v = bins[dir].rgb;
        saida[j2] = float4(v, alguma ? 1.0 : 0.0);
    }
    [unroll] for (uint j3 = 0u; j3 < MC_NB; ++j3) {
        curva.Store(j3 * 16u, asuint(saida[j3].r)); curva.Store(j3 * 16u + 4u, asuint(saida[j3].g));
        curva.Store(j3 * 16u + 8u, asuint(saida[j3].b)); curva.Store(j3 * 16u + 12u, asuint(saida[j3].w));
    }
    const float media = n > 0.0 ? slz / n : 0.0;
    const float desvio = n > 0.0 ? sqrt(max(slz2 / n - media * media, 0.0)) : 0.0;
    curva.Store(MC_NB * 16u, asuint(media + 1.2816 * desvio));
    curva.Store(MC_NB * 16u + 4u, asuint(alguma && n > 0.0 ? 1.0 : 0.0));
}
#endif

#if NR_MASC_PASSO == 9
Texture2D<float4> aux : register(t0);
Texture2DArray<float> profundidade : register(t2);
Texture2D<float> distz : register(t3);
Texture2DArray<float4> cor : register(t4);
RWTexture2D<float4> realce : register(u0);
RWByteAddressBuffer curva : register(u1);
[numthreads(8, 8, 1)]
void MascaraLimita(uint3 id : SV_DispatchThreadID) {
    if (id.x >= largura || id.y >= altura) return;
    if (!(asfloat(curva.Load(MC_NB * 16u + 4u)) > 0.5)) return;
    const float2 uv = (float2(id.xy) + 0.5) / float2(largura, altura);
    if (Nucleo(uv) || Cru(uv)) return;
    const float z = profundidade.Load(int4(id.xy, fatia, 0));
    if (!(z > 0.0) || !isfinite(z)) return;
    const float lz = log(z);
    if (!(lz < aux.Load(int3(id.xy / MC_S, 0)).z + lnFrente)) return;
    if (!(lz <= asfloat(curva.Load(MC_NB * 16u)))) return;
    const float peso = 1.0 - smoothstep(r0, r1, DistBil(id.xy, distz, aux, lz));
    if (!(peso > 0.0)) return;
    const float b = TomBase(cor.Load(int4(id.xy, fatia, 0)));
    if (!isfinite(b)) return;
    const float tt = clamp(b * MC_NB - 0.5, 0.0, float(MC_NB - 1));
    const uint i0 = uint(floor(tt)), i1 = min(i0 + 1u, uint(MC_NB - 1));
    const float f = tt - floor(tt);
    const float3 e0 = float3(asfloat(curva.Load(i0 * 16u)), asfloat(curva.Load(i0 * 16u + 4u)), asfloat(curva.Load(i0 * 16u + 8u)));
    const float3 e1 = float3(asfloat(curva.Load(i1 * 16u)), asfloat(curva.Load(i1 * 16u + 4u)), asfloat(curva.Load(i1 * 16u + 8u)));
    const float3 de = lerp(e0, e1, f);
    const float4 v = realce[id.xy];
    if (!all(isfinite(v.rgb))) return;
    realce[id.xy] = float4(v.rgb + peso * (min(v.rgb, de) - v.rgb), v.a);
}
#endif
