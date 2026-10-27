#include "rosto_k.hlsli"
RWTexture2D<float4> saidaRw : register(u1);
RWByteAddressBuffer hist : register(u2);

RWByteAddressBuffer camada : register(u3);
uint camEnd(uint metade, int2 q) { return (metade * w * h + uint(q.y) * w + uint(q.x)) * 8u; }
float4 camLer(uint metade, int2 q) { const uint2 a = camada.Load2(camEnd(metade, q)); return float4(f16tof32(a.x), f16tof32(a.x >> 16), f16tof32(a.y), f16tof32(a.y >> 16)); }
void camGravar(uint metade, int2 q, float4 v) { camada.Store2(camEnd(metade, q), uint2(f32tof16(v.r) | (f32tof16(v.g) << 16), f32tof16(v.b) | (f32tof16(v.a) << 16))); }

void camFresco(int2 p, float3 d, float C) {
    if (!(C > 0.0) || !all(isfinite(d))) return;
    const float4 a = camLer(padR2, p);
    float3 F0 = float3(0, 0, 0); float C0 = 0.0;
    if (a.w <= -1.0 && all(isfinite(a))) { C0 = saturate(-a.w - 1.0); F0 = a.rgb; }
    camGravar(padR2, p, float4(d + (1.0 - C) * F0, -(1.0 + (1.0 - (1.0 - C0) * (1.0 - C)))));
}
SamplerState linearClamp : register(s0);

#include "tom_comum.hlsli"
#include "prof_comum.hlsli"
#include "cr_comum.hlsli"
#include "area_rosto.hlsli"

float3 rjProtegeRealce(float3 c, float3 d) {
    const uint k = rapR >> 24;
    if (k == 0u || all(d >= 0.0)) return d;
    const float3 b = tmf3(c * expo);
    const float w = 1.0 - smoothstep(0.30 + 0.05 * float(k & 15u), 0.30 + 0.05 * float(k >> 4), max(b.r, max(b.g, b.b)));
    return float3(d.r < 0.0 ? d.r * w : d.r, d.g < 0.0 ? d.g * w : d.g, d.b < 0.0 ? d.b * w : d.b);
}

[numthreads(8,8,1)] void RjRecorte(uint3 id : SV_DispatchThreadID) {
    if (id.x == 0 && id.y == 0) compartilhado.Store(offTag, 0u);
    if (id.x >= n || id.y >= n) return;
    const float passo = float(lado) / float(n);
    if (passo < 1.0) {
        const float2 uv = (float2(rx, ry) + (float2(id.xy) + .5) * passo) / float2(w, h);
        float3 cb = saidaFsr.SampleLevel(linearClamp, uv, 0).rgb;cb = all(isfinite(cb)) ? cb : float3(0, 0, 0);
        compartilhado.Store2(offEnt + (id.y * n + id.x) * 8u, uint2(f32tof16(cb.r) | (f32tof16(cb.g) << 16), f32tof16(cb.b) | (f32tof16(1.0) << 16)));
        return;
    }
    const uint m = clamp(uint(ceil(passo - 1e-3)), 1u, 8u);
    float3 soma = 0;
    for (uint b = 0; b < m; ++b) for (uint a = 0; a < m; ++a) {
        const float2 f = (float2(id.xy) + (float2(a, b) + .5) / float(m)) * passo;
        const int2 p = clamp(int2(rx, ry) + int2(floor(f)), int2(0, 0), int2(int(w) - 1, int(h) - 1));
        const float3 c = saidaFsr.Load(int3(p, 0)).rgb;
        soma += all(isfinite(c)) ? c : float3(0, 0, 0);
    }
    soma /= float(m * m);
    compartilhado.Store2(offEnt + (id.y * n + id.x) * 8u, uint2(f32tof16(soma.r) | (f32tof16(soma.g) << 16), f32tof16(soma.b) | (f32tof16(1.0) << 16)));
}

