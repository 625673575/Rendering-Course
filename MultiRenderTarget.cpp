#include "MultiRenderTarget.h"
#include "Utils/Math/FalcorMath.h"
#include "Utils/UI/TextRenderer.h"

using namespace Falcor::math;

MultiRenderTarget::MultiRenderTarget(const SampleAppConfig& config) : SampleApp(config) {}

MultiRenderTarget::~MultiRenderTarget() {}

ref<Vao> MultiRenderTarget::createVao(const ref<Device>& device, const ref<TriangleMesh>& mesh)
{
    ref<VertexLayout> pLayout = VertexLayout::create();

    // Add the packed static vertex data layout.
    ref<VertexBufferLayout> pVertexLayout = VertexBufferLayout::create();
    pVertexLayout->addElement(
        VERTEX_POSITION_NAME, offsetof(PackedStaticVertexData, position), ResourceFormat::RGB32Float, 1, VERTEX_POSITION_LOC
    );
    pVertexLayout->addElement(
        VERTEX_PACKED_NORMAL_TANGENT_CURVE_RADIUS_NAME,
        offsetof(PackedStaticVertexData, packedNormalTangentCurveRadius),
        ResourceFormat::RGB32Float,
        1,
        VERTEX_PACKED_NORMAL_TANGENT_CURVE_RADIUS_LOC
    );
    pVertexLayout->addElement(
        VERTEX_TEXCOORD_NAME, offsetof(PackedStaticVertexData, texCrd), ResourceFormat::RG32Float, 1, VERTEX_TEXCOORD_LOC
    );
    pLayout->addBufferLayout(0, pVertexLayout);
    ref<Buffer> pVertexBuffer;
    const TriangleMesh::VertexList& vertices = mesh->getVertices();
    uint32_t vertexCount = vertices.size();
    ResourceBindFlags vbBindFlags = ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess | ResourceBindFlags::Vertex;
    pVertexBuffer = device->createStructuredBuffer(
        sizeof(PackedStaticVertexData), vertexCount, vbBindFlags, MemoryType::DeviceLocal, vertices.data(), false
    );
    const TriangleMesh::IndexList& indices = mesh->getIndices();
    uint32_t indexCount = indices.size();
    auto indexBuffer = device->createTypedBuffer<uint32_t>(
        indexCount, ResourceBindFlags::ShaderResource | ResourceBindFlags::Index, MemoryType::DeviceLocal, indices.data()
    );

    // Create VAO
    return Vao::create(Vao::Topology::TriangleList, pLayout, {pVertexBuffer}, indexBuffer, ResourceFormat::R32Uint);
}

void MultiRenderTarget::onLoad(RenderContext* pRenderContext)
{
    const auto& device = getDevice();
    // Load program
    mpRasterPass = RasterPass::create(device, "Samples/SampleAppTemplate/MultiRenderTarget.3d.slang", "vsMain", "psMain");
    mpVars = ProgramVars::create(device, mpRasterPass->getProgram()->getReflector());

    tringleMesh[0] = TriangleMesh::createFromFile(getRuntimeDirectory() / "data/framework/meshes/Arcade.fbx", true);
    // Create VAO
    mpVao[0] = createVao(device, tringleMesh[0]);

    // Create FBO
    float height = getConfig().windowDesc.height;
    float width = getConfig().windowDesc.width;
    mpFbo = Fbo::create(device);
    ref<Texture> texs[4];
    for (uint32_t i = 0; i < 4; i++)
    {
        texs[i] = device->createTexture2D(
            width,
            height,
            ResourceFormat::RGBA8UnormSrgb,
            1,
            1,
            nullptr,
            ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget
        );
        mpFbo->attachColorTarget(texs[i], i);
    }
    ref<Texture> depthTex =
        device->createTexture2D(width, height, ResourceFormat::D32Float, 1, 1, nullptr, ResourceBindFlags::DepthStencil);
    mpFbo->attachDepthStencilTarget(depthTex);

    DepthStencilState::Desc depthDesc;
    mpDepthStencil = DepthStencilState::create(depthDesc);

    RasterizerState::Desc rsDesc;
    rsDesc.setCullMode(RasterizerState::CullMode::Back);
    mpRasterizeState[0] = RasterizerState::create(rsDesc);
    rsDesc.setCullMode(RasterizerState::CullMode::Front);
    mpRasterizeState[1] = RasterizerState::create(rsDesc);
    rsDesc.setCullMode(RasterizerState::CullMode::None);
    mpRasterizeState[2] = RasterizerState::create(rsDesc);

    mpDiffuseMap = Texture::createFromFile(device, getRuntimeDirectory() / "data/lya.png", true, true);
    Sampler::Desc samplerDesc;
    gSampler = device->createSampler(samplerDesc);
    modelMatrix = float4x4::identity();

    // Model Transform
    modelMatrix = math::rotate(modelMatrix, math::radians(0.5f), float3(0, 1, 0));

    // Camera
    float near = 0.1f, far = 100.0f;
    float l = -1, r = 1, b = -(float)height / width, t = (float)height / width;
    // projectionMatrix = math::ortho(l,r,b,t, near, far);
    projectionMatrix = math::perspective(math::radians(60.0f), 16.0f / 9.0f, near, far);
    viewMatrix = math::matrixFromLookAt(float3(1, 2, 3), float3(), float3(0.0f, 1.0f, 0.0f));
    VP = mul(projectionMatrix, viewMatrix);

    rectFull = uint4{0, 0, width, height};
    rectUL = uint4{0, 0, width / 2, height / 2};
    rectUR = uint4{width / 2, 0, width, height / 2};
    rectBL = uint4{0, height / 2, width / 2, height};
    rectBR = uint4{width / 2, height / 2, width, height};
}

void MultiRenderTarget::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    pRenderContext->clearFbo(mpFbo.get(), float4(0.2f), 1.f, 0);
    mpRasterPass->getState()->setFbo(mpFbo);
    mpRasterPass->getState()->setVao(mpVao[0]);
    mpRasterPass->getState()->setDepthStencilState(mpDepthStencil);
    mpRasterPass->getState()->setRasterizerState(mpRasterizeState[0]);
    mpRasterPass->setVars(mpVars);
    ShaderVar var = mpVars->getRootVar();

    modelMatrix = math::rotate(modelMatrix, math::radians(0.25f), float3(0, 1, 0));
    var["gSampler"] = gSampler;
    var["PerFrameCB"][kViewProjMatrices] = VP;
    var["PerFrameCB"][kWorldMatrices] = modelMatrix;
    var["PerFrameCB"][kInverseTransposeWorldMatrices] = transpose(inverse(modelMatrix));
    var["PerFrameCB"]["diffuse"] = mpDiffuseMap;
    mpRasterPass->drawIndexed(pRenderContext, mpVao[0]->getIndexBuffer()->getElementCount(), 0, 0);

    pRenderContext->blit(mpFbo->getColorTexture(0)->getSRV(), pTargetFbo->getRenderTargetView(0), rectFull, rectUL);
    pRenderContext->blit(mpFbo->getColorTexture(1)->getSRV(), pTargetFbo->getRenderTargetView(0), rectFull, rectUR);
    pRenderContext->blit(mpFbo->getColorTexture(2)->getSRV(), pTargetFbo->getRenderTargetView(0), rectFull, rectBL);
    pRenderContext->blit(mpFbo->getColorTexture(3)->getSRV(), pTargetFbo->getRenderTargetView(0), rectFull, rectBR);
}
