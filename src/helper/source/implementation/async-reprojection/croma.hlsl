cbuffer K : register(b0) {
    uint largura, altura, fatia, fatiaAnterior;
    uint valido, raio, tonemap, passo;
    float alfa, alfaMov, delta, expo;
    float profRel, profAbs, refMov, limite;
    float sigma, reservado0, reservado1, reservado2;
};
Texture2D<float4> atual : register(t0);
Texture2D<float4> anterior : register(t1);
Texture2DArray<float2> movimento : register(t2);
Texture2DArray<float> profundidade : register(t3);
#if NR_BASE_PRONTA

Texture2D<float4> basePronta : register(t4);
#else
Texture2DArray<float4> cor : register(t4);
#endif
RWTexture2D<float4> estado : register(u0);
RWTexture2D<float4> saida : register(u1);

#include "tom_comum.hlsli"
static const float3 LUMA = NR_LUMA;
#include "prof_comum.hlsli"
#include "base_tom.hlsli"
#include "filtro_comum.hlsli"
#define NR_VALIDO_CROMA(v) (v.z >= 0.0)

#define CROMA_TELHA 16
groupshared float3 telhaBase[CROMA_TELHA * CROMA_TELHA];
groupshared float3 telhaRes[CROMA_TELHA * CROMA_TELHA];
void CromaPixel(uint3 id, const bool lds, int2 origem) {
    if (id.x >= largura || id.y >= altura) return;
    const float4 d0 = atual.Load(int3(id.xy, 0));
    const float4 sem = float4(0.0, 0.0, -1.0, -1.0);
    if (!all(isfinite(d0))) { saida[id.xy] = d0; estado[id.xy] = sem; return; }
    const float3 bc = lds ? telhaBase[(int(id.y) - origem.y) * CROMA_TELHA + (int(id.x) - origem.x)] : base_em(int2(id.xy));

    float3 sb = float3(0.0, 0.0, 0.0), sm = float3(0.0, 0.0, 0.0); float sw = 0.0;
    const int r = int(raio), st = int(max(passo, 1u));
    const float inv = 1.0 / max(sigma * sigma, 1e-6);
    for (int dy = -r; dy <= r; ++dy)
    for (int dx = -r; dx <= r; ++dx) {
        float3 bq, dq;
        if (lds) {
            const int2 t = int2(id.xy) + int2(dx, dy) * st - origem;
            bq = telhaBase[t.y * CROMA_TELHA + t.x]; dq = telhaRes[t.y * CROMA_TELHA + t.x];
        } else {
            const int2 q = clamp(int2(id.xy) + int2(dx, dy) * st, int2(0, 0), int2(int(largura) - 1, int(altura) - 1));
            bq = base_em(q);
            dq = atual.Load(int3(q, 0)).rgb;
            if (!all(isfinite(dq))) dq = float3(0.0, 0.0, 0.0);
        }
        const float3 e = bq - bc;
        const float wq = exp(-dot(e, e) * inv);
        sb += wq * bq; sm += wq * (bq + dq); sw += wq;
    }
    const float3 B = sb / max(sw, 1e-6), M = sm / max(sw, 1e-6);
    const float yb = dot(B, LUMA), ym = dot(M, LUMA);
    if (!(yb > 0.02 && ym > 0.02)) { saida[id.xy] = d0; estado[id.xy] = sem; return; }
    const float3 cb = B / yb;
    const float3 k = M / ym - cb;

    float a = 1.0; float3 kp = k;
    const float2 tam = float2(largura, altura);
    const float2 mv = movimento.Load(int4(id.xy, fatia, 0));
    const float2 uvp = (float2(id.xy) + 0.5) / tam + mv;
    if (valido != 0u && all(isfinite(uvp)) && all(uvp >= 0.0) && all(uvp <= 1.0)) {
        const uint2 o = min(uint2(uvp * tam), uint2(largura - 1, altura - 1));
        const float za = profundidade.Load(int4(id.xy, fatia, 0));
        const float zb = profundidade.Load(int4(o, fatiaAnterior, 0));
        const float tol = NR_TOL_PROF(profAbs, profRel, za, zb);

        float4 h = float4(0.0, 0.0, -1.0, -1.0);
        NR_BILINEAR_COM_ESTADO(h, uvp, tam, NR_VALIDO_CROMA)
        if (NR_MESMA_SUPERFICIE(za, zb, tol) && all(isfinite(h)) && h.z >= 0.0) {
            kp = float3(h.x, -(LUMA.r * h.x + LUMA.b * h.y) / LUMA.g, h.y);
            const float mov = refMov > 0.0 ? saturate(length(mv * tam) / refMov) : 0.0;
            const float a0 = lerp(alfa, alfaMov, mov);
            const float lam = delta > 0.0 ? saturate(length(cb.rb - h.zw) / delta) : 0.0;
            a = lerp(a0, 1.0, lam);
        }
    }
    float3 ks = lerp(kp, k, a);
    if (limite > 0.0) { const float n = length(ks); if (n > limite) ks *= limite / n; }
    const float3 m = bc + d0.rgb;
    const float3 m2 = max(m + max(dot(m, LUMA), 0.0) * (ks - k), 0.0);
    const float3 d2 = m2 - bc;
    saida[id.xy] = all(isfinite(d2)) ? float4(d2, d0.a) : d0;
    estado[id.xy] = all(isfinite(ks)) ? float4(ks.r, ks.b, cb.r, cb.b) : sem;
}
[numthreads(8, 8, 1)]
void Croma(uint3 id : SV_DispatchThreadID) { CromaPixel(id, false, int2(0, 0)); }
[numthreads(8, 8, 1)]
void CromaLds(uint3 id : SV_DispatchThreadID, uint3 grupo : SV_GroupID, uint gi : SV_GroupIndex) {
    const int2 origem = int2(grupo.xy) * 8 - 4;
    for (uint k = gi; k < CROMA_TELHA * CROMA_TELHA; k += 64u) {
        const int2 q = clamp(origem + int2(k % CROMA_TELHA, k / CROMA_TELHA), int2(0, 0), int2(int(largura) - 1, int(altura) - 1));
        telhaBase[k] = base_em(q);
        float3 dq = atual.Load(int3(q, 0)).rgb;
        if (!all(isfinite(dq))) dq = float3(0.0, 0.0, 0.0);
        telhaRes[k] = dq;
    }
    GroupMemoryBarrierWithGroupSync();
    CromaPixel(id, true, origem);
}
