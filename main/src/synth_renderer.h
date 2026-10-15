#pragma once

#include "config_manager.h"

#include <vector>

std::vector<float> RenderTrack(
    const TrackConfig &track,
    double sampleRate);