cbuffer C : register(b0) {
    int x0; int y0; float cw; uint n;
    uint fatia; uint inv; uint off; uint pad0;
}
Texture2DArray<float2> movHist : register(t1);
Texture2DArray<float> profHist : register(t2);
RWByteAddressBuffer saida : register(u0);

[numthreads(8,8,1)] void RjCabeca(uint3 id : SV_DispatchThreadID) {
    if (id.x >= n || id.y >= n) return;
    uint pw = 1u, ph = 1u, ps = 1u; profHist.GetDimensions(pw, ph, ps);
    float m = 0.0;
    [unroll] for (uint j = 0u; j < 4u; ++j) [unroll] for (uint i = 0u; i < 4u; ++i) {
        const float2 p = float2(x0, y0) + (float2(id.xy) + (float2(i, j) + 0.5) / 4.0) * cw;
        if (p.x < 0.0 || p.y < 0.0 || p.x >= float(pw) || p.y >= float(ph)) continue;
        float z = profHist.Load(int4(int2(p), fatia, 0));
        if (!isfinite(z)) z = 0.0;
        z = saturate(z);
        m = max(m, inv != 0u ? z : 1.0 - z);
    }
    saida.Store(off + (id.y * n + id.x) * 4u, asuint(m));
}
