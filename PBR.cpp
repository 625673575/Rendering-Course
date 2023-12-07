#include "PBR.h"
#include "Utils/Math/FalcorMath.h"
#include "Utils/UI/TextRenderer.h"

PBR::PBR(const SampleAppConfig& config) : SampleApp(config) {}

PBR::~PBR() {}

ref<Vao> PBR::createVao(const ref<Device>& device, const ref<TriangleMesh>& mesh)
{
    ref<VertexLayout> pLayout = VertexLayout::create();

    // Add the packed static vertex data layout.
    ref<VertexBufferLayout> pVertexLayout = VertexBufferLayout::create();
    pVertexLayout->addElement("POSITION", offsetof(VertexInput, position), ResourceFormat::RGB32Float, 1, VERTEX_POSITION_LOC);
    pVertexLayout->addElement(
        "NORMAL", offsetof(VertexInput, normal), ResourceFormat::RGB32Float, 1, VERTEX_PACKED_NORMAL_TANGENT_CURVE_RADIUS_LOC
    );
    pVertexLayout->addElement("TANGENT", offsetof(VertexInput, tangent), ResourceFormat::RGB32Float, 1, 2);
    pVertexLayout->addElement("BITANGENT", offsetof(VertexInput, bitangent), ResourceFormat::RGB32Float, 1, 3);
    pVertexLayout->addElement("TEXCOORD", offsetof(VertexInput, texCrd), ResourceFormat::RG32Float, 1, 4);
    pLayout->addBufferLayout(0, pVertexLayout);
    ref<Buffer> pVertexBuffer;
    const TriangleMesh::VertexList& vertices = mesh->getVertices();
    const TriangleMesh::IndexList& indices = mesh->getIndices();

    uint32_t vertexCount = vertices.size();
    std::vector<float3> tangent, bitangent;
    generateTangent(vertices, indices, tangent, bitangent);

    VertexInput* vertexInput = new VertexInput[vertexCount];
    for (uint32_t i = 0; i < vertexCount; i++)
    {
        vertexInput[i].position = vertices[i].position;
        vertexInput[i].normal = vertices[i].normal;
        vertexInput[i].tangent = tangent[i];
        vertexInput[i].bitangent = bitangent[i];
        vertexInput[i].texCrd = vertices[i].texCoord;
    }

    ResourceBindFlags vbBindFlags = ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess | ResourceBindFlags::Vertex;
    pVertexBuffer =
        device->createStructuredBuffer(sizeof(VertexInput), vertexCount, vbBindFlags, MemoryType::DeviceLocal, vertexInput, false);
    uint32_t indexCount = indices.size();
    auto indexBuffer = device->createTypedBuffer<uint32_t>(
        indexCount, ResourceBindFlags::ShaderResource | ResourceBindFlags::Index, MemoryType::DeviceLocal, indices.data()
    );

    // Create VAO
    return Vao::create(Vao::Topology::TriangleList, pLayout, {pVertexBuffer}, indexBuffer, ResourceFormat::R32Uint);
}

void PBR::generateTangent(
    const TriangleMesh::VertexList& vertexList,
    const TriangleMesh::IndexList& indicesList,
    std::vector<float3>& tangents,
    std::vector<float3>& bitangents
)
{
    tangents.resize(vertexList.size());
    bitangents.resize(vertexList.size());
    for (unsigned int i = 0; i < indicesList.size(); i += 3)
    {
        size_t i0 = indicesList[i];
        size_t i1 = indicesList[i + 1];
        size_t i2 = indicesList[i + 2];

        float3 e1 = vertexList[i1].position - vertexList[i0].position;
        float3 e2 = vertexList[i2].position - vertexList[i0].position;
        float delta_u1 = vertexList[i1].texCoord[0] - vertexList[i0].texCoord[0];
        float delta_u2 = vertexList[i2].texCoord[0] - vertexList[i0].texCoord[0];
        float delta_v1 = vertexList[i1].texCoord[1] - vertexList[i0].texCoord[1];
        float delta_v2 = vertexList[i2].texCoord[1] - vertexList[i0].texCoord[1];
        float3 tangent = (delta_v1 * e2 - delta_v2 * e1) / (delta_v1 * delta_u2 - delta_v2 * delta_u1);
        float3 bitangent = (-delta_u1 * e2 + delta_u2 * e1) / (delta_v1 * delta_u2 - delta_v2 * delta_u1);

        tangents[i0] = (tangent);
        tangents[i1] = (tangent);
        tangents[i2] = (tangent);

        // Same thing for binormals
        bitangents[i0] = (bitangent);
        bitangents[i1] = (bitangent);
        bitangents[i2] = (bitangent);
    }

    // See "Going Further"
    for (size_t i = 0; i < indicesList.size(); i += 1)
    {
        size_t index = indicesList[i];
        const float3& n = vertexList[index].normal;
        float3& t = tangents[index];
        float3& b = bitangents[index];

        // Gram-Schmidt orthogonalize
        t = math::normalize(t - n * math::dot(n, t));

        // Calculate handedness
        if (math::dot(math::cross(n, t), b) < 0.0f)
        {
            t = t * -1.0f;
        }
    }
}

