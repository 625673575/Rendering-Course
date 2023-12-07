#pragma once
#include "Falcor.h"
#include "GBuffer.h"
#include "Core/Pass/RasterPass.h"

using namespace Falcor;

class FXAA : public GBuffer
{
public:
    FXAA(const SampleAppConfig& config);
    ~FXAA();

    void onLoad(RenderContext* pRenderContext) override;
    void onShutdown() override;
    void onResize(uint32_t width, uint32_t height) override;
    void onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo) override;
    void onGuiRender(Gui* pGui) override;
    bool onKeyEvent(const KeyboardEvent& keyEvent) override;
    bool onMouseEvent(const MouseEvent& mouseEvent) override;
    void onHotReload(HotReloadFlags reloaded) override;
    void loadScene(const std::filesystem::path& path, const Fbo* pTargetFbo);

private:
    ref<FullScreenPass> mpFullScreenPass;
    float rcpX = 0.1f, rcpY = 0.1f;
    bool enableFXAA = true;
};
