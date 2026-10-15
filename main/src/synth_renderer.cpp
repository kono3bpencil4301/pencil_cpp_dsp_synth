#include "synth_renderer.h"

#include "voice.h"
#include "poly_synth.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    OscillatorType ParseWavetable(const std::string &name)
    {
        if (name == "sine")
            return OscillatorType::Sine;
        if (name == "square")
            return OscillatorType::Square;
        if (name == "sawtooth")
            return OscillatorType::Sawtooth;
        if (name == "triangle")
            return OscillatorType::Triangle;

        throw std::invalid_argument("Unknown wavetable: " + name);
    }

    CurveType ParseCurveType(const std::string &name)
    {
        if (name == "linear")
            return CurveType::Linear;
        if (name == "EaseIn")
            return CurveType::EaseIn;
        if (name == "EaseOut")
            return CurveType::EaseOut;

        throw std::invalid_argument("Unknown curve type: " + name);
    }

    void ConfigureVoice(
        Voice &voice,
        const TrackConfig &track,
        double sampleRate)
    {
        voice.setSampleRate(sampleRate);
        voice.setWavetable(ParseWavetable(track.wavetable));
        voice.setCurveType(ParseCurveType(track.curveType));

        voice.setADSRAll(
            track.adsr.attack,
            track.adsr.decay,
            track.adsr.sustain,
            track.adsr.release);
    }
} // namespace

std::vector<float> RenderTrack(
    const TrackConfig &track,
    double sampleRate)
{
    // 本章仍采用第五章的“一音符对应一 Voice”离线方案。
    std::vector<Voice> voices(track.notes.size());
    PolySynth synth;

    // 提前统一配置，再注册 Voice 指针。
    // 注册后不要让 voices 扩容，以免旧指针失效。
    for (auto &voice : voices)
    {
        ConfigureVoice(voice, track, sampleRate);
        synth.addVoice(&voice);
    }

    std::vector<int> noteNums;
    std::vector<float> velocities;
    std::vector<float> holdTimes;
    std::vector<float> startTimes;

    const auto count = track.notes.size();
    noteNums.reserve(count);
    velocities.reserve(count);
    holdTimes.reserve(count);
    startTimes.reserve(count);

    for (const auto &note : track.notes)
    {
        noteNums.push_back(note.number);
        velocities.push_back(note.velocity);
        holdTimes.push_back(note.holdTime);
        startTimes.push_back(note.startTime);
    }

    // 与第五章教程保持一致：四参数 generate。
    return synth.generate(
        noteNums,
        velocities,
        holdTimes,
        startTimes);
}