void PBR::postProcess(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo, const ref<Texture>& rt) {}

void PBR::rasterize(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    mpRasterPass->getState()->setFbo(mpRasterFbo);
    mpRasterPass->getState()->setVao(mpVao[0]);
    mpRasterPass->getState()->setDepthStencilState(mpDepthStencil);
    mpRasterPass->getState()->setRasterizerState(mpRasterizeState[0]);
    mpRasterPass->getState()->setProgram(mpRasterProgram);
    mpRasterPass->setVars(mpVars);
    const ShaderVar& var = mpVars->getRootVar();

    // Model Transform
    modelMatrix = math::rotate(modelMatrix, math::radians(0.5f), float3(0, 1, 0));

    var["gSampler"] = gSampler;
    // var["LightCB"]["lights"].setBlob(lights, (size_t)(4 * 16 * 2));
     var["LightCB"]["lights"][0].setBlob(&lights[0], (size_t)4 * 16); // HLSL has 16 byte padding
     var["LightCB"]["lights"][1].setBlob(&lights[1], (size_t)4 * 16);

    var["PerFrameCB"][kWorldMatrices] = modelMatrix;
    var["PerFrameCB"][kViewProjMatrices] = mpCamera->getViewProjMatrixNoJitter();

    var["PerFrameCB"][kInverseTransposeWorldMatrices] = transpose(inverse(modelMatrix));
    var["PerFrameCB"]["camPos"] = mpCamera->getPosition();
    var["PerFrameCB"]["albedo"] = albedo;
    var["PerFrameCB"]["metallicRoughness"] = metallicRoughness();
    var["PerFrameCB"]["albedoMap"] = mpAlbedoMap;
    var["PerFrameCB"]["normalMap"] = mpNormalMap;
    var["PerFrameCB"]["metallicMap"] = mpMetallicMap;
    var["PerFrameCB"]["roughnessMap"] = mpRoughnessMap;
    mpRasterPass->drawIndexed(pRenderContext, mpVao[0]->getIndexBuffer()->getElementCount(), 0, 0);

    mpRasterPass->getState()->setVao(mpVao[1]);
    var["PerFrameCB"][kWorldMatrices] = math::translate(modelMatrix, float3(2.0, 0, 0));
    mpRasterPass->drawIndexed(pRenderContext, mpVao[1]->getIndexBuffer()->getElementCount(), 0, 0);

    pRenderContext->blit(mpRasterFbo->getColorTexture(0)->getSRV(), pTargetFbo->getRenderTargetView(0));
}

void PBR::drawSkybox(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    mpRasterPass->getState()->setFbo(mpRasterFbo);
    mpRasterPass->getState()->setVao(mpVao[2]); // Skybox using cube as vertex input
    mpRasterPass->getState()->setDepthStencilState(mpSkyboxDepthStencil);
    mpRasterPass->getState()->setRasterizerState(mpSkyboxRasterizeState);
    mpRasterPass->getState()->setProgram(mpEnvProgram);
    mpRasterPass->setVars(mpEnvVars);
    const ShaderVar& rootVar = mpEnvVars->getRootVar();
    // rootVar["gTexture"] = mpEnvTexture;
    float4x4 world = float4x4::identity();
    rootVar["gWorld"] = world;
    rootVar["gScale"] = 1.0f;
    rootVar["gViewMat"] = mpCamera->getViewMatrix();
    rootVar["gProjMat"] = mpCamera->getProjMatrix();
    if (mpEnvMap)
        mpEnvMap->bindShaderData(rootVar["envMap"]);
    mpRasterPass->drawIndexed(pRenderContext, mpVao[2]->getIndexBuffer()->getElementCount(), 0, 0);
}

