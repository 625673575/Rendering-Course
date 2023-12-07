SamplerState gSampler;

Texture2D gColor;
Texture2D gAOMap;

float4 applyAmbientOcclusion(float4 color, Texture2D aoTex, SamplerState s, float2 texC)
{
    float aoFactor = aoTex.SampleLevel(s, texC, 0).r;
    return float4(color.rgb * aoFactor, color.a);
}

float4 main(float2 texC : TEXCOORD) : SV_TARGET0
{
    return applyAmbientOcclusion(gColor.SampleLevel(gSampler, texC, 0), gAOMap, gSampler, texC);
}
