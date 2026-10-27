#ifndef NR_BASE_TOM_HLSLI
#define NR_BASE_TOM_HLSLI
#if NR_BASE_PRONTA
float3 base_em(int2 q) { return basePronta.Load(int3(q, 0)).rgb; }
#else
float3 base_em(int2 q) {
    const float3 c = cor.Load(int4(q, fatia, 0)).rgb;
    if (!all(isfinite(c))) return float3(0.0, 0.0, 0.0);
    return tonemap != 0u ? float3(tmf(c.r * expo), tmf(c.g * expo), tmf(c.b * expo)) : saturate(c);
}
#endif
#endif
