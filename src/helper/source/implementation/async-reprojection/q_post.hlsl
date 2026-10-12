cbuffer K : register(b0) { uint w, h, fatia, bits; float gHi, gHiNeg, gLo, gLoNeg; float movA, movB, gradA, gradB; float profR, profA, profB, confP; float edgeA, edgeB, fcx, fcy; float fax, fay, fniv, pad2;
#ifdef NR_QMAPA_Q4
    float movR, expo; uint tom; float stopsMax;
#endif
}
Texture2D<float4> residuo : register(t0);
Texture2DArray<float2> movimento : register(t1);
Texture2DArray<float> prof : register(t2);
Texture2DArray<float4> cor : register(t3);
RWTexture2D<float4> saida : register(u0);
static const float3 LUMA = float3(.2126, .7152, .0722);
static const float EPS = 0.002;

float3 Res(int2 q) { const float3 v = residuo.Load(int3(clamp(q, int2(0, 0), int2(int(w) - 1, int(h) - 1)), 0)).rgb; return all(isfinite(v)) ? v : float3(0, 0, 0); }
float2 Mv(int2 q) { const float2 v = movimento.Load(int4(clamp(q, int2(0, 0), int2(int(w) - 1, int(h) - 1)), fatia, 0)); return all(isfinite(v)) ? v * float2(w, h) : float2(0, 0); }
float Lz(int2 q) { const float z = prof.Load(int4(clamp(q, int2(0, 0), int2(int(w) - 1, int(h) - 1)), fatia, 0)); return isfinite(z) ? log(max(abs(z), 1e-7)) : 0.0; }
float Ly(int2 q) { const float3 c = cor.Load(int4(clamp(q, int2(0, 0), int2(int(w) - 1, int(h) - 1)), fatia, 0)).rgb; const float y = dot(c, LUMA); return isfinite(y) ? log(max(y, 0.0) + 1e-3) : 0.0; }
#ifdef NR_QMAPA_Q4
float Tmf(float v) { v = max(v, 0.0); v = v / (1.0 + v); return v <= 0.0031308 ? v * 12.92 : 1.055 * pow(v, 1.0 / 2.4) - 0.055; }
float Tmi(float v) { v = saturate(v); v = v <= 0.04045 ? v / 12.92 : pow((v + 0.055) / 1.055, 2.4); v = min(v, 0.95); return v / (1.0 - v); }

float GanhoNegStops(float bl, float dl, float g) {
    if (!(dl < -1e-6) || g <= 1.0) return g;
    const bool t = (tom & 1u) != 0u;
    const float yp = t ? Tmi(bl) : saturate(bl);
    const float ym = t ? Tmi(bl + dl) : saturate(bl + dl);
    if (!(yp > 1e-7)) return 1.0;
    const float r = clamp(ym / yp, 1e-6, 1.0);

    const float s = log2(r), ex = (g - 1.0) * s;
    const float yt = yp * exp2(s + (stopsMax > 0.0 ? -stopsMax * tanh(-ex / stopsMax) : ex));
    const float bt = t ? Tmf(yt) : yt;
    const float k = (bt - bl) / dl;
    return isfinite(k) ? clamp(k, 1.0, g) : 1.0;
}
#endif

float Ganho(float3 banda, float gp, float gn, float m, float mn) { return lerp(1.0 + (gn - 1.0) * m * mn, 1.0 + (gp - 1.0) * m, smoothstep(-EPS, EPS, dot(banda, LUMA))); }

