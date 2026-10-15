#pragma once

#include "envelope_adsr.h"
#include "additive_synth.h"
#include "note.h"
#include "additive_wavetable_factory.h"
#include <vector>

class Voice

{
public:
    float sampleRate = 44100.0f;
    void noteOn(int noteNum, float velocity)
    {
        active = true;
        noteNumber = noteNum;
        this->velocity = velocity;

        note.noteNumber = noteNum;
        note.velocity = velocity;
        oscillator.setFrequency(static_cast<float>(note.getFrequency()));
        oscillator.setAmplitude(1.0f);

        // 用当前音符的正确频率重新生成波形表
        if (wavetableTypeSet)
            oscillator.setWavetable(wavetableType);

        envelope.trigger();
    }
    void noteOff()
    {
        envelope.noteOff();
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
    }
    void setFrequency(float freq)
    {
        oscillator.setFrequency(freq);
    }

    double process()
    {
        if (!active)
            return 0.0;

        const double env = envelope.process();

        if (envelope.isFinished())
        {
            active = false;
            return 0.0;
        }

        return oscillator.process() * env * velocity;
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

private:
    EnvelopeADSR envelope;
    Oscillator oscillator;
    Note note;
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    OscillatorType wavetableType = OscillatorType::Sine;
    bool wavetableTypeSet = false;
    CurveType attackCurveType = CurveType::Linear;
    CurveType decayCurveType = CurveType::Linear;
    CurveType releaseCurveType = CurveType::Linear;
};
