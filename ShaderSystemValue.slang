struct VSIn
{
    float3 pos : POSITION;
    float3 normal : PACKED_NORMAL_TANGENT_CURVE_RADIUS;
    float2 uv : TEXCOORD;
    // System values
    uint vertexID : SV_VertexID;
};
struct GBufferPSOut
{
    float4 albedo : SV_TARGET0;
    float4 primitiveID : SV_TARGET1;
    float4 sampleIndex: SV_TARGET2;
    float4 vertexIndex: SV_TARGET3;
    float4 coverge: SV_TARGET4;
    float4 isFrontFace: SV_TARGET5;
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
    float4 posW: POSITION;
    float3 normalW : NORMAL; //world normal
    float2 uv : TEXCOORD0; //tex
    float4 screenUV:TEXCOORD1;
    uint vertexIndex: TEXCOORD2;
};

inline float4 ComputeScreenPos(float4 pos) {
    float4 o = pos * 0.5f;
    o.xy = float2(o.x, o.y) + o.w;
    o.zw = pos.zw;
    return o;
}
static const float3 ColorTable[9] =
	{
		float3(0.0f, 0.0f,0.0f),
		float3(0.0f,0.0f,1.0f),
		float3(0.0f,1.0f,1.0f),
		float3(0.5f, 0.5f,0.5f),
		float3(0.0f,1.0f,0.0f),
		float3(1.0f,0.0f,1.0f),
		float3(1.0f,0.0f,0.0f),
		float3(1.0f,1.0f,1.0f),
		float3(1.0f,0.92f,0.016f)
	};
VSOut vsMain(VSIn vIn)
{
    VSOut vOut;
    vIn.pos = vIn.pos;
    vOut.posW = mul(worldMatrices,float4(vIn.pos,1.0));
    vOut.posH = mul(viewProjMatrices, vOut.posW);
    vOut.normalW = normalize(mul(inverseTransposeWorldMatrices, float4(vIn.normal,0)).xyz);
    vOut.uv = vIn.uv;
    vOut.screenUV = ComputeScreenPos(vOut.posH);
    vOut.vertexIndex = vIn.vertexID;
    return vOut;
}
GBufferPSOut psMain(VSOut vsOut, 
    uint triangleIndex:SV_PrimitiveID,
    uint sampleIndex:SV_SampleIndex,
    uint coverge:SV_Coverage,
    bool isFrontFace:SV_IsFrontFace)
{
    GBufferPSOut psOut = {};
    float2 uv = vsOut.uv;
    float3 worldNormal = vsOut.normalW;
    float3 worldPos = vsOut.posW.xyz/vsOut.posW.w;
    float4 baseColor = diffuse.Sample(gSampler,uv);
    // float4 screenPos = vsOut.screenUV;
    // float2 projUV = screenPos.xy / screenPos.w;
    // projUV.y = 1.0 - projUV.y;
    // psOut.screenTex = diffuse.Sample(gSampler,projUV);
    psOut.albedo = baseColor;
    psOut.primitiveID = float4(ColorTable[triangleIndex%9],1.0f);
    psOut.sampleIndex = float4(ColorTable[sampleIndex%9],1.0f);//Only available when enbling MSAA
    psOut.vertexIndex = float4(ColorTable[vsOut.vertexIndex%9],1.0f);
    psOut.coverge = float4(ColorTable[coverge%9],1.0f);
    psOut.isFrontFace = float4(isFrontFace ? ColorTable[7] : ColorTable[0],0);
    return psOut;
}