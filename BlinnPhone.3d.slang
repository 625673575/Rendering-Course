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
    float3 ambientColor;
    float3 baseColor;
    float3 specularColor;
    float glossy;
    Texture2D<float4> diffuse;

    #ifdef ENABLE_SHADOW_MAP
    float4x4 lightViewProjectionMatrix;
    Texture2D<float> shadowMap;
    #endif
};

cbuffer LightCB
{
    float3 lightColor;
    float lightAtten;
    float3 lightWorldPos;
    float lightBias;
};

struct VSOut
{
    float4 posH : SV_POSITION; //clipspace NDC
    float3 posW: POSW;
    float3 normalW : NORMAL; //world normal
    float2 uv : TEXCOORD0; //tex
    
    #ifdef ENABLE_SHADOW_MAP
    float4 shadowCoord:TEXCOORD1;
    float3 lightPos:TEXCOORD2;
    #endif
};

VSOut vsMain(VSIn vIn)
{
    VSOut vOut;
    vIn.pos = vIn.pos;
    vOut.posW = mul(worldMatrices,float4(vIn.pos,1.0)).xyz;
    vOut.posH = mul(viewProjMatrices, float4(vOut.posW, 1.f));
    vOut.normalW = normalize(mul(inverseTransposeWorldMatrices, float4(vIn.normal,0)).xyz);
    vOut.uv = vIn.uv;
    
    #ifdef ENABLE_SHADOW_MAP
    vOut.shadowCoord = mul(lightViewProjectionMatrix,float4(vOut.posW,1.0));
    vOut.lightPos = normalize(lightWorldPos - vOut.posW);
    #endif

    return vOut;
}
// Returns a random number based on a vec3 and an int.
float random(float3 seed, int i){
	float4 seed4 = float4(seed,i);
	float dot_product = dot(seed4, float4(12.9898,78.233,45.164,94.673));
	return frac(sin(dot_product) * 43758.5453);
}
#ifdef ENABLE_SHADOW_MAP
static const float2 poissonDisk[16] =
	{
		float2(0.2770745f, 0.6951455f),
		float2(0.1874257f, -0.02561589f),
		float2(-0.3381929f, 0.8713168f),
		float2(0.5867746f, 0.1087471f),
		float2(-0.3078699f, 0.188545f),
		float2(0.7993396f, 0.4595091f),
		float2(-0.09242552f, 0.5260149f),
		float2(0.3657553f, -0.5329605f),
		float2(-0.3829718f, -0.2476171f),
		float2(-0.01085108f, -0.6966301f),
		float2(0.8404155f, -0.3543923f),
		float2(-0.5186161f, -0.7624033f),
		float2(-0.8135794f, 0.2328489f),
		float2(-0.784665f, -0.2434929f),
		float2(0.9920505f, 0.0855163f),
		float2(-0.687256f, 0.6711345f)
	};
float ShadowCalculation(float4 fragPosLightSpace,float3 lightDir,float3 normal)
{
    // 执行透视除法,变换到[0,1]的范围,注意DirectX中uv的y是反的，此处是个大坑
    float2 projCoords = float2(fragPosLightSpace.x / fragPosLightSpace.w * 0.5 + 0.5,fragPosLightSpace.y / fragPosLightSpace.w * -0.5 + 0.5);
	float visibility = 1.0;
    if((saturate(projCoords.x) == projCoords.x) && (saturate(projCoords.y) == projCoords.y))
    {
        // 根据角度设置合适的bias
        float bias = max(lightBias*10*(1.0-dot(normal,lightDir)),lightBias);
        // 取得当前片元在光源视角下的深度
        float currentDepth = (fragPosLightSpace.z - bias) / fragPosLightSpace.w;
        // 检查当前片元是否在阴影中
        if(currentDepth > 1.0){
            return visibility;
        }
        // 取得最近点的深度(使用[0,1]范围下的fragPosLight当坐标)
        // float closestDepth = shadowMap.Sample(gSampler,projCoords.xy).r;
        // if(closestDepth < currentDepth){
        //     return 0;
        // }
        for(int i=0;i<4;i++){
            int index = i;
            float s = shadowMap.Sample(gSampler,projCoords.xy + poissonDisk[index]/700.0).r;
            visibility -= 0.2*(1-s);
        }
    }
    return visibility;
}
#endif
float4 psMain(VSOut vsOut, uint triangleIndex: SV_PrimitiveID) : SV_TARGET
{
    float2 uv = vsOut.uv;
    float3 worldNormal = vsOut.normalW;
    float3 worldPos = vsOut.posW;
    float4 baseColor = diffuse.Sample(gSampler,uv);
    float3 worldLightDir = normalize(lightWorldPos);
    float3 viewDir = normalize(lightWorldPos - worldPos);
    float3 halfDir = normalize(worldLightDir + viewDir);
    float3 diffuseColor = baseColor.rgb * lightColor * saturate(dot(worldNormal,halfDir));
    float3 specularColor = specularColor * lightColor * pow(saturate(dot(worldNormal,halfDir)),glossy);

    #ifdef ENABLE_SHADOW_MAP
    float visibility = ShadowCalculation(vsOut.shadowCoord,vsOut.normalW, normalize(vsOut.lightPos));
    baseColor.rgb = ambientColor + visibility * (diffuseColor  + specularColor)* lightAtten;
    #else
    baseColor.rgb = ambientColor + (diffuseColor + specularColor)* lightAtten;
    #endif

    return baseColor;
}