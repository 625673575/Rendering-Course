#include "Glitch.h"

void Glitch::onLoad(const SampleAppConfig& config, const ref<Device>& pDevice, RenderContext* pRenderContext)
{
    MultiPassPostProcess::onLoad(config, pDevice, pRenderContext);
    addPass("glitch", "Samples/SampleAppTemplate/PostFX/Glitch.ps.slang");
}
void Glitch::onFrameRender(RenderContext* pRenderContext, float time, ref<Texture> color, ref<Texture> depth, ref<Texture> normalWS,ref<Texture>posWS)
{
    auto lutPass = mpPass["glitch"];
    
    auto var = getRootVar(lutPass.pass);
    var["gTexture"] = color;
    var["gSampler"] = mpLinearSampler;
    var["strength"] = strength;
    var["iGlobalTime"] = time;
    executePass(lutPass, pRenderContext);
}

void Glitch::onGui(Gui* pGui)
{
    Gui::Window w(pGui, "Glitch", {300, 400}, {10, 80});
    w.slider("strength", strength, .0f, 1.0f);
}

ref<Texture> Glitch::getFinalColor()
{
    return mpPass["glitch"].fbo->getColorTexture(0);
}
