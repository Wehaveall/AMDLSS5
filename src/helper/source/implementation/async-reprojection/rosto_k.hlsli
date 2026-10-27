#ifndef NR_ROSTO_K_HLSLI
#define NR_ROSTO_K_HLSLI
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
    float corHx; float corHy; float corHl; float posMov;
    uint offT0;
    float gamaParado;
}
Texture2D<float4> saidaFsr : register(t0);
Texture2DArray<float2> movHist : register(t1);
Texture2DArray<float> profHist : register(t2);
RWByteAddressBuffer compartilhado : register(u0);

float profEm(float2 uv, uint s, uint pw, uint ph) {
    return profHist.Load(int4(min(uint2(saturate(uv) * float2(pw, ph)), uint2(pw - 1u, ph - 1u)), s, 0));
}
#endif
