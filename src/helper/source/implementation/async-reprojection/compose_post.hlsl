float3 ResiduoAmpliado(uint2 p);
#define NR_RESIDUAL_AT(p) ResiduoAmpliado(p)
#ifdef NR_POST_NPC

float3 NpcAjustar(uint2 p, float3 c, float3 e);
#endif
#include "../../../../nr-controls-complete/shader/compose.hlsl"

#ifdef NR_POST_GANHO
#define NR_RA_NOME ResiduoBase
#else
#define NR_RA_NOME ResiduoAmpliado
#endif
#ifdef NR_POST_CR

float3 NR_RA_NOME(uint2 p) {
    uint rw, rh; res.GetDimensions(rw, rh);
    const float2 s = (float2(p) + 0.5) * float2(rw, rh) / float2(w, h) - 0.5;
    const int2 b = int2(floor(s)); const float2 f = s - float2(b);
    const int2 m = int2(rw, rh) - 1;
    const float2 a0 = f * (-0.5 + f * (1.0 - 0.5 * f)), a1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    const float2 a2 = f * (0.5 + f * (2.0 - 1.5 * f)), a3 = f * f * (-0.5 + 0.5 * f);
    const float wx[4] = {a0.x, a1.x, a2.x, a3.x}; const float wy[4] = {a0.y, a1.y, a2.y, a3.y};
    float3 r = float3(0, 0, 0);
    [unroll] for (int j = 0; j < 4; ++j) { float3 l = float3(0, 0, 0);
        [unroll] for (int i = 0; i < 4; ++i) l += res[clamp(b + int2(i - 1, j - 1), 0, m)].rgb * wx[i];
        r += l * wy[j]; }
    const float3 v0 = res[clamp(b, 0, m)].rgb, v1 = res[clamp(b + int2(1, 0), 0, m)].rgb;
    const float3 v2 = res[clamp(b + int2(0, 1), 0, m)].rgb, v3 = res[clamp(b + int2(1, 1), 0, m)].rgb;
    return clamp(r, min(min(v0, v1), min(v2, v3)), max(max(v0, v1), max(v2, v3)));
}
#else
float3 NR_RA_NOME(uint2 p) {
    uint rw, rh; res.GetDimensions(rw, rh);

    const float2 s = (float2(p) + 0.5) * float2(rw, rh) / float2(w, h) - 0.5;
    const int2 b = int2(floor(s)); const float2 f = s - float2(b);
    const int2 m = int2(rw, rh) - 1;
    const float3 a = res[clamp(b, 0, m)].rgb, c = res[clamp(b + int2(1, 0), 0, m)].rgb;
    const float3 d = res[clamp(b + int2(0, 1), 0, m)].rgb, e = res[clamp(b + int2(1, 1), 0, m)].rgb;
    return lerp(lerp(a, c, f.x), lerp(d, e, f.x), f.y);
}
#endif
#ifdef NR_POST_GANHO
float3 ResiduoAmpliado(uint2 p) { return ResiduoBase(p) * asfloat(gp2); }
#endif
#ifdef NR_POST_NPC

cbuffer CN : register(b1) {
    uint npcPack;

    float npcFixo, npcFmin, npcFmax, npcAlfaF, npcAlfaRef, npcFpx;
    uint npcProfAB;
    float4 npcEnt[8];
    uint4 npcZ[2];
};
Texture2DArray<float> npcProf : register(t2);
RWByteAddressBuffer npcEstado : register(u2);
static const float npcV0 = -0.95, npcV1 = 6.2, npcU1 = 1.25;
uint NpcModo() { return npcPack & 15u; }
float2 NpcPontoZ(uint k) { const uint v = npcZ[k >> 2][k & 3u]; return float2(f16tof32(v & 0xffffu), f16tof32(v >> 16)); }
float NpcForma(float u, float v) {
    u = abs(u);
    float hw = lerp(0.8, 1.25, smoothstep(0.4, 1.3, v));
    hw = lerp(hw, 1.0, smoothstep(4.0, 5.0, v));
    return (1.0 - smoothstep(hw - 0.3, hw, u)) * smoothstep(npcV0, -0.6, v) * (1.0 - smoothstep(4.6, 6.2, v));
}
float NpcCamada(float2 uv, float zh, float esc) {
    uint pw, ph, pf; npcProf.GetDimensions(pw, ph, pf);
    const float a = f16tof32(npcProfAB & 0xffffu), b = f16tof32(npcProfAB >> 16);
    const uint fat = (npcPack >> 16) & 255u;
    const float2 s = uv * float2(pw, ph) - 0.5; const int2 q = int2(floor(s)); const float2 f = s - float2(q);
    const int2 mx = int2(pw, ph) - 1;
    float acc = 0.0;
    [unroll] for (int j = 0; j < 2; ++j) [unroll] for (int i = 0; i < 2; ++i) {
        const float z = npcProf.Load(int4(clamp(q + int2(i, j), int2(0, 0), mx), fat, 0));
        float wk = 0.0;
        if (isfinite(z) && abs(z) > 0.0) wk = 1.0 - smoothstep(a, b, abs(log(abs(z) / zh)) * esc);
        acc += wk * (i ? f.x : 1.0 - f.x) * (j ? f.y : 1.0 - f.y);
    }
    return acc;
}

