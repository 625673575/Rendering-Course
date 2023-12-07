#include "DrawInstancing.h"
#include "Utils/Math/FalcorMath.h"
#include "Utils/UI/TextRenderer.h"

using namespace Falcor::math;

DrawInstancing::DrawInstancing(const SampleAppConfig& config) : SampleApp(config) {}

DrawInstancing::~DrawInstancing() {}

ref<Vao> DrawInstancing::createVao(const ref<Device>& device, const ref<TriangleMesh>& mesh)
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

void DrawInstancing::onLoad(RenderContext* pRenderContext)
{
    const auto& device = getDevice();
    mpGraphicsState = GraphicsState::create(device);
    // Load program
    mpProgram = Program::createGraphics(device, "Samples/SampleAppTemplate/DrawInstancing.slang", "vsMain", "psMain");
    mpVars = ProgramVars::create(device, mpProgram->getReflector());

    tringleMesh = TriangleMesh::createCube();
    // Create VAO
    mpVao = createVao(device, tringleMesh);

    DepthStencilState::Desc depthDesc;
    mpDepthStencil = DepthStencilState::create(depthDesc);

    RasterizerState::Desc rsDesc;
    rsDesc.setCullMode(RasterizerState::CullMode::Back);
    mpRasterizeState = RasterizerState::create(rsDesc);

    mpDiffuseMap = Texture::createFromFile(device, getRuntimeDirectory() / "data/lya.png", true, true);
    Sampler::Desc samplerDesc;
    gSampler = device->createSampler(samplerDesc);
    modelMatrix = float4x4::identity();

    // Model Transform
    modelMatrix = math::rotate(modelMatrix, math::radians(0.5f), float3(0, 1, 0));

    // Camera
    float height = getConfig().windowDesc.height;
    float width = getConfig().windowDesc.width;
    float near = 0.1f, far = 100.0f;
    float l = -1, r = 1, b = -(float)height / width, t = (float)height / width;
    // projectionMatrix = math::ortho(l,r,b,t, near, far);
    projectionMatrix = math::perspective(math::radians(60.0f), 16.0f / 9.0f, near, far);
    viewMatrix = math::matrixFromLookAt(float3(1, 2, 3), float3(), float3(0.0f, 1.0f, 0.0f));
    VP = mul(projectionMatrix, viewMatrix);
}

void DrawInstancing::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    pRenderContext->clearFbo(pTargetFbo.get(), float4(0.2f), 1.f, 0);
    mpGraphicsState->setFbo(pTargetFbo);
    mpGraphicsState->setVao(mpVao);
    mpGraphicsState->setDepthStencilState(mpDepthStencil);
    mpGraphicsState->setRasterizerState(mpRasterizeState);
    mpGraphicsState->setProgram(mpProgram);
    ShaderVar var = mpVars->getRootVar();

    modelMatrix = math::rotate(modelMatrix, math::radians(0.25f), float3(0, 1, 0));
    var["gSampler"] = gSampler;
    var["PerFrameCB"][kViewProjMatrices] = VP;
    var["PerFrameCB"][kWorldMatrices] = modelMatrix;
    var["PerFrameCB"][kInverseTransposeWorldMatrices] = transpose(inverse(modelMatrix));
    var["PerFrameCB"]["diffuse"] = mpDiffuseMap;
    var["PerFrameCB"]["instanceCount"] = instanceCount;
    var["PerFrameCB"]["time"] = (float)getGlobalClock().getTime();

    //pRenderContext->drawIndexed(mpGraphicsState.get(), mpVars.get(), mpVao->getIndexBuffer()->getElementCount(), 0, 0);
    pRenderContext->drawIndexedInstanced(mpGraphicsState.get(), mpVars.get(), mpVao->getIndexBuffer()->getElementCount(), instanceCount, 0, 0, 0);
}

void DrawInstancing::onGuiRender(Gui* pGui)
{
    Gui::Window w(pGui, "PBR Settings", {300, 400}, {10, 80});
    w.slider("Instance Count", instanceCount, (uint32_t)1, (uint32_t)1000);
}
