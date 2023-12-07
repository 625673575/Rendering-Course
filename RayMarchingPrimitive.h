#pragma once
#include "Falcor.h"
#include "Core/SampleApp.h"
#include "GBuffer.h"
#include "Core/Pass/FullScreenPass.h"

using namespace Falcor;

class RayMarchingPrimitive : public GBuffer
{
public:
    RayMarchingPrimitive(const SampleAppConfig& config);
    ~RayMarchingPrimitive();
    void onLoad(RenderContext* pRenderContext) override;
    void onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo) override;
    void onGuiRender(Gui* pGui) override;
};
