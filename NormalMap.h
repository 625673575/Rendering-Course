#pragma once
#include "Falcor.h"
#include "Core/SampleApp.h"
#include "Core/Pass/RasterPass.h"
#include "Core/Pass/FullScreenPass.h"
#include <Scene/TriangleMesh.h>

using namespace Falcor;

class NormalMap : public SampleApp
{
    struct VertexInput
    {
        float3 position; ///< Position.
        float3 normal;   ///< Shading normal.
        float3 tangent; ///< Shading tangent. The bitangent is computed: cross(normal, tangent.xyz) * tangent.w. NOTE: The tangent is *only*
        float3 bitangent;
        float2 texCrd;  ///< Texture coordinates.
    };

public:
    NormalMap(const SampleAppConfig& config);
    ~NormalMap();

    void onLoad(RenderContext* pRenderContext) override;
    void onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo) override;
    void onGuiRender(Gui* pGui) override;

private:
    static const float4 kClearColor;
    const std::string kViewProjMatrices = "viewProjMatrices";
    const std::string kInverseTransposeWorldMatrices = "inverseTransposeWorldMatrices";
    const std::string kWorldMatrices = "worldMatrices";
    static ref<Vao> createVao(const ref<Device>& device, const ref<TriangleMesh>& mesh);
    void postProcess(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo, const ref<Texture>& rt);
    void rasterize(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo);
    static void generateTangent(
        const TriangleMesh::VertexList& vertexList,
        const TriangleMesh::IndexList& indicesList,std::vector<float3>& tangents,
        std::vector<float3>& bitangents
    );
    ref<TriangleMesh> tringleMesh[3];
    ref<RasterPass> mpRasterPass;
    ref<Vao> mpVao[3];
    ref<Fbo> mpRasterFbo;
    ref<ProgramVars> mpVars;
    ref<DepthStencilState> mpDepthStencil;
    ref<RasterizerState> mpRasterizeState[3];

    ref<Texture> mpAlbedoMap;
    ref<Texture> mpNormalMap;
    ref<Texture> mpMetallicMap;
    ref<Texture> mpRoughnessMap;

    ref<Sampler> gSampler;
    float4x4 modelMatrix;
    float3 albedo = float3(1.0f);
    float metallic = 0.0f, roughness = 0.0f, ao = 0.0f;
    float3 metallicRoughness() { return float3(metallic, roughness, ao); }
};
