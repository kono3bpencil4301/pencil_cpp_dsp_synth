#include "synth_renderer.h"
#include "filters/fir_designer.h"
#include "filters/fir_filter.h"
#include "filters/iir_designer.h"
#include "filters/iir_onepole_filter.h"
#include "filters/iir_biquad_filter.h"

#include "synth/voice.h"
#include "synth/poly_synth.h"

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

    IIRDesigner::BiquadType ParseFilterType(const std::string &name)
    {
        if (name == "lowpass")
            return IIRDesigner::BiquadType::Lowpass;
        if (name == "highpass")
            return IIRDesigner::BiquadType::Highpass;
        if (name == "bandpass")
            return IIRDesigner::BiquadType::Bandpass;
        if (name == "notch")
            return IIRDesigner::BiquadType::Notch;
        throw std::invalid_argument("Unknown filter type: " + name);
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
        voice.setVoiceFilter(
            track.voiceFilter.enabled,
            track.voiceFilter.amount,
            track.filter.cutoff,
            track.filter.resonance,
            ParseFilterType(track.filter.type),
            ParseCurveType(track.voiceFilter.curveType),
            track.voiceFilter.adsr.attack,
            track.voiceFilter.adsr.decay,
            track.voiceFilter.adsr.sustain,
            track.voiceFilter.adsr.release);
    }
} // namespace

TrackRenderResult RenderTrack(
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
    auto buffer = synth.generate(
        noteNums,
        velocities,
        holdTimes,
        startTimes);

    // 对混合后的整条音轨进行低通滤波，每次渲染使用独立的历史状态。
    TrackRenderResult result;

    result.dry = buffer;
    result.fir = firFilter(
        buffer,
        FIRDesigner::lowpass(
            track.filter.cutoff,
            static_cast<float>(sampleRate),
            101));
    OnePoleFilter iir(sampleRate, track.filter.cutoff);
    result.iir = iir.process(buffer);
    BiquadFilter biquad;
    biquad.setCoefficients(IIRDesigner::Biquad(
        ParseFilterType(track.filter.type),
        track.filter.cutoff, sampleRate, track.filter.resonance));
    result.biquad = biquad.process(buffer);

    return result;
}
