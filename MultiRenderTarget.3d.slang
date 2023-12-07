struct VSIn
{
    float3 pos : POSITION;
    float3 normal : PACKED_NORMAL_TANGENT_CURVE_RADIUS;
    float2 uv : TEXCOORD;
};
struct GBufferPSOut
{
    float4 posW : SV_TARGET0;
    float4 normW : SV_TARGET1;
    float4 albedo : SV_TARGET2;
    float4 screenTex : SV_TARGET3;
};
SamplerState gSampler;
cbuffer PerFrameCB
{
    float4x4 viewProjMatrices;
    float4x4 inverseTransposeWorldMatrices;
    float4x4 worldMatrices;
    Texture2D<float4> diffuse;
};

struct VSOut
{
    float4 posH : SV_POSITION; //clipspace NDC
    float3 posW: POSITION;
    float3 normalW : NORMAL; //world normal
    float2 uv : TEXCOORD0; //tex
    float4 screenUV:TEXCOORD1;
};

inline float4 ComputeScreenPos(float4 pos) {
    float4 o = pos * 0.5f;
    o.xy = float2(o.x, o.y) + o.w;
    o.zw = pos.zw;
    return o;
}
VSOut vsMain(VSIn vIn)
{
    VSOut vOut;
    vIn.pos = vIn.pos;
    vOut.posW = mul(worldMatrices,float4(vIn.pos,1.0)).xyz;
    vOut.posH = mul(viewProjMatrices, float4(vOut.posW, 1.f));
    vOut.normalW = normalize(mul(inverseTransposeWorldMatrices, float4(vIn.normal,0)).xyz);
    vOut.uv = vIn.uv;
    vOut.screenUV = ComputeScreenPos(vOut.posH);
    return vOut;
}
GBufferPSOut psMain(VSOut vsOut, uint triangleIndex: SV_PrimitiveID)
{
    GBufferPSOut psOut = {};
    float2 uv = vsOut.uv;
    float3 worldNormal = vsOut.normalW;
    float3 worldPos = vsOut.posW;
    float4 baseColor = diffuse.Sample(gSampler,uv);
    float4 screenPos = vsOut.screenUV;
    float2 projUV = screenPos.xy / screenPos.w;
    projUV.y = 1.0 - projUV.y;
    psOut.posW = float4(worldPos,1.0);
    psOut.normW = float4(worldNormal,1.0);
    psOut.albedo = baseColor;
    psOut.screenTex = diffuse.Sample(gSampler,projUV); // float4(triangleIndex%3);
    return psOut;
}