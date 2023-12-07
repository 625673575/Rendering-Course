#include "SSAO.h"
#include "Utils/Math/FalcorMath.h"

namespace
{
const Gui::DropdownList kDistributionDropdown = {
    {(uint32_t)SSAO::SampleDistribution::Random, "Random"},
    {(uint32_t)SSAO::SampleDistribution::UniformHammersley, "Uniform Hammersley"},
    {(uint32_t)SSAO::SampleDistribution::CosineHammersley, "Cosine Hammersley"}};

const std::string kAoMapSize = "aoMapSize";
const std::string kKernelSize = "kernelSize";
const std::string kNoiseSize = "noiseSize";
const std::string kDistribution = "distribution";
const std::string kRadius = "radius";
const std::string kBlurKernelWidth = "blurWidth";
const std::string kBlurSigma = "blurSigma";

const std::string kColorIn = "colorIn";
const std::string kColorOut = "colorOut";
const std::string kDepth = "depth";
const std::string kNormals = "normals";
const std::string kAoMap = "AoMap";
} // namespace

SSAO::SSAO(const SampleAppConfig& config) : GBuffer(config)
{
    //
}

SSAO::~SSAO()
{
    //
}
void SSAO::setSampleRadius(float radius)
{
    mData.radius = radius;
    mDirty = true;
}

void SSAO::setKernelSize(uint32_t kernelSize)
{
    kernelSize = math::clamp(kernelSize, 1u, SSAOData::kMaxSamples);
    mData.kernelSize = kernelSize;
    setKernel();
}

void SSAO::setDistribution(uint32_t distribution)
{
    mHemisphereDistribution = (SampleDistribution)distribution;
    setKernel();
}

void SSAO::setKernel()
{
    auto nextRandom11 = [&]() -> float { return getRandomFloat() * 2.0f - 1.0f; };
    for (uint32_t i = 0; i < mData.kernelSize; i++)
    {
        // Hemisphere in the Z+ direction
        float3 p;
        switch (mHemisphereDistribution)
        {
        case SampleDistribution::Random:
            p = math::normalize(float3(nextRandom11(), nextRandom11(), getRandomFloat()));
            break;

        case SampleDistribution::UniformHammersley:
            p = hammersleyUniform(i, mData.kernelSize);
            break;

        case SampleDistribution::CosineHammersley:
            p = hammersleyCosine(i, mData.kernelSize);
            break;
        }

        mData.sampleKernel[i] = float4(p, 0.0f);

        // Skew sample point distance on a curve so more cluster around the origin
        float dist = (float)i / (float)mData.kernelSize;
        dist = math::lerp(0.1f, 1.0f, dist * dist);
        mData.sampleKernel[i] *= dist;
    }

    mDirty = true;
}

uint packUnorm8_unsafe(float v)
{
    return (uint)trunc(v * 255.f + 0.5f);
}
uint packUnorm8(float v)
{
    v = isnan(v) ? 0.f : math::saturate(v);
    return packUnorm8_unsafe(v);
}
uint packUnorm4x8(float4 v)
{
    return (packUnorm8(v.w) << 24) | (packUnorm8(v.z) << 16) | (packUnorm8(v.y) << 8) | packUnorm8(v.x);
}
void SSAO::setNoiseTexture(uint32_t width, uint32_t height)
{
    std::vector<uint32_t> data;
    data.resize(width * height);

    for (uint32_t i = 0; i < width * height; i++)
    {
        // Random directions on the XY plane
        float2 dir = math::normalize(float2(getRandomFloat(), getRandomFloat()));
        data[i] = packUnorm4x8(float4(dir, 0.0f, 1.0f));
    }

    mpNoiseTexture = getDevice()->createTexture2D(
        width,
        height,
        ResourceFormat::RGBA8UnormSrgb,
        1,
        1,
        data.data(),
        ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget
    );

    mData.noiseScale = float2(mpAOFbo->getWidth(), mpAOFbo->getHeight()) / float2(width, height);

    mDirty = true;
}

