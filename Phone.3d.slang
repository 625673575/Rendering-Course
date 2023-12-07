struct VSIn
{
    float3 pos : POSITION;
    float3 normal : PACKED_NORMAL_TANGENT_CURVE_RADIUS;
    float2 uv : TEXCOORD;
};
SamplerState gSampler;
cbuffer PerFrameCB
{
    float4x4 viewProjMatrices;
    float4x4 inverseTransposeWorldMatrices;
    float4x4 worldMatrices;
    float scale;
    Texture2D diffuse;
};

struct VSOut
{
    float4 posH : SV_POSITION; //clipspace NDC
    float3 posW: POSW;
    float3 normalW : NORMAL; //world normal
    float2 uv : TEXCOORD; //tex
};

VSOut vsMain(VSIn vIn)
{
    VSOut vOut;
    vIn.pos = vIn.pos;
    vOut.posW = mul(worldMatrices,float4(vIn.pos,1.0)).xyz;
    vOut.posH = mul(viewProjMatrices, float4(vOut.posW, 1.f));
    vOut.normalW = normalize(mul(inverseTransposeWorldMatrices, float4(vIn.normal,0)).xyz);
    vOut.uv = vIn.uv;
    return vOut;
}

float4 psMain(VSOut vsOut, uint triangleIndex: SV_PrimitiveID) : SV_TARGET
{
    float2 uv = vsOut.uv;
    float3 normal = vsOut.normalW;
    float3 worldPos = vsOut.posW;
    float4 baseColor = diffuse.Sample(gSampler,uv);
    return baseColor;
}
