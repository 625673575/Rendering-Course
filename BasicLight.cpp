#include "BasicLight.h"
#include "Utils/Math/FalcorMath.h"
#include "Utils/UI/TextRenderer.h"

BasicLight::BasicLight(const SampleAppConfig& config) : SampleApp(config) {}

BasicLight::~BasicLight() {}

ref<Vao> BasicLight::createVao(const ref<Device>& device, const ref<TriangleMesh>& mesh)
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

using namespace Falcor::math;
void BasicLight::onLoad(RenderContext* pRenderContext)
{
    const auto& device = getDevice();
    // Load program
    mpRasterPass = RasterPass::create(device, "Samples/SampleAppTemplate/BlinnPhone.3d.slang", "vsMain", "psMain");
    mpVars = ProgramVars::create(device, mpRasterPass->getProgram()->getReflector());

    trigleMesh[0] = TriangleMesh::createFromFile(getRuntimeDirectory() / "data/framework/meshes/Arcade.fbx", true);
    trigleMesh[1] = TriangleMesh::createCube();
    trigleMesh[2] = TriangleMesh::createSphere();
    // Create VAO
    mpVao[0] = createVao(device, trigleMesh[0]);
    mpVao[1] = createVao(device, trigleMesh[1]);
    mpVao[2] = createVao(device, trigleMesh[2]);

    // Create FBO
    float height = getConfig().windowDesc.height;
    float width = getConfig().windowDesc.width;
    mpFbo = Fbo::create(device);
    ref<Texture> tex = device->createTexture2D( width, height, ResourceFormat::RGBA8UnormSrgb, 1, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget);
    ref<Texture> depthTex = device->createTexture2D(width, height, ResourceFormat::D32Float, 1, 1, nullptr, ResourceBindFlags::DepthStencil);
    mpFbo->attachColorTarget(tex, 0);
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
}

void BasicLight::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    pRenderContext->clearFbo(mpFbo.get(), float4(0.2f), 1.f, 0);
    mpRasterPass->getState()->setFbo(mpFbo);
    mpRasterPass->getState()->setVao(mpVao[1]);
    mpRasterPass->getState()->setDepthStencilState(mpDepthStencil);
    mpRasterPass->getState()->setRasterizerState(mpRasterizeState[0]);
    mpRasterPass->setVars(mpVars);
    ShaderVar var = mpVars->getRootVar();
    
    // Model Transform
    modelMatrix = math::rotate(modelMatrix, math::radians(0.5f), float3(0, 1, 0));

    // Camera
    float height = getConfig().windowDesc.height;
    float width = getConfig().windowDesc.width;
    float near = 0.1f, far = 100.0f;
    float l = -1, r = 1, b = -(float)height / width, t = (float)height / width;
    // float4x4 projectionMatrix = math::ortho(l,r,b,t, near, far);
    float4x4 projectionMatrix = math::perspective(math::radians(60.0f), 16.0f / 9.0f, near, far);
    float4x4 viewMatrix = math::matrixFromLookAt(float3(1, 2, 3), float3(), float3(0.0f, 1.0f, 0.0f));
    float4x4 VP = mul(projectionMatrix, viewMatrix);

    var["gSampler"] = gSampler;
    var["PerFrameCB"][kViewProjMatrices] = VP;
    var["PerFrameCB"][kWorldMatrices] = modelMatrix;
    var["PerFrameCB"][kInverseTransposeWorldMatrices] = transpose(inverse(modelMatrix));
    var["PerFrameCB"]["diffuse"] = mpDiffuseMap;
    var["PerFrameCB"]["baseColor"] = float3(1.0f, 1.0f, 1.0f);
    var["PerFrameCB"]["ambientColor"] = float3(0.1f);
    var["PerFrameCB"]["specularColor"] = float3(1.0f, 1.0f, 1.0f);
    var["PerFrameCB"]["glossy"] = 128.0f;
    var["LightCB"]["lightColor"] = float3(1.0f, 1.0f, 1.0f);
    var["LightCB"]["lightWorldPos"] = float3(1.0f, 5.0f, 3.0f);
    var["LightCB"]["lightAtten"] = 1.0f;
    mpRasterPass->drawIndexed(pRenderContext,mpVao[1]->getIndexBuffer()->getElementCount(), 0, 0);

    mpRasterPass->getState()->setVao(mpVao[2]);
    //mpRasterPass->getState()->setRasterizerState(mpRasterizeState[1]);
    var["PerFrameCB"][kWorldMatrices] = math::translate(modelMatrix, float3(2.0, 0, 0));
    mpRasterPass->drawIndexed(pRenderContext, mpVao[2]->getIndexBuffer()->getElementCount(), 0, 0);

    pRenderContext->blit(mpFbo->getColorTexture(0)->getSRV(), pTargetFbo->getRenderTargetView(0));
}
