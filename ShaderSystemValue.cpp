#include "ShaderSystemValue.h"
#include "Utils/Math/FalcorMath.h"
#include "Utils/UI/TextRenderer.h"

using namespace Falcor::math;

ShaderSystemValue::ShaderSystemValue(const SampleAppConfig& config) : SampleApp(config) {}

ShaderSystemValue::~ShaderSystemValue() {}

ref<Vao> ShaderSystemValue::createVao(const ref<Device>& device, const ref<TriangleMesh>& mesh)
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

void ShaderSystemValue::onLoad(RenderContext* pRenderContext)
{
    const auto& device = getDevice();
    // Load program
    mpRasterPass = RasterPass::create(device, "Samples/SampleAppTemplate/ShaderSystemValue.slang", "vsMain", "psMain");
    mpVars = ProgramVars::create(device, mpRasterPass->getProgram()->getReflector());

    tringleMesh[0] = TriangleMesh::createFromFile(getRuntimeDirectory() / "data/framework/meshes/Arcade.fbx", true);
    // Create VAO
    mpVao[0] = createVao(device, tringleMesh[0]);

    // Create FBO, MSAA not compatible with Depth Texture, so do not attach DepthStencil, you can use PreDepthPass to get Depth Texture
    mpFbo = Fbo::create(device);
    ref<Texture> texs[kTargetCount];
    float height = getConfig().windowDesc.height;
    float width = getConfig().windowDesc.width;
    for (uint32_t i = 0; i < kTargetCount; i++)
    {
        texs[i] = device->createTexture2DMS(
            width / 4, height / 4, ResourceFormat::RGBA8UnormSrgb, 8, 1, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget
        );

        mpResolvedTexture[i] = getDevice()->createTexture2D(width / 4, height / 4, ResourceFormat::RGBA8UnormSrgb, 1, 1);
        mpFbo->attachColorTarget(texs[i], i);
    }
    // ref<Texture> depthTex = device->createTexture2D(width, height, ResourceFormat::D32Float, 1, 1, nullptr,
    // ResourceBindFlags::DepthStencil); mpFbo->attachDepthStencilTarget(depthTex);

    DepthStencilState::Desc depthDesc;
    mpDepthStencil = DepthStencilState::create(depthDesc);

    RasterizerState::Desc rsDesc;
    rsDesc.setCullMode(RasterizerState::CullMode::Back);
    mpRasterizeState[0] = RasterizerState::create(rsDesc);
    rsDesc.setCullMode(RasterizerState::CullMode::Front);
    mpRasterizeState[1] = RasterizerState::create(rsDesc);
    rsDesc.setCullMode(RasterizerState::CullMode::None);
    mpRasterizeState[2] = RasterizerState::create(rsDesc);
    dropdownList = {
        {0, "Back"},
        {1, "Front"},
        {2, "None"},
    };

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
    rectEL = uint4{0, height, width / 2, height + height / 2};
    rectER = uint4{width / 2, height, width, height + height / 2};
}

void ShaderSystemValue::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    pRenderContext->clearFbo(mpFbo.get(), float4(0.2f), 1.f, 0);
    mpRasterPass->getState()->setFbo(mpFbo);
    mpRasterPass->getState()->setVao(mpVao[0]);
    mpRasterPass->getState()->setDepthStencilState(mpDepthStencil);
    mpRasterPass->getState()->setRasterizerState(mpRasterizeState[dropdownValue]);
    mpRasterPass->setVars(mpVars);
    ShaderVar var = mpVars->getRootVar();

    modelMatrix = math::rotate(modelMatrix, math::radians(0.25f), float3(0, 1, 0));
    var["gSampler"] = gSampler;
    var["PerFrameCB"][kViewProjMatrices] = VP;
    var["PerFrameCB"][kWorldMatrices] = modelMatrix;
    var["PerFrameCB"][kInverseTransposeWorldMatrices] = transpose(inverse(modelMatrix));
    var["PerFrameCB"]["diffuse"] = mpDiffuseMap;
    mpRasterPass->drawIndexed(pRenderContext, mpVao[0]->getIndexBuffer()->getElementCount(), 0, 0);

    // MSAA Resolve
    for (uint32_t i = 0; i < kTargetCount; i++)
        pRenderContext->resolveResource(mpFbo->getColorTexture(i), mpResolvedTexture[i]);
    /*
    float4 albedo : SV_TARGET0;
    float4 primitiveID : SV_TARGET1;
    float4 sampleIndex: SV_TARGET2;
    float4 vertexIndex: SV_TARGET3;
    float4 coverge: SV_TARGET4;
    float4 isFrontFace: SV_TARGET5;
    */
    pRenderContext->blit(mpResolvedTexture[0]->getSRV(), pTargetFbo->getRenderTargetView(0), rectFull, rectUL);
    pRenderContext->blit(mpResolvedTexture[1]->getSRV(), pTargetFbo->getRenderTargetView(0), rectFull, rectUR);
    pRenderContext->blit(mpResolvedTexture[2]->getSRV(), pTargetFbo->getRenderTargetView(0), rectFull, rectBL);
    pRenderContext->blit(mpResolvedTexture[3]->getSRV(), pTargetFbo->getRenderTargetView(0), rectFull, rectBR);
    pRenderContext->blit(mpResolvedTexture[4]->getSRV(), pTargetFbo->getRenderTargetView(0), rectFull, rectEL);
    pRenderContext->blit(mpResolvedTexture[5]->getSRV(), pTargetFbo->getRenderTargetView(0), rectFull, rectER);
}

void ShaderSystemValue::onGuiRender(Gui* pGui)
{
    Gui::Window w(pGui, "PBR Settings", {300, 400}, {10, 80});
    w.dropdown("Cull Mode", dropdownList, dropdownValue);
}
