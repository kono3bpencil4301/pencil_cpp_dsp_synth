
#include "voice.h"
#include "poly_synth.h"
#include "wav_and_pcm.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ==================================================
// 1. 数据结构
// ==================================================

struct NoteConfig
{
    int number;
    float velocity;
    float holdTime;
    float startTime;
};

struct SynthConfig
{
    OscillatorType wavetable;
    CurveType curve;

    float attack;
    float decay;
    float sustain;
    float release;

    std::vector<NoteConfig> notes;
};

// ==================================================
// 2. JSON 字符串转换为枚举
// ==================================================

OscillatorType parseWavetable(const std::string &name)
{
    if (name == "sine")
        return OscillatorType::Sine;
    if (name == "sawtooth")
        return OscillatorType::Sawtooth;
    if (name == "square")
        return OscillatorType::Square;
    if (name == "triangle")
        return OscillatorType::Triangle;

    throw std::invalid_argument("Unknown wavetable: " + name);
}

CurveType parseCurveType(const std::string &name)
{
    if (name == "linear")
        return CurveType::Linear;
    if (name == "EaseIn")
        return CurveType::EaseIn;
    if (name == "EaseOut")
        return CurveType::EaseOut;

    throw std::invalid_argument("Unknown curve: " + name);
}

// ==================================================
// 3. 默认 JSON 配置
// ==================================================

json MakeDefaultConfig()
{
    return {
        {"poly_chord", {{"note", {{"noteNumber", {60, 64, 67}}, {"noteVelocity", {0.8, 0.7, 0.6}}, {"holdTimes", {3.0, 3.0, 3.0}}, {"startTime", {0.0, 0.0, 0.0}}}}, {"ADSR_config", {0.05, 0.3, 0.3, 1.5}}, {"wavetable", "sawtooth"}, {"curveType", "linear"}}},
        {"poly_arpeggio", {{"note", {{"noteNumber", {60, 64, 67, 72}}, {"noteVelocity", {0.8, 0.7, 0.6, 0.5}}, {"holdTimes", {2.0, 2.0, 2.0, 2.0}}, {"startTime", {0.0, 0.8, 1.6, 2.4}}}}, {"ADSR_config", {0.05, 0.3, 0.3, 1.5}}, {"wavetable", "sine"}, {"curveType", "linear"}}}};
}

// ==================================================
// 4. 加载 JSON，不存在则创建
// ==================================================

json LoadJson(const std::string &path)
{
    if (!std::filesystem::exists(path))
    {
        json defaults = MakeDefaultConfig();

        auto parent = std::filesystem::path(path).parent_path();

        if (!parent.empty())
            std::filesystem::create_directories(parent);

        std::ofstream output(path);

        if (!output)
            throw std::runtime_error("Cannot create JSON: " + path);

        output << defaults.dump(4);
        output.close();

        return defaults;
    }

    std::ifstream input(path);

    if (!input)
        throw std::runtime_error("Cannot open JSON: " + path);

    json config;
    input >> config;

    return config;
}

// ==================================================
// 5. JSON -> C++ 合成器配置
// ==================================================

SynthConfig ParseSynthConfig(const json &j)
{
    SynthConfig cfg{};

    cfg.wavetable = parseWavetable(
        j.at("wavetable").get<std::string>());

    cfg.curve = parseCurveType(
        j.at("curveType").get<std::string>());

    const auto &adsr = j.at("ADSR_config");

    if (!adsr.is_array() || adsr.size() != 4)
        throw std::runtime_error("Invalid ADSR_config");

    cfg.attack = adsr.at(0).get<float>();
    cfg.decay = adsr.at(1).get<float>();
    cfg.sustain = adsr.at(2).get<float>();
    cfg.release = adsr.at(3).get<float>();

    const auto &n = j.at("note");

    auto numbers =
        n.at("noteNumber").get<std::vector<int>>();

    auto velocities =
        n.at("noteVelocity").get<std::vector<float>>();

    auto holds =
        n.at("holdTimes").get<std::vector<float>>();

    auto starts =
        n.at("startTime").get<std::vector<float>>();

    const size_t count = numbers.size();

    if (count != velocities.size() ||
        count != holds.size() ||
        count != starts.size())
    {
        throw std::runtime_error(
            "Note array lengths do not match");
    }

    for (size_t i = 0; i < count; ++i)
    {
        if (!std::isfinite(starts[i]) || starts[i] < 0.0f ||
            !std::isfinite(holds[i]) || holds[i] < 0.0f)
        {
            throw std::runtime_error("Invalid note timing");
        }

        cfg.notes.push_back({numbers[i],
                             velocities[i],
                             holds[i],
                             starts[i]});
    }

    return cfg;
}

