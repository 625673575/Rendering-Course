#pragma once
#include "Falcor.h"
#include "Core/SampleApp.h"
#include "Core/Pass/RasterPass.h"
#include <Scene/TriangleMesh.h>
using namespace Falcor;

class DrawInstancing : public SampleApp
{
public:
    DrawInstancing(const SampleAppConfig& config);
    ~DrawInstancing();

    void onLoad(RenderContext* pRenderContext) override;
    void onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo) override;
    void onGuiRender(Gui* pGui) override;

private:
    static const float4 kClearColor;
    const std::string kViewProjMatrices = "viewProjMatrices";
    const std::string kInverseTransposeWorldMatrices = "inverseTransposeWorldMatrices";
    const std::string kWorldMatrices = "worldMatrices";
    static ref<Vao> createVao(const ref<Device>& device, const ref<TriangleMesh>& mesh);
    ref<TriangleMesh> tringleMesh;
    ref<Vao> mpVao;
    ref<Program> mpProgram;
    ref<ProgramVars> mpVars;
    ref<GraphicsState> mpGraphicsState;
    ref<DepthStencilState> mpDepthStencil;
    ref<RasterizerState> mpRasterizeState;
    ref<Texture> mpDiffuseMap;
    ref<Sampler> gSampler;
    float4x4 modelMatrix;
    float4x4 projectionMatrix;
    float4x4 viewMatrix;
    float4x4 VP;
    uint32_t instanceCount = 10;
};
