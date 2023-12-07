#include "GBuffer.h"

static const float4 kClearColor(0.38f, 0.52f, 0.10f, 1);
static const std::string kDefaultScene = "Arcade/Arcade.pyscene";

const ChannelList GBuffer::kGBufferChannels = {
    // clang-format off
    { "color",          "gColor",           "Final result",                                      true /* optional */, ResourceFormat::RGBA32Float },
    { "posW",           "gPosW",            "Position in world space",                           true /* optional */, ResourceFormat::RGBA32Float },
    { "normW",          "gNormW",           "Shading normal in world space",                     true /* optional */, ResourceFormat::RGBA32Float },
    { "tangentW",       "gTangentW",        "Shading tangent in world space (xyz) and sign (w)", true /* optional */, ResourceFormat::RGBA32Float },
    { "faceNormalW",    "gFaceNormalW",     "Face normal in world space",                        true /* optional */, ResourceFormat::RGBA32Float },
    { "mvec",           "gMotionVector",    "Motion vector in clip space",                       true /* optional */, ResourceFormat::RG32Float },
    //{ "texC",           "gTexC",            "Texture coordinate",                                true /* optional */, ResourceFormat::RG32Float   },
    //{ "texGrads",       "gTexGrads",        "Texture gradients (ddx, ddy)",                      true /* optional */, ResourceFormat::RGBA16Float },
    //{ "mvec",           "gMotionVector",    "Motion vector",                                     true /* optional */, ResourceFormat::RG32Float   },
    //{ "mtlData",        "gMaterialData",    "Material data (ID, header.x, header.y, lobes)",     true /* optional */, ResourceFormat::RGBA32Uint  },
    // clang-format on
};
GBuffer::GBuffer(const SampleAppConfig& config) : SampleApp(config)
{
    screenHeight = config.windowDesc.height;
    screenWidth = config.windowDesc.width;
}

GBuffer::~GBuffer()
{
    //
}

void GBuffer::onLoad(RenderContext* pRenderContext)
{
    mpFbo = Fbo::create(getDevice());

    float height = getConfig().windowDesc.height;
    float width = getConfig().windowDesc.width;
    for (int i = 0; i < kGBufferChannels.size(); i++)
    {
        mpRTs[i] = getDevice()->createTexture2D(
            width, height, kGBufferChannels[i].format, 1, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget
        );
        mpFbo->attachColorTarget(mpRTs[i], i);
    }
    mpDepthRT = getDevice()->createTexture2D(
        width, height, ResourceFormat::D32Float, 1, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::DepthStencil
    );
    mpFbo->attachDepthStencilTarget(mpDepthRT);
    Sampler::Desc samplerDesc;
    samplerDesc.setComparisonFunc(ComparisonFunc::Never);
    gSampler = getDevice()->createSampler(samplerDesc);
    loadScene(kDefaultScene, mpFbo.get());

    if (enableJitter)
        updateSamplePattern(); // for TAA sample, camera jitter
}

void GBuffer::onShutdown() {}

void GBuffer::onResize(uint32_t width, uint32_t height)
{
    //
}

void GBuffer::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    pRenderContext->clearFbo(pTargetFbo.get(), kClearColor, 1.0f, 0, FboAttachmentType::All);
    if (mpScene)
    {
        updateFrameDim(uint2(pTargetFbo->getWidth(), pTargetFbo->getHeight()));
        Scene::UpdateFlags updates = mpScene->update(pRenderContext, getGlobalClock().getTime());
        if (is_set(updates, Scene::UpdateFlags::GeometryChanged))
            FALCOR_THROW("This sample does not support scene geometry changes.");
        if (is_set(updates, Scene::UpdateFlags::RecompileNeeded))
            FALCOR_THROW("This sample does not support scene changes that require shader recompilation.");

        FALCOR_ASSERT(mpScene);
        FALCOR_PROFILE(pRenderContext, "renderRaster");

        mpRasterPass->getRootVar()["PerFrameCB"]["gFrameDim"] = mFrameDim;

        mpRasterPass->getState()->setFbo(pTargetFbo);
        mpScene->rasterize(pRenderContext, mpRasterPass->getState().get(), mpRasterPass->getVars().get());
    }
}

