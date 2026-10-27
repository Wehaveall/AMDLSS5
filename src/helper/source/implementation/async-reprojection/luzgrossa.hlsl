cbuffer K : register(b0) {
    uint largura, altura, fatia, fatiaAnterior;
    uint valido, raio, tonemap, passo;
    float alfa, alfaMov, delta, expo;
    float profRel, profAbs, refMov, limite;
    float teto, piso, reservado2, reservado3;
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
#define NR_VALIDO_LUZ(v) (v.w > 0.0)

[numthreads(8, 8, 1)]
void LuzGrossa(uint3 id : SV_DispatchThreadID) {
    if (id.x >= largura || id.y >= altura) return;
    const float4 d0 = atual.Load(int3(id.xy, 0));
    const float4 sem = float4(0.0, 0.0, 0.0, -1.0);
    if (!all(isfinite(d0))) { saida[id.xy] = d0; estado[id.xy] = sem; return; }
    float yb = 0.0, ym = 0.0, n = 0.0;
    float3 cb = 0.0, cm = 0.0;
    const int r = int(raio), st = int(max(passo, 1u));
    for (int dy = -r; dy <= r; ++dy)
    for (int dx = -r; dx <= r; ++dx) {
        const int2 q = clamp(int2(id.xy) + int2(dx, dy) * st, int2(0, 0), int2(int(largura) - 1, int(altura) - 1));
        const float3 bq = base_em(q);
        float3 dq = atual.Load(int3(q, 0)).rgb;
        if (!all(isfinite(dq))) dq = float3(0.0, 0.0, 0.0);
        yb += dot(bq, LUMA); ym += max(dot(bq + dq, LUMA), 0.0); n += 1.0;
        cb += bq; cm += max(bq + dq, 0.0);
    }
    yb /= n; ym /= n; cb /= n; cm /= n;
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
        const float tol = NR_TOL_PROF(profAbs, profRel, za, zb);
        float4 h = sem;
        NR_BILINEAR_COM_ESTADO(h, uvp, tam, NR_VALIDO_LUZ)
        if (NR_MESMA_SUPERFICIE(za, zb, tol) && all(isfinite(h)) && h.w > 0.0) {
            gp = h.x;
            const float mov = refMov > 0.0 ? saturate(length(mv * tam) / refMov) : 0.0;
            const float a0 = lerp(alfa, alfaMov, mov);
            const float lam = delta > 0.0 ? saturate(abs(lb - h.y) / delta) : 0.0;
            a = lerp(a0, 1.0, lam);
        }
    }
    const float gs = lerp(gp, g, a);

    float alvo = teto > 0.0 ? min(gs, teto) : gs;
    if (piso > 0.0) alvo = max(alvo, -piso);
    const float corr = clamp(alvo - g, -limite, limite);
    const float3 bc = base_em(int2(id.xy));
    const float3 m = bc + d0.rgb;
    float3 k3 = exp2(corr).xxx;

    if (reservado2 > 0.0 && piso > 0.0 && teto > 0.0 && all(cb > 1e-3) && all(cm > 1e-3)) {
        const float3 gc = log2(cm / cb);
        k3 = exp2(clamp(clamp(gc, -piso, teto) - gc, -limite, limite));
    }
    const float3 d2 = max(m * k3, 0.0) - bc;
    saida[id.xy] = all(isfinite(d2)) ? float4(d2, d0.a) : d0;
    estado[id.xy] = isfinite(gs) ? float4(g + corr, lb, 0.0, 1.0) : sem;
}
