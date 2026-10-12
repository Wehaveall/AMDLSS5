cbuffer C : register(b0) {
    uint gw; uint gh; uint offD; uint offMv;
    uint fatia; uint inv; uint pad0; uint pad1;
}
Texture2DArray<float2> movHist : register(t1);
Texture2DArray<float> profHist : register(t2);
RWByteAddressBuffer grade : register(u0);

[numthreads(8,8,1)] void RjGrade(uint3 id : SV_DispatchThreadID) {
    if (id.x >= gw || id.y >= gh) return;
    uint mw = 1u, mh = 1u, ms = 1u; movHist.GetDimensions(mw, mh, ms);
    uint pw = 1u, ph = 1u, ps = 1u; profHist.GetDimensions(pw, ph, ps);
    const float2 uv = (float2(id.xy) + 0.5) / float2(gw, gh);
    float2 m = movHist.Load(int4(min(uint2(uv * float2(mw, mh)), uint2(mw - 1u, mh - 1u)), fatia, 0));
    float z = profHist.Load(int4(min(uint2(uv * float2(pw, ph)), uint2(pw - 1u, ph - 1u)), fatia, 0));
    if (!isfinite(z)) z = 0.0;
    z = saturate(z);
    const float p = inv != 0u ? z : 1.0 - z;
    if (!all(isfinite(m))) m = float2(0.0, 0.0);
    const float2 px = m * float2(gw, gh);
    const uint i = id.y * gw + id.x;
    grade.Store(offD + i * 4u, asuint(p));
    grade.Store(offMv + i * 4u, f32tof16(px.x) | (f32tof16(px.y) << 16));
}
