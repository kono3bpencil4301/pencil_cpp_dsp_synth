#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct NoteConfig
{
    int number = 60;
    float velocity = 1.0f;
    float holdTime = 1.0f;
    float startTime = 0.0f;
};

struct ADSRConfig
{
    float attack = 0.05f;
    float decay = 0.3f;
    float sustain = 0.3f;
    float release = 1.5f;
};

struct filterConfig
{
    float cutoff = 2000.0f;
    float resonance = 0.70710678f; // Biquad Q；约 0.707 为 Butterworth 响应
    std::string type = "lowpass";
};

struct VoiceFilterConfig
{
    bool enabled = false;
    float amount = 2.0f;
    std::string curveType = "linear";
    ADSRConfig adsr;
};

struct TrackConfig
{
    std::string name;
    std::string wavetable = "sine";
    std::string curveType = "linear";
    ADSRConfig adsr;
    filterConfig filter;
    VoiceFilterConfig voiceFilter;
    std::vector<NoteConfig> notes;
};

struct ProjectConfig
{
    int channels = 2;
    std::vector<TrackConfig> tracks;
};

// 文件不存在时生成默认 JSON；存在时保留用户修改。
void EnsureDefaultConfig(const std::filesystem::path &path);

// 读取并验证每条音轨，最终返回普通 C++ 数据。
std::vector<TrackConfig> LoadTracks(const std::filesystem::path &path);
ProjectConfig LoadProjectConfig(
    const std::filesystem::path &synthConfigPath,
    const std::filesystem::path &notesConfigPath);
std::string LoadConfiguredMidiFile(
    const std::filesystem::path &synthConfigPath);