void PBR::onLoad(RenderContext* pRenderContext)
{
    const auto& device = getDevice();
    // Load program
    mpRasterPass = RasterPass::create(device, "Samples/SampleAppTemplate/PBR.3d.slang", "vsMain", "psMain");
    mpRasterProgram = mpRasterPass->getProgram();
    mpVars = ProgramVars::create(device, mpRasterProgram->getReflector());

    tringleMesh[0] = TriangleMesh::createSphere();
    tringleMesh[1] = TriangleMesh::createCube();
    tringleMesh[2] = TriangleMesh::createFromFile(kCubePath);
    // Create VAO
    mpVao[0] = createVao(device, tringleMesh[0]);
    mpVao[1] = createVao(device, tringleMesh[1]);
    mpVao[2] = createVao(device, tringleMesh[2]);

    // Create FBO
    float height = getConfig().windowDesc.height;
    float width = getConfig().windowDesc.width;
    mpRasterFbo = Fbo::create(device);
    ref<Texture> tex = device->createTexture2D(
        width, height, ResourceFormat::RGBA8UnormSrgb, 1, 1, nullptr, ResourceBindFlags::ShaderResource | ResourceBindFlags::RenderTarget
    );
    ref<Texture> depthTex =
        device->createTexture2D(width, height, ResourceFormat::D32Float, 1, 1, nullptr, ResourceBindFlags::DepthStencil);
    mpRasterFbo->attachColorTarget(tex, 0);
    mpRasterFbo->attachDepthStencilTarget(depthTex);

    DepthStencilState::Desc depthDesc;
    mpDepthStencil = DepthStencilState::create(depthDesc);

    RasterizerState::Desc rsDesc;
    rsDesc.setCullMode(RasterizerState::CullMode::Back);
    mpRasterizeState[0] = RasterizerState::create(rsDesc);
    rsDesc.setCullMode(RasterizerState::CullMode::Front);
    mpRasterizeState[1] = RasterizerState::create(rsDesc);
    rsDesc.setCullMode(RasterizerState::CullMode::None);
    mpRasterizeState[2] = RasterizerState::create(rsDesc);

    Sampler::Desc samplerDesc;
    gSampler = device->createSampler(samplerDesc);
    // Camera
    mpCamera = Camera::create();
    float near = 0.1f, far = 100.0f;
    float l = -1, r = 1, b = -(float)height / width, t = (float)height / width;
    // projectionMatrix = math::ortho(l,r,b,t, near, far);
    projectionMatrix = math::perspective(math::radians(45.0f), 16.0f / 9.0f, near, far);

    mpCamera->setProjectionMatrix(projectionMatrix);
    mpCamera->setPosition(float3(2, 3, 5));
    mpCamera->setTarget(float3(0));

    viewMatrix = math::matrixFromLookAt(float3(2, 3, 3), float3(), float3(0.0f, 1.0f, 0.0f));
    VP = mul(projectionMatrix, viewMatrix);
    modelMatrix = float4x4::identity();

    mpAlbedoMap = Texture::createFromFile(device, getRuntimeDirectory() / "data/pbr/rustediron2_basecolor.png", true, true);
    mpMetallicMap = Texture::createFromFile(device, getRuntimeDirectory() / "data/pbr/rustediron2_metallic.png", true, true);
    mpNormalMap = Texture::createFromFile(device, getRuntimeDirectory() / "data/pbr/rustediron2_normal.png", true, false);
    mpRoughnessMap = Texture::createFromFile(device, getRuntimeDirectory() / "data/pbr/rustediron2_roughness.png", true, true);

    for (int i = 0; i < LIGHT_COUNT; i++)
    {
        lights[i].dirW = float4(1, 1, 1, 1);
        lights[i].posW = float4(1.0f, 5.0f * (i % 2 == 0 ? -1.0f : 1.0f), 3.0f, 1.0f);
        lights[i].intensity = float4((i % 2 == 0 ? 1.0f : .0f), (i % 2 == 1 ? 1.0f : .0f), 0, 1.0f);
    }

    mpEnvProgram = Program::createGraphics(getDevice(), "Samples/SampleAppTemplate/SkyBox.slang", "vs", "ps");
    mpEnvVars = ProgramVars::create(device, mpEnvProgram->getReflector());
    mpEnvMap = EnvMap::createFromFile(getDevice(), kEnvMapPath);

    if (mpEnvMap == nullptr)
        std::cout << "environment map is null";
    else
        mpEnvProgram->addDefine("_USE_ENV_MAP");
    RasterizerState::Desc rastDesc;
    rastDesc.setCullMode(RasterizerState::CullMode::None).setDepthClamp(true);
    mpSkyboxRasterizeState = RasterizerState::create(rastDesc);

    DepthStencilState::Desc dsDesc;
    dsDesc.setDepthWriteMask(false).setDepthFunc(ComparisonFunc::LessEqual);
    mpSkyboxDepthStencil = DepthStencilState::create(dsDesc);
}

