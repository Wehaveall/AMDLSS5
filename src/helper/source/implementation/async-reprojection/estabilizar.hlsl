cbuffer K : register(b0) { uint largura, altura, fatia; float alfa; uint valido; float refParado; uint semLimite; float alargar; float profRel, profAbs; uint fatiaAnterior; float alfaMov, alfaLuz, limite; float alfaLuzMov; };
Texture2D<float4> atual : register(t0);
Texture2D<float4> anterior : register(t1);
Texture2DArray<float2> movimento : register(t2);
Texture2DArray<float> profundidade : register(t3);
Texture2DArray<float4> cor : register(t4);
RWTexture2D<float4> saida : register(u0);
SamplerState linear0 : register(s0);

#ifdef NR_EST_CR
float4 AmostraCR(float2 uv) {
    float w0, h0; anterior.GetDimensions(w0, h0); const float2 tam = float2(w0, h0);
    const float2 pos = uv * tam; const float2 c1 = floor(pos - 0.5) + 0.5; const float2 f = pos - c1;
    const float2 a0 = f * (-0.5 + f * (1.0 - 0.5 * f)), a1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    const float2 a2 = f * (0.5 + f * (2.0 - 1.5 * f)), a3 = f * f * (-0.5 + 0.5 * f);
    const float2 a12 = a1 + a2; const float2 o12 = c1 + a2 / a12;
    const float2 t0 = (c1 - 1.0) / tam, t3 = (c1 + 2.0) / tam, t12 = o12 / tam;
    float4 r = anterior.SampleLevel(linear0, float2(t0.x, t0.y), 0) * a0.x * a0.y + anterior.SampleLevel(linear0, float2(t12.x, t0.y), 0) * a12.x * a0.y
             + anterior.SampleLevel(linear0, float2(t3.x, t0.y), 0) * a3.x * a0.y + anterior.SampleLevel(linear0, float2(t0.x, t12.y), 0) * a0.x * a12.y
             + anterior.SampleLevel(linear0, float2(t12.x, t12.y), 0) * a12.x * a12.y + anterior.SampleLevel(linear0, float2(t3.x, t12.y), 0) * a3.x * a12.y
             + anterior.SampleLevel(linear0, float2(t0.x, t3.y), 0) * a0.x * a3.y + anterior.SampleLevel(linear0, float2(t12.x, t3.y), 0) * a12.x * a3.y
             + anterior.SampleLevel(linear0, float2(t3.x, t3.y), 0) * a3.x * a3.y;
    const int2 b = int2(floor(pos - 0.5)); const int2 m = int2(int(w0) - 1, int(h0) - 1);
    const float4 v0 = anterior.Load(int3(clamp(b, int2(0, 0), m), 0)), v1 = anterior.Load(int3(clamp(b + int2(1, 0), int2(0, 0), m), 0));
    const float4 v2 = anterior.Load(int3(clamp(b + int2(0, 1), int2(0, 0), m), 0)), v3 = anterior.Load(int3(clamp(b + int2(1, 1), int2(0, 0), m), 0));
    return clamp(r, min(min(v0, v1), min(v2, v3)), max(max(v0, v1), max(v2, v3)));
}
#define EST_AMOSTRA(uv) AmostraCR(uv)
#else
#define EST_AMOSTRA(uv) anterior.SampleLevel(linear0, uv, 0)
#endif

