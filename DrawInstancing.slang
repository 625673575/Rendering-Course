struct VSIn
{
    float3 pos : POSITION;
    float3 normal : PACKED_NORMAL_TANGENT_CURVE_RADIUS;
    float2 uv : TEXCOORD;
    uint instanceId : SV_InstanceID;
};

SamplerState gSampler;
cbuffer PerFrameCB
{
    float4x4 viewProjMatrices;
    float4x4 inverseTransposeWorldMatrices;
    float4x4 worldMatrices;
    Texture2D<float4> diffuse;
    uint instanceCount;
    float time;
};

struct VSOut
{
    float4 posH : SV_POSITION; //clipspace NDC
    float3 posW: POSITION;
    float3 normalW : NORMAL; //world normal
    float2 uv : TEXCOORD0; //tex
};
// Returns a random number based on a float3 and an int.
float random(float3 seed, int i){
	float4 seed4 = float4(seed,i);
	float dot_product = dot(seed4, float4(12.9898,78.233,45.164,94.673));
	return frac(sin(dot_product) * 43758.5453);
}
float3 random3(float3 seed, int i){
    return float3(random(seed+1.643534,i),random(seed+2.67657,i),random(seed+3.32323,i));
}
VSOut vsMain(VSIn vIn)
{
    VSOut vOut;
    // float3 scale = random3(1.54,vIn.instanceId) / instanceCount;
    // float3 trans =  random3(0.24324355,vIn.instanceId)/ instanceCount;
    vIn.pos = (vIn.pos + vIn.instanceId) / instanceCount * 3.0 ;
    vOut.posW = mul(worldMatrices,float4(vIn.pos,1.0)).xyz;
    vOut.posH = mul(viewProjMatrices, float4(vOut.posW, 1.f));
    vOut.normalW = normalize(mul(inverseTransposeWorldMatrices, float4(vIn.normal,0)).xyz);
    vOut.uv = vIn.uv;
    return vOut;
}
float4 psMain(VSOut vsOut, uint triangleIndex: SV_PrimitiveID):SV_TARGET
{
    float2 uv = vsOut.uv;
    float3 worldNormal = vsOut.normalW;
    float3 worldPos = vsOut.posW;
    float4 baseColor = diffuse.Sample(gSampler,uv);
    return baseColor;
}