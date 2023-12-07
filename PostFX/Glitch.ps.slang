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
//remaps inteval [a;b] to [0;1]
float remap(float t, float a, float b)
{
    return saturate((t - a) / (b - a));
}

//note: /\ t=[0;0.5;1], y=[0;1;0]
float linterp(float t)
{
    return saturate(1.0 - abs(2.0 * t - 1.0));
}

float3 spectrum_offset(float t)
{
    float t0 = 3.0 * t - 1.5;
    return clamp(float3(-t0, 1.0 - abs(t0), t0), 0.0, 1.0);
}

//note: [0;1]
float rand(float2 n)
{
    return frac(sin(dot(n.xy, float2(12.9898, 78.233))) * 43758.5453);
}

//note: [-1;1]
float srand(float2 n)
{
    return rand(n) * 2.0 - 1.0;
}

float mytrunc(float x, float num_levels)
{
    return floor(x * num_levels) / num_levels;
}
float2 mytrunc(float2 x, float num_levels)
{
    return floor(x * num_levels) / num_levels;
}

float4 calcColor(float2 uv)
{
    float time = mod(iGlobalTime, 32.0); // + modelmat[0].x + modelmat[0].z;

    float GLITCH = strength;
    
    float gnm = saturate(GLITCH);
    float rnd0 = rand(mytrunc(float2(time, time), 6.0));
    float r0 = saturate((1.0 - gnm) * 0.7 + rnd0);
    float rnd1 = rand(float2(mytrunc(uv.x, 10.0 * r0), time)); //horz
	//float r1 = 1.0f - sat( (1.0f-gnm)*0.5f + rnd1 );
    float r1 = 0.5 - 0.5 * gnm + rnd1;
    r1 = 1.0 - max(0.0, ((r1 < 1.0) ? r1 : 0.9999999)); //note: weird ass bug on old drivers
    float rnd2 = rand(float2(mytrunc(uv.y, 40.0 * r1), time)); //vert
    float r2 = saturate(rnd2);

    float rnd3 = rand(float2(mytrunc(uv.y, 10.0 * r0), time));
    float r3 = (1.0 - saturate(rnd3 + 0.8)) - 0.1;

    float pxrnd = rand(uv + time);

    float ofs = 0.05 * r2 * GLITCH * (rnd0 > 0.5 ? 1.0 : -1.0);
    ofs += 0.5 * pxrnd * ofs;

    uv.y += 0.1 * r3 * GLITCH;

    const int NUM_SAMPLES = 10;
    const float RCP_NUM_SAMPLES_F = 1.0 / float(NUM_SAMPLES);
    
    float4 sum = float4(0.0);
    float3 wsum = float3(0.0);
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        float t = float(i) * RCP_NUM_SAMPLES_F;
        uv.x = saturate(uv.x + ofs * t);
        float4 samplecol = gTexture.Sample(gSampler, uv);
        float3 s = spectrum_offset(t);
        samplecol.rgb = samplecol.rgb * s;
        sum += samplecol;
        wsum += s;
    }
    sum.rgb /= wsum;
    sum.a *= RCP_NUM_SAMPLES_F;
    return sum;
}

float4 main(in float2 texC : TEXCOORD) : SV_TARGET
{
    return calcColor(texC);
}
