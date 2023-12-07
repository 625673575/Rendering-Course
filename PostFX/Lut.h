#pragma once
#include "../MultiPassPostProcess.h"
class Lut : public MultiPassPostProcess
{
    public:
        void onLoad(const SampleAppConfig& config, const ref<Device>& pDevice, RenderContext* pRenderContext)override;
        void onFrameRender(
            RenderContext* pRenderContext,
            float time,
            ref<Texture> color,
            ref<Texture> depth,
            ref<Texture> normalWS,
            ref<Texture> posWS
        ) override;
        void onGui(Gui* pGui) override;
        ref<Texture> getFinalColor() override;
        ref<Texture> lutTex;

        float amount = 0.0f;
};

