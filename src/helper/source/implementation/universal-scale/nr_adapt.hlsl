cbuffer Adapt : register(b0) { float4 Mul; uint2 Size; }
Texture2D<float4> Source : register(t0);
RWTexture2D<float4> Destination4 : register(u0);
RWTexture2D<float2> Destination2 : register(u0);
RWTexture2D<float> Destination1 : register(u0);
Texture2D<float> Source1 : register(t0);

[numthreads(8,8,1)] void AdaptColorCS(uint3 id:SV_DispatchThreadID){
    if(any(id.xy>=Size))return;
    const float4 c=Source.Load(int3(id.xy,0));
    Destination4[id.xy]=float4(c.rgb*Mul.x,c.a);
}

[numthreads(8,8,1)] void AdaptMotionCS(uint3 id:SV_DispatchThreadID){
    if(any(id.xy>=Size))return;

    Destination2[id.xy]=Source.Load(int3(int2((float2(id.xy)+0.5)*Mul.zw),0)).xy*Mul.xy;
}

[numthreads(8,8,1)] void AdaptDepthCS(uint3 id:SV_DispatchThreadID){
    if(any(id.xy>=Size))return;
    Destination1[id.xy]=Source1.Load(int3(id.xy,0));
}

[numthreads(1,1,1)] void AdaptExposureCS(uint3 id:SV_DispatchThreadID){
    Destination1[uint2(0,0)]=Source.Load(int3(0,0,0)).x;
}

[numthreads(8,8,1)] void CopyOutputCS(uint3 id:SV_DispatchThreadID){
    if(any(id.xy>=Size))return;
    const float4 c=Source.Load(int3(id.xy,0));
    Destination4[id.xy]=float4(c.rgb*Mul.x,c.a);
}

[numthreads(8,8,1)] void ScaleOutputCS(uint3 id:SV_DispatchThreadID){
    if(any(id.xy>=Size))return;
    const float4 c=Destination4[id.xy];
    Destination4[id.xy]=float4(c.rgb*Mul.x,c.a);
}
