cbuffer K : register(b0) { uint w, h, fatia, tone; float expo, forca, raio; uint qw, qh, pad0, pad1, pad2; }
Texture2D<float4> residuo : register(t0);
Texture2DArray<float4> cor : register(t1);
Texture2DArray<float> prof : register(t2);
Texture2D<float4> mom : register(t3);
Texture2D<float4> refs : register(t4);
RWTexture2D<float4> momOut : register(u0);
RWTexture2D<float4> refOut : register(u1);
RWTexture2D<float4> saida : register(u2);
static const float3 LUMA = float3(.2126, .7152, .0722);
static const float PISO = 1.0 / 512.0;
float tmf(float v) { v = max(v, 0.0); v = v / (1.0 + v); return v <= 0.0031308 ? v * 12.92 : 1.055 * pow(v, 1.0 / 2.4) - 0.055; }
float tmi(float v) { v = saturate(v); v = v <= 0.04045 ? v / 12.92 : pow((v + 0.055) / 1.055, 2.4); v = min(v, 0.95); return v / (1.0 - v); }
float3 tmf3(float3 v) { return float3(tmf(v.r), tmf(v.g), tmf(v.b)); }
float3 tmi3(float3 v) { return float3(tmi(v.r), tmi(v.g), tmi(v.b)); }

void Partes(uint2 p, float3 d, out float3 b, out float3 o, out float3 m) {
    const float3 c = cor.Load(int4(p, fatia, 0)).rgb;
    if (tone & 1u) { const float3 s = max(c * expo, 0.0); b = tmf3(s); o = tmi3(b); m = tmi3(b + d); }
    else { b = saturate(c); o = b; m = saturate(b + d); }
}
float LnG(uint2 p) {
    const float3 d = residuo[p].rgb;
    if (!all(isfinite(d)) || all(d == float3(0, 0, 0))) return 0.0;
    float3 b, o, m; Partes(p, d, b, o, m);
    const float g = log((dot(m, LUMA) + PISO) / (dot(o, LUMA) + PISO));
    return isfinite(g) ? g : 0.0;
}

float PesoProf(float za, float zb) {
    if (!isfinite(za) || !isfinite(zb)) return 0.0;
    const float rel = abs(za - zb) / max(max(abs(za), abs(zb)), 1e-6);
    return saturate(1.0 - (rel - 0.05) / 0.10);
}
float ProfPixel(int2 p) { return prof.Load(int4(clamp(p, int2(0, 0), int2(int(w) - 1, int(h) - 1)), fatia, 0)); }
float ProfTexel(int2 q) { return prof.Load(int4(min(uint2(q) * 4u + 2u, uint2(w - 1, h - 1)), fatia, 0)); }

float2 Grad(float z0, float xp, float xm, float yp, float ym, bool bx0, bool bx1, bool by0, bool by1) {
    const float ax = xp - z0, bx = z0 - xm, ay = yp - z0, by = z0 - ym;
    float gx = bx0 ? ax : bx1 ? bx : (abs(ax) < abs(bx) ? ax : bx), gy = by0 ? ay : by1 ? by : (abs(ay) < abs(by) ? ay : by);
    if (!isfinite(gx)) gx = 0.0;
    if (!isfinite(gy)) gy = 0.0;
    return float2(gx, gy);
}
float2 GradTexel(int2 q, float z0) {
    const int2 m = int2(qw, qh) - 1;
    return Grad(z0, ProfTexel(min(q + int2(1, 0), m)), ProfTexel(max(q - int2(1, 0), int2(0, 0))), ProfTexel(min(q + int2(0, 1), m)), ProfTexel(max(q - int2(0, 1), int2(0, 0))),
                q.x <= 0, q.x >= m.x, q.y <= 0, q.y >= m.y);
}
float2 GradPixel(int2 p, float z0) {
    const int2 m = int2(w, h) - 1;
    return Grad(z0, ProfPixel(p + int2(1, 0)), ProfPixel(p - int2(1, 0)), ProfPixel(p + int2(0, 1)), ProfPixel(p - int2(0, 1)),
                p.x <= 0, p.x >= m.x, p.y <= 0, p.y >= m.y);
}