[numthreads(8,8,1)] void RjRostoImg(uint3 id : SV_DispatchThreadID) {
    if (id.x >= fw || id.y >= fh) return;

    const float2 orig = lado > 0u ? float2(rx, ry) : float2(0, 0);
    const float2 tam = lado > 0u ? float2(lado, n > 0u ? n : lado) : float2(w, h);
    const float2 passo = tam / float2(fw, fh);
    const uint2 m = clamp(uint2(ceil(passo - 1e-3)), uint2(1, 1), uint2(8, 8));
    float3 soma = 0;
    for (uint b = 0; b < m.y; ++b) for (uint a = 0; a < m.x; ++a) {
        const float2 f = orig + (float2(id.xy) + (float2(a, b) + .5) / float2(m)) * passo;
        const int2 p = clamp(int2(floor(f)), int2(0, 0), int2(int(w) - 1, int(h) - 1));
        const float3 c = saidaFsr.Load(int3(p, 0)).rgb;
        soma += all(isfinite(c)) ? c : float3(0, 0, 0);
    }
    soma /= float(m.x * m.y);
    compartilhado.Store2(offEnt + (id.y * fw + id.x) * 8u, uint2(f32tof16(soma.r) | (f32tof16(soma.g) << 16), f32tof16(soma.b) | (f32tof16(1.0) << 16)));
}

float3 celula(uint off, uint nn, int2 t) {
    t = clamp(t, int2(0, 0), int2(int(nn) - 1, int(nn) - 1));
    const uint2 a = compartilhado.Load2(off + (uint(t.y) * nn + uint(t.x)) * 8u);
    const float3 d = float3(f16tof32(a.x), f16tof32(a.x >> 16), f16tof32(a.y));
    return all(isfinite(d)) ? d : float3(0, 0, 0);
}

bool lerDelta(float2 uv0, uint passos, uint off, uint nn, int ox, int oy, uint L, float cx, float cy, uint pw, uint ph, out float3 d, out float conf) {
    d = float3(0, 0, 0); conf = 1.0;
    if (passos >= historico) return false;
    float2 uv = uv0; uint s = fatia;
    [loop] for (uint k = 0; k < passos; ++k) {
        const float2 mv = movHist.SampleLevel(linearClamp, float3(uv, s), 0);
        if (!all(isfinite(mv))) return false;
        const float2 chegada = uv + mv;
        if (any(chegada < 0.0) || any(chegada > 1.0)) return false;
        const uint anterior = (s + historico - 1u) % historico;
        if (((mascara >> anterior) & 1u) == 0u) return false;
        const float pa = profEm(uv, s, pw, ph), pb = profEm(chegada, anterior, pw, ph);
        if (!isfinite(pa) || !isfinite(pb)) return false;
        const float tol = NR_TOL_PROF(profAbs, profRel, pa, pb);
        if (!(tol > 0.0) || abs(pa - pb) > tol) return false;
        conf = min(conf, saturate(2.0 - 2.0 * abs(pa - pb) / tol));
        uv = chegada; s = anterior;
    }
    {
        const float zA = profEm(uv, s, pw, ph), zh = profEm((float2(cx, cy) + 0.5) / float2(w, h), s, pw, ph);
        if (isfinite(zA) && isfinite(zh) && abs(zA) > 0.0 && abs(zh) > 0.0) {
            const float r = max(abs(zA), abs(zh)) / min(abs(zA), abs(zh));
            conf *= 1.0 - smoothstep(camada0, camada1, r);
        }
    }
    const float2 t = (uv * float2(w, h) - float2(ox, oy)) * float(nn) / float(L) - 0.5;
    if (t.x < -0.5 || t.y < -0.5 || t.x > float(nn) - 0.5 || t.y > float(nn) - 0.5) return false;
    const int2 b = int2(floor(t)); const float2 f = t - float2(b);
    d = lerp(lerp(celula(off, nn, b), celula(off, nn, b + int2(1, 0)), f.x), lerp(celula(off, nn, b + int2(0, 1)), celula(off, nn, b + int2(1, 1)), f.x), f.y);
    return all(isfinite(d)) && conf > 0.0;
}

static const uint HCAB = 64u;
uint histBase(uint b) { return b * (HCAB + janela * janela * 16u); }
float4 histLo(uint hb, int2 q) { const uint2 a = hist.Load2(hb + HCAB + (uint(q.y) * janela + uint(q.x)) * 16u); return float4(f16tof32(a.x), f16tof32(a.x >> 16), f16tof32(a.y), f16tof32(a.y >> 16)); }
float3 histHi(uint hb, int2 q) { const uint2 a = hist.Load2(hb + HCAB + (uint(q.y) * janela + uint(q.x)) * 16u + 8u); return float3(f16tof32(a.x), f16tof32(a.x >> 16), f16tof32(a.y)); }
float4 cr(float f) {
    return float4(NR_CR0(f), NR_CR1(f), NR_CR2(f), NR_CR3(f));
}