float NpcPeso(uint k, float2 p, float zh) {
    const float4 E = npcEnt[k];
    if ((asuint(E.w) & 1u) == 0u || !(zh > 0.0) || !(E.z > 0.0)) return 0.0;
    const float u = (p.x - E.x) / E.z, v = (p.y - E.y) / E.z;
    if (abs(u) >= npcU1 || v <= npcV0 || v >= npcV1) return 0.0;
    const float fo = NpcForma(u, v);
    if (!(fo > 0.0)) return 0.0;
    return fo * NpcCamada(p / float2(w, h), zh, npcFpx / E.z);
}
float NpcY(float3 x) { return min(dot(max(x, 0.0), float3(0.2126, 0.7152, 0.0722)), 16.0); }

static const float npcBordas[4][7] = {{-0.95, 6.2, 6.2, 6.2, 6.2, 6.2, 6.2}, {-0.95, 0.75, 6.2, 6.2, 6.2, 6.2, 6.2},
                                      {-0.95, 0.75, 2.0, 3.5, 6.2, 6.2, 6.2}, {-0.95, 0.4, 1.1, 1.9, 2.7, 3.7, 6.2}};
uint NpcZonas() { const uint z = (npcPack >> 24) & 7u; return z >= 6u ? 6u : z >= 4u ? 4u : z >= 2u ? 2u : 1u; }
uint NpcEsquema(uint z) { return z == 6u ? 3u : z == 4u ? 2u : z == 2u ? 1u : 0u; }
float NpcFv(uint b, float v) {
    const uint z = NpcZonas(), s = NpcEsquema(z);
    float cAnt = 0.5 * (npcBordas[s][0] + npcBordas[s][1]), fAnt = asfloat(npcEstado.Load(b + 40u));
    if (v <= cAnt || z == 1u) return fAnt;
    [loop] for (uint i = 1u; i < z; ++i) {
        const float c = 0.5 * (npcBordas[s][i] + npcBordas[s][i + 1u]), f = asfloat(npcEstado.Load(b + 40u + 4u * i));
        if (v <= c) return lerp(fAnt, f, (v - cAnt) / (c - cAnt));
        cAnt = c; fAnt = f;
    }
    return fAnt;
}
float3 NpcAjustar(uint2 pi, float3 c, float3 e) {
    const uint n = min((npcPack >> 12) & 15u, 8u);
    if (n < 2u) return e;
    const float2 p = float2(pi) + 0.5;
    float m = 0.0, sw = 0.0, sf = 0.0;
    [loop] for (uint k = 1u; k < n; ++k) {
        const float4 E = npcEnt[k];
        if ((asuint(E.w) & 1u) == 0u) continue;
        const float u = (p.x - E.x) / E.z, v = (p.y - E.y) / E.z;
        if (abs(u) >= npcU1 || v <= npcV0 || v >= npcV1) continue;
        const uint b = k * 128u;
        if (npcEstado.Load(b) == 0u) continue;
        const float zh = asfloat(npcEstado.Load(b + 4u));
        const float mk = NpcPeso(k, p, zh);
        if (!(mk > 0.0)) continue;
        const float fk = NpcFv(b, v);
        m = max(m, mk); sw += mk; sf += mk * fk;
    }
    const bool viz = ((npcPack >> 27) & 1u) != 0u;
    if (!(m > 0.0)) return viz ? float3(0.0, 0.0, 0.0) : e;
    const float zg = asfloat(npcEstado.Load(4u));
    if (zg > 0.0) m *= 1.0 - saturate(NpcPeso(0u, p, zg));
    if (viz) return float3(m, 0.0, 0.0);
    if (!(m > 0.0)) return e;
    const float a = 1.0 + m * (sf / sw - 1.0);
    if (a == 1.0) return e;
    return NpcModo() == 2u ? e * a : c + a * (e - c);
}
float3 NpcComposto(uint2 p, float4 c) {
    float3 d = NR_RESIDUAL_AT(p); float3 e;
    if (tone & 4) { if (flags.Load(12) != 0) d = (tone & 2) ? prev[p].rgb : float3(0, 0, 0); }
    uint di = (tone >> 8) & 63, co = (tone >> 14) & 127, gu = (tone >> 21) & 255;
    bool bridge = (tone & 0x80000000) != 0 && !(di == 20 && co == 20 && gu == 40);
    if (!bridge) {
        if (tone & 1) { float3 s = c.rgb * expo; float3 b = float3(tmf(s.r), tmf(s.g), tmf(s.b));
            e = max(s + tmi3(NR_GRADE(b + d)) - float3(tmi(b.r), tmi(b.g), tmi(b.b)), 0.0) / expo;
        } else e = NR_GRADE(saturate(c.rgb + d));
    } else if (all(d == float3(0, 0, 0))) {
        e = c.rgb;
    } else {
        float detail = min(di, 40u) / 20.0, color = min(co, 80u) / 20.0, guard = clamp(gu, 20u, 160u) / 20.0;
        if (tone & 1) {
            float3 s = c.rgb * expo, b = float3(tmf(s.r), tmf(s.g), tmf(s.b));
            float3 proxy = float3(tmi(b.r), tmi(b.g), tmi(b.b));
            float3 model = tmi3(NR_GRADE(b + d));
            e = max(ComposeControls(s, proxy, model, detail, color, guard), 0.0) / expo;
        } else e = saturate(ComposeControls(c.rgb, c.rgb, NR_GRADE(saturate(c.rgb + d)), detail, color, guard));
    }
    return e;
}
float NpcAlvo(float rref, float r) {
    if (NpcModo() == 2u) return clamp(rref / max(r, 1e-3), npcFmin, npcFmax);
    const float den = 1.0 - r, num = 1.0 - rref;
    if (abs(den) < 0.03) return 1.0;
    const float f = num / den;
    return f > 0.0 ? clamp(f, npcFmin, npcFmax) : npcFmin;
}
groupshared float npcS[18][256];
groupshared float npcZs[18];
groupshared float npcZh[2];
float NpcMediana(uint o) {
    float v[9] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; uint n = 0u;
    [unroll] for (uint i = 0u; i < 9u; ++i) { const float z = npcZs[o + i]; if (z > 0.0) { v[n] = z; ++n; } }
    if (n < 3u) return 0.0;
    [loop] for (uint a = 1u; a < n; ++a) {
        const float x = v[a]; uint b = a;
        [loop] for (uint it = 0u; it < 9u; ++it) { if (b == 0u) break; if (!(v[b - 1u] > x)) break; v[b] = v[b - 1u]; --b; }
        v[b] = x;
    }
    return v[n / 2u];
}

