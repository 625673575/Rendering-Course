#pragma once
#include "Falcor.h"
#include "GBuffer.h"
#include "Core/Pass/RasterPass.h"

using namespace Falcor;

class SSAO : public GBuffer
{
public:
    enum class SampleDistribution : uint32_t
    {
        Random,
        UniformHammersley,
        CosineHammersley
    };
    struct SSAOData
    {
        static const uint32_t kMaxSamples = 32;

        float4 sampleKernel[kMaxSamples];
        float2 noiseScale = float2(1, 1);
        uint32_t kernelSize = 16;
        float radius = 0.1f;
    };

public:
    SSAO(const SampleAppConfig& config);
    ~SSAO();

    void setSampleRadius(float radius);
    void setKernelSize(uint32_t kernelSize);
    void setDistribution(uint32_t distribution);
    void setKernel();
    void setNoiseTexture(uint32_t width, uint32_t height);

    ref<Texture> generateAOMap(
        RenderContext* pRenderContext,
        const Camera* pCamera,
        const ref<Texture>& pDepthTexture,
        const ref<Texture>& pNoiseTexture,
        const ref<Texture>& pNormalTexture
    );
    void blurMap(RenderContext* pRenderContext, const ref<Texture>& pDepthTexture, uint32_t downSample);
    void onLoad(RenderContext* pRenderContext) override;
    void onShutdown() override;
    void onResize(uint32_t width, uint32_t height) override;
    void onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo) override;
    void onGuiRender(Gui* pGui) override;
    bool onKeyEvent(const KeyboardEvent& keyEvent) override;
    bool onMouseEvent(const MouseEvent& mouseEvent) override;
    void onHotReload(HotReloadFlags reloaded) override;
    void loadScene(const std::filesystem::path& path, const Fbo* pTargetFbo);

private:
    ref<FullScreenPass> mpSSAOPass;
    ref<Fbo> mpAOFbo;

    static const uint32_t DOWNSAMPLE_COUNT = 4;

    ref<ComputePass> mpDownsamplePass, mpMergePass;
    ref<Texture> mpDownsampleTexture[DOWNSAMPLE_COUNT];
    ref<Texture> mpMergedBlurTexture;

    uint2 mAoMapSize = uint2(2048, 1024);

    bool enableSSAO = false;
    bool showSSAOMap = false;
    bool mShowNoiseTex = false;
    uint32_t blurImage = 0;

    SSAOData mData;
    bool mDirty = false;
    SampleDistribution mHemisphereDistribution = SampleDistribution::CosineHammersley;

    ref<Sampler> mpNoiseSampler;
    ref<Texture> mpNoiseTexture;
    uint2 mNoiseSize = uint2(16);

    struct
    {
        ref<FullScreenPass> pApplySSAOPass;
        ref<Fbo> pFbo;
    } mComposeData;
};
