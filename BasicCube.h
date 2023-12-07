#pragma once
#include "Falcor.h"
#include "Core/SampleApp.h"
#include "Core/Pass/RasterPass.h"
#include <Scene/TriangleMesh.h>
using namespace Falcor;

class BasicCube : public SampleApp
{
public:
    BasicCube(const SampleAppConfig& config);
    ~BasicCube();

    void onLoad(RenderContext* pRenderContext) override;
    void onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo) override;

private:
    static const float4 kClearColor;
    ref<TriangleMesh> cubeMesh;
    ref<RasterPass> mpRasterPass;
    ref<Vao> mpVao;
    ref<Fbo> mpFbo;
    uint32_t mFrame = 0;
};