[numthreads(8,8,1)] void RjCompor(uint3 id : SV_DispatchThreadID) {
    if (id.x >= fw || id.y >= fh) return;
    const int2 p = int2(fx, fy) + int2(id.xy);
    if (p.x < 0 || p.y < 0 || p.x >= int(w) || p.y >= int(h)) return;
    if (historico == 0u || historico > 16u || fatia >= historico || ((mascara >> fatia) & 1u) == 0u) return;
    uint pw = 1u, ph = 1u, pf = 1u; profHist.GetDimensions(pw, ph, pf);
    const float2 uv0 = (float2(p) + 0.5) / float2(w, h);
    float3 d = float3(0, 0, 0), d2 = float3(0, 0, 0); float conf = 0.0, c2 = 0.0;

    const bool ok1 = compartilhado.Load(offTag) == jobEsperado && lerDelta(uv0, idade, offEnt, n, rx, ry, lado, hx, hy, pw, ph, d, conf);
    bool ok2 = false;
    if (job2 != 0u && (ok1 ? peso < 1.0 : sozinho != 0u) && compartilhado.Load(offTag2) == job2)
        ok2 = lerDelta(uv0, idade2, offEnt2, n2, rx2, ry2, lado2, hx2, hy2, pw, ph, d2, c2);
    if (ok1) {
        if (ok2) {
            const float dif = abs(dot(d - d2, NR_LUMA));
            const float a = max(peso, saturate((dif - limiar) / max(limiar, 1e-4)));
            d = lerp(d2, d, a);
        }
    } else if (ok2) { d = d2; conf = c2; }
    else return;
    d *= ganho * conf;
    if (all(d == float3(0, 0, 0)) || !all(isfinite(d))) return;
    const float4 c = saidaRw[p];
    if ((rapR & 32u) != 0u) { camFresco(p, d, saturate(ganho * conf)); return; }
    float3 e;
    if (tonemap != 0u) { d = rjProtegeRealce(c.rgb, d); NR_COMPOR_TOM(e, c.rgb, d, expo) }
    else e = saturate(c.rgb + d);
    if (!all(isfinite(e))) return;
    saidaRw[p] = float4(e, c.a);
}

struct CelR { float3 rgb; float4 rgba; };
CelR celulaR(uint off, uint nn, int2 t) {
    t = clamp(t, int2(0, 0), int2(int(nn) - 1, int(nn) - 1));
    const uint2 a = compartilhado.Load2(off + (uint(t.y) * nn + uint(t.x)) * 8u);
    const float4 d = float4(f16tof32(a.x), f16tof32(a.x >> 16), f16tof32(a.y), f16tof32(a.y >> 16));
    CelR c; c.rgb = all(isfinite(d.rgb)) ? d.rgb : float3(0, 0, 0); c.rgba = all(isfinite(d)) ? d : float4(0, 0, 0, 0);
    return c;
}

