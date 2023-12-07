cbuffer PerFrameCB : register(b0)
{
    Texture2D gTexture;
    Texture2D<float3> gLut;
    SamplerState gSampler;
    float amount;
};
#define X_BLOCK_NNUM 32.0
#define Y_BLOCK_NNUM 1.0
#define BLOCK_PIXELS 32

float4 main(float2 uv : TEXCOORD) : SV_TARGET0
{
    float4 color = gTexture.Sample(gSampler, uv);
    // blue 固定， 也是index。
    float idx = color.b * (BLOCK_PIXELS -1);
    float2 block, lowBlock, highBlock;
    //float2 highBlock;

    // 二维数组的一维存储。
    block.y = idx / Y_BLOCK_NNUM;
    block.x = idx - lowBlock.y * Y_BLOCK_NNUM;

    lowBlock.y = floor( idx / Y_BLOCK_NNUM);
    lowBlock.x = floor(idx - lowBlock.y * Y_BLOCK_NNUM);
    
    highBlock.y = ceil(idx / Y_BLOCK_NNUM);
    highBlock.x = ceil(idx - highBlock.y * Y_BLOCK_NNUM);

    float2 uvlow, uvhigh;
    uvlow.x = lowBlock.x / X_BLOCK_NNUM + 0.5 / 1024 + ((1/ X_BLOCK_NNUM - 1 / 1024) * color.r);
    uvlow.y = lowBlock.y / Y_BLOCK_NNUM + 0.5 / 32 + ((1 / Y_BLOCK_NNUM - 1 / 32) * color.g);;

    uvhigh.x = highBlock.x / X_BLOCK_NNUM + 0.5 / 1024 + ((1 / X_BLOCK_NNUM - 1/ 1024) * color.r);;
    uvhigh.y = highBlock.y / Y_BLOCK_NNUM + 0.5 / 32 + ((1 / Y_BLOCK_NNUM - 1 / 32) * color.g);;

    float3 gradedColLow = gLut.Sample(gSampler, uvlow);
    float3 gradedColHigh =gLut.Sample(gSampler, uvhigh);

    float3 mixColor = lerp(gradedColLow, gradedColHigh, frac(idx));
    color.rgb = lerp(color.rgb, mixColor.rgb, amount);
    return color;
}
