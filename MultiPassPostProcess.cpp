#include "MultiPassPostProcess.h"

void MultiPassPostProcess::onLoad(const SampleAppConfig& config,const ref<Device>& pDevice, RenderContext* pRenderContext)
{
    screenHeight = config.windowDesc.height;
    screenWidth = config.windowDesc.width;
    device = pDevice;
    renderContext = pRenderContext;
    // create rasterizer state
    RasterizerState::Desc rsDesc;
    mpNoCullRastState = RasterizerState::create(rsDesc);

    // Depth test
    DepthStencilState::Desc dsDesc;
    dsDesc.setDepthEnabled(false);
    mpNoDepthDS = DepthStencilState::create(dsDesc);

    // Blend state
    BlendState::Desc blendDesc;
    mpOpaqueBS = BlendState::create(blendDesc);

    // Texture sampler
    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(TextureFilteringMode::Linear, TextureFilteringMode::Linear, TextureFilteringMode::Linear).setMaxAnisotropy(8);
    mpLinearSampler = device->createSampler(samplerDesc);

}

void MultiPassPostProcess::onResize(uint32_t width, uint32_t height)
{
    mAspectRatio = (float(width) / float(height));
}

//void MultiPassPostProcess::setToyShaderParameter(const Pass& pass)
//{ 
//        auto var = getRootVar(pass.pass);
//        var["iResolution"] = float2(screenWidth, screenWidth);
//        var["iGlobalTime"] = time;
//        var["gSampler"] = mpLinearSampler;
//}
void MultiPassPostProcess::addPass(const std::string& passName, const std::string& shaderFile,bool createColorTexture)
{
    ref<FullScreenPass> shaderPass = FullScreenPass::create(device, shaderFile);
    ref<Fbo> fbo = Fbo::create(device);
    if (createColorTexture)
    {
        ref<Texture> tex = device->createTexture2D(
            screenWidth,
            screenHeight,
            ResourceFormat::RGBA8UnormSrgb,
            1,
            1,
            nullptr,
            ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget
        );
        fbo->attachColorTarget(tex, 0);
    }
    Pass pass{shaderPass, fbo};
    mpPass.emplace(passName, pass);
}