float pesoCamada(float2 uv, uint s, uint pw, uint ph, float zh) {
    const float2 t = uv * float2(pw, ph) - 0.5;
    const int2 i0 = int2(floor(t)); const float2 f = t - float2(i0);
    float4 wz;
    [unroll] for (int k = 0; k < 4; ++k) {
        const int2 q = clamp(i0 + int2(k & 1, k >> 1), int2(0, 0), int2(int(pw) - 1, int(ph) - 1));
        const float z = profHist.Load(int4(q, s, 0));
        float wk = ((rapR & 512u) != 0u && z == 0.0) ? 0.0 : 1.0;
        if (isfinite(z) && abs(z) > 0.0) { const float r = max(abs(z), abs(zh)) / min(abs(z), abs(zh)); wk = 1.0 - smoothstep(camada0, camada1, r); }
        wz[k] = wk;
    }
    return lerp(lerp(wz.x, wz.y, f.x), lerp(wz.z, wz.w, f.x), f.y);
}
static uint g_mot = 0u, g_passo = 0u;
static float2 g_uvCap = float2(-1.0, -1.0);
bool lerDeltaR(float2 uv0, uint passos, uint off, uint nn, int ox, int oy, uint L, float cx, float cy, uint pw, uint ph, out float3 d, out float3 dl, out float dp, out float conf, out float lcap, out bool foraCamada) {
    d = float3(0, 0, 0); dl = float3(0, 0, 0); dp = 0.0; lcap = -1.0; conf = 1.0; foraCamada = false;
    if (passos >= historico) { g_mot = 3u; return false; }
    float2 uv = uv0; uint s = fatia;
    float pa = profEm(uv, s, pw, ph);
    [loop] for (uint k = 0; k < passos; ++k) {
        const float2 mv = movHist.SampleLevel(linearClamp, float3(uv, s), 0);
        if (!all(isfinite(mv))) { g_mot = 4u; return false; }
        const float2 chegada = uv + mv;
        if (any(chegada < 0.0) || any(chegada > 1.0)) { g_mot = 5u; return false; }
        const uint anterior = (s + historico - 1u) % historico;
        if (((mascara >> anterior) & 1u) == 0u) { g_mot = 6u; return false; }
        const float pb = profEm(chegada, anterior, pw, ph);
        if (!isfinite(pa) || !isfinite(pb)) { g_mot = 7u; return false; }
        const float tol = NR_TOL_PROF(profAbs, profRel, pa, pb);
        if (!(tol > 0.0) || abs(pa - pb) > tol) { g_mot = 8u + 0u * k; g_passo = k; return false; }
        conf = min(conf, saturate(2.0 - 2.0 * abs(pa - pb) / tol));
        uv = chegada; s = anterior; pa = pb;
    }
    {
        const float zA = pa, zh = profEm((float2(cx, cy) + 0.5) / float2(w, h), s, pw, ph);
        if ((rapR & 4u) != 0u) {
            if (isfinite(zh) && abs(zh) > 0.0) { const float wc = pesoCamada(uv, s, pw, ph, zh); conf *= wc; foraCamada = !(wc > 0.0); }
        } else
        if (isfinite(zA) && isfinite(zh) && abs(zA) > 0.0 && abs(zh) > 0.0) {
            const float r = max(abs(zA), abs(zh)) / min(abs(zA), abs(zh));
            conf *= 1.0 - smoothstep(camada0, camada1, r);
        } else if ((rapR & 512u) != 0u && zA == 0.0) { conf = 0.0; foraCamada = true; }
    }
    const float2 t = (uv * float2(w, h) - float2(ox, oy)) * float(nn) / float(L) - 0.5;
    if (t.x < -0.5 || t.y < -0.5 || t.x > float(nn) - 0.5 || t.y > float(nn) - 0.5) { g_mot = 9u; return false; }
    const int2 b = int2(floor(t)); const float2 f = t - float2(b);
    const CelR a00 = celulaR(off, nn, b), a10 = celulaR(off, nn, b + int2(1, 0)), a01 = celulaR(off, nn, b + int2(0, 1)), a11 = celulaR(off, nn, b + int2(1, 1));
    d = lerp(lerp(a00.rgb, a10.rgb, f.x), lerp(a01.rgb, a11.rgb, f.x), f.y);
    if (!(all(isfinite(d)) && conf > 0.0)) { g_mot = foraCamada ? 30u : (all(isfinite(d)) ? 31u : 32u); return false; }
    dp = lerp(lerp(a00.rgba.w, a10.rgba.w, f.x), lerp(a01.rgba.w, a11.rgba.w, f.x), f.y);
    const uint ol = off + bytesMeio;
    const CelR c00 = celulaR(ol, nn, b), c10 = celulaR(ol, nn, b + int2(1, 0)), c01 = celulaR(ol, nn, b + int2(0, 1)), c11 = celulaR(ol, nn, b + int2(1, 1));
    dl = lerp(lerp(c00.rgb, c10.rgb, f.x), lerp(c01.rgb, c11.rgb, f.x), f.y);
    lcap = lerp(lerp(c00.rgba.w, c10.rgba.w, f.x), lerp(c01.rgba.w, c11.rgba.w, f.x), f.y);
    if (!(all(isfinite(dl)) && isfinite(dp))) { g_mot = 33u; return false; }
    g_uvCap = uv;
    return true;
}

