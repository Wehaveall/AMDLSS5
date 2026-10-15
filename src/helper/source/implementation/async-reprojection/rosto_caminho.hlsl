cbuffer C : register(b0) {
    uint w; uint h; int rx; int ry;
    uint lado; uint n; uint offEnt; uint offTag;
    uint jobEsperado; uint idade; uint fatia; uint mascara;
    uint historico; int fx; int fy; uint fw;
    uint fh; float expo; uint tonemap; float ganho;
    float hx; float hy; float profRel; float profAbs;
    float camada0; float camada1; uint offEnt2; uint offTag2;
    uint job2; uint idade2; int rx2; int ry2;
    uint lado2; uint n2; float hx2; float hy2;
    float peso; float limiar; uint sozinho; uint pad3;
    float aBaixa; float aAlta; float gama; uint acum;
    uint quadro; uint geracao; uint bytesMeio; uint janela;
    uint rapR; uint padR0; uint padR1; uint padR2;
    float bxForca; float aLenta; float aRapida; float movPx;
}
Texture2D<float4> saidaFsr : register(t0);
Texture2DArray<float2> movHist : register(t1);
Texture2DArray<float> profHist : register(t2);
RWByteAddressBuffer compartilhado : register(u0);
SamplerState linearClamp : register(s0);

float profEm(float2 uv, uint s, uint pw, uint ph) {
    return profHist.Load(int4(min(uint2(saturate(uv) * float2(pw, ph)), uint2(pw - 1u, ph - 1u)), s, 0));
}

bool caminhar(float2 p, uint k, uint pw, uint ph, out float2 d) {
    d = float2(0, 0);
    if (historico == 0u || k >= historico) return false;
    const float2 uv0 = (p + 0.5) / float2(w, h);
    if (any(uv0 < 0.0) || any(uv0 > 1.0)) return false;
    float2 uv = uv0; uint s = fatia;
    [loop] for (uint i = 0; i < k; ++i) {
        const float2 mv = movHist.SampleLevel(linearClamp, float3(uv, s), 0);
        if (!all(isfinite(mv))) return false;
        const float2 chegada = uv + mv;
        if (any(chegada < 0.0) || any(chegada > 1.0)) return false;
        const uint anterior = (s + historico - 1u) % historico;
        if (((mascara >> anterior) & 1u) == 0u) return false;
        const float pa = profEm(uv, s, pw, ph), pb = profEm(chegada, anterior, pw, ph);
        if (!isfinite(pa) || !isfinite(pb)) return false;
        const float tol = max(profAbs, profRel * max(abs(pa), abs(pb)));
        if (!(tol > 0.0) || abs(pa - pb) > tol) return false;
        uv = chegada; s = anterior;
    }
    d = (uv - uv0) * float2(w, h);
    return true;
}
float mediana5(float a0, float a1, float a2, float a3, float a4, uint m) {
    float v[5] = { a0, a1, a2, a3, a4 };
    [unroll] for (uint i = 1; i < 5u; ++i) [unroll] for (uint j = 4u; j >= i; --j) if (j < m && v[j - 1u] > v[j]) { const float t = v[j]; v[j] = v[j - 1u]; v[j - 1u] = t; }
    return (m & 1u) != 0u ? v[m / 2u] : 0.5 * (v[max(m / 2u, 1u) - 1u] + v[m / 2u]);
}
[numthreads(16,1,1)] void RjCaminho(uint3 id : SV_DispatchThreadID) {
    const uint k = id.x;
    if (k >= 16u) return;
    uint pw = 1u, ph = 1u, pf = 1u; profHist.GetDimensions(pw, ph, pf);
    const float a = 0.2 * max(ganho, 0.0);
    float vx[5] = { 0, 0, 0, 0, 0 }, vy[5] = { 0, 0, 0, 0, 0 }; uint m = 0u;
    [unroll] for (uint i = 0; i < 5u; ++i) {
        const float2 p = float2(hx, hy) + (i == 1u ? float2(-a, 0) : i == 2u ? float2(a, 0) : i == 3u ? float2(0, -a) : i == 4u ? float2(0, a) : float2(0, 0));
        float2 d; if (caminhar(p, k, pw, ph, d)) { vx[m] = d.x; vy[m] = d.y; ++m; }
    }
    const float2 r = m > 0u ? float2(mediana5(vx[0], vx[1], vx[2], vx[3], vx[4], m), mediana5(vy[0], vy[1], vy[2], vy[3], vy[4], m)) : float2(0, 0);
    compartilhado.Store4(offEnt + k * 16u, uint4(asuint(r.x), asuint(r.y), m, quadro));
}
