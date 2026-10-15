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

struct TrackConfig
{
    std::string name;
    std::string wavetable = "sine";
    std::string curveType = "linear";
    ADSRConfig adsr;
    std::vector<NoteConfig> notes;
};

// 文件不存在时生成默认 JSON；存在时保留用户修改。
void EnsureDefaultConfig(const std::filesystem::path &path);

// 读取并验证每条音轨，最终返回普通 C++ 数据。
std::vector<TrackConfig> LoadTracks(const std::filesystem::path &path);