#pragma once

#include "config_manager.h"

#include <vector>

struct TrackRenderResult
{
    std::vector<float> dry;
    std::vector<float> fir;
    std::vector<float> iir;
    std::vector<float> biquad;
};

// 返回原始音频及按音轨参数滤波的 FIR、一阶 IIR、Biquad 音频。
TrackRenderResult RenderTrack(
    const TrackConfig &track,
    double sampleRate);
