struct VSIn
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float2 uv : TEXCOORD;
};

struct VSOut
{
    float4 posH : SV_POSITION; //clipspace NDC
    float3 posW: POSW;
    float3 normalW : NORMAL; //world normal
    float3 tangentW : TANGENT; //world tangent
    float3 bitangentW : BITANGENT; //world tangent
    float2 uv : TEXCOORD0; //tex
};
SamplerState gSampler;
cbuffer PerFrameCB
{
    float4x4 worldMatrices;
    float4x4 viewProjMatrices;
    float4x4 inverseTransposeWorldMatrices;
    float3 camPos;
    float3 albedo;
    float3 metallicRoughness; // metallic,roughness,ao
    Texture2D albedoMap;
    Texture2D normalMap;
    Texture2D metallicMap;
    Texture2D roughnessMap;
};
static const uint LIGHT_COUNT=4;
cbuffer LightCB
{
    float3 lightColor[LIGHT_COUNT];
    float lightAtten[LIGHT_COUNT];
    float3 lightWorldPos[LIGHT_COUNT];
    float lightBias[LIGHT_COUNT];
};

/** Convert RGB to normal (unnormalized).
*/
float3 rgbToNormal(float3 rgb)
{
    return rgb * 2.f - 1.f;
}

VSOut vsMain(VSIn vIn)
{
    VSOut vOut;
    vOut.uv = vIn.uv;
    vOut.posW = mul(worldMatrices,float4(vIn.pos,1.0)).xyz;
    vOut.posH = mul(viewProjMatrices, float4(vOut.posW, 1.f));
    vOut.normalW = mul(inverseTransposeWorldMatrices, float4(vIn.normal,0)).xyz;
    vOut.tangentW = normalize(mul((float3x3)worldMatrices, vIn.tangent));//normalize(mul((float3x3)worldMatrices, vIn.tangent.xyz));
    vOut.bitangentW = normalize(cross(vOut.normalW, vOut.tangentW)*vOut.posH.w);//normalize(mul((float3x3)worldMatrices, vIn.bitangent.xyz));
    return vOut;
}
// Returns a random number based on a float3 and an int.
float random(float3 seed, int i){
	float4 seed4 = float4(seed,i);
	float dot_product = dot(seed4, float4(12.9898,78.233,45.164,94.673));
	return frac(sin(dot_product) * 43758.5453);
}

float4 psMain(VSOut vsOut, uint triangleIndex: SV_PrimitiveID) : SV_TARGET
{
    float2 uv = vsOut.uv;
    float3 worldNormal = vsOut.normalW;
    float3 worldPos = vsOut.posW;
    float3 albedo = albedoMap.Sample(gSampler, uv).rgb * albedo;
    float3 normalTS = rgbToNormal(normalMap.Sample(gSampler,uv).rgb);
    float3x3 TBN = (float3x3(vsOut.tangentW,vsOut.bitangentW,vsOut.normalW));                 //建立TBN矩阵
    float3 normalWS = normalize(mul(normalTS,TBN));// normalize(mul(vsOut.tangentW,normalTS.x) + mul(vsOut.bitangentW,normalTS.y) + mul(worldNormal,normalTS.z));
    float metallic  = metallicMap.Sample(gSampler , uv).r;
    float roughness = roughnessMap.Sample(gSampler, uv).r;
    
    float3 worldLightDir = normalize(lightWorldPos[0]);
    float3 lightDir = normalize(lightWorldPos[0] - worldPos);
    float3 halfDir = normalize(worldLightDir + lightDir);
    float3 diffuseColor = albedo * lightColor[0] * saturate(dot(normalWS,halfDir));

    return float4(diffuseColor,1.0);
}
