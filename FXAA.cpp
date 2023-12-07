#include "FXAA.h"

FXAA::FXAA(const SampleAppConfig& config) : GBuffer(config)
{
    //
}

FXAA::~FXAA()
{
    //
}

void FXAA::onLoad(RenderContext* pRenderContext)
{
    DefineList defineList;
    //defineList.add("FXAA_SEARCH_ACCELERATION", "1");
    mpFullScreenPass = FullScreenPass::create(getDevice(), "Samples/SampleAppTemplate/FXAA.ps.slang", defineList);
    GBuffer::onLoad(pRenderContext);
}

void FXAA::onShutdown()
{
    GBuffer::onShutdown();
}

void FXAA::onResize(uint32_t width, uint32_t height)
{
    GBuffer::onResize(width,height);
}

void FXAA::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    GBuffer::onFrameRender(pRenderContext, mpFbo);

    if (enableFXAA)
    {
        auto var = mpFullScreenPass->getRootVar()["fxaaBuf"];
        var["tex"] = mpFbo->getColorTexture(0);
        var["splr"] = gSampler;
        var["rcpFrame"] = float4(rcpX, rcpY, 1.0f, 1.0f);
        // run final pass
        mpFullScreenPass->execute(pRenderContext, pTargetFbo);
    }
    else
    {
        pRenderContext->blit(mpFbo->getColorTexture(0)->getSRV(), pTargetFbo->getRenderTargetView(0));
    }
}

void FXAA::onGuiRender(Gui* pGui)
{
    GBuffer::onGuiRender(pGui);

    Gui::Window w(pGui, "FXAA", {250, 500});
    w.slider("RCPX", rcpX, 0.01f, 1.0f);
    w.slider("RCPY", rcpY, 0.01f, 1.0f);
    w.checkbox("FXAA", enableFXAA);
}

bool FXAA::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return GBuffer::onKeyEvent(keyEvent);
}

bool FXAA::onMouseEvent(const MouseEvent& mouseEvent)
{
    return GBuffer::onMouseEvent(mouseEvent);
}

void FXAA::onHotReload(HotReloadFlags reloaded)
{
    GBuffer::onHotReload(reloaded);
}

void FXAA::loadScene(const std::filesystem::path& path, const Fbo* pTargetFbo)
{
    GBuffer::loadScene(path, pTargetFbo);
}
