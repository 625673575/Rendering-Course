#include "NormalMap.h"
#include "Utils/Math/FalcorMath.h"
#include "Utils/UI/TextRenderer.h"

NormalMap::NormalMap(const SampleAppConfig& config) : SampleApp(config) {}

NormalMap::~NormalMap() {}

ref<Vao> NormalMap::createVao(const ref<Device>& device, const ref<TriangleMesh>& mesh)
{
    ref<VertexLayout> pLayout = VertexLayout::create();

    // Add the packed static vertex data layout.
    ref<VertexBufferLayout> pVertexLayout = VertexBufferLayout::create();
    pVertexLayout->addElement("POSITION", offsetof(VertexInput, position), ResourceFormat::RGB32Float, 1, VERTEX_POSITION_LOC);
    pVertexLayout->addElement( "NORMAL", offsetof(VertexInput, normal), ResourceFormat::RGB32Float, 1, VERTEX_PACKED_NORMAL_TANGENT_CURVE_RADIUS_LOC);
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

    VertexInput *vertexInput = new VertexInput[vertexCount];
    for (uint32_t i = 0; i < vertexCount; i++)
    {
        vertexInput[i].position = vertices[i].position;
        vertexInput[i].normal = vertices[i].normal;
        vertexInput[i].tangent = tangent[i];
        vertexInput[i].bitangent = bitangent[i];
        vertexInput[i].texCrd = vertices[i].texCoord;
    }

    ResourceBindFlags vbBindFlags = ResourceBindFlags::ShaderResource | ResourceBindFlags::UnorderedAccess | ResourceBindFlags::Vertex;
    pVertexBuffer = device->createStructuredBuffer(sizeof(VertexInput), vertexCount, vbBindFlags, MemoryType::DeviceLocal, vertexInput, false);
    uint32_t indexCount = indices.size();
    auto indexBuffer = device->createTypedBuffer<uint32_t>(
        indexCount, ResourceBindFlags::ShaderResource | ResourceBindFlags::Index, MemoryType::DeviceLocal, indices.data()
    );

    // Create VAO
    return Vao::create(Vao::Topology::TriangleList, pLayout, {pVertexBuffer}, indexBuffer, ResourceFormat::R32Uint);
}

void NormalMap::postProcess(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo, const ref<Texture>& rt) {}

void NormalMap::rasterize(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    mpRasterPass->getState()->setFbo(mpRasterFbo);
    mpRasterPass->getState()->setVao(mpVao[0]);
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
    float4x4 projectionMatrix = math::perspective(math::radians(45.0f), 16.0f / 9.0f, near, far);
    float4x4 viewMatrix = math::matrixFromLookAt(float3(2, 3, 3), float3(), float3(0.0f, 1.0f, 0.0f));
    float4x4 VP = mul(projectionMatrix, viewMatrix);

    var["gSampler"] = gSampler;
    var["LightCB"]["lightColor"] = float3(1.0f);
    var["LightCB"]["lightWorldPos"] = float3(1.0f, 5.0f, 3.0f);
    var["LightCB"]["lightAtten"] = float(1.0f);
    var["LightCB"]["lightBias"] = float(0.01f);

    var["PerFrameCB"][kWorldMatrices] = modelMatrix;
    var["PerFrameCB"][kViewProjMatrices] = VP;
    var["PerFrameCB"][kInverseTransposeWorldMatrices] = transpose(inverse(modelMatrix));
    var["PerFrameCB"]["camPos"] = math::normalize(float3(2, 3, 3));
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

void NormalMap::generateTangent(
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
        size_t i1 = indicesList[i+1];
        size_t i2 = indicesList[i+2];

        float3 e1 = vertexList[i1].position - vertexList[i0].position;
        float3 e2 = vertexList[i2].position - vertexList[i0].position;
        float delta_u1 = vertexList[i1].texCoord[0] - vertexList[i0].texCoord[0];
        float delta_u2 = vertexList[i2].texCoord[0] - vertexList[i0].texCoord[0];
        float delta_v1 = vertexList[i1].texCoord[1] - vertexList[i0].texCoord[1];
        float delta_v2 = vertexList[i2].texCoord[1] - vertexList[i0].texCoord[1];
        float3 tangent = (delta_v1 * e2 - delta_v2 * e1) / (delta_v1 * delta_u2 - delta_v2 * delta_u1);
        float3 bitangent = (-delta_u1 * e2 + delta_u2 * e1) / (delta_v1 * delta_u2 - delta_v2 * delta_u1);
        // Shortcuts for vertices
        //const float3& v0 = vertexList[i + 0].position;
        //const float3& v1 = vertexList[i + 1].position;
        //const float3& v2 = vertexList[i + 2].position;

        //// Shortcuts for UVs
        //const float2& uv0 = vertexList[i + 0].texCoord;
        //const float2& uv1 = vertexList[i + 1].texCoord;
        //const float2& uv2 = vertexList[i + 2].texCoord;

        //// Edges of the triangle : postion delta
        //float3 deltaPos1 = v1 - v0;
        //float3 deltaPos2 = v2 - v0;

        //// UV delta
        //float2 deltaUV1 = uv1 - uv0;
        //float2 deltaUV2 = uv2 - uv0;

        //float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        //float3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
        //float3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

        // Set the same tangent for all three vertices of the triangle.
        // They will be merged later, in vboindexer.cpp
        tangents[i0]=(tangent);
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

void NormalMap::onLoad(RenderContext* pRenderContext)
{
    const auto& device = getDevice();
    // Load program
    mpRasterPass = RasterPass::create(device, "Samples/SampleAppTemplate/NormalMap.3d.slang", "vsMain", "psMain");
    mpVars = ProgramVars::create(device, mpRasterPass->getProgram()->getReflector());

    tringleMesh[0] = TriangleMesh::createSphere();
    tringleMesh[1] = TriangleMesh::createCube();
    // Create VAO
    mpVao[0] = createVao(device, tringleMesh[0]);
    mpVao[1] = createVao(device, tringleMesh[1]);

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
    modelMatrix = float4x4::identity();

    mpAlbedoMap = Texture::createFromFile(device, getRuntimeDirectory() / "data/pbr/rustediron2_basecolor.png", true, true);
    mpMetallicMap = Texture::createFromFile(device, getRuntimeDirectory() / "data/pbr/rustediron2_metallic.png", true, true);
    mpNormalMap = Texture::createFromFile(device, getRuntimeDirectory() / "data/pbr/normal.png", true, false);
    mpRoughnessMap = Texture::createFromFile(device, getRuntimeDirectory() / "data/pbr/rustediron2_roughness.png", true, true);
}

void NormalMap::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    pRenderContext->clearFbo(mpRasterFbo.get(), float4(0.2f), 1.f, 0);
    rasterize(pRenderContext, pTargetFbo);
    postProcess(pRenderContext, pTargetFbo, mpRasterFbo->getColorTexture(0));
}

void NormalMap::onGuiRender(Gui* pGui)
{
    Gui::Window w(pGui, "NormalMap Settings", {300, 400}, {10, 80});

    w.rgbColor("albedo", albedo);
    w.slider("metallic", metallic, .0f, 1.0f);
    w.slider("roughness", roughness, .0f, 1.0f);
    w.slider("ao", ao, .0f, 1.0f);
}
