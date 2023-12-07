#pragma once
#include "Falcor.h"
#include "GBuffer.h"
#include "Core/Pass/RasterPass.h"

using namespace Falcor;

class TAA : public GBuffer
{
public:
    TAA(const SampleAppConfig& config);
    ~TAA();

    void onLoad(RenderContext* pRenderContext) override;
    void onResize(uint32_t width, uint32_t height) override;
    void onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo) override;
    void onGuiRender(Gui* pGui) override;
    void loadScene(const std::filesystem::path& path, const Fbo* pTargetFbo) override;

private:
    void allocatePrevColor(const Texture* pColorOut);
    bool enableTAA = false;
    ref<FullScreenPass> mpTAAPass;
    ref<Fbo> mpTaaFbo;

    struct
    {
        float jitter = 0.5f; // pixel offset
        float alpha = 0.1f;  // lerp with history, the bigger, the less anti, but less ghosting
        float colorBoxSigma = 1.0f;
        float reduceMotionScale = 100.0f;
        bool antiFlicker = true;
    } mControls;

    ref<Texture> mpPrevColor;
};
