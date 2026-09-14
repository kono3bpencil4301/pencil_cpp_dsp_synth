#include "voice.h"
#include "wav_and_pcm.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

// ============================================================================
// Voice + ADSR 包络 Demo：逐采样点生成带包络的音符并写入 WAV 文件
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

        // ---------- 创建 Voice 并配置 ADSR 包络 ----------
        Voice voice;
        voice.setWavetable(OscillatorType::Sawtooth);
        voice.setSampleRate(sampleRate);
        voice.setADSRAll(0.5f, 2.0f, 0.1f, 4.0f);

        // ---------- 逐采样点生成音频 ----------
        auto buffer = voice.generate(60, 0.8f, 5.0f);

        // ---------- 写入 WAV 文件 ----------
        GenerateWavFile("voice_adsr.wav", buffer, sampleRate);
    }

    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
