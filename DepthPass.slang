import Scene.Raster;

VSOut vsMain(VSIn vIn)
{
    return defaultVS(vIn);
}

void psMain(VSOut vsOut, uint triangleIndex : SV_PrimitiveID) : SV_TARGET
{
#if USE_ALPHA_TEST
    let lod = ImplicitLodTextureSampler();
    if (alphaTest(vsOut, triangleIndex, lod)) discard;
#endif
}