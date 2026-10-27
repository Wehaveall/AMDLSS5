#include "rosto_jogador.hlsl"

static const uint RHCAB = 64u;
uint rBase(uint metade) { return metade * (RHCAB + janela * janela * 8u); }
float4 rLer(uint hb, int2 q) { const uint2 a = camada.Load2(hb + RHCAB + (uint(q.y) * janela + uint(q.x)) * 8u); return float4(f16tof32(a.x), f16tof32(a.x >> 16), f16tof32(a.y), f16tof32(a.y >> 16)); }
void rGravar(uint hb, int2 q, float4 v) { camada.Store2(hb + RHCAB + (uint(q.y) * janela + uint(q.x)) * 8u, uint2(f32tof16(v.r) | (f32tof16(v.g) << 16), f32tof16(v.b) | (f32tof16(v.a) << 16))); }
float lg(float3 c) { return log2(max(dot(c, NR_LUMA), 6.103515625e-05)); }
uint escala() { return clamp(n2, 1u, 8u); }
float3 corEm(int2 p) {
    p = clamp(p, int2(0, 0), int2(int(w) - 1, int(h) - 1));
    const float3 c = saidaFsr.Load(int3(p, 0)).rgb;
    return all(isfinite(c)) ? max(c, 0.0) : float3(0, 0, 0);
}

float3 corCel(int2 p0, uint e) {
    if (e == 1u) return corEm(p0);
    float3 s = float3(0, 0, 0);
    for (uint b = 0; b < e; ++b) for (uint a = 0; a < e; ++a) s += corEm(p0 + int2(a, b));
    return s / float(e * e);
}

bool sBil(uint hb, float2 pp, uint hw, uint hh, out float3 s) {
    s = float3(0, 0, 0);
    const int2 i0 = int2(floor(pp)); const float2 f = pp - float2(i0);
    if (i0.x < 0 || i0.y < 0 || i0.x + 1 >= int(hw) || i0.y + 1 >= int(hh)) return false;
    const float4 a = rLer(hb, i0), b = rLer(hb, i0 + int2(1, 0)), c = rLer(hb, i0 + int2(0, 1)), d = rLer(hb, i0 + int2(1, 1));
    s = lerp(lerp(a.rgb, b.rgb, f.x), lerp(c.rgb, d.rgb, f.x), f.y);
    return all(isfinite(s));
}

