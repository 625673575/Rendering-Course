import Scene.Camera.Camera;
import Utils.Math.MatrixUtils;
struct SSAOData
{
    static const uint32_t kMaxSamples = 32;

    float4 sampleKernel[kMaxSamples];
    float2 noiseScale;
    uint32_t kernelSize;
    float radius;
};
cbuffer StaticCB
{
    SSAOData gData;
}

cbuffer PerFrameCB
{
    Camera gCamera;
}

SamplerState gNoiseSampler;
SamplerState gTextureSampler;

Texture2D gDepthTex;
Texture2D gNormalTex;
Texture2D gNoiseTex;

float4 getPosition(float2 uv)
{
    float4 pos;
    pos.x = uv.x * 2.0f - 1.0f;
    pos.y = (1.0f - uv.y) * 2.0f - 1.0f;
#ifdef FALCOR_FLIP_Y
    // NDC Y is top-to-bottom
    pos.y = -pos.y;
#endif
    pos.z = gDepthTex.SampleLevel(gTextureSampler, uv, 0).r;
    pos.w = 1.0f;

    float4 posW = mul(gCamera.data.invViewProj, pos);
    posW /= posW.w;

    return posW;
}

float4 main(float2 texC : TEXCOORD) : SV_TARGET0
{
    if (gDepthTex.SampleLevel(gTextureSampler, texC, 0).r >= 1)
    {
        return 1.0f;
    }

    // Calculate world position of pixel
    float3 posW = getPosition(texC).xyz;
    float3 normal = normalize(gNormalTex.Sample(gTextureSampler, texC).xyz * 2.0f - 1.0f);
    float originDist = length(posW - gCamera.data.posW);
    float3 randDir = gNoiseTex.Sample(gNoiseSampler, texC * gData.noiseScale).xyz * 2.0f - 1.0f;

    float3 tangent = normalize(randDir - normal * dot(randDir, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 tbn = float3x3FromCols(tangent, bitangent, normal);

    float occlusion = 0.0f;
    for (uint i = 0; i < gData.kernelSize; i++)
    {
        // Orient sample
        float3 kernelPos = mul(tbn, gData.sampleKernel[i].xyz);

        // Calculate sample world space pos
        float3 samplePosW = posW + (kernelPos * gData.radius);
        float sampleDepth = length(samplePosW - gCamera.data.posW);

        // Get screen space pos of sample
        float4 samplePosProj = mul(gCamera.data.viewProjMat, float4(samplePosW, 1.0f));
        samplePosProj /= samplePosProj.w;

        // Sample depth buffer at the same place as sample
#ifdef FALCOR_FLIP_Y
        // NDC Y is top-to-bottom
        samplePosProj.y = -samplePosProj.y;
#endif
        float2 sampleUV = saturate(float2(samplePosProj.x, -samplePosProj.y) * 0.5f + 0.5f);
        float sceneDepth = length(getPosition(sampleUV).xyz - gCamera.data.posW);

        float rangeCheck = step(abs(sampleDepth - sceneDepth), gData.radius);
        occlusion += step(sceneDepth, sampleDepth) * rangeCheck;
    }

    float factor = 1 - (occlusion / float(gData.kernelSize));
    return float4(factor.xxx, 1);
}
