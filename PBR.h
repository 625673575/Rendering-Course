#pragma once
#include "Falcor.h"
#include "Core/SampleApp.h"
#include "Core/Pass/RasterPass.h"
#include "Core/Program/Program.h"
#include "Core/Pass/FullScreenPass.h"
#include <Scene/TriangleMesh.h>

using namespace Falcor;

class PBR : public SampleApp
{
    struct VertexInput
    {
        float3 position; ///< Position.
        float3 normal;   ///< Shading normal.
        float3 tangent; ///< Shading tangent. The bitangent is computed: cross(normal, tangent.xyz) * tangent.w. NOTE: The tangent is *only*
        float3 bitangent;
        float2 texCrd; ///< Texture coordinates.
    };
    /// <summary>
    /// use float4 for hlsl buffer padding
    /// </summary>
    struct SLight
    {
        float4 intensity;
        float4 dirW;
        float4 posW;
        uint32_t type;
        float bias;
    };

public:
    PBR(const SampleAppConfig& config);
    ~PBR();

    void onLoad(RenderContext* pRenderContext) override;
    void onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo) override;
    void onGuiRender(Gui* pGui) override;

private:
    static const float4 kClearColor;
    const std::filesystem::path kCubePath = getProjectDirectory() / "data/framework/meshes/cube.obj";
    const std::filesystem::path kEnvMapPath = getProjectDirectory() / "data/desertpreview.jpg";
    const std::string kViewProjMatrices = "viewProjMatrices";
    const std::string kInverseTransposeWorldMatrices = "inverseTransposeWorldMatrices";
    const std::string kWorldMatrices = "worldMatrices";
    static ref<Vao> createVao(const ref<Device>& device, const ref<TriangleMesh>& mesh);
    void postProcess(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo, const ref<Texture>& rt);
    void rasterize(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo);
    void drawSkybox(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo);
    static void generateTangent(
        const TriangleMesh::VertexList& vertexList,
        const TriangleMesh::IndexList& indicesList,
        std::vector<float3>& tangents,
        std::vector<float3>& bitangents
    );
    ref<TriangleMesh> tringleMesh[3];
    ref<RasterPass> mpRasterPass;
    ref<Program> mpRasterProgram;
    ref<Vao> mpVao[3];
    ref<Fbo> mpRasterFbo;
    ref<ProgramVars> mpVars;
    ref<DepthStencilState> mpDepthStencil;
    ref<RasterizerState> mpRasterizeState[3];
    ref<Camera> mpCamera;
    float4x4 projectionMatrix;
    float4x4 viewMatrix;
    float4x4 VP;
    float4x4 modelMatrix;
    ref<Texture> mpAlbedoMap;
    ref<Texture> mpNormalMap;
    ref<Texture> mpMetallicMap;
    ref<Texture> mpRoughnessMap;

    ref<Sampler> gSampler;
    float3 albedo = float3(1.0f);
    float metallic = 0.86f, roughness = 0.13f;
    float3 metallicRoughness() { return float3(metallic, roughness, 0.0f); }

    static const int LIGHT_COUNT = 2;
    float3 lightColor[LIGHT_COUNT] = {float3(1.0f, 0, 0), float3(0, 1.0, 0)};
    float lightIntensity[LIGHT_COUNT] = {60.0, 90.0};
    SLight lights[LIGHT_COUNT];

    // Skybox
    ref<DepthStencilState> mpSkyboxDepthStencil;
    ref<RasterizerState> mpSkyboxRasterizeState;
    ref<Texture> mpEnvTexture;
    ref<EnvMap> mpEnvMap;
    ref<Program> mpEnvProgram;
    ref<ProgramVars> mpEnvVars;
};
