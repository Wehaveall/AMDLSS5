cbuffer K : register(b0) {
    uint largura, altura, cl, ch;
    uint bloco, raio, cor, pad1;
    float cima, baixo, forca, eps;
};
RWTexture2D<float4> saida : register(u0);
RWTexture2D<float4> ref : register(u1);
RWTexture2D<float4> sai : register(u2);
RWTexture2D<float4> razao : register(u3);

static const float3 LUMA = float3(0.2126, 0.7152, 0.0722);

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

[numthreads(8, 8, 1)]
void TLMedeSai(uint3 id : SV_DispatchThreadID) {
    if (id.x >= cl || id.y >= ch) return;
    sai[id.xy] = media_bloco(id.xy);
}

float trava(float a, float b) {
    float k = 1.0;
    const float hi = a * (1.0 + cima), lo = a / (1.0 + baixo);
    if (b > hi) k = (hi + eps) / (b + eps);
    else if (b < lo) k = (lo + eps) / (b + eps);
    k = clamp(lerp(1.0, k, forca), 0.25, 4.0);
    return isfinite(k) ? k : 1.0;
}

[numthreads(8, 8, 1)]
void TLRazao(uint3 id : SV_DispatchThreadID) {
    if (id.x >= cl || id.y >= ch) return;
    const int r = int(raio);
    float4 a = 0.0, b = 0.0;
    for (int dy = -r; dy <= r; ++dy)
    for (int dx = -r; dx <= r; ++dx) {
        const int2 q = clamp(int2(id.xy) + int2(dx, dy), int2(0, 0), int2(int(cl) - 1, int(ch) - 1));
        a += ref[q]; b += sai[q];
    }
    float3 k;
    if (cor != 0u) k = float3(trava(a.r, b.r), trava(a.g, b.g), trava(a.b, b.b));
    else k = trava(a.a, b.a).xxx;
    razao[id.xy] = float4(k, 1.0);
}

[numthreads(8, 8, 1)]
void TLAplica(uint3 id : SV_DispatchThreadID) {
    if (id.x >= largura || id.y >= altura) return;
    const float2 s = (float2(id.xy) + 0.5) / float(bloco) - 0.5;
    const int2 b0 = int2(floor(s)); const float2 f = s - float2(b0);
    const int2 m = int2(int(cl) - 1, int(ch) - 1);
    const float3 k00 = razao[clamp(b0, int2(0, 0), m)].rgb, k10 = razao[clamp(b0 + int2(1, 0), int2(0, 0), m)].rgb;
    const float3 k01 = razao[clamp(b0 + int2(0, 1), int2(0, 0), m)].rgb, k11 = razao[clamp(b0 + int2(1, 1), int2(0, 0), m)].rgb;
    const float3 k = lerp(lerp(k00, k10, f.x), lerp(k01, k11, f.x), f.y);
    if (all(k == 1.0)) return;
    const float4 v = saida[id.xy];
    if (!all(isfinite(v))) return;
    saida[id.xy] = float4(v.rgb * k, v.a);
}
