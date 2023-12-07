#pragma once
#include "Falcor.h"
#include "Core/SampleApp.h"
#include "Core/Pass/RasterPass.h"
#include <Scene/TriangleMesh.h>
using namespace Falcor;

class MultiRenderTarget : public SampleApp
{
public:
    MultiRenderTarget(const SampleAppConfig& config);
    ~MultiRenderTarget();

    void onLoad(RenderContext* pRenderContext) override;
    void onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo) override;

private:
    static const float4 kClearColor;
    const std::string kViewProjMatrices = "viewProjMatrices";
    const std::string kInverseTransposeWorldMatrices = "inverseTransposeWorldMatrices";
    const std::string kWorldMatrices = "worldMatrices";
    static ref<Vao> createVao(const ref<Device>& device, const ref<TriangleMesh>& mesh);
    ref<TriangleMesh> tringleMesh[3];
    ref<RasterPass> mpRasterPass;
    ref<Vao> mpVao[3];
    ref<Fbo> mpFbo;
    ref<ProgramVars> mpVars;
    ref<DepthStencilState> mpDepthStencil;
    ref<RasterizerState> mpRasterizeState[3];
    ref<Texture> mpDiffuseMap;
    ref<Sampler> gSampler;
    uint4 rectFull,rectUL,rectUR,rectBL,rectBR;
    float4x4 modelMatrix;
    float4x4 projectionMatrix;
    float4x4 viewMatrix;
    float4x4 VP;
};
