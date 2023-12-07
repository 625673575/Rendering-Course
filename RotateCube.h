#pragma once
#include "Falcor.h"
#include "Core/SampleApp.h"
#include "Core/Pass/RasterPass.h"
#include <Scene/TriangleMesh.h>
#include "Scene/Camera/Camera.h"
using namespace Falcor;

class RotateCube : public SampleApp
{
public:
    RotateCube(const SampleAppConfig& config);
    ~RotateCube();

    void onLoad(RenderContext* pRenderContext) override;
    void onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo) override;

private:
    static const float4 kClearColor;
    ref<RasterPass> mpRasterPass;
    ref<Vao> mpVao;
    ref<Fbo> mpFbo;
    ref<ProgramVars> mpVars;
    ref<DepthStencilState> mpDepthStencil;
    ref<RasterizerState> mpRasterizeState;
    const int width = 2560,height = 1600;
    uint32_t mFrame = 0;
    float4x4 modelMatrix;
};
