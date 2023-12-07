#include "Lut.h"

void Lut::onLoad(const SampleAppConfig& config, const ref<Device>& pDevice, RenderContext* pRenderContext)
{
    MultiPassPostProcess::onLoad(config, pDevice, pRenderContext);
    addPass("lut", "Samples/SampleAppTemplate/PostFX/Lut.ps.slang");
    lutTex = Texture::createFromFile(device,getRuntimeDirectory() / "data/LUT/Warm Purple.png",false,false);
}
void Lut::onFrameRender(RenderContext* pRenderContext, float time, ref<Texture> color, ref<Texture> depth, ref<Texture> normalWS,ref<Texture>posWS)
{
    auto lutPass = mpPass["lut"];
    
    auto var = getRootVar(lutPass.pass);
    var["gTexture"] = color;
    var["gLut"] = lutTex;
    var["gSampler"] = mpLinearSampler;
    var["amount"] = amount;
    executePass(lutPass, pRenderContext);
}

void Lut::onGui(Gui* pGui)
{
    Gui::Window w(pGui, "Lut", {300, 400}, {10, 80});
    w.slider("amount", amount, .0f, 1.0f);

    if (w.button("Load LUT Texture"))
    {
        std::filesystem::path filename;
        FileDialogFilterVec filters = {{"bmp"}, {"jpg"}, {"dds"}, {"png"}, {"tiff"}, {"tif"}, {"tga"}};
        if (openFileDialog(filters, filename))
        {
            lutTex = Texture::createFromFile(device, filename, false, false);
        }
    }
}

ref<Texture> Lut::getFinalColor()
{
    return mpPass["lut"].fbo->getColorTexture(0);
}