void GBuffer::onGuiRender(Gui* pGui)
{
    Gui::Window w(pGui, "Scene", {300, 500});
    renderGlobalUI(pGui);
    mpScene->renderUI(w);
    const uint2 windowSize = {screenWidth / 4 + 8, screenHeight / 4 + 8};
    const uint2 imageSize = {screenWidth / 4, screenHeight / 4};
#define GUI_CB(name, idx)                            \
    w.checkbox(#name, show##name);                   \
    if (show##name)                                    \
    {                                                \
        Gui::Window d(pGui, #name, windowSize);      \
        d.image(#name, mpRTs[idx].get(), imageSize); \
    }
    
    GUI_CB(PosW, 1)
    GUI_CB(NormalW, 2)
    GUI_CB(TangentW, 3)
    GUI_CB(FaceNormalW, 4)
    GUI_CB(MotionVector, 5)
}

bool GBuffer::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return mpScene ? mpScene->onKeyEvent(keyEvent) : false;
}

bool GBuffer::onMouseEvent(const MouseEvent& mouseEvent)
{
    return mpScene ? mpScene->onMouseEvent(mouseEvent) : false;
}

void GBuffer::onHotReload(HotReloadFlags reloaded) {}

void GBuffer::loadScene(const std::filesystem::path& path, const Fbo* pTargetFbo)
{
    mpScene = Scene::create(getDevice(), path);
    mpCamera = mpScene->getCamera();

    // Update the controllers
    float radius = mpScene->getSceneBounds().radius();
    mpScene->setCameraSpeed(radius * 0.25f);
    float nearZ = std::max(0.1f, radius / 750.0f);
    float farZ = radius * 10;
    mpCamera->setDepthRange(nearZ, farZ);
    mpCamera->setAspectRatio((float)pTargetFbo->getWidth() / (float)pTargetFbo->getHeight());

    // Get shader modules and type conformances for types used by the scene.
    // These need to be set on the program in order to use Falcor's material system.
    auto shaderModules = mpScene->getShaderModules();
    auto typeConformances = mpScene->getTypeConformances();

    // Get scene defines. These need to be set on any program using the scene.
    auto defines = mpScene->getSceneDefines();

    // Create raster pass.
    // This utility wraps the creation of the program and vars, and sets the necessary scene defines.
    ProgramDesc rasterProgDesc;
    rasterProgDesc.addShaderModules(shaderModules);
    rasterProgDesc.addShaderLibrary("Samples//SampleAppTemplate/GBuffer.3d.slang").vsEntry("vsMain").psEntry("psMain");
    rasterProgDesc.addTypeConformances(typeConformances);

    mpRasterPass = RasterPass::create(getDevice(), rasterProgDesc, defines);
}

ref<CPUSampleGenerator> GBuffer::createSamplePattern(SamplePattern type, uint32_t sampleCount)
{
    switch (type)
    {
    case SamplePattern::Center:
        return nullptr;
    case SamplePattern::DirectX:
        return DxSamplePattern::create(sampleCount);
    case SamplePattern::Halton:
        return HaltonSamplePattern::create(sampleCount);
    case SamplePattern::Stratified:
        return StratifiedSamplePattern::create(sampleCount);
    default:
        FALCOR_UNREACHABLE();
        return nullptr;
    }
}

void GBuffer::updateSamplePattern()
{
    mpSampleGenerator = createSamplePattern(mSamplePattern, mSampleCount);
    if (mpSampleGenerator)
        mSampleCount = mpSampleGenerator->getSampleCount();
}

void GBuffer::updateFrameDim(const uint2 frameDim)
{
    FALCOR_ASSERT(frameDim.x > 0 && frameDim.y > 0);
    mFrameDim = frameDim;
    mInvFrameDim = 1.f / float2(frameDim);

    // Update sample generator for camera jitter.
    if (mpScene)
        mpScene->getCamera()->setPatternGenerator(mpSampleGenerator, mInvFrameDim);
}
