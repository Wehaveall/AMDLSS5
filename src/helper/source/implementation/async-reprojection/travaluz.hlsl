cbuffer K : register(b0) {
    uint largura, altura, cl, ch;
    uint bloco, raio, cor, pad1;
    float cima, baixo, forca, eps;
};
RWTexture2D<float4> saida : register(u0);
RWTexture2D<float4> ref : register(u1);

#include "tom_comum.hlsli"
static const float3 LUMA = NR_LUMA;

float4 media_bloco(uint2 c) {
    const uint2 o = c * bloco;
    float4 s = 0.0; float n = 0.0;
    for (uint y = 0; y < bloco; ++y)
    for (uint x = 0; x < bloco; ++x) {
        const uint2 p = o + uint2(x, y);
        if (p.x >= largura || p.y >= altura) continue;
        const float3 v = max(saida[p].rgb, 0.0);
        if (!all(isfinite(v))) continue;
        s += float4(v, dot(v, LUMA)); n += 1.0;
    }
    return n > 0.0 ? s / n : 0.0;
}

[numthreads(8, 8, 1)]
void TLMedeRef(uint3 id : SV_DispatchThreadID) {
    if (id.x >= cl || id.y >= ch) return;
    ref[id.xy] = media_bloco(id.xy);
}
