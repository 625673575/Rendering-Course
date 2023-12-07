cbuffer PerFrameCB
{
    uint2   gResolution;
    float2  gInvRes;
}
SamplerState gLinearSampler;
Texture2D<float4> gSrc;
RWTexture2D<float4> gDst;

float4 blurFilter3x3(Texture2D<float4> src, const float2 uv, const float uscale, const float vscale)
{
    // 3x3 bilinear-tap kernel, effective footprint 6x6, with offsets chosen to sample from the binomial 1 5 10 10 5 1
    const float w0 = (10.f + 10.f) / 32.f;
    const float x1 = 1.5f + 1.f / (1.f + 5.f), w1 = (1.f + 5.f) / 32.f;
    const float x1u = x1 * uscale;
    const float x1v = x1 * vscale;
    return
        (src.SampleLevel(gLinearSampler, uv + float2(-x1u, -x1v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(x1u, -x1v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(-x1u, x1v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(x1u, x1v), 0.f)) * (w1 * w1) +
        (src.SampleLevel(gLinearSampler, uv + float2(0.f, -x1v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(0.f, x1v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(-x1u, 0.f), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(x1u, 0.f), 0.f)) * (w1 * w0) +
        (src.SampleLevel(gLinearSampler, uv + float2(0.f, 0.f), 0.f)) * (w0 * w0);
}

float4 blurFilter5x5(Texture2D<float4> src, const float2 uv, const float uscale, const float vscale)
{
    // 5x5 bilinear-tap kernel, effective footprint 10x10, with offsets chosen to sample from the binomial 1 9 36 84 126 126 84 32 9 1
    const float w0 = (126.f + 126.f) / 512.f;
    const float x1 = 1.5f + 36.f / (36.f + 84.f), w1 = (36.f + 84.f) / 512.f;
    const float x2 = 3.5f + 1.f / (1.f + 9.f), w2 = (1.f + 10.f) / 512.f;
    const float x1u = x1 * uscale, x2u = x2 * uscale;
    const float x1v = x1 * vscale, x2v = x2 * vscale;
    float4 s0 =
        (src.SampleLevel(gLinearSampler, uv + float2(-x2u, -x2v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(x2u, -x2v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(-x2u, x2v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(x2u, x2v), 0.f)) * (w2 * w2) +
        (src.SampleLevel(gLinearSampler, uv + float2(-x1u, -x2v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(x1u, -x2v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(-x1u, x2v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(x1u, x2v), 0.f)) * (w1 * w2) +
        (src.SampleLevel(gLinearSampler, uv + float2(0.f, -x2v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(0.f, x2v), 0.f)) * (w0 * w2);
    float4 s1 =
        (src.SampleLevel(gLinearSampler, uv + float2(-x2u, -x1v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(x2u, -x1v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(-x2u, x1v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(x2u, x1v), 0.f)) * (w2 * w1) +
        (src.SampleLevel(gLinearSampler, uv + float2(-x1u, -x1v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(x1u, -x1v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(-x1u, x1v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(x1u, x1v), 0.f)) * (w1 * w1) +
        (src.SampleLevel(gLinearSampler, uv + float2(0.f, -x1v), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(0.f, x1v), 0.f)) * (w0 * w1);
    float4 s2 =
        (src.SampleLevel(gLinearSampler, uv + float2(-x2u, 0.f), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(x2u, 0.f), 0.f)) * (w2 * w0) +
        (src.SampleLevel(gLinearSampler, uv + float2(-x1u, 0.f), 0.f) + src.SampleLevel(gLinearSampler, uv + float2(x1u, 0.f), 0.f)) * (w1 * w0) +
        (src.SampleLevel(gLinearSampler, uv + float2(0.f, 0.f), 0.f)) * (w0 * w0);
    return s0 + s1 + s2;
}

[numthreads(16, 16, 1)]
void downsample(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (any(dispatchThreadId.xy >= gResolution)) return;
    int2 pixelPos = dispatchThreadId.xy;
    float2 uv = (pixelPos + 0.5f) * gInvRes;
    gDst[pixelPos] = blurFilter5x5(gSrc, uv, gInvRes.x * 0.5f, gInvRes.y * 0.5f);
}

Texture2D<float4> gSrcArray[4];
uint gSampleCount;
[numthreads(16, 16, 1)]
void merge(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (any(dispatchThreadId.xy >= gResolution)) return;
    int2 pixelPos = dispatchThreadId.xy;
    float2 uv = (pixelPos + 0.5f) * gInvRes;
    gDst[pixelPos] = float4(0);
    for(uint i = 0; i< gSampleCount;i++){
        gDst[pixelPos] += gSrcArray[i].SampleLevel(gLinearSampler, uv , 0.f);
    }
    gDst[pixelPos]/=(float)gSampleCount;
}