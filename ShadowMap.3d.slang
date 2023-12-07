struct VSIn
{
    float3 pos : POSITION;
    float3 normal : PACKED_NORMAL_TANGENT_CURVE_RADIUS;
    float2 uv : TEXCOORD;
};
cbuffer PerFrameCB
{
    float4x4 viewProjMatrices;
    float4x4 worldMatrices;
};
Texture2D<float> DepthTex;
SamplerState gSampler;
struct VSOut
{
    float4 posH : SV_POSITION; //clipspace
    float3 posW: POSW;
};

VSOut vsMain(VSIn vIn)
{
    VSOut vOut;
    vIn.pos = vIn.pos;
    vOut.posW = mul(worldMatrices,float4(vIn.pos,1.0)).xyz;
    vOut.posH = mul(viewProjMatrices, float4(vOut.posW, 1.f));
    return vOut;
}

float psMain(VSOut vsOut, uint triangleIndex: SV_PrimitiveID) : SV_TARGET
{
    const float linearZ = vsOut.posH.z * vsOut.posH.w;
    return linearZ;
}

struct VsOut
{
    float2 texC : TEXCOORD;
#ifndef _VIEWPORT_MASK
    float4 posH : SV_POSITION;
#else
    float4 posH : POSITION;
#endif
};
float main(VsOut vsOut, uint triangleIndex: SV_PrimitiveID) : SV_TARGET
{
    float depth = DepthTex.Sample(gSampler,vsOut.texC);
    //depth = LinearizeDepth(1.0,100.0,depth);
    return float(depth);
}
float LinearizeDepth(float nearv,float farv,float Nativedepth)
{
    float z = Nativedepth * 2.0 - 1.0; // back to NDC 
    return (2.0 * nearv * farv) / (farv + nearv - z * (farv - nearv)) /(farv -nearv);
}
