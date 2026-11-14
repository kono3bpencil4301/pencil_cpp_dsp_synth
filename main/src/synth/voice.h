#pragma once

#include "envelope_adsr.h"
#include "additive_synth.h"
#include "note.h"
#include "additive_wavetable_factory.h"
#include "iir_biquad_filter.h"
#include "iir_designer.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

class Voice

{
public:
    Voice()
    {
        setSampleRate(44100.0f);
    }
    void noteOn(int noteNum, float velocity, uint8_t channel = 0)
    {
        active = true;
        midiChannel = channel;
        this->velocity = std::clamp(velocity, 0.0f, 1.0f);
        noteNumber = noteNum;

        note.noteNumber = noteNum;
        note.velocity = velocity;
        oscillator.setFrequency(static_cast<float>(note.getFrequency()));
        oscillator.setAmplitude(1.0f);

        // 用当前音符的正确频率重新生成波形表
        if (wavetableTypeSet)
            oscillator.setWavetable(wavetableType);

        envelope.trigger();
        filterEnvelope.trigger();
        filter.reset();
        keyDown = true;
    }
    void noteOff()
    {
        if (!active || !keyDown)
            return;
        keyDown = false;
        envelope.noteOff();
        filterEnvelope.noteOff();
    }
    void setAttackTime(float t) { envelope.setAttackTime(t); }
    void setDecayTime(float t) { envelope.setDecayTime(t); }
    void setSustainLevel(float l) { envelope.setSustainLevel(l); }
    void setReleaseTime(float t) { envelope.setReleaseTime(t); }
    void setADSRAll(float attackTime, float decayTime, float sustainLevel, float releaseTime)
    {
        envelope.setADSRAll(attackTime, decayTime, sustainLevel, releaseTime);
    }
    void setAttackCurveType(CurveType type)
    {
        attackCurveType = type;
        envelope.setAttackCurveType(type);
    }
    void setDecayCurveType(CurveType type)
    {
        decayCurveType = type;
        envelope.setDecayCurveType(type);
    }
    void setReleaseCurveType(CurveType type)
    {
        releaseCurveType = type;
        envelope.setReleaseCurveType(type);
    }
    void setCurveType(CurveType type)
    {
        attackCurveType = decayCurveType = releaseCurveType = type;
        envelope.setCurveType(type);
    }

    CurveType getAttackCurveType() const { return attackCurveType; }
    CurveType getDecayCurveType() const { return decayCurveType; }
    CurveType getReleaseCurveType() const { return releaseCurveType; }

    void setCurveStrength(float attackStrength, float decayStrength, float releaseStrength)
    {
        envelope.setStrength(attackStrength, decayStrength, releaseStrength);
    }

    float getAttackStrength() const { return envelope.getAttackStrength(); }
    float getDecayStrength() const { return envelope.getDecayStrength(); }
    float getReleaseStrength() const { return envelope.getReleaseStrength(); }
    void setSampleRate(float sr)
    {
        sampleRate = sr;
        envelope.setSampleRate(sr);
        oscillator.setSampleRate(sr);
        filterEnvelope.setSampleRate(sr);
    }
    float getSampleRate() const
    {
        return static_cast<float>(sampleRate);
    }
    void setFrequency(float freq)
    {
        oscillator.setFrequency(freq);
    }

    void setVoiceFilter(
        bool enabled,
        float amount,
        float cutoff,
        float resonance,
        IIRDesigner::BiquadType type,
        CurveType curveType,
        float attack,
        float decay,
        float sustain,
        float release)
    {
        filterEnabled = enabled;
        filterAmount = std::max(0.0f, amount);
        filterBaseCutoff = cutoff;
        filterResonance = resonance;
        filterType = type;
        filterEnvelope.setADSRAll(attack, decay, sustain, release);
        filterEnvelope.setCurveType(curveType);
        filter.setCoefficients(
            IIRDesigner::Biquad(filterType, cutoff, sampleRate, resonance));
    }

    double process()
    {
        if (!active)
            return 0.0;

        const double env = envelope.process();

        if (envelope.isFinished())
        {
            active = false;
            keyDown = false;
            return 0.0;
        }

        double sample = oscillator.process();
        if (filterEnabled)
        {
            const double filterEnv = filterEnvelope.process();
            const double cutoff = std::clamp(
                filterBaseCutoff * std::pow(2.0, filterAmount * filterEnv),
                20.0, sampleRate * 0.49);
            filter.setCoefficients(
                IIRDesigner::Biquad(
                    filterType, cutoff, sampleRate, filterResonance));
            sample = filter.process(sample);
        }
        return sample * env * velocity;
    }

    void setWavetable(OscillatorType type)
    {
        wavetableType = type;
        wavetableTypeSet = true;
        oscillator.setWavetable(type);
    }

    bool isActive() const
    {
        return active;
    }
    EnvelopeADSR::Stage getStage() const { return envelope.getStage(); }

    std::vector<float> generate(int noteNum, float velocity, float holdTime)
    {
        std::vector<float> buffer;
        noteOn(noteNum, velocity);
        const int holdSamples = static_cast<int>(holdTime * sampleRate);
        for (int i = 0; i < holdSamples; ++i)
        {
            buffer.push_back(
                static_cast<float>(process()));
        }

        noteOff();
        while (active)
        {
            buffer.push_back(
                static_cast<float>(process()));
        }

        return buffer;
    }

    bool matchesNote(int noteNum, uint8_t channel) const
    {
        return active && noteNumber == noteNum && midiChannel == channel;
    }

private:
    EnvelopeADSR envelope;
    Oscillator oscillator;
    Note note;
    double sampleRate = 44100.0;
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    OscillatorType wavetableType = OscillatorType::Sine;
    bool wavetableTypeSet = false;
    uint8_t midiChannel = 0;
    bool keyDown = false;
    CurveType attackCurveType = CurveType::Linear;
    CurveType decayCurveType = CurveType::Linear;
    CurveType releaseCurveType = CurveType::Linear;
    EnvelopeADSR filterEnvelope;
    BiquadFilter filter;
    bool filterEnabled = false;
    float filterAmount = 2.0f;
    float filterBaseCutoff = 2000.0f;
    float filterResonance = 0.70710678f;
    IIRDesigner::BiquadType filterType = IIRDesigner::BiquadType::Lowpass;
};