void PBR::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    pRenderContext->clearFbo(mpRasterFbo.get(), float4(0.2f), 1.f, 0);
    drawSkybox(pRenderContext, pTargetFbo);
    rasterize(pRenderContext, pTargetFbo);
    postProcess(pRenderContext, pTargetFbo, mpRasterFbo->getColorTexture(0));
}

void PBR::onGuiRender(Gui* pGui)
{
    Gui::Window w(pGui, "PBR Settings", {300, 400}, {10, 80});
    mpCamera->renderUI(w);
    mpEnvMap->renderUI(w);
    w.rgbColor("albedo", albedo);
    w.slider("metallic", metallic, .0f, 1.0f);
    w.slider("roughness", roughness, .0f, 1.0f);
    if (w.button("Load Image"))
    {
        std::filesystem::path filename;
        FileDialogFilterVec filters = {{"bmp"}, {"jpg"}, {"dds"}, {"png"}, {"tiff"}, {"tif"}, {"tga"}};
        if (openFileDialog(filters, filename))
        {
            mpEnvTexture = Texture::createFromFile(getDevice(), filename, false, false);
            if (mpEnvTexture)
            {
                mpEnvProgram->addDefine(mpEnvTexture->getType() == Texture::Type::TextureCube ? "_USE_ENV_MAP" : "_USE_SPHERICAL_MAP");
            }
        }
    }

    Gui::Window l0(pGui, "Light[0] Settings", {300, 400}, {310, 80});
    l0.rgbColor("color", lightColor[0]);
    l0.slider("intensity", lightIntensity[0], .0f, 1000.0f);
    l0.slider("pos x", lights[0].posW.x, -10.0f, 10.0f);
    l0.slider("pos y", lights[0].posW.y, -10.0f, 10.0f);
    l0.slider("pos z", lights[0].posW.z, -10.0f, 10.0f);
    l0.slider("dir x", lights[0].dirW.x, -10.0f, 10.0f);
    l0.slider("dir y", lights[0].dirW.y, -10.0f, 10.0f);
    l0.slider("dir z", lights[0].dirW.z, -10.0f, 10.0f);
    lights[0].intensity = float4(lightColor[0] * lightIntensity[0],1.0f);

    Gui::Window l1(pGui, "Light[1] Settings", {300, 400}, {610, 80});
    l1.rgbColor("color", lightColor[1]);
    l1.slider("intensity", lightIntensity[1], .0f, 1000.0f);
    l1.slider("pos x", lights[1].posW.x, -10.0f, 10.0f);
    l1.slider("pos y", lights[1].posW.y, -10.0f, 10.0f);
    l1.slider("pos z", lights[1].posW.z, -10.0f, 10.0f);
    l1.slider("dir x", lights[1].dirW.x, -10.0f, 10.0f);
    l1.slider("dir y", lights[1].dirW.y, -10.0f, 10.0f);
    l1.slider("dir z", lights[1].dirW.z, -10.0f, 10.0f);
    lights[1].intensity = float4(lightColor[1] * lightIntensity[1],1.0f);
}