float PesoCamada(float zc, float2 g, float2 o, float zt) {
    const float zp = zc + dot(g, o);
    if (!isfinite(zt) || !isfinite(zp)) return 0.0;
    return 1.0 - smoothstep(0.02, 0.08, abs(zt - zp) / max(max(abs(zt), abs(zp)), 1e-9));
}
static const int2 D8I[8] = { int2(1, 0), int2(-1, 0), int2(0, 1), int2(0, -1), int2(1, 1), int2(-1, 1), int2(1, -1), int2(-1, -1) };
static const float2 D8F[8] = { float2(1, 0), float2(-1, 0), float2(0, 1), float2(0, -1), float2(0.70710678, 0.70710678), float2(-0.70710678, 0.70710678),
                               float2(0.70710678, -0.70710678), float2(-0.70710678, -0.70710678) };
static const float RAIOS[9] = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 8.0, 10.0, 12.0 };
[numthreads(8, 8, 1)] void HaloPostMomentos(uint3 id : SV_DispatchThreadID) {
    if (id.x >= qw || id.y >= qh) return;
    const int2 q = int2(id.xy);
    const float zc = ProfTexel(q);
    float sg = 0.0, sw = 0.0, st = 0.0, n = 0.0;
    for (uint y = 0; y < 4u; ++y) for (uint x = 0; x < 4u; ++x) {
        const uint2 p = id.xy * 4u + uint2(x, y);
        if (p.x >= w || p.y >= h) continue;
        const float g = LnG(p);
        st += g; n += 1.0;
        const float wp = PesoProf(zc, prof.Load(int4(p, fatia, 0)));
        if (wp > 0.0) { sg += wp * g; sw += wp; }
    }
    const float media = sw > 0.0 ? sg / sw : (n > 0.0 ? st / n : 0.0);

    const float Rt = raio / 4.0;
    float f = 0.0;
    if (Rt > 0.0 && isfinite(zc)) {
        const float2 g2 = GradTexel(q, zc);
        [unroll] for (int k = 0; k < 8; ++k) {
            const float passo = k < 4 ? 1.0 : 1.41421356;
            [loop] for (int r = 1; float(r) * passo <= Rt + 0.5; ++r) {
                const int2 o = D8I[k] * r, t = q + o;
                if (any(t < 0) || t.x >= int(qw) || t.y >= int(qh)) break;
                const float zq = ProfTexel(t);
                if (!isfinite(zq)) continue;
                const float zp = zc + dot(g2, float2(o));
                const float s = (zq - zp) / max(max(abs(zq), abs(zp)), 1e-9);
                f = max(f, smoothstep(0.02, 0.08, s) * saturate((Rt + 1.0 - float(r) * passo) / max(Rt, 1e-4)));
            }
        }
    }
    momOut[id.xy] = float4(media, f, 0.0, 0.0);
}
[numthreads(8, 8, 1)] void HaloPostRef(uint3 id : SV_DispatchThreadID) {
    if (id.x >= qw || id.y >= qh) return;
    const int2 q = int2(id.xy), qn = int2(qw, qh);
    const float proprio = mom.Load(int3(q, 0)).x;
    const float zc = ProfTexel(q);
    if (!isfinite(zc) || !(raio > 0.0)) { refOut[id.xy] = float4(proprio, proprio, 0.0, 0.0); return; }
    const float2 g2 = GradTexel(q, zc);
    float v[32], pv[32]; float W = 0.0;
    [unroll] for (int k = 0; k < 8; ++k) {
        [unroll] for (int j = 0; j < 4; ++j) {
            const int n = k * 4 + j; v[n] = 0.0; pv[n] = 0.0;
            const int2 t = q + int2(round(D8F[k] * ((1.5 + 0.5 * float(j)) * raio / 4.0)));
            if (any(t < 0) || any(t >= qn)) continue;
            const float2 mt = mom.Load(int3(t, 0)).xy;
            const float wl = PesoCamada(zc, g2, float2(t - q), ProfTexel(t)) * (1.0 - saturate(mt.y));
            if (wl > 0.0 && isfinite(mt.x)) { v[n] = mt.x; pv[n] = wl; W += wl; }
        }
    }
    if (!(W > 1e-4)) { refOut[id.xy] = float4(proprio, proprio, 0.0, 0.0); return; }
    float acc = 0.0, accW = 0.0;
    [loop] for (int i = 0; i < 32; ++i) {
        if (!(pv[i] > 0.0)) continue;
        float abaixo = 0.0;
        [unroll] for (int j2 = 0; j2 < 32; ++j2) abaixo += pv[j2] * (v[j2] < v[i] ? 1.0 : (v[j2] == v[i] ? 0.5 : 0.0));
        const float posto = abaixo / W;
        const float wi = pv[i] * smoothstep(0.1, 0.3, posto) * (1.0 - smoothstep(0.7, 0.9, posto));
        acc += wi * v[i]; accW += wi;
    }
    const float r = accW > 1e-6 ? acc / accW : proprio;
    refOut[id.xy] = float4(isfinite(r) ? r : proprio, proprio, accW > 1e-6 ? saturate(W / 4.0) : 0.0, 0.0);
}
[numthreads(8, 8, 1)] void HaloPostAplicar(uint3 id : SV_DispatchThreadID) {
    if (id.x >= w || id.y >= h) return;
    const float4 rd = residuo[id.xy];
    saida[id.xy] = rd;
    const float3 d = rd.rgb;
    if (!(forca > 0.0) || !(raio > 0.0) || !all(isfinite(d)) || all(d == float3(0, 0, 0))) return;
    const int2 p = int2(id.xy);
    const float z0 = ProfPixel(p);
    if (!isfinite(z0)) return;

    const float2 g = GradPixel(p, z0);
    float f = 0.0;
    [loop] for (int k = 0; k < 8; ++k) {
        const float passo = k < 4 ? 1.0 : 1.41421356;
        [loop] for (int i = 0; i < 9; ++i) {
            const int r = max(1, int(round(RAIOS[i] * raio / 12.0)));
            const float dist = float(r) * passo;
            if (dist > raio + 0.5) break;
            const int2 o = D8I[k] * r, t = p + o;
            if (any(t < 0) || t.x >= int(w) || t.y >= int(h)) break;
            const float zq = prof.Load(int4(t, fatia, 0));
            if (!isfinite(zq)) continue;
            const float zp = z0 + dot(g, float2(o));
            const float s = (zq - zp) / max(max(abs(zq), abs(zp)), 1e-9);
            f = max(f, smoothstep(0.02, 0.08, s) * saturate((raio + 1.0 - dist) / raio));
        }
    }
    if (!(f > 0.0)) return;

    const int2 m = int2(qw, qh) - 1;
    const float2 s2 = clamp((float2(p) + 0.5) / 4.0 - 0.5, float2(0.0, 0.0), float2(m));
    const int2 b0 = min(int2(floor(s2)), m); const float2 fr = s2 - float2(b0); const int2 e = min(b0 + 1, m);
    const int2 t4[4] = { b0, int2(e.x, b0.y), int2(b0.x, e.y), e };
    const float wb[4] = { (1.0 - fr.x) * (1.0 - fr.y), fr.x * (1.0 - fr.y), (1.0 - fr.x) * fr.y, fr.x * fr.y };
    float3 acc = float3(0.0, 0.0, 0.0); float soma = 0.0;
    [unroll] for (int j = 0; j < 4; ++j) {
        const float wj = wb[j] * PesoProf(z0, ProfTexel(t4[j]));
        if (wj > 0.0) { const float3 v = refs.Load(int3(t4[j], 0)).xyz; if (all(isfinite(v))) { acc += wj * v; soma += wj; } }
    }
    if (!(soma > 1e-4)) return;
    acc /= soma;
    const float exc = max(acc.y - acc.x, 0.0), kt = min(forca, 1.0) * f * saturate(acc.z);
    if (!(kt > 0.0) || !(exc > 0.0)) return;
    float3 b, o, mo; Partes(id.xy, d, b, o, mo);
    const float ym = dot(mo, LUMA), yo = dot(o, LUMA);
    const float g0 = log((ym + PISO) / (yo + PISO));
    if (!isfinite(g0)) return;
    const float g1 = max(g0 - kt * exc, min(g0, acc.x));
    if (!(g1 < g0) || !(ym > 1e-6)) return;
    const float esc = max(((ym + PISO) * exp(g1 - g0) - PISO) / ym, 0.0);
    const float3 m2 = mo * esc;
    const float3 d2 = (tone & 1u) ? tmf3(m2) - b : m2 - b;
    if (all(isfinite(d2))) saida[id.xy] = float4(d2, rd.a);
}
