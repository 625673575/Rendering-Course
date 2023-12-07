#include "TAA.h"
TAA::TAA(const SampleAppConfig& config) : GBuffer(config) {}

TAA::~TAA() {}

void TAA::onLoad(RenderContext* pRenderContext)
{
    DefineList defineList;
    mpTAAPass = FullScreenPass::create(getDevice(), "Samples/SampleAppTemplate/TAA.slang", defineList);
    enableJitter = true;
    GBuffer::onLoad(pRenderContext);
}

void TAA::onResize(uint32_t width, uint32_t height)
{
    GBuffer::onResize(width, height);
}

void TAA::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    // (jitterX and jitterY are expressed as subpixel quantities divided by the screen resolution
    //  for instance to apply an offset of half pixel along the X axis we set jitterX = 0.5f / Width)
    // float4x4 jitterMat = math::matrixFromTranslation(float3(2.0f * mData.jitterX, 2.0f * mData.jitterY, 0.0f));
    // Apply jitter matrix to the projection matrix, or you can change m02,m12 in matrix
    // mData.projMat = mul(jitterMat, mData.projMat);
    // mpCamera->setJitter(mControls.jitter / screenHeight, mControls.jitter / screenWidth);

    GBuffer::onFrameRender(pRenderContext, mpFbo);

    if (enableTAA)
    {
        allocatePrevColor(mpFbo->getColorTexture(0).get());
        auto var = mpTAAPass->getRootVar()["taaBuf"];
        var["tex"] = mpFbo->getColorTexture(0);
        var["motionVectorTex"] = mpFbo->getColorTexture(5);
        var["prevTex"] = mpPrevColor;
        var["alpha"] = mControls.alpha;
        var["colorBoxSigma"] = mControls.colorBoxSigma;
        var["antiFlicker"] = mControls.antiFlicker;
        var["reduceMotionScale"] = mControls.reduceMotionScale;
        var["gSampler"] = gSampler;
        //// run final pass
        mpTAAPass->execute(pRenderContext, pTargetFbo);
        pRenderContext->blit(pTargetFbo->getColorTexture(0)->getSRV(), mpPrevColor->getRTV());
    }
    else
    {
        pRenderContext->blit(mpFbo->getColorTexture(0)->getSRV(), pTargetFbo->getRenderTargetView(0));
    }
}

void TAA::onGuiRender(Gui* pGui)
{
    GBuffer::onGuiRender(pGui);

    Gui::Window w(pGui, "TAA", {250, 500});
    w.checkbox("TAA", enableTAA);
    w.checkbox("Anti Flicker", mControls.antiFlicker);
    w.slider("Box Sigma", mControls.colorBoxSigma,0.1f,10.0f);
    w.slider("Jitter", mControls.jitter, 0.1f, 100.0f);
    w.slider("Reduce Motion Scale", mControls.reduceMotionScale, 10.0f, 1000.0f);
    w.slider("Lerp", mControls.alpha, 0.01f, 1.0f);
}

void TAA::loadScene(const std::filesystem::path& path, const Fbo* pTargetFbo)
{
    GBuffer::loadScene("MEASURE_ONE/MEASURE_ONE.pyscene", pTargetFbo);
}
void TAA::allocatePrevColor(const Texture* pColorOut)
{
    bool allocate = mpPrevColor == nullptr;
    allocate = allocate || (mpPrevColor->getWidth() != pColorOut->getWidth());
    allocate = allocate || (mpPrevColor->getHeight() != pColorOut->getHeight());
    allocate = allocate || (mpPrevColor->getDepth() != pColorOut->getDepth());
    allocate = allocate || (mpPrevColor->getFormat() != pColorOut->getFormat());
    FALCOR_ASSERT(pColorOut->getSampleCount() == 1);

    if (allocate)
        mpPrevColor = getDevice()->createTexture2D(
            pColorOut->getWidth(),
            pColorOut->getHeight(),
            pColorOut->getFormat(),
            1,
            1,
            nullptr,
            ResourceBindFlags::RenderTarget | ResourceBindFlags::ShaderResource
        );
}
