import Scene.Raster;
import Utils.Sampling.TinyUniformSampleGenerator;
import Rendering.Lights.LightHelpers;

cbuffer PerFrameCB
{
    uint2 gFrameDim;
};

// GBuffer channels
struct GBufferPSOut
{
    float4 color : SV_TARGET0;
    float4 posW : SV_TARGET1;
    float4 normW : SV_TARGET2;
    float4 tangentW : SV_TARGET3;
    float4 faceNormalW : SV_TARGET4;
    float2 mvec : SV_TARGET5;
};
float2 calcMotionVector(float2 pixelCrd, float4 prevPosH, float2 renderTargetDim)
{
    float2 prevCrd = prevPosH.xy / prevPosH.w;
#ifdef FALCOR_FLIP_Y
    prevCrd *= float2(0.5, 0.5);
#else
    prevCrd *= float2(0.5, -0.5);
#endif
    prevCrd += 0.5f;
    float2 normalizedCrd = pixelCrd / renderTargetDim;
    return prevCrd - normalizedCrd;
}
float2 computeMotionVector(const VSOut vsOut, const int2 ipos)
{
    // Current sample in pixel coords.
    float2 pixelPos = ipos + float2(0.5, 0.5);
    // Sample in previous frame in clip space coords, no jittering applied.
    float4 prevPosH = vsOut.prevPosH;
    // Remove camera jitter from motion vector
    return calcMotionVector(pixelPos, prevPosH, gFrameDim) + float2(gScene.camera.data.jitterX, -gScene.camera.data.jitterY);
}

GBufferPSOut prepareGBufferData(const ShadingData sd, const VertexData v, const IMaterialInstance mi, const BSDFProperties bsdfProperties)
{
    GBufferPSOut gbuf = {};

    // We store the final normal in the G-buffer but pass along the original geometric tangent and its handedness (sign) unmodified.
    // This is needed for correctly orthonormalizing the tangent frame and computing the bitangent in passes that consume the G-buffer data.
    float bitangentSign = sd.frame.getHandednessSign();

    gbuf.posW = float4(sd.posW, 1.f);
    gbuf.normW = float4(sd.frame.N, 1.f); // to see it on imgui,set alpha to 1.0f
    gbuf.tangentW = v.tangentW;
    gbuf.faceNormalW = float4(sd.faceN, 1.f); // to see it on imgui,set alpha to 1.0f
    return gbuf;
}

VSOut vsMain(VSIn vIn)
{
    return defaultVS(vIn);
}

GBufferPSOut psMain(VSOut vsOut, uint triangleIndex: SV_PrimitiveID) : SV_TARGET
{
    let lod = ImplicitLodTextureSampler();
    if (alphaTest(vsOut, triangleIndex, lod))
        discard;

    float3 viewDir = normalize(gScene.camera.getPosition() - vsOut.posW);
    ShadingData sd = prepareShadingData(vsOut, triangleIndex, viewDir, lod);

    float3 faceNormal = gScene.getFaceNormalW(vsOut.instanceID, triangleIndex);
    VertexData v = prepareVertexData(vsOut, faceNormal);
    // Create material instance.
    let mi = gScene.materials.getMaterialInstance(sd, lod);
    let bsdfProperties = mi.getProperties(sd);

    float3 color = mi.getProperties(sd).emission;

    uint3 launchIndex = DispatchRaysIndex();
    TinyUniformSampleGenerator sg = TinyUniformSampleGenerator(launchIndex.xy, 0);
    GBufferPSOut gbuf = prepareGBufferData(sd, v, mi, bsdfProperties);
    int2 ipos = int2(vsOut.posH.xy);
    gbuf.mvec = computeMotionVector(vsOut, ipos);
    // Direct lighting from analytic light sources
    for (int i = 0; i < gScene.getLightCount(); i++)
    {
        AnalyticLightSample ls;
        evalLightApproximate(sd.posW, gScene.getLight(i), ls);
        color += mi.eval(sd, ls.dir, sg) * ls.Li;
    }
    gbuf.color = float4(color, 1.f);
    return gbuf;
}
