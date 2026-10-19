#pragma once

#include "config_manager.h"

#include <vector>

// 按 track.filter.cutoff 进行 101 抽头 FIR 低通滤波，保持原采样数量。
std::vector<float> RenderTrack(
    const TrackConfig &track,
    double sampleRate);
