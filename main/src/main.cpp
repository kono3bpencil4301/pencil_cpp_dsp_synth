#include "config_manager.h"
#include "synth_renderer.h"
#include "wav_and_pcm.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

// 本章复用前面章节的 WAV Header / 小端写入工具。
// 这里输入 buffer 为单声道 Float PCM，再复制到 L/R 两个声道。
void GenerateWavFile(
    const std::filesystem::path &path,
    const std::vector<float> &buffer,
    double sampleRate)
{
    constexpr int channels = 2;

    // 经典 PCM WAV 使用 32-bit chunk 大小，限制数据长度。
    if (buffer.size() >
        (std::numeric_limits<uint32_t>::max() - 36u) / (channels * 2u))
    {
        throw std::runtime_error("WAV is too large");
    }

    const auto frames = static_cast<uint32_t>(buffer.size());
    std::ofstream out(path, std::ios::binary);
    if (!out)
        throw std::runtime_error("Cannot create WAV: " + path.string());

    WriteWavHeader(out, sampleRate, channels, frames);

    float peak = 0.0f;
    for (float value : buffer)
    {
        if (!std::isfinite(value))
            throw std::runtime_error("Non-finite sample in audio buffer");
        peak = std::max(peak, std::abs(value));
    }

    const float gain = peak > 1.0f ? 1.0f / peak : 1.0f;

    for (float value : buffer)
    {
        const long scaled = std::lround(value * gain * 32767.0f);
        const int16_t sample = static_cast<int16_t>(
            std::clamp(scaled, -32768L, 32767L));

        for (int ch = 0; ch < channels; ++ch)
            WriteU16LE(out, static_cast<uint16_t>(sample));
    }

    if (!out)
        throw std::runtime_error("Failed to write WAV: " + path.string());
}

int main()
{
    try
    {
        constexpr double sampleRate = 44100.0;
        const std::filesystem::path configPath = "config/config.json";
        const std::filesystem::path outputDir = "output";

        EnsureDefaultConfig(configPath);
        const auto tracks = LoadTracks(configPath);

        std::filesystem::create_directories(outputDir);

        for (const auto &track : tracks)
        {
            const auto buffer = RenderTrack(track, sampleRate);
            const auto outputPath = outputDir / (track.name + ".wav");

            GenerateWavFile(outputPath, buffer, sampleRate);
            std::cout << "Done: " << outputPath.string() << '\n';
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}