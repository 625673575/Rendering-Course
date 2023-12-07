cbuffer PerFrameCB : register(b0)
{
    Texture2D gTexture;
    SamplerState gSampler;
    float iGlobalTime;
    float strength;
};

float mod(float x, float y)
{
    return x - y * floor(x / y);
}
float4 calcColor(float2 uv)
{
    float4 fragColor = gTexture.Sample(gSampler, uv);
    
   float x = (uv.x + 4.0) * (uv.y + 4.0) * (iGlobalTime * 10.0);
    float4 grain = float4(mod((mod(x, 13) + 1.0) * (mod(x, 123.0) + 1.0), 0.01) - 0.005) * strength;
    
    if (abs(uv.x - 0.5) < 0.002)
        fragColor = float4(0.0);
    
    if (uv.x > 0.5)
    {
        grain = float4(1.0) - grain;
        fragColor = fragColor * grain;
    }
    else
    {
        fragColor = fragColor + grain;
    }
    return fragColor;
}

float4 main(in float2 texC : TEXCOORD) : SV_TARGET
{
    return calcColor(texC);
}
