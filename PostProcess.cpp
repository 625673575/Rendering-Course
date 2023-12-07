#include "PostProcess.h"

PostProcess::PostProcess(const SampleAppConfig& config):GBuffer(config) {}

PostProcess::~PostProcess() {}

void PostProcess::onLoad(RenderContext* pRenderContext)
{
    GBuffer::onLoad(pRenderContext);
    for (auto& postPass : pp)
    {
        postPass->onLoad(getConfig(), getDevice(), pRenderContext);
    }
}
void PostProcess::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    GBuffer::onFrameRender(pRenderContext, mpFbo);

    for (size_t i =0;i< pp.size();i++)
    {
        pp[i]->onFrameRender(
            pRenderContext, (float)getGlobalClock().getTime(), i == 0 ? mpRTs[0] : pp[i - 1]->getFinalColor(), mpDepthRT, nullptr, nullptr
        );
    }
    pRenderContext->blit(pp[pp.size() - 1]->getFinalColor()->getSRV(), pTargetFbo->getRenderTargetView(0));
}

void PostProcess::onGuiRender(Gui* pGui)
{
    GBuffer::onGuiRender(pGui);
    for (auto& postPass : pp)
    {
        postPass->onGui(pGui);
    }
}
