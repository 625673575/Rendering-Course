#pragma once
#include "Falcor.h"
#include "Core/SampleApp.h"
#include "Core/Pass/RasterPass.h"
#include <Scene/TriangleMesh.h>
#include "Core/Pass/FullScreenPass.h"
using namespace Falcor;

class ShadowMap : public SampleApp
{
public:
    ShadowMap(const SampleAppConfig& config);
    ~ShadowMap();

    void onLoad(RenderContext* pRenderContext) override;
    void onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo) override;
    void onGuiRender(Gui* pGui) override;

private:
    void renderShadowMap(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo);
    void renderScene(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo);
    static const float4 kClearColor;
    const std::string kViewProjMatrices = "viewProjMatrices";
    const std::string kInverseTransposeWorldMatrices = "inverseTransposeWorldMatrices";
    const std::string kWorldMatrices = "worldMatrices";
    static ref<Vao> createVao(const ref<Device>& device, const ref<TriangleMesh>& mesh);
    ref<TriangleMesh> trigleMesh[3];
    ref<RasterPass> mpRasterPass;
    ref<RasterPass> mpShadowPass;
    ref<Vao> mpVao[3];
    ref<Fbo> mpFbo;
    ref<Fbo> mpShadowFbo;
    ref<ProgramVars> mpVars;
    ref<ProgramVars> mpShadowVars;
    ref<DepthStencilState> mpDepthStencil;
    ref<RasterizerState> mpRasterizeState[3];
    ref<Texture> mpDiffuseMap;
    ref<Sampler> gSampler;

    GraphicsState::Viewport lightPassView;
    GraphicsState::Viewport mainPassView;

    uint32_t mFrame = 0;
    uint32_t mIndexCount;

    float near = 0.25f, far = 100.0f;
    float4x4 modelMatrix;
    float4x4 projectionMatrix;
    float4x4 viewMatrix;
    float4x4 ViewProjectionMatrix;
    float3 lightPos;
    float4x4 lightProjectionMatrix;
    float4x4 lightViewMatrix;
    float4x4 lightViewProjectionMatrix;
    bool bRenderShadowMap = false;
};