[numthreads(8, 8, 1)]
void QMapa(uint3 id : SV_DispatchThreadID) {
    if (id.x >= w || id.y >= h) return;
    const int2 p = int2(id.xy);
    const float4 c = residuo.Load(int3(p, 0));
    if (!all(isfinite(c))) { saida[id.xy] = float4(0, 0, 0, 0); return; }
    float3 lo = float3(0, 0, 0);
    if (bits & 1u) {
        const int st = (bits & 512u) ? 2 : 1;
        const float k[5] = {1.0, 4.0, 6.0, 4.0, 1.0};
        [unroll] for (int j = -2; j <= 2; ++j) {
            float3 l = float3(0, 0, 0);
            [unroll] for (int i = -2; i <= 2; ++i) l += Res(p + int2(i, j) * st) * k[i + 2];
            lo += l * k[j + 2];
        }
        lo /= 256.0;
    }
    const float3 hi = c.rgb - lo;
    float mHi = 1.0, mLo = 1.0, mNeg = 1.0;
    if (bits & 6u) {
#ifdef NR_QMAPA_Q4
        float vm = length(Mv(p));
        if ((bits & 8192u) && movR > 0.0) {
            const float2 dv[8] = {float2(1, 0), float2(-1, 0), float2(0, 1), float2(0, -1), float2(.7071, .7071), float2(-.7071, .7071), float2(.7071, -.7071), float2(-.7071, -.7071)};
            [unroll] for (int r = 1; r <= 4; ++r) {
                const float d = movR * float(r) * 0.25;
                [unroll] for (int k3 = 0; k3 < 8; ++k3) vm = max(vm, length(Mv(p + int2(round(dv[k3] * d)))));
            }
        }
        float m = smoothstep(movA, movB, vm);
#else
        float m = smoothstep(movA, movB, length(Mv(p)));
#endif
        if (bits & 1024u) {
            float L[25];
            [unroll] for (int j = 0; j < 5; ++j) [unroll] for (int i = 0; i < 5; ++i) L[j * 5 + i] = Ly(p + int2(i - 2, j - 2));
            float e = 0.0;
            [unroll] for (int j2 = 1; j2 <= 3; ++j2) [unroll] for (int i2 = 1; i2 <= 3; ++i2)
                e = max(e, length(float2(L[j2 * 5 + i2 + 1] - L[j2 * 5 + i2 - 1], L[(j2 + 1) * 5 + i2] - L[(j2 - 1) * 5 + i2]) * 0.5));
            m *= smoothstep(edgeA, edgeB, e);
        }
        m = 1.0 - m;
        if (bits & 2u) mHi *= m;
        if (bits & 4u) mLo *= m;
    }
    if (bits & 24u) {
        const float2 v0 = Mv(p); float g = 0.0;
        [unroll] for (int j = -1; j <= 1; ++j)
        [unroll] for (int i = -1; i <= 1; ++i) if (i != 0 || j != 0) g = max(g, length(Mv(p + int2(i, j) * 2) - v0));
        const float m = 1.0 - smoothstep(gradA, gradB, g);
        if (bits & 8u) mHi *= m;
        if (bits & 16u) mLo *= m;
    }
    if ((bits & 2144u) && profR > 0.0) {
        const float z0 = Lz(p); float prox = 0.0;
        const float2 dirs[8] = {float2(1, 0), float2(-1, 0), float2(0, 1), float2(0, -1), float2(.7071, .7071), float2(-.7071, .7071), float2(.7071, -.7071), float2(-.7071, -.7071)};
        [unroll] for (int r = 1; r <= 4; ++r) {
            const float d = profR * float(r) * 0.25;
            const float wd = 1.0 - smoothstep(0.5 * profR, 1.25 * profR, d);
            [unroll] for (int k2 = 0; k2 < 8; ++k2) {
                const float dz = abs(Lz(p + int2(round(dirs[k2] * d))) - z0);
                prox = max(prox, smoothstep(profA, profB, dz) * wd);
            }
        }
        const float m = 1.0 - prox;
        if (bits & 2048u) mNeg = m;
        else { if (bits & 32u) mHi *= m; if (bits & 64u) mLo *= m; }
    }
    if (bits & 128u) { const float m = pow(saturate(c.a), max(confP, 0.0)); mHi *= m; mLo *= m; }

    if ((bits & 4096u) && fniv > 0.0) {
        const float r = length((float2(p) + 0.5 - float2(fcx, fcy)) / float2(fax, fay));
        const float m = 1.0 - fniv * (1.0 - smoothstep(1.0, 1.25, r));
        mHi *= m; mLo *= m;
    }
#ifdef NR_QMAPA_Q4
    float gHiN = gHiNeg, gLoN = gLoNeg;
    if (bits & 16384u) {
        const float3 cb = cor.Load(int4(p, fatia, 0)).rgb;
        const float3 b3 = (tom & 1u) ? float3(Tmf(cb.r * expo), Tmf(cb.g * expo), Tmf(cb.b * expo)) : saturate(cb);
        const float bl = all(isfinite(b3)) ? dot(b3, LUMA) : 0.0;
        gHiN = GanhoNegStops(bl, dot(hi, LUMA), gHiNeg);
        gLoN = GanhoNegStops(bl + dot(hi, LUMA), dot(lo, LUMA), gLoNeg);
    }
    const float gh = Ganho(hi, gHi, gHiN, mHi, mNeg);
    const float gl = Ganho(lo, gLo, gLoN, mLo, mNeg);
#else
    const float gh = Ganho(hi, gHi, gHiNeg, mHi, mNeg);
    const float gl = Ganho(lo, gLo, gLoNeg, mLo, mNeg);
#endif
    const float3 o = lo * gl + hi * gh;
    saida[id.xy] = all(isfinite(o)) ? float4(o, c.a) : float4(c.rgb, c.a);
}
