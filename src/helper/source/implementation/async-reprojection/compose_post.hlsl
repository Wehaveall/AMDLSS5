float3 ResiduoAmpliado(uint2 p);
#define NR_RESIDUAL_AT(p) ResiduoAmpliado(p)
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
