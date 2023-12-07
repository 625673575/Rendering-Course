#include "RayMarchingPrimitive.h"

RayMarchingPrimitive::RayMarchingPrimitive(const SampleAppConfig& config):GBuffer(config) {}

RayMarchingPrimitive::~RayMarchingPrimitive() {}

void RayMarchingPrimitive::onLoad(RenderContext* pRenderContext)
{
    GBuffer::onLoad(pRenderContext);
}
void RayMarchingPrimitive::onFrameRender(RenderContext* pRenderContext, const ref<Fbo>& pTargetFbo)
{
    GBuffer::onFrameRender(pRenderContext, pTargetFbo);
}

void RayMarchingPrimitive::onGuiRender(Gui* pGui)
{

}
