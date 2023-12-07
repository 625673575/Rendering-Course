struct VSIn
{
    float3 pos : POSITION;
};
cbuffer PerFrameCB
{
    float4x4 transform;
    float scale;
};

struct VSOut
{
    float4 posH : SV_POSITION;
};

VSOut vsMain(VSIn vIn)
{
    VSOut vOut;
    vOut.posH = mul(transform,float4(vIn.pos, 1.0f));
    vOut.posH.z = 0.5;
    return vOut;
}

float4 psMain(VSOut vsOut, uint triangleIndex: SV_PrimitiveID) : SV_TARGET
{
    return float4(0.8f);
}
