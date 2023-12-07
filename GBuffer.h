#pragma once
#include "Falcor.h"
#include "Core/SampleApp.h"
#include "Core/Pass/RasterPass.h"
#include "Core/Pass/FullScreenPass.h"
#include "Utils/SampleGenerators/DxSamplePattern.h"
#include "Utils/SampleGenerators/HaltonSamplePattern.h"
#include "Utils/SampleGenerators/StratifiedSamplePattern.h"
#include <random>

using namespace Falcor;

struct ChannelDesc
{
    std::string name;      ///< Render pass I/O pin name.
    std::string texname;   ///< Name of corresponding resource in the shader, or empty if it's not a shader variable.
    std::string desc;      ///< Human-readable description of the data.
    bool optional = false; ///< Set to true if the resource is optional.
    ResourceFormat format = ResourceFormat::Unknown; ///< Default format is 'Unknown', which means let the system decide.
};
enum class SamplePattern : uint32_t
{
    Center,
    DirectX,
    Halton,
    Stratified,
};

FALCOR_ENUM_INFO(
    SamplePattern,
    {
        {SamplePattern::Center, "Center"},
        {SamplePattern::DirectX, "DirectX"},
        {SamplePattern::Halton, "Halton"},
        {SamplePattern::Stratified, "Stratified"},
    }
);
using ChannelList = std::vector<ChannelDesc>;
class GBuffer : public SampleApp
{
public:
    GBuffer(const SampleAppConfig& config);
    ~GBuffer();

    virtual void onLoad(RenderContext* pRenderContext) override;
    virtual void onShutdown() override;
    virtual void onResize(uint32_t width, uint32_t height) override;
    virtual void onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo) override;
    virtual void onGuiRender(Gui* pGui) override;
    virtual bool onKeyEvent(const KeyboardEvent& keyEvent) override;
    virtual bool onMouseEvent(const MouseEvent& mouseEvent) override;
    virtual void onHotReload(HotReloadFlags reloaded) override;
    virtual void loadScene(const std::filesystem::path& path, const Fbo* pTargetFbo);

protected:
    ref<CPUSampleGenerator> createSamplePattern(SamplePattern type, uint32_t sampleCount);
    void updateSamplePattern();
    void updateFrameDim(const uint2 frameDim);
    std::mt19937 rng;
    std::uniform_int_distribution<uint32_t> distInt = std::uniform_int_distribution<uint32_t>(0, 100);
    std::uniform_real<float> distReal = std::uniform_real<float>(0.0f, 1.0f);
    uint32_t getRandomInt() { return distInt(rng); }
    float getRandomFloat() { return distReal(rng); }
    uint32_t screenWidth, screenHeight;
    static const ChannelList kGBufferChannels;
    ref<Texture> mpRTs[6];
    ref<Texture> mpDepthRT;
    ref<Scene> mpScene;
    bool showPosW = false;
    bool showNormalW = false;
    bool showTangentW = false;
    bool showFaceNormalW = false;
    bool showMotionVector = false;
    bool enableJitter = false;

    ref<Camera> mpCamera;
    /// Sample generator for camera jitter.
    ref<CPUSampleGenerator> mpSampleGenerator;
    /// Which camera jitter sample pattern to use.
    SamplePattern mSamplePattern = SamplePattern::Halton;
    /// Sample count for camera jitter.
    uint32_t mSampleCount = 16;

    ref<Fbo> mpFbo;
    ref<Sampler> gSampler;
    ref<RasterPass> mpRasterPass;

    uint32_t mFrameCount = 0;
    /// Current frame dimension in pixels. Note this may be different from the window size.
    uint2 mFrameDim = {};
    float2 mInvFrameDim = {};
};