[numthreads(8, 8, 1)]
void Estabilizar(uint3 id : SV_DispatchThreadID) {
    if (id.x >= largura || id.y >= altura) return;

    if (semLimite == 2u) {
        const float2 mv = movimento.Load(int4(id.xy, fatia, 0));
        const float2 uvh = (float2(id.xy) + 0.5) / float2(largura, altura) + mv;
        float4 h = float4(0.0, 0.0, 0.0, 0.0);
        if (all(isfinite(uvh)) && all(uvh >= 0.0) && all(uvh <= 1.0)) {
            const uint2 origem = min(uint2(uvh * float2(largura, altura)), uint2(largura - 1, altura - 1));
            const float a = profundidade.Load(int4(id.xy, fatia, 0));
            const float b = profundidade.Load(int4(origem, fatiaAnterior, 0));
            const float tolerancia = max(profAbs, profRel * max(abs(a), abs(b)));
            if (isfinite(a) && isfinite(b) && tolerancia > 0.0 && abs(a - b) <= tolerancia)
                h = EST_AMOSTRA(uvh);
        }
        if (alfa > 0.0) h.rgb *= alfa;
        saida[id.xy] = all(isfinite(h)) ? h : float4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    if (semLimite == 3u) {
        const float4 c3 = atual.Load(int3(id.xy, 0));
        if (valido == 0 || !all(isfinite(c3))) { saida[id.xy] = c3; return; }
        float3 mn3 = c3.rgb, mx3 = c3.rgb, soma3 = float3(0.0, 0.0, 0.0);
        [unroll] for (int dy3 = -1; dy3 <= 1; ++dy3)
        [unroll] for (int dx3 = -1; dx3 <= 1; ++dx3) {
            const int2 q = clamp(int2(id.xy) + int2(dx3, dy3), int2(0, 0), int2(int(largura) - 1, int(altura) - 1));
            float3 v = atual.Load(int3(q, 0)).rgb;
            if (!all(isfinite(v))) v = c3.rgb;
            mn3 = min(mn3, v); mx3 = max(mx3, v); soma3 += v;
        }
        const float3 cl = soma3 / 9.0;
        const float2 tam = float2(largura, altura);
        const float2 mv3 = movimento.Load(int4(id.xy, fatia, 0));
        const float2 uvp3 = (float2(id.xy) + 0.5) / tam + mv3;
        if (!all(isfinite(uvp3)) || any(uvp3 < 0.0) || any(uvp3 > 1.0)) { saida[id.xy] = c3; return; }
        {
            const uint2 origem = min(uint2(uvp3 * tam), uint2(largura - 1, altura - 1));
            const float a = profundidade.Load(int4(id.xy, fatia, 0));
            const float b = profundidade.Load(int4(origem, fatiaAnterior, 0));
            const float tolerancia = max(profAbs, profRel * max(abs(a), abs(b)));
            if (!(isfinite(a) && isfinite(b) && tolerancia > 0.0 && abs(a - b) <= tolerancia)) { saida[id.xy] = c3; return; }
        }
        const float3 p3 = EST_AMOSTRA(uvp3).rgb;
        if (!all(isfinite(p3))) { saida[id.xy] = c3; return; }
        float3 pl = float3(0.0, 0.0, 0.0);
        [unroll] for (int dy4 = -1; dy4 <= 1; ++dy4)
        [unroll] for (int dx4 = -1; dx4 <= 1; ++dx4) pl += anterior.SampleLevel(linear0, uvp3 + float2(dx4, dy4) / tam, 0).rgb;
        pl /= 9.0;
        if (!all(isfinite(pl))) pl = p3;
        const float parado = refParado > 0.0 ? saturate(1.0 - length(mv3 * tam) / refParado) : 1.0;
        const float3 e3 = (mx3 - mn3) * alargar * lerp(0.5, 1.0, parado);
        const float3 ph = clamp(p3 - pl, (mn3 - cl) - e3, (mx3 - cl) + e3);
        const float3 h3 = lerp(ph, c3.rgb - cl, lerp(alfaMov, alfa, parado));
        const float3 l3 = lerp(pl, cl, lerp(alfaLuzMov > 0.0 ? alfaLuzMov : alfaMov, alfaLuz > 0.0 ? alfaLuz : alfa, parado));
        float3 o3 = l3 + h3;
        if (limite > 0.0 && parado > 0.0) {
            const float y = max(dot(cor.Load(int4(id.xy, fatia, 0)).rgb, float3(0.2126, 0.7152, 0.0722)), 0.0);
            const float lim = limite * (isfinite(y) ? y : 0.0) + limite * 0.015625;
            o3 = lerp(o3, p3 + clamp(o3 - p3, -lim, lim), parado);
        }
        saida[id.xy] = all(isfinite(o3)) ? float4(o3, c3.a) : c3;
        return;
    }
    const float4 c = atual.Load(int3(id.xy, 0));
    if (valido == 0 || !all(isfinite(c))) { saida[id.xy] = c; return; }
    float3 mn = c.rgb, mx = c.rgb;
    [unroll] for (int dy = -1; dy <= 1; ++dy)
    [unroll] for (int dx = -1; dx <= 1; ++dx) {
        const int2 q = clamp(int2(id.xy) + int2(dx, dy), int2(0, 0), int2(int(largura) - 1, int(altura) - 1));
        const float3 v = atual.Load(int3(q, 0)).rgb;
        if (all(isfinite(v))) { mn = min(mn, v); mx = max(mx, v); }
    }
    const float2 uv = (float2(id.xy) + 0.5) / float2(largura, altura);
    const float2 mv = movimento.Load(int4(id.xy, fatia, 0));
    const float2 uvp = uv + mv;
    if (!all(isfinite(uvp)) || any(uvp < 0.0) || any(uvp > 1.0)) { saida[id.xy] = c; return; }
    float3 p = EST_AMOSTRA(uvp).rgb;
    if (!all(isfinite(p))) { saida[id.xy] = c; return; }
    if (semLimite != 0 && length(mv * float2(largura, altura)) < 0.5) { const float3 e = (mx - mn) * alargar; p = clamp(p, mn - e, mx + e); }
    else p = clamp(p, mn, mx);
    float a = alfa;
    if (refParado > 0.0) a = lerp(alfa, 1.0, saturate(length(mv * float2(largura, altura)) / refParado));
    saida[id.xy] = float4(lerp(p, c.rgb, a), c.a);
}
