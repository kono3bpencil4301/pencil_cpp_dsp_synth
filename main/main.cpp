#include "voice.h"
#include "poly_synth.h"
#include "wav_and_pcm.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdexcept>

// ============================================================================
// PolySynth 多音复音 Demo
// ============================================================================

void GenerateWavFile(const std::string &filename, const std::vector<float> &buffer, double sampleRate)
{
    const int channels = 2; // 立体声
    const uint32_t frameCount = static_cast<uint32_t>(buffer.size());

    std::ofstream out(filename, std::ios::binary);
    if (!out)
        throw std::runtime_error("failed to open file");

    WriteWavHeader(out, sampleRate, channels, frameCount);

    // 峰值归一化：防止削波
    float peak = 0.0f;
    for (uint32_t i = 0; i < frameCount; ++i)
        peak = std::max(peak, std::abs(buffer[i]));
    const float gain = (peak > 1.0f) ? (1.0f / peak) : 1.0f;

    for (uint32_t i = 0; i < frameCount; ++i)
    {
        float value = buffer[i] * gain;
        const long integer_value = std::lround(value * 32767.0);
        const auto sample = static_cast<int16_t>(
            std::clamp(integer_value, -32768L, 32767L));

        for (int ch = 0; ch < channels; ++ch)
        {
            WriteU16LE(out, static_cast<uint16_t>(sample));
        }
    }
    if (!out)
        throw std::runtime_error("failed to write file");
    std::cout << "Done: " << filename << " (" << frameCount << " frames)\n";
}

int main()
{
    try
    {
        const float sampleRate = 44100.0f;

        // ============================
        // Demo 1：C 大调三和弦 (C4-E4-G4) —— 3 个独立 Voice 同时演奏
        // ============================
        {
            // 每个音符使用独立 Voice，避免音符覆盖和振荡器重复推进。
            Voice voices[3];
            PolySynth synth;
            for (auto &voice : voices)
            {
                voice.setWavetable(OscillatorType::Sawtooth);
                voice.setSampleRate(sampleRate);
                voice.setCurveType(CurveType::Linear);
                voice.setADSRAll(0.05f, 0.3f, 0.3f, 1.5f);
                synth.addVoice(&voice);
            }

            // ---------- C 大调三和弦：C4(60), E4(64), G4(67) ----------
            // 3 个 Voice 分别演奏根音、三度、五度
            std::vector<int> noteNums = {60, 64, 67};
            std::vector<float> velocities = {0.8f, 0.7f, 0.6f};
            std::vector<float> holdTimes = {3.0f, 3.0f, 3.0f};

            auto buffer = synth.generate(noteNums, velocities, holdTimes);
            GenerateWavFile("poly_chord.wav", buffer, sampleRate);

        }

        // ============================
        // Demo 2：C 大调琶音 (C4-E4-G4) —— 每个 Voice 错开 0.8 秒依次进入
        // ============================
        {
            // 每个音符使用独立 Voice，保留各自的振荡器和包络状态。
            Voice voices[3];
            PolySynth synth;
            for (auto &voice : voices)
            {
                voice.setWavetable(OscillatorType::Sine);
                voice.setSampleRate(sampleRate);
                voice.setCurveType(CurveType::Linear);
                voice.setADSRAll(0.05f, 0.3f, 0.3f, 1.5f);
                synth.addVoice(&voice);
            }

            // ---------- C 大调琶音：每个音错开 0.8 秒 ----------
            std::vector<int> noteNums = {60, 64, 67};
            std::vector<float> velocities = {0.8f, 0.7f, 0.6f};
            std::vector<float> holdTimes = {2.0f, 2.0f, 2.0f};
            std::vector<float> startTimes = {0.0f, 0.8f, 1.6f};

            auto buffer = synth.generate(noteNums, velocities, holdTimes, startTimes);
            GenerateWavFile("poly_arpeggio.wav", buffer, sampleRate);

        }
    }

    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
