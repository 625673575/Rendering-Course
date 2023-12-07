#pragma once
#include "Falcor.h"
#include "GBuffer.h"
#include "Core/Pass/RasterPass.h"

using namespace Falcor;

class SSR : public GBuffer
{
public:
    SSR(const SampleAppConfig& config);
    ~SSR();

    void onLoad(RenderContext* pRenderContext) override;
    void onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo) override;
    void onGuiRender(Gui* pGui) override;
    void loadScene(const std::filesystem::path& path, const Fbo* pTargetFbo);

private:
    bool enableSSR = false;
    ref<FullScreenPass> mpSSRPass;
    ref<Fbo> mpSsrFbo;
};
