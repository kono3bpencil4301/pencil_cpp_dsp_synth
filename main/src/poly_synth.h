#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include "voice.h"

class PolySynth
{
public:
    PolySynth() {}
    ~PolySynth() {}

    void addVoice(Voice *voice)
    {
        voices.push_back(voice);
    }

    double process()
    {
        double mix = 0.0;
        for (auto *voice : voices)
        {
            mix += voice->process();
        }
        return mix;
    }

    std::vector<float> generate(const std::vector<int> &noteNums,
                                const std::vector<float> &velocities,
                                const std::vector<float> &holdTimes,
                                const std::vector<float> &startTimes = {})
    {
        if (noteNums.size() != velocities.size() || noteNums.size() != holdTimes.size())
            throw std::invalid_argument("noteNums, velocities, holdTimes must have the same size");
        const float sampleRate = voices.empty() ? 44100.0f : voices[0]->sampleRate;

        int maxFrames = 0;
        for (size_t i = 0; i < voices.size(); ++i)
        {
            float startT = (i < startTimes.size()) ? startTimes[i] : 0.0f;
            int totalSamples = static_cast<int>(std::round((startT + holdTimes[i] + 5.0f) * sampleRate));
            maxFrames = std::max(maxFrames, totalSamples);
        }

        for (size_t i = 0; i < voices.size(); ++i)
        {
            float startT = (i < startTimes.size()) ? startTimes[i] : 0.0f;
            if (startT <= 0.0f)
                voices[i]->noteOn(noteNums[i], velocities[i]);
        }

        std::vector<float> buffer;
        buffer.resize(maxFrames);

        for (int frame = 0; frame < maxFrames; ++frame)
        {
            // 延迟触发：到达 startFrame 时才 noteOn
            for (size_t i = 0; i < voices.size(); ++i)
            {
                float startT = (i < startTimes.size()) ? startTimes[i] : 0.0f;
                if (startT > 0.0f)
                {
                    int startFrame = static_cast<int>(std::round(startT * sampleRate));
                    if (frame == startFrame)
                        voices[i]->noteOn(noteNums[i], velocities[i]);
                }
            }

            for (size_t i = 0; i < voices.size(); ++i)
            {
                float startT = (i < startTimes.size()) ? startTimes[i] : 0.0f;
                int offFrame = static_cast<int>(std::round((startT + holdTimes[i]) * sampleRate));
                if (frame == offFrame)
                {
                    voices[i]->noteOff();
                }
            }
            buffer[frame] = static_cast<float>(process());

            bool allDone = true;
            for (size_t i = 0; i < voices.size(); ++i)
            {
                float startT = (i < startTimes.size()) ? startTimes[i] : 0.0f;
                int startFrame = static_cast<int>(std::round(startT * sampleRate));
                if (frame < startFrame || voices[i]->isActive())
                {
                    allDone = false;
                    break;
                }
            }
            if (allDone)
                break;
        }
        return buffer;
    }

private:
    std::vector<Voice *> voices;
};
