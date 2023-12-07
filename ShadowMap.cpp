#include "ShadowMap.h"
#include "Utils/Math/FalcorMath.h"
#include "Utils/UI/TextRenderer.h"

ShadowMap::ShadowMap(const SampleAppConfig& config) : SampleApp(config) {}

ShadowMap::~ShadowMap() {}

ref<Vao> ShadowMap::createVao(const ref<Device>& device, const ref<TriangleMesh>& mesh)
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
const int kTriangleCount = 2;
void ShadowMap::onLoad(RenderContext* pRenderContext)
{
    const auto& device = getDevice();
    // Load program
    mpRasterPass = RasterPass::create(device, "Samples/SampleAppTemplate/BlinnPhone.3d.slang", "vsMain", "psMain", {{"ENABLE_SHADOW_MAP", ""}});
    mpShadowPass = RasterPass::create(device, "Samples/SampleAppTemplate/ShadowMap.3d.slang", "vsMain", "psMain");
    mpVars = ProgramVars::create(device, mpRasterPass->getProgram()->getReflector());
    mpShadowVars = ProgramVars::create(device, mpShadowPass->getProgram()->getReflector());

    mpDiffuseMap = Texture::createFromFile(device, getRuntimeDirectory() / "data/lya.png", true, true);
    Sampler::Desc samplerDesc;
    gSampler = device->createSampler(samplerDesc);

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
    ref<Texture> tex = device->createTexture2D(
        width, height, ResourceFormat::RGBA8UnormSrgb, 1, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget
    );
    ref<Texture> depthTex =
        device->createTexture2D(width, height, ResourceFormat::D32Float, 1, 1, nullptr, ResourceBindFlags::DepthStencil);
    mpFbo->attachColorTarget(tex, 0);
    mpFbo->attachDepthStencilTarget(depthTex);

    // Create Shadow FBO
    mpShadowFbo = Fbo::create(device);
    ref<Texture> shadowMapTex = device->createTexture2D(
        width, height, ResourceFormat::R32Float, 1, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget
    );
    ref<Texture> lightDepthTex = device->createTexture2D(
        width, height, ResourceFormat::D32Float, 1, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::DepthStencil
    );
    mpShadowFbo->attachColorTarget(shadowMapTex, 0);
    mpShadowFbo->attachDepthStencilTarget(lightDepthTex);

    DepthStencilState::Desc depthDesc;
    mpDepthStencil = DepthStencilState::create(depthDesc);

    RasterizerState::Desc rsDesc;
    rsDesc.setCullMode(RasterizerState::CullMode::Back);
    mpRasterizeState[0] = RasterizerState::create(rsDesc);
    rsDesc.setCullMode(RasterizerState::CullMode::Front);
    mpRasterizeState[1] = RasterizerState::create(rsDesc);
    rsDesc.setCullMode(RasterizerState::CullMode::None);
    mpRasterizeState[2] = RasterizerState::create(rsDesc);

    lightPassView = GraphicsState::Viewport(0, 0, 0.5f, 0.5f, 0, 1);
    mainPassView = GraphicsState::Viewport(0.5f, 0.5f, 0.5f, 0.5f, 0, 1);

    modelMatrix = float4x4::identity();

    lightPos = float3(-0.5f, 3, 3);
    // Compute the MVP matrix from the light's point of view
    lightProjectionMatrix = math::ortho(-4.0f, 4.0f, -4.0f, 4.0f, near, far);
    //lightProjectionMatrix = math::perspective(math::radians(60.0f), 16.0f / 9.0f, near, far);
}

void ShadowMap::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    // Model Transform
    //modelMatrix = math::rotate(modelMatrix, math::radians(0.1f), float3(0, 1, 0));

    lightViewMatrix = math::matrixFromLookAt(lightPos, float3(), float3(0, 1, 0));

    lightViewProjectionMatrix = mul(lightProjectionMatrix, lightViewMatrix);
    // light transform as camera transform

    renderShadowMap(pRenderContext, pTargetFbo);
    renderScene(pRenderContext, pTargetFbo);
}

void ShadowMap::onGuiRender(Gui* pGui)
{
    Gui::Window w(pGui, "Shadow Settings", {300, 400}, {10, 80});

    w.checkbox("Render ShadowMap", bRenderShadowMap);
    w.text("Position");
    w.slider("PX", lightPos.x, -10.0f, 10.0f);
    w.slider("PY", lightPos.y, -10.0f, 10.0f);
    w.slider("PZ", lightPos.z, -10.0f, 10.0f);
}

