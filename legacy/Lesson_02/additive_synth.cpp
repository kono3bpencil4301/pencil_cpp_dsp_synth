#include "additive_synth.h"
#include "wav_and_pcm.h"
#include "additive_wavetable_factory.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

// ============================================================================
// 加法合成器 Demo：用 AdditiveSynth 生成含有泛音的正弦波并写入 WAV 文件
// ============================================================================

AdditiveSynth additiveSynth;

void GenerateWavFile(const std::string &filename, const std::vector<float> &buffer)
{
    // ---------- 音频参数 ----------
    const double sampleRate = 44100.0;
    const int channels = 2; // 立体声
    const uint32_t frameCount = static_cast<uint32_t>(buffer.size());

    std::ofstream out(filename, std::ios::binary);
    if (!out)
        throw std::runtime_error("failed to open file");

    WriteWavHeader(out, sampleRate, channels, frameCount);

    // 峰值归一化：防止多分量叠加导致削波
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
    std::cout << "Done: " << filename << "\n";
}

int main()
{
    try

    {
        GenerateWavFile("additive_synth.wav", additiveSynth.SetBaseConfig(220.0, 0.8, 0).Add(0.5, 0).Add(0.2, 2).Generate());
        GenerateWavFile("additive_synth_sine.wav", AdditiveWavetableFactory::CreateSine(220.0, 1.0, 0.5));
        GenerateWavFile("additive_synth_square.wav", AdditiveWavetableFactory::CreateSquare(220.0, 1.0, 0.5));
        GenerateWavFile("additive_synth_sawtooth.wav", AdditiveWavetableFactory::CreateSawtooth(220.0, 1.0, 0.5));
        GenerateWavFile("additive_synth_triangle.wav", AdditiveWavetableFactory::CreateTriangle(220.0, 1.0, 0.5));
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