// ==================================================
// 6. 创建 Voice
// ==================================================

void ConfigureVoice(
    Voice &voice,
    const SynthConfig &cfg,
    double sampleRate)
{
    voice.setSampleRate(sampleRate);
    voice.setWavetable(cfg.wavetable);
    voice.setCurveType(cfg.curve);

    voice.setADSRAll(
        cfg.attack,
        cfg.decay,
        cfg.sustain,
        cfg.release);
}

// ==================================================
// 7. 合成音轨
// ==================================================

std::vector<float> RenderTrack(
    const SynthConfig &cfg,
    double sampleRate)
{
    std::vector<float> mix;

    // 每个音符独立合成，再按 startTime 混音
    for (const auto &note : cfg.notes)
    {
        Voice voice;
        ConfigureVoice(voice, cfg, sampleRate);

        PolySynth synth;
        synth.addVoice(&voice);

        std::vector<int> numbers{note.number};
        std::vector<float> velocities{note.velocity};
        std::vector<float> holds{note.holdTime};

        auto buffer = synth.generate(
            numbers,
            velocities,
            holds);

        size_t offset = static_cast<size_t>(
            std::llround(note.startTime * sampleRate));

        size_t requiredSize = offset + buffer.size();

        if (mix.size() < requiredSize)
            mix.resize(requiredSize, 0.0f);

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            mix[offset + i] += buffer[i];
        }
    }

    return mix;
}

// ==================================================
// 8. WAV 导出
// ==================================================

void GenerateWavFile(
    const std::string &filename,
    const std::vector<float> &buffer,
    double sampleRate)
{
    const int channels = 2;
    const uint32_t frameCount =
        static_cast<uint32_t>(buffer.size());

    std::ofstream out(filename, std::ios::binary);

    if (!out)
        throw std::runtime_error("Cannot open WAV file");

    WriteWavHeader(out, sampleRate, channels, frameCount);

    float peak = 0.0f;

    for (float value : buffer)
        peak = std::max(peak, std::abs(value));

    const float gain =
        peak > 1.0f ? 1.0f / peak : 1.0f;

    for (float value : buffer)
    {
        long scaled = std::lround(value * gain * 32767.0f);

        int16_t sample = static_cast<int16_t>(
            std::clamp(scaled, -32768L, 32767L));

        for (int ch = 0; ch < channels; ++ch)
            WriteU16LE(out, static_cast<uint16_t>(sample));
    }

    if (!out)
        throw std::runtime_error("Failed to write WAV");

    std::cout << "Done: " << filename << '\n';
}

// ==================================================
// 9. Main
// ==================================================

int main()
{
    try
    {
        const double sampleRate = 44100.0;
        const std::string path = "./config/config.json";

        json config = LoadJson(path);

        // 自动遍历所有音轨
        for (auto it = config.begin(); it != config.end(); ++it)
        {
            const std::string trackName = it.key();

            SynthConfig cfg = ParseSynthConfig(it.value());

            auto buffer = RenderTrack(cfg, sampleRate);

            GenerateWavFile(
                trackName + ".wav",
                buffer,
                sampleRate);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
