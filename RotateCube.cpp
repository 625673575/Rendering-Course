#include "RotateCube.h"
#include "Utils/Math/FalcorMath.h"
#include "Utils/UI/TextRenderer.h"

RotateCube::RotateCube(const SampleAppConfig& config) : SampleApp(config) {}

RotateCube::~RotateCube() {}
using namespace Falcor::math;
const int kTriangleCount = 2;
void RotateCube::onLoad(RenderContext* pRenderContext)
{
    // Load program
    mpRasterPass = RasterPass::create(getDevice(), "Samples/SampleAppTemplate/Unlit.3d.slang", "vsMain", "psMain");
    mpVars = ProgramVars::create(getDevice(), mpRasterPass->getProgram()->getReflector());

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
    ref<Texture> tex = getDevice()->createTexture2D(width, height, ResourceFormat::RGBA8UnormSrgb, 1, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget);
    ref<Texture> depthTex = getDevice()->createTexture2D(width, height, ResourceFormat::D32Float, 1, 1, nullptr, ResourceBindFlags::DepthStencil);
    mpFbo->attachColorTarget(tex, 0);
    mpFbo->attachDepthStencilTarget(depthTex);

    DepthStencilState::Desc depthDesc;
    depthDesc.setDepthWriteMask(true);
    mpDepthStencil = DepthStencilState::create(depthDesc);

    RasterizerState::Desc rsDesc;
    rsDesc.setFillMode(RasterizerState::FillMode::Wireframe);
    rsDesc.setCullMode(RasterizerState::CullMode::Back);
    mpRasterizeState = RasterizerState::create(rsDesc);

    modelMatrix = float4x4::identity();
}

void RotateCube::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    pRenderContext->clearFbo(mpFbo.get(), float4(0.2f), 1.f, 0);
    mpRasterPass->getState()->setFbo(mpFbo);
    mpRasterPass->getState()->setVao(mpVao);
    mpRasterPass->getState()->setDepthStencilState(mpDepthStencil);
    mpRasterPass->getState()->setRasterizerState(mpRasterizeState);
    mpRasterPass->setVars(mpVars);
    auto var = mpVars->getRootVar();
    // Model Transform
    modelMatrix = math::rotate(modelMatrix, math::radians(1.0f), float3(1, 1, 0));
    // Orthographic Camera
    ref<Camera> cam = Camera::create();
    cam->setPosition(float3(1, 3, 3));
    cam->setTarget(float3());

    float near = 0.1f, far = 100.0f;
    float l = -1, r = 1, b = -(float)height / width, t = (float)height / width;
    // float4x4 projectionMatrix = math::ortho(l,r,b,t, near, far);
    float4x4 projectionMatrix = math::perspective(math::radians(90.0f), 16.0f / 9.0f, near, far);
    float4x4 viewMatrix = math::matrixFromLookAt(float3(4, 3, 3), float3(), float3(0.0f, 1.0f, 0.0f));
    float4x4 MV = mul(viewMatrix, modelMatrix);
    float4x4 VP = mul(projectionMatrix, viewMatrix);
    float4x4 MVP = mul(cam->getViewProjMatrix(), modelMatrix);

    var["PerFrameCB"]["transform"] = MVP;
    var["PerFrameCB"]["scale"] = 1.0f;
    mpRasterPass->drawIndexed(pRenderContext, 36, 0, 0);
    pRenderContext->blit(mpFbo->getColorTexture(0)->getSRV(), pTargetFbo->getRenderTargetView(0));
}
