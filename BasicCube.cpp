#include "BasicCube.h"

BasicCube::BasicCube(const SampleAppConfig& config) : SampleApp(config) {}

BasicCube::~BasicCube() {}

const int kTriangleCount = 2;
void BasicCube::onLoad(RenderContext* pRenderContext)
{
    // Load program
    mpRasterPass = RasterPass::create(getDevice(), "Samples/SampleAppTemplate/Unlit.3d.slang", "vsMain", "psMain");

    const float3 positions[6][4] = {
        {{-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, -0.5f}},
        {{-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}},
        {{-0.5f, 0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}},
        {{0.5f, 0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}},
        {{-0.5f, 0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}},
        {{0.5f, 0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}},
    };

    const float3 normals[6] = {
        {0.f, -1.f, 0.f},
        {0.f, 1.f, 0.f},
        {0.f, 0.f, -1.f},
        {0.f, 0.f, 1.f},
        {-1.f, 0.f, 0.f},
        {1.f, 0.f, 0.f},
    };
    std::vector<uint32_t> indices;
    std::vector<float3> vertices;
    for (size_t i = 0; i < 6; ++i)
    {
        uint32_t idx = (uint32_t)vertices.size();
        indices.emplace_back(idx);
        indices.emplace_back(idx + 2);
        indices.emplace_back(idx + 1);
        indices.emplace_back(idx);
        indices.emplace_back(idx + 3);
        indices.emplace_back(idx + 2);
        for (size_t j = 0; j < 4; ++j)
        {
            vertices.emplace_back(positions[i][j]);
        }
    }
    const float2 texCoords[4] = {{0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f}};

    auto vertexBuffer = getDevice()->createTypedBuffer<float3>(
        vertices.size(), ResourceBindFlags::ShaderResource | ResourceBindFlags::Vertex, MemoryType::DeviceLocal, vertices.data()
    );

    auto indexBuffer = getDevice()->createTypedBuffer<uint32_t>(
        indices.size(), ResourceBindFlags::ShaderResource | ResourceBindFlags::Index, MemoryType::DeviceLocal, indices.data()
    );

    // Create vertex layout
    auto bufferLayout = VertexBufferLayout::create();
    bufferLayout->addElement("POSITION", 0, ResourceFormat::RGB32Float, 1, 0);
    auto layout = VertexLayout::create();
    layout->addBufferLayout(0, bufferLayout);

    // Create VAO
    mpVao = Vao::create(Vao::Topology::TriangleList, layout, {vertexBuffer}, indexBuffer, ResourceFormat::R32Uint);

    // Create FBO
    mpFbo = Fbo::create(getDevice());
    ref<Texture> tex = getDevice()->createTexture2DMS(
        1024, 1024, ResourceFormat::RGBA32Float, 1, 1, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget
    );
    mpFbo->attachColorTarget(tex, 0);
}

void BasicCube::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    pRenderContext->clearFbo(mpFbo.get(), float4(0.2f), 0.f, 0);

    mpRasterPass->getState()->setFbo(mpFbo);
    mpRasterPass->getState()->setVao(mpVao);
    mpRasterPass->drawIndexed(pRenderContext, 36, 0,0);
    pRenderContext->blit(mpFbo->getColorTexture(0)->getSRV(), pTargetFbo->getRenderTargetView(0));
}