void ShadowMap::renderShadowMap(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    pRenderContext->clearFbo(mpShadowFbo.get(), float4(1.0f), 1.f, 0);
    mpShadowPass->getState()->setFbo(mpShadowFbo);
    mpShadowPass->getState()->setVao(mpVao[0]);
    mpShadowPass->getState()->setDepthStencilState(mpDepthStencil);
    mpShadowPass->getState()->setRasterizerState(mpRasterizeState[1]); // Cull Front
    mpShadowPass->setVars(mpShadowVars);
    ShaderVar var = mpShadowVars->getRootVar();

    var["PerFrameCB"][kViewProjMatrices] = lightViewProjectionMatrix;
    var["PerFrameCB"][kWorldMatrices] = modelMatrix;
    mpShadowPass->drawIndexed(pRenderContext, mpVao[0]->getIndexBuffer()->getElementCount(), 0, 0);

    float height = getConfig().windowDesc.height;
    float width = getConfig().windowDesc.width;
    if (bRenderShadowMap)
    {
        pRenderContext->blit(
            mpShadowFbo->getColorTexture(0)->getSRV(),
            pTargetFbo->getRenderTargetView(0),
            {0, 0, width, height},
            {0, 0, width / 2, height / 2}
        );
    }
}

void ShadowMap::renderScene(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    pRenderContext->clearFbo(mpFbo.get(), float4(0.2f), 1.f, 0);
    mpRasterPass->getState()->setFbo(mpFbo);
    mpRasterPass->getState()->setVao(mpVao[0]);
    mpRasterPass->getState()->setDepthStencilState(mpDepthStencil);
    mpRasterPass->getState()->setRasterizerState(mpRasterizeState[0]);
    mpRasterPass->setVars(mpVars);
    ShaderVar var = mpVars->getRootVar();

    // Camera
    float height = getConfig().windowDesc.height;
    float width = getConfig().windowDesc.width;
    float l = -1, r = 1, b = -(float)height / width, t = (float)height / width;
    //projectionMatrix = math::ortho(l,r,b,t, near, far);
    projectionMatrix = math::perspective(math::radians(60.0f), 16.0f / 9.0f, near, far);
    viewMatrix = math::matrixFromLookAt(float3(1, 2, 3), float3(), float3(0.0f, 1.0f, 0.0f));
    ViewProjectionMatrix = mul(projectionMatrix, viewMatrix);
    float4x4 depthMVP = mul(lightViewProjectionMatrix, modelMatrix);

    float4x4 biasMatrix = math::matrixFromTranslation(float3(0.5f));
    biasMatrix = mul(biasMatrix, math::matrixFromScaling(float3(0.5f)));
    // float4x4{0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.5, 0.5, 0.5, 1.0};
    float4x4 depthBiasMVP = mul(biasMatrix, depthMVP);

    var["gSampler"] = gSampler;
    var["PerFrameCB"][kViewProjMatrices] = ViewProjectionMatrix;
    var["PerFrameCB"][kWorldMatrices] = modelMatrix;
    var["PerFrameCB"][kInverseTransposeWorldMatrices] = transpose(inverse(modelMatrix));

    var["PerFrameCB"]["diffuse"] = mpDiffuseMap;
    var["PerFrameCB"]["baseColor"] = float3(1.0f, 1.0f, 1.0f);
    var["PerFrameCB"]["ambientColor"] = float3(0.05f);
    var["PerFrameCB"]["specularColor"] = float3(1.0f, 1.0f, 1.0f);
    var["PerFrameCB"]["glossy"] = 128.0f;

    var["LightCB"]["lightColor"] = float3(1.0f, 1.0f, 1.0f);
    var["LightCB"]["lightWorldPos"] = lightPos;
    var["LightCB"]["lightAtten"] = 1.0f;
    var["LightCB"]["lightBias"] = 0.0001f;

    var["PerFrameCB"]["lightViewProjectionMatrix"] = lightViewProjectionMatrix;
    var["PerFrameCB"]["shadowMap"] = mpShadowFbo->getColorTexture(0);
    mpRasterPass->drawIndexed(pRenderContext, mpVao[0]->getIndexBuffer()->getElementCount(), 0, 0);

    pRenderContext->blit(
        mpFbo->getColorTexture(0)->getSRV(),
        pTargetFbo->getRenderTargetView(0),
        {0, 0, width, height}, bRenderShadowMap ? uint4{width / 2, height / 2, width, height}: uint4{0, 0, width, height}
    );
}
