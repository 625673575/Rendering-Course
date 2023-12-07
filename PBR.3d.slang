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
    
    #ifdef ENABLE_SHADOW_MAP
    float4 shadowCoord:TEXCOORD1;
    float3 lightPos:TEXCOORD2;
    #endif
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

    #ifdef ENABLE_SHADOW_MAP
    float4x4 lightViewProjectionMatrix;
    Texture2D<float> shadowMap;
    #endif
};
struct SLight
{
    float4  intensity;
    float4  dirW;
    float4  posW;
    uint32_t    type;     
    float   bias;
};
static const uint LIGHT_COUNT = 2;
cbuffer LightCB
{
    SLight lights[LIGHT_COUNT];
};
/** Convert RGB to normal (unnormalized).
*/
float3 rgbToNormal(float3 rgb)
{
    return rgb * 2.f - 1.f;
}

/** Convert RG to normal (unnormalized).
*/
float3 rgToNormal(float2 rg)
{
    float3 n;
    n.xy = rg * 2.f - 1.f;

    // Saturate because error from BC5 can break the sqrt
    n.z = saturate(dot(rg, rg)); // z = r*r + g*g
    n.z = sqrt(1.f - n.z);
    return n;
}

static const float PI = 3.14159265359;
float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
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
    
    #ifdef ENABLE_SHADOW_MAP
    vOut.shadowCoord = mul(lightViewProjectionMatrix,float4(vOut.posW,1.0));
    vOut.lightPos = normalize(lightWorldPos - vOut.posW);
    #endif

    return vOut;
}
// Returns a random number based on a float3 and an int.
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
    float2 projCoords = float2(fragPosLightSpace.x / fragPosLightSpace.w * 0.5 + 0.5,fragPosLightSpace.y / fragPosLightSpace.w * -0.5 + 0.5);
	float visibility = 1.0;
    if((saturate(projCoords.x) == projCoords.x) && (saturate(projCoords.y) == projCoords.y))
    {
        float bias = max(lightBias*10*(1.0-dot(normal,lightDir)),lightBias);
        float currentDepth = (fragPosLightSpace.z - bias) / fragPosLightSpace.w;
        if(currentDepth > 1.0){
            return visibility;
        }
        for(int i = 0; i < 4; i++){
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
    float3 albedo = albedoMap.Sample(gSampler, uv).rgb * albedo;
    float3 normalTS = rgbToNormal(normalMap.Sample(gSampler,uv).rgb);
    float3x3 TBN = (float3x3(vsOut.tangentW,vsOut.bitangentW,vsOut.normalW));                 //建立TBN矩阵
    float3 normalWS = normalize(mul(normalTS,TBN));// normalize(mul(vsOut.tangentW,normalTS.x) + mul(vsOut.bitangentW,normalTS.y) + mul(worldNormal,normalTS.z));
    float metallic  = metallicMap.Sample(gSampler , uv).r * metallicRoughness.r;
    float roughness = roughnessMap.Sample(gSampler, uv).r * metallicRoughness.g;

    float3 N = normalize(normalWS);
    float3 V = normalize(camPos - worldPos);
    float3 FO = float3(0.04);
    FO = lerp(FO,albedo,metallic);
    float3 Lo = float3(0.0);
    for(int i = 0; i < LIGHT_COUNT; i++){
        float3 L = normalize(lights[i].posW.xyz - worldPos);
        float3 H = normalize(V + L);
        float distance = length(lights[i].posW.xyz - worldPos);
        float attenuation = 1.0/(distance * distance);
        float3 radiance = lights[i].intensity.xyz * attenuation;
        //Cook-torrance BRDF
        float NDF = DistributionGGX(N,H,roughness);
        float G = GeometrySmith(N,V,L,roughness);
        float3 F = fresnelSchlick(max(dot(H,V),0.0),FO);

        float3 kS = F;
        float3 kD = float3(1.0) - kS;
        kD *= 1.0 - metallic;

        float3 numerator =  NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        float3 specular = numerator / denominator;  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);                
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    } 

    float3 ambient = 0.03 * albedo;
    float3 color = ambient + Lo;
	
    color = color / (color + 1.0);
    //color = pow(color, float(1.0/2.2)); //gamma correction

    #ifdef ENABLE_SHADOW_MAP
    float visibility = ShadowCalculation(vsOut.shadowCoord,vsOut.normalW, normalize(vsOut.lightPos));
    albedo.rgb *= visibility;
    #endif
    
    return float4(color,1.0);
}