void diagGravar(int2 p, uint codigo, float conf, float wf) {
    saidaRw[p] = float4(1000.0 + float(codigo), 1000.0 + round(100.0 * saturate(conf)), 1000.0 + round(100.0 * saturate(wf)), 1.0);
}
[numthreads(8,8,1)] void RjComporR(uint3 id : SV_DispatchThreadID) {
    if (id.x >= fw || id.y >= fh) return;
    const int2 o0 = int2(offT0 & 0xFFFFu, offT0 >> 16);
    const int2 p = int2(fx, fy) + int2(id.xy);
    if (p.x < 0 || p.y < 0 || p.x >= int(w) || p.y >= int(h)) return;
    if (historico == 0u || historico > 16u || fatia >= historico || ((mascara >> fatia) & 1u) == 0u) return;
    uint pw = 1u, ph = 1u, pf = 1u; profHist.GetDimensions(pw, ph, pf);
    const float2 uv0 = (float2(p) + 0.5) / float2(w, h);
    float3 d = float3(0, 0, 0); float conf = 0.0;
    const uint cur = quadro & 1u, hb = histBase(cur), hpv = histBase(cur ^ 1u);
    if (id.x == 0u && id.y == 0u) { hist.Store4(hb, uint4(quadro, geracao, asuint(fx), asuint(fy))); hist.Store2(hb + 16u, uint2(fw, fh)); }
    const uint ho = hb + HCAB + (id.y * janela + id.x) * 16u;
    float3 dl = 0; float dp = 0; float lcap = -1.0;

    const bool seguraQ = (rapR & 1u) != 0u;
    bool fora1 = false, fora2 = false;
    g_mot = 0u; const bool tagOk = compartilhado.Load(offTag) == jobEsperado; if (!tagOk) g_mot = 2u;
    bool ok = !seguraQ && compartilhado.Load(offTag) == jobEsperado && lerDeltaR(uv0, idade, offEnt, n, rx, ry, lado, hx, hy, pw, ph, d, dl, dp, conf, lcap, fora1);
    const uint mot1 = g_mot, passo1 = g_passo;
    if (!ok && !seguraQ && job2 != 0u && sozinho != 0u && compartilhado.Load(offTag2) == job2)
        ok = lerDeltaR(uv0, idade2, offEnt2, n2, rx2, ry2, lado2, hx2, hy2, pw, ph, d, dl, dp, conf, lcap, fora2);

    bool fora = (rapR & (4u | 512u)) != 0u && (fora1 || fora2);

    float wBorda = 1.0;
    if ((rapR & 8u) != 0u && !seguraQ && idade < historico) {
        const uint sCap = (fatia + historico - idade) % historico;
        const float zh = profEm((float2(hx, hy) + 0.5) / float2(w, h), sCap, pw, ph);
        if (isfinite(zh) && abs(zh) > 0.0) {
            const float lar = asfloat(padR0);
            if (!(lar > 0.75)) {
            const float2 o = 0.75 / float2(pw, ph);
            wBorda = 0.25 * (pesoCamada(uv0 + float2(-o.x, -o.y), fatia, pw, ph, zh) + pesoCamada(uv0 + float2(o.x, -o.y), fatia, pw, ph, zh)
                           + pesoCamada(uv0 + float2(-o.x, o.y), fatia, pw, ph, zh) + pesoCamada(uv0 + float2(o.x, o.y), fatia, pw, ph, zh));
            } else {
                const float2 o = lar / float2(pw, ph); float acc = 0.0;
                [unroll] for (int j = -1; j <= 1; ++j) [unroll] for (int i = -1; i <= 1; ++i)
                    acc += float((2 - abs(i)) * (2 - abs(j))) * pesoCamada(uv0 + float2(i, j) * o, fatia, pw, ph, zh);
                wBorda = acc / 16.0;
            }
            if (!(wBorda > 0.0)) fora = true;
        }
    }
    const bool segura = seguraQ || (!ok && !fora && (rapR & 2u) != 0u);
    if (segura) { d = float3(0, 0, 0); dl = float3(0, 0, 0); dp = 0.0; lcap = -1.0; }
    if (!ok && !segura) { hist.Store4(ho, uint4(0, 0, 0, 0)); if ((rapR & 256u) != 0u) diagGravar(p, 40u + mot1, float(passo1) / 100.0, 1.0); return; }
    const float3 dh = d - dl;
    float3 Sl = dl, Sh = dh; bool temH = false; float recP = 0.0, recN = 0.0;
    const uint4 H = hist.Load4(hpv); const uint2 H2 = hist.Load2(hpv + 16u);
    uint dg = H.x + 1u != quadro ? 10u : (H.y != geracao ? 11u : ((H2.x > 4u && H2.y > 4u) ? 13u : 12u));
    if (H.x + 1u == quadro && H.y == geracao && H2.x > 4u && H2.y > 4u && H2.x <= janela && H2.y <= janela) {
        const float2 mv = movHist.SampleLevel(linearClamp, float3(uv0, fatia), 0);
        const float2 uvp = uv0 + mv;
        const uint ant = (fatia + historico - 1u) % historico;
        bool okm = all(isfinite(mv)) && all(uvp >= 0.0) && all(uvp <= 1.0) && ((mascara >> ant) & 1u) != 0u;
        if (okm) { dg = 14u;
            const float pa = profEm(uv0, fatia, pw, ph), pb = profEm(uvp, ant, pw, ph);
            const float tol = NR_TOL_PROF(profAbs, profRel, pa, pb);
            okm = NR_MESMA_SUPERFICIE(pa, pb, tol);
        }
        const float2 pp = uvp * float2(w, h) - 0.5 - float2(float(asint(H.z)), float(asint(H.w)));
        if (okm) dg = 15u;
        if (okm && all(pp >= 1.0) && pp.x <= float(H2.x) - 3.0 && pp.y <= float(H2.y) - 3.0) { dg = 16u;
            const int2 i0 = int2(floor(pp)); const float2 f = pp - float2(i0);
            const float4 l00 = histLo(hpv, i0), l10 = histLo(hpv, i0 + int2(1, 0)), l01 = histLo(hpv, i0 + int2(0, 1)), l11 = histLo(hpv, i0 + int2(1, 1));
            const float valido = min(min(l00.w, l10.w), min(l01.w, l11.w));
            recP = posMov > 0.0 ? saturate(valido - 1.0) : 0.0; recN = recP * posMov;
            if (valido > 0.5) { dg = 17u;
                const float3 Plo = lerp(lerp(l00.rgb, l10.rgb, f.x), lerp(l01.rgb, l11.rgb, f.x), f.y);
                const float4 wx = cr(f.x), wy = cr(f.y);
                float3 Phi = 0;
                [unroll] for (int j = 0; j < 4; ++j) {
                    float3 lin = 0;
                    [unroll] for (int i = 0; i < 4; ++i) lin += wx[i] * histHi(hpv, i0 + int2(i - 1, j - 1));
                    Phi += wy[j] * lin;
                }
                if (all(isfinite(Plo)) && all(isfinite(Phi))) {
                    temH = true; dg = 20u;
                    if (segura) { Sl = Plo; Sh = Phi; }
                    else {
                    const float difl = abs(dot(dl - Plo, NR_LUMA));

                    float aB = aBaixa;

                    const float movN = smoothstep(0.25 * movPx, movPx, length(mv * float2(w, h)));
                    if (aLenta > 0.0) aB = lerp(aLenta, aRapida, max(movN, recP));
                    recN = max(movN, recP * posMov);
                    const float al = max(aB, saturate((difl - limiar) / max(limiar, 1e-4)));
                    Sl = lerp(Plo, dl, al);

                    float gm = gama;
                    if (gamaParado > 0.0) gm = lerp(max(gamaParado, gama), gama, max(movN, recP));
                    const float g = gm * dp;

                    float aH = aAlta;
                    if ((rapR & 64u) != 0u) aH = lerp(aAlta, 1.0, saturate(max(movN, recP)));

                    if ((rapR & 128u) != 0u) aH = max(aH, saturate((float(idade) - 2.0) / 4.0));
                    Sh = lerp(clamp(Phi, dh - g, dh + g), dh, aH);
                    }
                }
            }
        }
    }
    if (segura) { if (!temH) { hist.Store4(ho, uint4(0, 0, 0, 0)); if ((rapR & 256u) != 0u) diagGravar(p, 140u + mot1, float(passo1) / 100.0, 1.0); return; } conf = 1.0; }
    conf *= wBorda;
    float wfD = 1.0;

    const float4 c = saidaRw[p];
    const float fres = asfloat(pad3);
    if (fres > 0.0 && lcap >= 0.0) {
        float lc;
        if ((rapR & 4096u) != 0u) {

            float s = 0.0;
            [unroll] for (int j = -2; j <= 2; j += 2) [unroll] for (int i = -2; i <= 2; i += 2) {
                const int2 q = clamp(int2(id.xy) + int2(i, j), int2(0, 0), int2(int(fw) - 1, int(fh) - 1));
                const float3 cq = saidaFsr.Load(int3(q + o0, 0)).rgb;
                const float3 tq = tonemap != 0u ? tmf3(max(cq, 0.0) * expo) : saturate(cq);
                s += all(isfinite(tq)) ? dot(tq, NR_LUMA) : 0.0;
            }
            lc = s / 9.0;
        } else {
            const float3 tc = tonemap != 0u ? tmf3(max(c.rgb, 0.0) * expo) : saturate(c.rgb);
            lc = dot(tc, NR_LUMA);
        }
        const float wf = 1.0 - smoothstep(fres, 2.5 * fres, abs(lc - lcap));
        if (isfinite(wf)) { Sl *= wf; Sh *= wf; wfD = wf; }
    }
    { const float3 hl = Sl, hh = Sh;
      hist.Store4(ho, uint4(f32tof16(hl.r) | (f32tof16(hl.g) << 16), f32tof16(hl.b) | (f32tof16(1.0 + recN) << 16), f32tof16(hh.r) | (f32tof16(hh.g) << 16), f32tof16(hh.b))); }
    if ((rapR & 256u) != 0u) { diagGravar(p, dg + (segura ? 100u : 0u), conf, segura ? float(mot1) / 100.0 : wfD); return; }
    d = bxForca > 0.0 ? Sl * bxForca + Sh : Sl + Sh;
    d *= ganho * conf;
    if (all(d == float3(0, 0, 0)) || !all(isfinite(d))) return;

    if ((rapR & 16u) != 0u && tonemap != 0u && idade < historico) {
        const uint sCap = (fatia + historico - idade) % historico;
        const float zh = profEm((float2(hx, hy) + 0.5) / float2(w, h), sCap, pw, ph);
        if (isfinite(zh) && abs(zh) > 0.0) {
            const float2 o = 1.0 / float2(pw, ph);
            const float cov = 0.25 * (pesoCamada(uv0 + float2(-o.x, -o.y), fatia, pw, ph, zh) + pesoCamada(uv0 + float2(o.x, -o.y), fatia, pw, ph, zh)
                                    + pesoCamada(uv0 + float2(-o.x, o.y), fatia, pw, ph, zh) + pesoCamada(uv0 + float2(o.x, o.y), fatia, pw, ph, zh));
            const float bnd = saturate(2.0 * min(cov, 1.0 - cov));
            if (bnd > 0.0) {
                const float3 lw = NR_LUMA;
                float mx = 0.0, s1 = 0.0, s2 = 0.0;
                [unroll] for (int j = -1; j <= 1; ++j) [unroll] for (int i = -1; i <= 1; ++i) {
                    const int2 q = clamp(int2(id.xy) + int2(i, j), int2(0, 0), int2(int(fw) - 1, int(fh) - 1));
                    const float3 cq = saidaFsr.Load(int3(q + o0, 0)).rgb;
                    const float lq = all(isfinite(cq)) ? dot(tmf3(max(cq, 0.0) * expo), lw) : 0.0;
                    mx = max(mx, lq); s1 += lq; s2 += lq * lq;
                }
                const float dp = sqrt(max(s2 / 9.0 - (s1 / 9.0) * (s1 / 9.0), 0.0));
                const float lc = dot(tmf3(max(c.rgb, 0.0) * expo), lw);
                const float teto = max(mx + asfloat(padR1) * dp, lc);
                const float ex = dot(tmf3(max(c.rgb, 0.0) * expo) + d, lw) - teto;
                if (ex > 0.0 && isfinite(ex)) d -= ex * bnd;
            }
        }
    }
    if ((rapR & 32u) != 0u) { camFresco(p, d, saturate(ganho * conf)); return; }
    float3 e;
    if (tonemap != 0u) { d = rjProtegeRealce(c.rgb, d); NR_COMPOR_TOM(e, c.rgb, d, expo) }
    else e = saturate(c.rgb + d);
    if (!all(isfinite(e))) return;

    if ((rapR & 8192u) != 0u && tonemap != 0u && corHl > 0.0) {
        const float2 pc = (g_uvCap.x >= 0.0 ? g_uvCap : uv0) * float2(w, h);
        const float ex = (pc.x - corHx) / max(NR_AREA_SEMI_X * corHl, 1.0), ey = (pc.y - corHy) / max(NR_AREA_SEMI_Y * corHl, 1.0);
        const float wq = 1.0 - smoothstep(NR_AREA_BORDA0, NR_AREA_BORDA1, sqrt(ex * ex + ey * ey));
        const float3 g = saidaFsr.Load(int3(int2(id.xy) + o0, 0)).rgb;
        if (wq > 0.0 && all(isfinite(g))) {
            const float3 lw = NR_LUMA;
            const float3 gt = tmf3(max(g, 0.0) * expo), tt = tmf3(max(e, 0.0) * expo);
            const float yg = dot(gt, lw), yt = dot(tt, lw);
            float3 tn = float3(yt, yt, yt);
            if (yg > 1e-4) { const float kk = clamp(yt / yg, 0.0, 4.0); tn = gt * kk + (yt - yg * kk); }
            const float3 e2 = max(tmi3(saturate(lerp(tt, tn, wq * (float((rapR >> 16) & 255u) / 255.0)))), 0.0) / expo;
            if (all(isfinite(e2))) e = e2;
        }
    }
    saidaRw[p] = float4(e, c.a);
}

