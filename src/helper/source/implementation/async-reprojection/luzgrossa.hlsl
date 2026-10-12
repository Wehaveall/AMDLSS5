cbuffer K : register(b0) {
    uint largura, altura, fatia, fatiaAnterior;
    uint valido, raio, tonemap, passo;
    float alfa, alfaMov, delta, expo;
    float profRel, profAbs, refMov, limite;
    float reservado0, reservado1, reservado2, reservado3;
};
Texture2D<float4> atual : register(t0);
Texture2D<float4> anterior : register(t1);
Texture2DArray<float2> movimento : register(t2);
Texture2DArray<float> profundidade : register(t3);
Texture2DArray<float4> cor : register(t4);
RWTexture2D<float4> estado : register(u0);
RWTexture2D<float4> saida : register(u1);

static const float3 LUMA = float3(0.2126, 0.7152, 0.0722);
float tmf(float v) { v = max(v, 0.0); v = v / (1.0 + v); return v <= 0.0031308 ? v * 12.92 : 1.055 * pow(v, 1.0 / 2.4) - 0.055; }
float3 base_em(int2 q) {
    const float3 c = cor.Load(int4(q, fatia, 0)).rgb;
    if (!all(isfinite(c))) return float3(0.0, 0.0, 0.0);
    return tonemap != 0u ? float3(tmf(c.r * expo), tmf(c.g * expo), tmf(c.b * expo)) : saturate(c);
}

[numthreads(8, 8, 1)]
void LuzGrossa(uint3 id : SV_DispatchThreadID) {
    if (id.x >= largura || id.y >= altura) return;
    const float4 d0 = atual.Load(int3(id.xy, 0));
    const float4 sem = float4(0.0, 0.0, 0.0, -1.0);
    if (!all(isfinite(d0))) { saida[id.xy] = d0; estado[id.xy] = sem; return; }
    float yb = 0.0, ym = 0.0, n = 0.0;
    const int r = int(raio), st = int(max(passo, 1u));
    for (int dy = -r; dy <= r; ++dy)
    for (int dx = -r; dx <= r; ++dx) {
        const int2 q = clamp(int2(id.xy) + int2(dx, dy) * st, int2(0, 0), int2(int(largura) - 1, int(altura) - 1));
        const float3 bq = base_em(q);
        float3 dq = atual.Load(int3(q, 0)).rgb;
        if (!all(isfinite(dq))) dq = float3(0.0, 0.0, 0.0);
        yb += dot(bq, LUMA); ym += max(dot(bq + dq, LUMA), 0.0); n += 1.0;
    }
    yb /= n; ym /= n;
    if (!(yb > 1e-3 && ym > 1e-3)) { saida[id.xy] = d0; estado[id.xy] = sem; return; }
    const float g = log2(ym / yb), lb = log2(yb);
    float a = 1.0, gp = g;
    const float2 tam = float2(largura, altura);
    const float2 mv = movimento.Load(int4(id.xy, fatia, 0));
    const float2 uvp = (float2(id.xy) + 0.5) / tam + mv;
    if (valido != 0u && all(isfinite(uvp)) && all(uvp >= 0.0) && all(uvp <= 1.0)) {
        const uint2 o = min(uint2(uvp * tam), uint2(largura - 1, altura - 1));
        const float za = profundidade.Load(int4(id.xy, fatia, 0));
        const float zb = profundidade.Load(int4(o, fatiaAnterior, 0));
        const float tol = max(profAbs, profRel * max(abs(za), abs(zb)));
        float4 h = sem;
        {
            const float2 s = uvp * tam - 0.5; const int2 b0 = int2(floor(s)); const float2 f = s - float2(b0);
            float4 acc = float4(0.0, 0.0, 0.0, 0.0); float wsum = 0.0;
            [unroll] for (int j = 0; j < 4; ++j) {
                const int2 off = int2(j & 1, j >> 1);
                const int2 q = clamp(b0 + off, int2(0, 0), int2(int(largura) - 1, int(altura) - 1));
                const float4 v = anterior.Load(int3(q, 0));
                const float wv = (off.x ? f.x : 1.0 - f.x) * (off.y ? f.y : 1.0 - f.y);
                if (all(isfinite(v)) && v.w > 0.0 && wv > 0.0) { acc += wv * v; wsum += wv; }
            }
            if (wsum > 0.25) h = acc / wsum;
        }
        if (isfinite(za) && isfinite(zb) && tol > 0.0 && abs(za - zb) <= tol && all(isfinite(h)) && h.w > 0.0) {
            gp = h.x;
            const float mov = refMov > 0.0 ? saturate(length(mv * tam) / refMov) : 0.0;
            const float a0 = lerp(alfa, alfaMov, mov);
            const float lam = delta > 0.0 ? saturate(abs(lb - h.y) / delta) : 0.0;
            a = lerp(a0, 1.0, lam);
        }
    }
    const float gs = lerp(gp, g, a);
    const float corr = clamp(gs - g, -limite, limite);
    const float3 bc = base_em(int2(id.xy));
    const float3 m = bc + d0.rgb;
    const float3 d2 = max(m * exp2(corr), 0.0) - bc;
    saida[id.xy] = all(isfinite(d2)) ? float4(d2, d0.a) : d0;
    estado[id.xy] = isfinite(gs) ? float4(g + corr, lb, 0.0, 1.0) : sem;
}