[numthreads(8,8,1)] void RjRuido(uint3 id : SV_DispatchThreadID) {
    if (id.x >= fw || id.y >= fh || fw > janela || fh > janela) return;
    const uint e = escala();
    const uint cur = quadro & 1u, hb = rBase(cur), hp = rBase(cur ^ 1u);
    if (id.x == 0u && id.y == 0u) { camada.Store4(hb, uint4(quadro, geracao, asuint(fx), asuint(fy))); camada.Store3(hb + 16u, uint3(fw, fh, e)); }
    const int2 q = int2(id.xy);
    const int2 p0 = int2(fx, fy) + q * int(e);
    const float3 c = corCel(p0, e);
    const float l = lg(c);
    float3 S = c; float sg = -1.0;
    const uint4 H = camada.Load4(hp); const uint3 H2 = camada.Load3(hp + 16u);
    bool ok = H.x + 1u == quadro && H.y == geracao && H2.z == e && H2.x > 4u && H2.y > 4u && H2.x <= janela && H2.y <= janela
              && historico > 0u && historico <= 16u && fatia < historico && ((mascara >> fatia) & 1u) != 0u;
    float2 pp = float2(0, 0);
    if (ok) {
        uint pw = 1u, ph = 1u, pf = 1u; profHist.GetDimensions(pw, ph, pf);
        const float2 uv0 = (float2(p0) + 0.5 * float(e)) / float2(w, h);
        const float2 mv = movHist.SampleLevel(linearClamp, float3(uv0, fatia), 0);
        const float2 uvp = uv0 + mv;
        const uint ant = (fatia + historico - 1u) % historico;
        ok = all(isfinite(mv)) && all(uvp >= 0.0) && all(uvp <= 1.0) && ((mascara >> ant) & 1u) != 0u;
        if (ok) {
            const float pa = profEm(uv0, fatia, pw, ph), pb = profEm(uvp, ant, pw, ph);
            const float tol = NR_TOL_PROF(profAbs, profRel, pa, pb);
            ok = NR_MESMA_SUPERFICIE(pa, pb, tol);
            if ((rapR & 512u) != 0u && pa == 0.0) ok = false;
        }
        pp = (uvp * float2(w, h) - float2(float(asint(H.z)), float(asint(H.w)))) / float(e) - 0.5;
        ok = ok && all(pp >= 1.0) && pp.x <= float(H2.x) - 3.0 && pp.y <= float(H2.y) - 3.0;
    }
    if (ok) {

        const int2 i0 = int2(floor(pp)); const float2 f = pp - float2(i0);
        const float4 a00 = rLer(hp, i0), a10 = rLer(hp, i0 + int2(1, 0)), a01 = rLer(hp, i0 + int2(0, 1)), a11 = rLer(hp, i0 + int2(1, 1));
        const float4 wx = cr(f.x), wy = cr(f.y);
        float3 Sp = float3(0, 0, 0);
        [unroll] for (int j = 0; j < 4; ++j) {
            float3 lin = float3(0, 0, 0);
            [unroll] for (int i = 0; i < 4; ++i) lin += wx[i] * rLer(hp, i0 + int2(i - 1, j - 1)).rgb;
            Sp += wy[j] * lin;
        }
        Sp = clamp(Sp, min(min(a00.rgb, a10.rgb), min(a01.rgb, a11.rgb)), max(max(a00.rgb, a10.rgb), max(a01.rgb, a11.rgb)));
        const float sp = min(min(a00.a, a10.a), min(a01.a, a11.a));
        if (all(isfinite(Sp)) && isfinite(sp)) {
            Sp = max(Sp, 0.0);
            const float d = l - lg(Sp);

            const int k = int(clamp(round(movPx / float(e)), 1.0, 8.0));
            float db = 0.0, sw = 0.0;
            [unroll] for (int jj = -1; jj <= 1; ++jj) [unroll] for (int ii = -1; ii <= 1; ++ii) {
                const float wt = float((2 - abs(ii)) * (2 - abs(jj)));
                float dd = d;
                if (ii != 0 || jj != 0) { float3 sv; if (sBil(hp, pp + float2(ii * k, jj * k), H2.x, H2.y, sv)) dd = lg(corCel(p0 + int2(ii * k, jj * k) * int(e), e)) - lg(max(sv, 0.0)); }
                db += wt * dd; sw += wt;
            }
            db /= sw;
            const float V0 = sp >= 0.0 ? sp * sp : -1.0;
            const float Vn = V0 < 0.0 ? db * db : V0 + aAlta * (db * db - V0);
            const float sv0 = sqrt(max(V0, 0.0));
            float al = 1.0;
            if (V0 >= 0.0) al = max(aBaixa, max(smoothstep(gama * sv0, 2.0 * gama * sv0 + 1e-6, abs(db)), smoothstep(aLenta, 2.0 * aLenta, abs(d))));
            S = Sp + al * (c - Sp);
            sg = sqrt(max(Vn, 0.0));
            if (!all(isfinite(S)) || !isfinite(sg)) { S = c; sg = -1.0; }
        }
    }
    rGravar(hb, q, float4(S, sg));
}

float4 celEm(int2 p, uint hb, bool jan, int ox, int oy, uint jw, uint jh, uint e) {
    if (!jan) return float4(0, 0, 0, -1);
    const int2 q = int2(floor(float2(p - int2(ox, oy)) / float(e)));
    if (q.x < 0 || q.y < 0 || q.x >= int(jw) || q.y >= int(jh)) return float4(0, 0, 0, -1);
    const float4 v = rLer(hb, q);
    return (v.a >= 0.0 && all(isfinite(v))) ? v : float4(0, 0, 0, -1);
}
float pesoDe(float4 v) { return v.a >= 0.0 ? smoothstep(limiar, peso, v.a) : 0.0; }

