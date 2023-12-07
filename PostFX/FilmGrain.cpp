#include "FilmGrain.h"

void FilmGrain::onLoad(const SampleAppConfig& config, const ref<Device>& pDevice, RenderContext* pRenderContext)
{
    MultiPassPostProcess::onLoad(config, pDevice, pRenderContext);
    addPass("film", "Samples/SampleAppTemplate/PostFX/FilmGrain.ps.slang");
}
void FilmGrain::onFrameRender(RenderContext* pRenderContext, float time, ref<Texture> color, ref<Texture> depth, ref<Texture> normalWS,ref<Texture>posWS)
{
    auto lutPass = mpPass["film"];
    
    auto var = getRootVar(lutPass.pass);
    var["gTexture"] = color;
    var["gSampler"] = mpLinearSampler;
    var["strength"] = strength;
    var["iGlobalTime"] = time;
    executePass(lutPass, pRenderContext);
}

void FilmGrain::onGui(Gui* pGui)
{
    Gui::Window w(pGui, "FilmGrain", {300, 400}, {10, 80});
    w.slider("strength", strength, .0f, 100.0f);
}

ref<Texture> FilmGrain::getFinalColor()
{
    return mpPass["film"].fbo->getColorTexture(0);
}
