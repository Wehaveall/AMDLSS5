cbuffer Dimensions : register(b0) { uint2 SourceSize; uint2 DestinationSize; }
Texture2D<float4> Source4 : register(t0);
Texture2D<float2> Source2 : register(t0);
Texture2D<float> Source1 : register(t0);
RWTexture2D<float4> Destination4 : register(u0);
RWTexture2D<float2> Destination2 : register(u0);
RWTexture2D<float> Destination1 : register(u0);

float2 source_position(uint2 pixel) {
    return (float2(pixel) + 0.5) * float2(SourceSize) / float2(DestinationSize) - 0.5;
}
int2 bounded(int2 pixel) { return clamp(pixel, int2(0,0), int2(SourceSize)-1); }
float4 bilinear4(uint2 pixel) {
    float2 p=source_position(pixel);int2 a=int2(floor(p));float2 f=frac(p);
    float4 x0=lerp(Source4.Load(int3(bounded(a),0)),Source4.Load(int3(bounded(a+int2(1,0)),0)),f.x);
    float4 x1=lerp(Source4.Load(int3(bounded(a+int2(0,1)),0)),Source4.Load(int3(bounded(a+int2(1,1)),0)),f.x);
    return lerp(x0,x1,f.y);
}
float2 bilinear2(uint2 pixel) {
    float2 p=source_position(pixel);int2 a=int2(floor(p));float2 f=frac(p);
    float2 x0=lerp(Source2.Load(int3(bounded(a),0)),Source2.Load(int3(bounded(a+int2(1,0)),0)),f.x);
    float2 x1=lerp(Source2.Load(int3(bounded(a+int2(0,1)),0)),Source2.Load(int3(bounded(a+int2(1,1)),0)),f.x);
    return lerp(x0,x1,f.y);
}
[numthreads(8,8,1)] void ColorCS(uint3 id:SV_DispatchThreadID){if(any(id.xy>=DestinationSize))return;Destination4[id.xy]=bilinear4(id.xy);}
[numthreads(8,8,1)] void MotionCS(uint3 id:SV_DispatchThreadID){if(any(id.xy>=DestinationSize))return;Destination2[id.xy]=bilinear2(id.xy);}
[numthreads(8,8,1)] void DepthCS(uint3 id:SV_DispatchThreadID){if(any(id.xy>=DestinationSize))return;uint2 p=min(((2*id.xy+1)*SourceSize)/(2*DestinationSize),SourceSize-1);Destination1[id.xy]=Source1.Load(int3(p,0));}
