#pragma once
#include "../MultiPassPostProcess.h"
class FilmGrain : public MultiPassPostProcess
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

        float strength = 0.0f;
};

