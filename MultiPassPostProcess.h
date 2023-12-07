#pragma once
#include "Falcor.h"
#include "SampleAppTemplate.h"
#include "Core/Pass/FullScreenPass.h"

using namespace Falcor;

class MultiPassPostProcess 
{
public:
    struct Pass
    {
        ref<FullScreenPass> pass;
        ref<Fbo> fbo;
    };

    virtual void onLoad(const SampleAppConfig& config, const ref<Device>& pDevice, RenderContext* pRenderContext);
    virtual void onResize(uint32_t width, uint32_t height);
    virtual void onFrameRender(
        RenderContext* pRenderContext,
        float time,
        ref<Texture> color,
        ref<Texture> depth,
        ref<Texture> normalWS,
        ref<Texture> posWS
    )=0;
    virtual void onGui(Gui* pGui) {}
    ShaderVar getRootVar(const ref<FullScreenPass>& pass) { return pass->getRootVar()["PerFrameCB"]; }
    ref<FullScreenPass> getPass(const std::string& passName) { return mpPass[passName].pass; }
    ref<Fbo> getFbo(const std::string& passName) { return mpPass[passName].fbo; }
    ref<Texture> getTexture(const std::string& passName) { return mpPass[passName].fbo->getColorTexture(0); }

    //void setToyShaderParameter(const Pass& pass);
    void addPass(const std::string& passName, const std::string& shaderFile, bool createColorTexture = true);
    void executePass(const Pass& pass, RenderContext* pRenderContext) { pass.pass->execute(pRenderContext, pass.fbo); }
    void executePass(const std::string& passName, RenderContext* pRenderContext)
    {
        auto& pass = mpPass[passName];
        pass.pass->execute(pRenderContext, pass.fbo);
    }
    virtual ref<Texture> getFinalColor() = 0;

    ref<Device> device;
    RenderContext* renderContext;
    ref<Sampler> mpLinearSampler;
    float mAspectRatio = 0;
    ref<RasterizerState> mpNoCullRastState;
    ref<DepthStencilState> mpNoDepthDS;
    ref<BlendState> mpOpaqueBS;
    uint32_t screenHeight, screenWidth;
    std::map<std::string, Pass> mpPass;
};
