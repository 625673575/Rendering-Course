#pragma once
#include "Falcor.h"
#include "Core/SampleApp.h"
#include "GBuffer.h"
#include "Core/Pass/FullScreenPass.h"
#include "PostFX/Lut.h"
#include "PostFX/FilmGrain.h"
#include "PostFX/Glitch.h"

using namespace Falcor;

class PostProcess : public GBuffer
{
public:
    PostProcess(const SampleAppConfig& config);
    ~PostProcess();
    std::vector<std::shared_ptr<MultiPassPostProcess>> pp = {
        std::make_shared<Lut>(),
        std::make_shared<FilmGrain>(),
        std::make_shared<Glitch>()};
    void onLoad(RenderContext* pRenderContext) override;
    void onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo) override;
    void onGuiRender(Gui* pGui) override;
};
