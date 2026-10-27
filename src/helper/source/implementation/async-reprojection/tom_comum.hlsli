#ifndef NR_TOM_COMUM_HLSLI
#define NR_TOM_COMUM_HLSLI
float tmf(float v) { v = max(v, 0.0); v = v / (1.0 + v); return v <= 0.0031308 ? v * 12.92 : 1.055 * pow(v, 1.0 / 2.4) - 0.055; }
float tmi(float v) { v = saturate(v); v = v <= 0.04045 ? v / 12.92 : pow((v + 0.055) / 1.055, 2.4); v = min(v, 0.95); return v / (1.0 - v); }
float3 tmf3(float3 v) { return float3(tmf(v.r), tmf(v.g), tmf(v.b)); }
float3 tmi3(float3 v) { return float3(tmi(v.r), tmi(v.g), tmi(v.b)); }

#define NR_LUMA float3(0.2126, 0.7152, 0.0722)
#define NR_COMPOR_TOM(e, c, d, expo) { const float3 sx = (c) * (expo), bx = tmf3(sx); e = max(sx + tmi3(bx + (d)) - tmi3(bx), 0.0) / (expo); }
#endif
