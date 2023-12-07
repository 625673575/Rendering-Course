#include "SSR.h"
SSR::SSR(const SampleAppConfig& config) : GBuffer(config) {}

SSR::~SSR() {}

void SSR::onLoad(RenderContext* pRenderContext)
{
    DefineList defineList;
    mpSSRPass = FullScreenPass::create(getDevice(), "Samples/SampleAppTemplate/SSR.slang", defineList);

    GBuffer::onLoad(pRenderContext);
}

void SSR::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    // (jitterX and jitterY are expressed as subpixel quantities divided by the screen resolution
    //  for instance to apply an offset of half pixel along the X axis we set jitterX = 0.5f / Width)
    // float4x4 jitterMat = math::matrixFromTranslation(float3(2.0f * mData.jitterX, 2.0f * mData.jitterY, 0.0f));
    // Apply jitter matrix to the projection matrix, or you can change m02,m12 in matrix
    // mData.projMat = mul(jitterMat, mData.projMat);
    // mpCamera->setJitter(mControls.jitter / screenHeight, mControls.jitter / screenWidth);

    GBuffer::onFrameRender(pRenderContext, mpFbo);

    if (enableSSR)
    {
        auto var = mpSSRPass->getRootVar()["ssrBuf"];
        var["tex"] = mpFbo->getColorTexture(0);
        var["worldNormalTex"] = mpFbo->getColorTexture(2);
        var["worldPosTex"] = mpFbo->getColorTexture(1);
        var["depthTex"] = mpDepthRT;
        var["gSampler"] = gSampler;
        var["invProj"] = math::inverse( mpCamera->getProjMatrix());
        mpCamera->bindShaderData(var["gCamera"]);
        //// run final pass
        mpSSRPass->execute(pRenderContext, pTargetFbo);
    }
    else
    {
        pRenderContext->blit(mpFbo->getColorTexture(0)->getSRV(), pTargetFbo->getRenderTargetView(0));
    }
}

void SSR::onGuiRender(Gui* pGui)
{
    GBuffer::onGuiRender(pGui);

    Gui::Window w(pGui, "SSR", {250, 500});
    w.checkbox("SSR", enableSSR);
}

void SSR::loadScene(const std::filesystem::path& path, const Fbo* pTargetFbo)
{
    GBuffer::loadScene(path, pTargetFbo);
}
