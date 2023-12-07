import Utils.Color.ColorHelpers;
cbuffer taaBuf
{
    Texture2D<float4> tex;
    Texture2D<float4> prevTex;
    Texture2D<float2> motionVectorTex;
    float alpha;
    bool antiFlicker;
    float colorBoxSigma;
    float reduceMotionScale;
    SamplerState gSampler;
};
// Catmull-Rom filtering code from http://vec3.ca/bicubic-filtering-in-fewer-taps/
float3 bicubicSampleCatmullRom(Texture2D tex, SamplerState samp, float2 samplePos, float2 texDim)
{
    float2 invTextureSize = 1.0 / texDim;
    float2 tc = floor(samplePos - 0.5f) + 0.5f;
    float2 f = samplePos - tc;
    float2 f2 = f * f;
    float2 f3 = f2 * f;

    float2 w0 = f2 - 0.5f * (f3 + f);
    float2 w1 = 1.5f * f3 - 2.5f * f2 + 1.f;
    float2 w3 = 0.5f * (f3 - f2);
    float2 w2 = 1 - w0 - w1 - w3;

    float2 w12 = w1 + w2;

    float2 tc0 = (tc - 1.f) * invTextureSize;
    float2 tc12 = (tc + w2 / w12) * invTextureSize;
    float2 tc3 = (tc + 2.f) * invTextureSize;

    // clang-format off
    float3 result =
        tex.SampleLevel(samp, float2(tc0.x,  tc0.y), 0.f).rgb  * (w0.x  * w0.y) +
        tex.SampleLevel(samp, float2(tc0.x,  tc12.y), 0.f).rgb * (w0.x  * w12.y) +
        tex.SampleLevel(samp, float2(tc0.x,  tc3.y), 0.f).rgb  * (w0.x  * w3.y) +
        tex.SampleLevel(samp, float2(tc12.x, tc0.y), 0.f).rgb  * (w12.x * w0.y) +
        tex.SampleLevel(samp, float2(tc12.x, tc12.y), 0.f).rgb * (w12.x * w12.y) +
        tex.SampleLevel(samp, float2(tc12.x, tc3.y), 0.f).rgb  * (w12.x * w3.y) +
        tex.SampleLevel(samp, float2(tc3.x,  tc0.y), 0.f).rgb  * (w3.x  * w0.y) +
        tex.SampleLevel(samp, float2(tc3.x,  tc12.y), 0.f).rgb * (w3.x  * w12.y) +
        tex.SampleLevel(samp, float2(tc3.x,  tc3.y), 0.f).rgb  * (w3.x  * w3.y);
    // clang-format on
    return result;
}

float4 main(float2 texC: TEXCOORD) : SV_Target
{   
    const int2 offset[8] = {
        int2(-1, -1),
        int2(-1, 1),
        int2(1, -1),
        int2(1, 1),
        int2(1, 0),
        int2(0, -1),
        int2(0, 1),
        int2(-1, 0),
    };
    uint2 texDim;
    uint levels;
    tex.GetDimensions(0, texDim.x, texDim.y, levels);

    float2 pos = texC * texDim;
    int2 ipos = int2(pos);
    // Fetch the current pixel color and compute the color bounding box
    // Details here: http://www.gdcvault.com/play/1023521/From-the-Lab-Bench-Real
    // and here: http://cwyman.org/papers/siga16_gazeTrackedFoveatedRendering.pdf
    float3 color = tex.Load(int3(ipos, 0)).rgb;
    color = RGBToYCgCo(color);

    // Find the longest motion vector
    float2 motion = motionVectorTex.Load(int3(ipos, 0)).xy;
    [unroll]
    for (int a = 0; a < 8; a++)
    {
        float2 m = motionVectorTex.Load(int3(ipos + offset[a], 0)).rg;
        motion = dot(m, m) > dot(motion, motion) ? m : motion;
    }    
    // Use motion vector to fetch previous frame color (history)
    float3 historyColor = bicubicSampleCatmullRom(prevTex, gSampler, (texC + motion) * texDim, texDim);
    historyColor = RGBToYCgCo(historyColor);

    // Anti-flickering, based on Brian Karis talk @Siggraph 2014
    // https://de45xmedrsdbp.cloudfront.net/Resources/files/TemporalAA_small-59732822.pdf
    // Reduce blend factor when history is near clamping
    float reduceAlpha = alpha;
    reduceAlpha = saturate(reduceAlpha + length(motion) * reduceMotionScale);
    if (antiFlicker)
    {
        float3 colorAvg = color;
        float3 colorVar = color * color;
        [unroll]
        for (int k = 0; k < 8; k++)
        {
            float3 c = tex.Load(int3(ipos + offset[k], 0)).rgb;
            c = RGBToYCgCo(c);
            colorAvg += c;
            colorVar += c * c;
        }

        float oneOverNine = 1.f / 9.f;
        colorAvg *= oneOverNine;
        colorVar *= oneOverNine;

        float3 sigma = sqrt(max(0.f, colorVar - colorAvg * colorAvg));
        float3 colorMin = colorAvg - colorBoxSigma * sigma;
        float3 colorMax = colorAvg + colorBoxSigma * sigma;
        float distToClamp = min(abs(colorMin.x - historyColor.x), abs(colorMax.x - historyColor.x));
        reduceAlpha = clamp((reduceAlpha * distToClamp) / (distToClamp + colorMax.x - colorMin.x), 0.f, 1.f);
    }
    float3 finalColor = YCgCoToRGB(lerp(historyColor, color, reduceAlpha));
    return float4(finalColor,1.0) ;
}
