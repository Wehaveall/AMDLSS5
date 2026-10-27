cbuffer K : register(b0) {
    uint largura, altura, fatia, fatiaAnterior;
    uint valido, raio, tonemap, passo;
    float alfa, alfaMov, delta, expo;
    float profRel, profAbs, refMov, limite;
    float sigma, reservado0, reservado1, reservado2;
};
Texture2DArray<float4> cor : register(t4);
RWTexture2D<float4> basePronta : register(u0);

#include "tom_comum.hlsli"
#include "base_tom.hlsli"
[numthreads(8, 8, 1)]
void BaseTom(uint3 id : SV_DispatchThreadID) {
    if (id.x >= largura || id.y >= altura) return;
    basePronta[id.xy] = float4(base_em(int2(id.xy)), 1.0);
}