[numthreads(8,8,1)] void RjCamada(uint3 id : SV_DispatchThreadID) {
    if (id.x >= fw || id.y >= fh) return;
    const int2 p = int2(fx, fy) + int2(id.xy);
    if (p.x < 0 || p.y < 0 || p.x >= int(w) || p.y >= int(h)) return;
    const uint cur = padR2 & 1u, ant = cur ^ 1u;
    const float4 a = camLer(cur, p);
    float3 Ff = float3(0, 0, 0); float C = 0.0;
    if (a.w <= -1.0 && all(isfinite(a))) { C = saturate(-a.w - 1.0); Ff = a.rgb; }
    if (p.x < rx || p.y < ry || p.x >= int(lado) || p.y >= int(n)) { camGravar(cur, p, float4(0, 0, 0, 0)); return; }
    float3 Fr = float3(0, 0, 0); float idR = 0.0; bool levou = false;
    if (idade2 != 0u && historico > 0u && historico <= 16u && fatia < historico && C < 1.0) {
        uint pw = 1u, ph = 1u, pf = 1u; profHist.GetDimensions(pw, ph, pf);
        const float2 uv0 = (float2(p) + 0.5) / float2(w, h);
        const float2 mv = movHist.SampleLevel(linearClamp, float3(uv0, fatia), 0);
        const float2 uvp = uv0 + mv;
        const uint sa = (fatia + historico - 1u) % historico;
        bool ok = all(isfinite(mv)) && all(uvp >= 0.0) && all(uvp <= 1.0) && ((mascara >> sa) & 1u) != 0u && ((mascara >> fatia) & 1u) != 0u;
        if (ok) {
            const float pa = profEm(uv0, fatia, pw, ph), pb = profEm(uvp, sa, pw, ph);
            const float tol = NR_TOL_PROF(profAbs, profRel, pa, pb);
            ok = NR_MESMA_SUPERFICIE(pa, pb, tol);
            if ((rapR & 512u) != 0u && pa == 0.0) ok = false;
        }
        if (ok) {
            const float2 pp = uvp * float2(w, h) - 0.5;
            const int2 i0 = int2(floor(pp)); const float2 f = pp - float2(i0);
            if (i0.x >= 1 && i0.y >= 1 && i0.x < int(w) - 2 && i0.y < int(h) - 2) {
                const float4 c00 = camLer(ant, i0), c10 = camLer(ant, i0 + int2(1, 0)), c01 = camLer(ant, i0 + int2(0, 1)), c11 = camLer(ant, i0 + int2(1, 1));
                const float4 wx = cr(f.x), wy = cr(f.y);
                float3 s = float3(0, 0, 0);
                [unroll] for (int j = 0; j < 4; ++j) {
                    float3 lin = float3(0, 0, 0);
                    [unroll] for (int i = 0; i < 4; ++i) lin += wx[i] * camLer(ant, i0 + int2(i - 1, j - 1)).rgb;
                    s += wy[j] * lin;
                }
                s = clamp(s, min(min(c00.rgb, c10.rgb), min(c01.rgb, c11.rgb)), max(max(c00.rgb, c10.rgb), max(c01.rgb, c11.rgb)));
                const float ida = max(max(c00.w, c10.w), max(c01.w, c11.w));
                if (all(isfinite(s)) && isfinite(ida) && ida >= 0.0) {
                    idR = ida + 1.0; Fr = s;
                    if (idR > float(sozinho)) Fr *= peso;
                    if (idR > asfloat(pad3)) Fr = float3(0, 0, 0);
                    else levou = true;
                }
            }
        }
    }
    float3 F = Ff + (1.0 - C) * Fr;

    if ((rapR & 1024u) != 0u && !levou && C > 0.0) F = Ff / max(C, 0.25);

    if ((rapR & 2048u) != 0u && C > 0.0) { const float wf = smoothstep(0.35, 0.6, C); F = lerp(F, Ff / max(C, 0.25), wf); }
    const float idade_n = all(F == float3(0, 0, 0)) ? 0.0 : (1.0 - C) * idR;
    camGravar(cur, p, float4(F, idade_n));
    const float3 d = F * ganho;
    if (all(d == float3(0, 0, 0)) || !all(isfinite(d))) return;
    const float4 c = saidaRw[p];
    float3 e;
    if (tonemap != 0u) { const float3 dp = rjProtegeRealce(c.rgb, d); NR_COMPOR_TOM(e, c.rgb, dp, expo) }
    else e = saturate(c.rgb + d);
    if (!all(isfinite(e))) return;
    saidaRw[p] = float4(e, c.a);
}
