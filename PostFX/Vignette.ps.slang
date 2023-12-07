cbuffer PerFrameCB : register(b0)
{
    Texture2D gTexture;
	SamplerState	gSampler;
};


cbuffer VignetteCB : register(b1)
{
    float2 _ScreenParams;
    float2 _Vignette_Center;
    float4 _Vignette_Settings; //new Vector4(settings.intensity * 3f, settings.smoothness * 5f, roundness, settings.rounded ? 1f : 0f));
    float3 _Vignette_Color;
};
float4 main(float2 uv  : TEXCOORD) : SV_TARGET0
{
    float4 color = gTexture.Sample(gSampler, uv);
    float2 d = abs(uv - _Vignette_Center) * _Vignette_Settings.x;
    d.x *= lerp(1.0, _ScreenParams.x / _ScreenParams.y, _Vignette_Settings.w);
    d = pow(d, _Vignette_Settings.z);
    float vfactor = pow(saturate(1.0 - dot(d, d)), _Vignette_Settings.y);
    color.xyz *= lerp(_Vignette_Color, (1.0).xxx, vfactor);		
    return color;
}
