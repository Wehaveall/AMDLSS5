cbuffer K : register(b0) {
    uint largura, altura, fatia, fatiaAnterior;
    uint valido, tonemap, escuro, banda;
    float forca, limiarEntrada, limiarResiduo, expo;
    float profRel, profAbs, refMov, escuroBaixo;
    float escuroAlto, reservado0, reservado1, reservado2;
};
Texture2D<float4> atual : register(t0);
Texture2D<float4> anterior : register(t1);
Texture2DArray<float2> movimento : register(t2);
Texture2DArray<float> profundidade : register(t3);
Texture2DArray<float4> cor : register(t4);
RWTexture2D<float4> estado : register(u0);
RWTexture2D<float4> saida : register(u1);
SamplerState linear0 : register(s0);

#include "tom_comum.hlsli"
static const float3 LUMA = NR_LUMA;
#include "prof_comum.hlsli"
float3 tom(float3 c) {
    if (!all(isfinite(c))) return float3(0.0, 0.0, 0.0);
    return tonemap != 0u ? float3(tmf(c.r * expo), tmf(c.g * expo), tmf(c.b * expo)) : saturate(c);
}
float maxc(float3 v) { return max(v.r, max(v.g, v.b)); }

float4 catmull(float2 pos) {
    const float2 s = pos - 0.5; const int2 b = int2(floor(s)); const float2 f = s - float2(b);
    const float2 f2 = f * f, f3 = f2 * f;
    const float2 w0 = -0.5 * f3 + f2 - 0.5 * f, w1 = 1.5 * f3 - 2.5 * f2 + 1.0, w2 = -1.5 * f3 + 2.0 * f2 + 0.5 * f, w3 = 0.5 * f3 - 0.5 * f2;
    const float wx[4] = { w0.x, w1.x, w2.x, w3.x }, wy[4] = { w0.y, w1.y, w2.y, w3.y };
    float4 acc = float4(0.0, 0.0, 0.0, 0.0);
    [unroll] for (int j = 0; j < 4; ++j)
    [unroll] for (int i = 0; i < 4; ++i) {
        const int2 q = clamp(b + int2(i - 1, j - 1), int2(0, 0), int2(int(largura) - 1, int(altura) - 1));
        float4 v = anterior.Load(int3(q, 0));
        if (!all(isfinite(v))) v = float4(0.0, 0.0, 0.0, 0.0);
        acc += (wx[i] * wy[j]) * v;
    }
    return acc;
}

float4 correcao(int2 p) {
    const float4 d0 = atual.Load(int3(p, 0));
    if (!all(isfinite(d0))) return float4(0.0, 0.0, 0.0, 0.0);
    const float2 tam = float2(largura, altura);
    const float2 mv = movimento.Load(int4(p, fatia, 0));
    const float2 uvp = (float2(p) + 0.5) / tam + mv;
    if (!all(isfinite(uvp)) || any(uvp < 0.0) || any(uvp > 1.0)) return float4(0.0, 0.0, 0.0, 0.0);
    const uint2 o = min(uint2(uvp * tam), uint2(largura - 1, altura - 1));
    const float za = profundidade.Load(int4(p, fatia, 0));
    const float zb = profundidade.Load(int4(o, fatiaAnterior, 0));
    const float tol = NR_TOL_PROF(profAbs, profRel, za, zb);
    if (!NR_MESMA_SUPERFICIE(za, zb, tol)) return float4(0.0, 0.0, 0.0, 0.0);
    const float4 h = catmull(uvp * tam);
    if (!all(isfinite(h)) || h.a < 0.999) return float4(0.0, 0.0, 0.0, 0.0);
    const float3 bc = tom(cor.Load(int4(p, fatia, 0)).rgb);
    const float3 bp = tom(cor.SampleLevel(linear0, float3(uvp, float(fatiaAnterior)), 0).rgb);
    const float dE = maxc(abs(bc - bp)), dR = maxc(abs(d0.rgb - h.rgb));
    float w = forca * saturate(1.0 - dE / max(limiarEntrada, 1e-6));
    if (limiarResiduo > 0.0) w *= saturate(1.0 - dR / limiarResiduo);
    if (refMov > 0.0) w *= saturate(1.0 - length(mv * tam) / refMov);
    if (escuro != 0u && dot(bp + h.rgb, LUMA) < escuroBaixo && dot(bc + d0.rgb, LUMA) > escuroAlto) w = 0.0;
    return float4(w * (h.rgb - d0.rgb), w);
}

[numthreads(8, 8, 1)]
void Suave(uint3 id : SV_DispatchThreadID) {
    if (id.x >= largura || id.y >= altura) return;
    const float4 d0 = atual.Load(int3(id.xy, 0));
    if (!all(isfinite(d0))) { saida[id.xy] = d0; estado[id.xy] = float4(0.0, 0.0, 0.0, 0.0); return; }
    if (valido == 0u) { saida[id.xy] = d0; estado[id.xy] = float4(d0.rgb, 1.0); return; }
    float3 c;
    if (banda != 0u) {
        c = float3(0.0, 0.0, 0.0);
        [unroll] for (int dy = -1; dy <= 1; ++dy)
        [unroll] for (int dx = -1; dx <= 1; ++dx) {
            const int2 q = clamp(int2(id.xy) + int2(dx, dy), int2(0, 0), int2(int(largura) - 1, int(altura) - 1));
            c += ((dx == 0 ? 0.5 : 0.25) * (dy == 0 ? 0.5 : 0.25)) * correcao(q).rgb;
        }
    } else c = correcao(int2(id.xy)).rgb;
    const float3 r = d0.rgb + c;
    if (!all(isfinite(r))) { saida[id.xy] = d0; estado[id.xy] = float4(d0.rgb, 1.0); return; }
    saida[id.xy] = float4(r, d0.a);
    estado[id.xy] = float4(r, 1.0);
}