ref<Texture> SSAO::generateAOMap(
    RenderContext* pRenderContext,
    const Camera* pCamera,
    const ref<Texture>& pDepthTexture,
    const ref<Texture>& pNoiseTexture,
    const ref<Texture>& pNormalTexture
)
{
    if (mDirty)
    {
        ShaderVar var = mpSSAOPass->getRootVar()["StaticCB"];
        if (var.isValid())
            var.setBlob(mData);
        mDirty = false;
    }

    {
        ShaderVar var = mpSSAOPass->getRootVar()["PerFrameCB"];
        pCamera->bindShaderData(var["gCamera"]);
    }

    // Update state/vars
    auto rootVar = mpSSAOPass->getRootVar();

    rootVar["gNoiseSampler"] = mpNoiseSampler;
    rootVar["gTextureSampler"] = gSampler;
    rootVar["gDepthTex"] = pDepthTexture;
    rootVar["gNoiseTex"] = pNoiseTexture;
    rootVar["gNormalTex"] = pNormalTexture;

    // Generate AO
    mpSSAOPass->execute(pRenderContext, mpAOFbo);
    return mpAOFbo->getColorTexture(0);
}

void SSAO::blurMap(RenderContext* pRenderContext, const ref<Texture>& pSrc, uint32_t downSample)
{
    const uint2 resolution = uint2(pSrc->getWidth(), pSrc->getHeight());
    auto var = mpDownsamplePass->getRootVar();
    var["gLinearSampler"] = gSampler;
    for (uint32_t level = 0; level < downSample; ++level)
    {
        uint2 res = {std::max(1u, resolution.x >> (level + 1)), std::max(1u, resolution.y >> (level + 1))};
        float2 invres = float2(1.f / res.x, 1.f / res.y);
        var["PerFrameCB"]["gResolution"] = res;
        var["PerFrameCB"]["gInvRes"] = invres;
        var["gSrc"] = level > 0 ? mpDownsampleTexture[level - 1] : pSrc;
        var["gDst"] = mpDownsampleTexture[level];
        mpDownsamplePass->execute(pRenderContext, uint3(res, 1));
    }
    var = mpMergePass->getRootVar();
    var["PerFrameCB"]["gResolution"] = resolution;
    float2 invres = float2(1.f / resolution.x, 1.f / resolution.y);
    var["PerFrameCB"]["gInvRes"] = invres;
    var["gDst"] = mpMergedBlurTexture;
    var["gSampleCount"] = downSample;
    for (uint32_t level = 0; level < downSample; ++level)
    {
        var["gSrcArray"][level] = mpDownsampleTexture[level];
    }
    mpMergePass->execute(pRenderContext, uint3(resolution, 1));
}

void SSAO::onLoad(RenderContext* pRenderContext)
{
    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(TextureFilteringMode::Point, TextureFilteringMode::Point, TextureFilteringMode::Point)
        .setAddressingMode(TextureAddressingMode::Wrap, TextureAddressingMode::Wrap, TextureAddressingMode::Wrap);
    mpNoiseSampler = getDevice()->createSampler(samplerDesc);

    mpSSAOPass = FullScreenPass::create(getDevice(), "Samples/SampleAppTemplate/SSAO.ps.slang");

    // mpBlurGraph = GaussianBlur::create(getDevice(), mBlurDict);

    mComposeData.pApplySSAOPass = FullScreenPass::create(getDevice(), "Samples/SampleAppTemplate/SSAOApply.ps.slang");
    mComposeData.pApplySSAOPass->getRootVar()["gSampler"] = gSampler;
    mComposeData.pFbo = Fbo::create(getDevice());

    ref<Texture> aoTex = getDevice()->createTexture2D(
        mAoMapSize.x,
        mAoMapSize.y,
        ResourceFormat::R8Unorm,
        1,
        1,
        nullptr,
        ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget
    );
    mpAOFbo = Fbo::create(getDevice());
    mpAOFbo->attachColorTarget(aoTex, 0);

    mpDownsamplePass = ComputePass::create(getDevice(), "Samples/SampleAppTemplate/Blur.cs.slang", "downsample");
    mpMergePass = ComputePass::create(getDevice(), "Samples/SampleAppTemplate/Blur.cs.slang", "merge");
    for (uint32_t i = 0; i < DOWNSAMPLE_COUNT; i++)
    {
        uint32_t blurScale = 1 << (i + 1);
        mpDownsampleTexture[i] = getDevice()->createTexture2D(
            mAoMapSize.x / blurScale,
            mAoMapSize.y / blurScale,
            ResourceFormat::RGBA16Float,
            1,
            1,
            nullptr,
            ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess
        );
    }

    mpMergedBlurTexture = getDevice()->createTexture2D(
        mAoMapSize.x,
        mAoMapSize.y,
        ResourceFormat::RGBA16Float,
        1,
        1,
        nullptr,
        ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess
    );
    setSampleRadius(0.5f);
    setKernelSize(32);
    setNoiseTexture(mNoiseSize.x, mNoiseSize.y);

    GBuffer::onLoad(pRenderContext);
}