[numthreads(256, 1, 1)] void NpcMede(uint3 g : SV_GroupID, uint gi : SV_GroupIndex) {
    const uint k = g.x, n = min((npcPack >> 12) & 15u, 8u);
    if (k >= n) return;
    const float4 E = npcEnt[k];
    const uint bits = asuint(E.w);
    const bool on = (bits & 1u) != 0u && E.z > 0.0;
    uint pw, ph, pf; npcProf.GetDimensions(pw, ph, pf);
    const uint fat = (npcPack >> 16) & 255u;
    if (gi < 18u) {
        const uint e2 = gi < 9u ? k : 0u, j = gi < 9u ? gi : gi - 9u;
        const float4 Q = npcEnt[e2];
        float z = 0.0;
        if ((asuint(Q.w) & 1u) != 0u && Q.z > 0.0) {
            const float2 c0 = NpcPontoZ(e2) + (float2(float(j % 3u), float(j / 3u)) - 1.0) * 0.12 * Q.z;
            const float2 uv = saturate(c0 / float2(w, h));
            const float zz = npcProf.Load(int4(min(uint2(uv * float2(pw, ph)), uint2(pw - 1u, ph - 1u)), fat, 0));
            if (isfinite(zz)) z = abs(zz);
        }
        npcZs[gi] = z;
    }
    GroupMemoryBarrierWithGroupSync();
    if (gi == 0u) { npcZh[0] = NpcMediana(0u); npcZh[1] = NpcMediana(9u); }
    GroupMemoryBarrierWithGroupSync();
    const float zh = npcZh[0], zg = npcZh[1];
    const uint Z = NpcZonas(), s = NpcEsquema(Z);
    float acc[18] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    if (k >= 1u && on && zh > 0.0) {
        [loop] for (uint i = 0u; i < 4u; ++i) {
            const uint idx = gi * 4u + i;
            const float u = -npcU1 + 2.0 * npcU1 * (float(idx & 31u) + 0.5) / 32.0;
            const float v = npcV0 + (npcV1 - npcV0) * (float(idx >> 5) + 0.5) / 32.0;
            const float2 pf2 = E.xy + float2(u, v) * E.z;
            if (any(pf2 < 0.0) || pf2.x >= float(w) || pf2.y >= float(h)) continue;
            const uint2 pi = uint2(pf2);
            const float2 pc = float2(pi) + 0.5;
            const float vv = (pc.y - E.y) / E.z;
            float wt = NpcForma((pc.x - E.x) / E.z, vv);
            if (!(wt > 0.0)) continue;
            wt *= NpcCamada(pc / float2(w, h), zh, npcFpx / E.z);
            if (zg > 0.0) wt *= 1.0 - saturate(NpcPeso(0u, pc, zg));
            if (!(wt > 0.01)) continue;
            const float4 c = outp[pi];
            const float3 e = NpcComposto(pi, c);
            const float yc = NpcY(c.rgb * expo), ye = NpcY(e * expo);
            uint zi = 0u;
            [loop] for (uint q = 1u; q < Z; ++q) if (vv >= npcBordas[s][q]) zi = q;
            [unroll] for (uint q2 = 0u; q2 < 6u; ++q2) if (q2 == zi) { acc[3u * q2] += wt; acc[3u * q2 + 1u] += wt * yc; acc[3u * q2 + 2u] += wt * ye; }
        }
    }
    [unroll] for (uint a = 0u; a < 18u; ++a) npcS[a][gi] = acc[a];
    GroupMemoryBarrierWithGroupSync();
    [loop] for (uint st = 128u; st > 0u; st >>= 1) {
        if (gi < st) { [unroll] for (uint a2 = 0u; a2 < 18u; ++a2) npcS[a2][gi] += npcS[a2][gi + st]; }
        GroupMemoryBarrierWithGroupSync();
    }
    if (gi != 0u) return;
    const uint b = k * 128u;
    npcEstado.Store(b + 4u, asuint(zh));
    npcEstado.Store(b + 12u, npcEstado.Load(b + 12u) + 1u);
    if (k == 0u) return;
    const bool zera = (bits & 2u) != 0u || !on;
    const uint ini = max((npcPack >> 4) & 255u, 1u);
    float peso = 0.0; bool algum = false;
    [loop] for (uint zz = 0u; zz < Z; ++zz) {
        const uint o = 4u * zz;
        float rref = asfloat(npcEstado.Load(b + 16u + o)), fs = asfloat(npcEstado.Load(b + 40u + o));
        uint nz = npcEstado.Load(b + 88u + o);
        if (zera) { rref = 0.0; fs = 1.0; nz = 0u; }
        const float sm = npcS[3u * zz][0], sc = npcS[3u * zz + 1u][0], se = npcS[3u * zz + 2u][0];
        peso += sm;
        if (on && zh > 0.0 && sm >= 8.0 && sc > 1e-5) {
            const float r = se / sc, rc = clamp(r, 0.3, 1.5);
            nz += 1u; algum = true;
            if (nz <= ini) { rref = rc; fs = 1.0; }
            else if (nz <= 2u * ini) { rref += (rc - rref) / float(nz - ini); fs = 1.0; }
            else {

                if (npcAlfaRef > 0.0) rref = lerp(rref, rc, npcAlfaRef);
                else if (npcAlfaRef == 0.0) rref += (rc - rref) / float(min(nz - ini, max((npcPack >> 28) & 15u, 1u) * 64u));
                fs = lerp(fs, NpcAlvo(rref, r), npcAlfaF);
            }
            npcEstado.Store(b + 64u + o, asuint(r));
        }
        if (NpcModo() == 3u && nz > 0u) fs = npcFixo;
        npcEstado.Store(b + 16u + o, asuint(rref)); npcEstado.Store(b + 40u + o, asuint(fs)); npcEstado.Store(b + 88u + o, nz);
    }
    uint nn = zera ? 0u : npcEstado.Load(b);
    if (algum) nn += 1u;
    npcEstado.Store(b, nn); npcEstado.Store(b + 8u, asuint(peso));
}
#endif
