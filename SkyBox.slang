import Scene.Lights.EnvMap;
import Scene.Lights.EnvMapData;
import Utils.Math.MathHelpers;

#ifdef _USE_SPHERICAL_MAP
Texture2D gTexture;
#else
TextureCube gTexture;
#endif
SamplerState envSampler;

uniform float4x4 gWorld;
uniform float4x4 gViewMat;
uniform float4x4 gProjMat;
uniform float gScale;
uniform EnvMap envMap;
uniform EnvMapData data;

void vs(uint vid : SV_VertexID,float3 posL : POSITION, out float3 dir : NORMAL, out float4 posH : SV_POSITION)
{
    const float3 cube_corner_vertex_offsets[8] = {float3(1.0, 1.0, 1.0),
                                                float3(1.0, 1.0, -1.0),
                                                float3(1.0, -1.0, -1.0),
                                                float3(1.0, -1.0, 1.0),
                                                float3(-1.0, 1.0, 1.0),
                                                float3(-1.0, 1.0, -1.0),
                                                float3(-1.0, -1.0, -1.0),
                                                float3(-1.0, -1.0, 1.0)};

    const int cube_triangle_index[36] = {0,1, 2,2, 3, 0, 4,5,1, 1,0,4,7, 6, 5,5,4,7,3,2, 6,6, 7, 3, 4, 0, 3, 3, 7,4, 1, 5,6, 6, 2, 1}; // x+, y+, x-, y-, z+, z-

    float3 world_position = cube_corner_vertex_offsets[cube_triangle_index[vid]];

    dir = posL;
    float4 viewPos = mul(mul(float4(posL,1.0), gWorld), gViewMat);
    posH = mul(viewPos, gProjMat);
    //posH = mul(mul(gProjMat,gViewMat),float4(posL,1.0));
    posH.xy *= gScale;
    posH.z = posH.w * 0.99999;
}

float4 ps(float3 dir : NORMAL) : SV_TARGET
{
#ifdef _USE_ENV_MAP
    float3 color = envMap.eval(dir);
    return float4(color, 1.f);
#else
#ifdef _USE_SPHERICAL_MAP
    float2 uv = world_to_latlong_map(dir);
    return gTexture.Sample(envSampler, uv);
#else
    return gTexture.SampleLevel(envSampler, normalize(dir), 0);
#endif
#endif // _USE_ENV_MAP
}