void SSAO::onShutdown()
{
    GBuffer::onShutdown();
}

void SSAO::onResize(uint32_t width, uint32_t height) {}

void SSAO::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    GBuffer::onFrameRender(pRenderContext, mpFbo);
    // Run the AO pass
    if (mShowNoiseTex)
    {
        pRenderContext->blit(mpNoiseTexture->getSRV(), pTargetFbo->getRenderTargetView(0));
        return;
    }
    if (enableSSAO)
    {
        auto pAoMap = generateAOMap(pRenderContext, mpCamera.get(), mpDepthRT, mpNoiseTexture, mpRTs[2]);

        if (showSSAOMap)
        {
            pRenderContext->blit(pAoMap->getSRV(), pTargetFbo->getRenderTargetView(0));
            return;
        }
        ShaderVar var = mComposeData.pApplySSAOPass->getRootVar();
        var["gColor"] = mpRTs[0];
        var["gAOMap"] = pAoMap;
        mComposeData.pFbo->attachColorTarget(pTargetFbo->getColorTexture(0), 0);
        mComposeData.pApplySSAOPass->execute(pRenderContext, mComposeData.pFbo);
    }
    else
    {
        if (blurImage > 0)
        {
            blurMap(pRenderContext, mpRTs[0], blurImage);
            //pRenderContext->blit(mpDownsampleTexture[blurImage]->getSRV(), pTargetFbo->getRenderTargetView(0));
            pRenderContext->blit(mpMergedBlurTexture->getSRV(), pTargetFbo->getRenderTargetView(0));
        }
        else
        {
            pRenderContext->blit(mpRTs[0]->getSRV(), pTargetFbo->getRenderTargetView(0));
        }
    }
}

void SSAO::onGuiRender(Gui* pGui)
{
    GBuffer::onGuiRender(pGui);

    Gui::Window w(pGui, "Falcor", {250, 500});
    uint32_t distribution = (uint32_t)mHemisphereDistribution;
    if (w.dropdown("Kernel Distribution", kDistributionDropdown, distribution))
        setDistribution(distribution);

    uint32_t size = mData.kernelSize;
    if (w.var("Kernel Size", size, 1u, SSAOData::kMaxSamples))
        setKernelSize(size);

    float radius = mData.radius;
    if (w.var("Sample Radius", radius, 0.001f, FLT_MAX, 0.001f))
        setSampleRadius(radius);

    w.checkbox("SSAO", enableSSAO);
    if (enableSSAO)
    {
        w.checkbox("SSAO Map", showSSAOMap);
        w.checkbox("Show Noise Texture", mShowNoiseTex);
    }
    else
    {
        w.slider("Show Blur", blurImage, (uint32_t)0, DOWNSAMPLE_COUNT - 1);
    }
}

bool SSAO::onKeyEvent(const KeyboardEvent& keyEvent)
{
    return GBuffer::onKeyEvent(keyEvent);
}

bool SSAO::onMouseEvent(const MouseEvent& mouseEvent)
{
    return GBuffer::onMouseEvent(mouseEvent);
}

void SSAO::onHotReload(HotReloadFlags reloaded)
{
    GBuffer::onHotReload(reloaded);
}

void SSAO::loadScene(const std::filesystem::path& path, const Fbo* pTargetFbo)
{
    GBuffer::loadScene(path, pTargetFbo);
}