float3 corMix(int2 p, float3 c, uint hb, bool jan, int ox, int oy, uint jw, uint jh, uint e) {
    if ((rapR & 1u) == 0u) return c;
    const float4 v = celEm(p, hb, jan, ox, oy, jw, jh, e);
    const float wt = pesoDe(v);
    return wt > 0.0 ? lerp(c, v.rgb, wt) : c;
}
[numthreads(8,8,1)] void RjRecorteRuido(uint3 id : SV_DispatchThreadID) {
    if (id.x == 0 && id.y == 0) compartilhado.Store(offTag, 0u);
    if (id.x >= n || id.y >= n) return;
    const uint hb = rBase(quadro & 1u);
    const uint4 H = camada.Load4(hb); const uint3 H2 = camada.Load3(hb + 16u);
    const bool jan = H.x == quadro && H.y == geracao && H2.x <= janela && H2.y <= janela && H2.z >= 1u && H2.z <= 8u;
    const int ox = asint(H.z), oy = asint(H.w); const uint e = max(H2.z, 1u);
    const float passo = float(lado) / float(n);

    const int2 pc = clamp(int2(floor(float2(rx, ry) + (float2(id.xy) + .5) * passo)), int2(0, 0), int2(int(w) - 1, int(h) - 1));
    const float wa = 0.5 * pesoDe(celEm(pc, hb, jan, ox, oy, H2.x, H2.y, e));
    if (passo < 1.0) {
        const float2 uv = (float2(rx, ry) + (float2(id.xy) + .5) * passo) / float2(w, h);
        float3 cb = saidaFsr.SampleLevel(linearClamp, uv, 0).rgb;cb = all(isfinite(cb)) ? cb : float3(0, 0, 0);
        if (jan && (rapR & 1u) != 0u) {
            const float2 t = uv * float2(w, h) - 0.5; const int2 i0 = int2(floor(t)); const float2 f = t - float2(i0);
            float3 dl[4];
            [unroll] for (int k = 0; k < 4; ++k) {
                const int2 pk = clamp(i0 + int2(k & 1, k >> 1), int2(0, 0), int2(int(w) - 1, int(h) - 1));
                const float3 ck = saidaFsr.Load(int3(pk, 0)).rgb;
                dl[k] = all(isfinite(ck)) ? corMix(pk, ck, hb, jan, ox, oy, H2.x, H2.y, e) - ck : float3(0, 0, 0);
            }
            const float3 corr = lerp(lerp(dl[0], dl[1], f.x), lerp(dl[2], dl[3], f.x), f.y);
            if (any(corr != 0.0) && all(isfinite(corr))) cb += corr;
        }
        compartilhado.Store2(offEnt + (id.y * n + id.x) * 8u, uint2(f32tof16(cb.r) | (f32tof16(cb.g) << 16), f32tof16(cb.b) | (f32tof16(wa) << 16)));
        return;
    }
    const uint m = clamp(uint(ceil(passo - 1e-3)), 1u, 8u);
    float3 soma = 0;
    for (uint b = 0; b < m; ++b) for (uint a = 0; a < m; ++a) {
        const float2 f = (float2(id.xy) + (float2(a, b) + .5) / float(m)) * passo;
        const int2 p = clamp(int2(rx, ry) + int2(floor(f)), int2(0, 0), int2(int(w) - 1, int(h) - 1));
        const float3 c = saidaFsr.Load(int3(p, 0)).rgb;
        soma += all(isfinite(c)) ? corMix(p, c, hb, jan, ox, oy, H2.x, H2.y, e) : float3(0, 0, 0);
    }
    soma /= float(m * m);
    compartilhado.Store2(offEnt + (id.y * n + id.x) * 8u, uint2(f32tof16(soma.r) | (f32tof16(soma.g) << 16), f32tof16(soma.b) | (f32tof16(wa) << 16)));
}
