#pragma once

#include <cmath>

enum class CurveType
{
    Linear, // 线性
    EaseIn,
    EaseOut
};

class EnvelopeADSR
{
    static constexpr float kDefaultCurveStrength = 0.5f;

    float attackTime = 0.0f;
    float decayTime = 0.0f;
    float sustainLevel = 0.0f;
    float releaseTime = 0.0f;
    float sampleRate = 44100.0f;
    // setAccel() 保留旧接口名，参数是 [0, 1] 的归一化曲线强度。
    float attackCurveStrength = kDefaultCurveStrength;
    float decayCurveStrength = kDefaultCurveStrength;
    float releaseCurveStrength = kDefaultCurveStrength;

    CurveType attackCurveType = CurveType::Linear;
    CurveType decayCurveType = CurveType::Linear;
    CurveType releaseCurveType = CurveType::Linear;

public:
    enum class Stage
    {
        Attack,
        Decay,
        Sustain,
        Release
    };
    EnvelopeADSR() = default;

    void trigger()
    {
        stage = Stage::Attack;
        currentVolume = 0.0f;
        finished = false;
        stageSamplesRemaining = static_cast<int>(attackTime * sampleRate);
        stageSamplesTotal = stageSamplesRemaining;

        // 零时长阶段必须立即跳过，否则 velocity 为 0，包络会永远停在该阶段。
        if (stageSamplesRemaining <= 0)
            beginDecay();
    }

    void noteOn()
    {
        trigger();
    }

    void noteOff()
    {
        releaseStartValue = currentVolume;
        stage = Stage::Release;
        stageSamplesRemaining = static_cast<int>(releaseTime * sampleRate);
        stageSamplesTotal = stageSamplesRemaining;
        if (stageSamplesRemaining <= 0 || releaseStartValue <= 0.0f)
        {
            currentVolume = 0.0f;
            finished = true;
        }
    }
    void setAttackTime(float attackTime)
    {
        this->attackTime = attackTime > 0.0f ? attackTime : 0.0f;
    }
    void setDecayTime(float decayTime)
    {
        this->decayTime = decayTime > 0.0f ? decayTime : 0.0f;
    }
    void setSustainLevel(float sustainLevel)
    {
        if (sustainLevel < 0.0f)
            sustainLevel = 0.0f;
        else if (sustainLevel > 1.0f)
            sustainLevel = 1.0f;
        this->sustainLevel = sustainLevel;
    }
    void setReleaseTime(float releaseTime)
    {
        this->releaseTime = releaseTime > 0.0f ? releaseTime : 0.0f;
    }
    void setSampleRate(float sampleRate)
    {
        if (sampleRate > 0.0f)
            this->sampleRate = sampleRate;
    }
    float getReleaseTime() const
    {
        return releaseTime;
    }

    void setADSRAll(float attackTime, float decayTime, float sustainLevel, float releaseTime)
    {
        setAttackTime(attackTime);
        setDecayTime(decayTime);
        setSustainLevel(sustainLevel);
        setReleaseTime(releaseTime);
    }

    void setAttackCurveType(CurveType type) { attackCurveType = type; }
    void setDecayCurveType(CurveType type) { decayCurveType = type; }
    void setReleaseCurveType(CurveType type) { releaseCurveType = type; }
    void setCurveType(CurveType type) { attackCurveType = decayCurveType = releaseCurveType = type; }

    void setStrength(float attackStrength, float decayStrength, float releaseStrength)
    {
        attackCurveStrength = clampCurveStrength(attackStrength);
        decayCurveStrength = clampCurveStrength(decayStrength);
        releaseCurveStrength = clampCurveStrength(releaseStrength);
    }

    float getAttackStrength() const { return attackCurveStrength; }
    float getDecayStrength() const { return decayCurveStrength; }
    float getReleaseStrength() const { return releaseCurveStrength; }

    double process()
    {
        if (finished)
            return 0.0;

        switch (stage)
        {
        case Stage::Attack:
        {
            --stageSamplesRemaining;
            if (stageSamplesRemaining <= 0)
            {
                currentVolume = 1.0f;
                beginDecay();
            }
            else
            {
                currentVolume = curveProgress(attackCurveType, attackCurveStrength);
            }
            break;
        }
        case Stage::Decay:
        {
            --stageSamplesRemaining;
            if (stageSamplesRemaining <= 0)
            {
                currentVolume = sustainLevel;
                stage = Stage::Sustain;
            }
            else
            {
                const float progress = curveProgress(decayCurveType, decayCurveStrength);
                currentVolume = 1.0f - (1.0f - sustainLevel) * progress;
            }
            break;
        }
        case Stage::Sustain:
            // 保持持续音量，直到 noteOff 将 stage 切换为 Release
            currentVolume = sustainLevel;
            break;
        case Stage::Release:
        {
            --stageSamplesRemaining;
            if (stageSamplesRemaining <= 0)
            {
                currentVolume = 0.0f;
                finished = true;
            }
            else
            {
                const float progress = curveProgress(releaseCurveType, releaseCurveStrength);
                currentVolume = releaseStartValue * (1.0f - progress);
            }
            break;
        }
        }

        return static_cast<double>(currentVolume);
    }

    bool isFinished() const
    {
        return finished;
    }

    void setStage(Stage stage)
    {
        this->stage = stage;
    }
    Stage getStage() const
    {
        return stage;
    }

private:
    static float clampCurveStrength(float strength)
    {
        if (strength < 0.0f)
            return 0.0f;
        if (strength > 1.0f)
            return 1.0f;
        return strength;
    }

    float curveProgress(CurveType curveType, float strength) const
    {
        const float linearProgress = 1.0f -
                                     static_cast<float>(stageSamplesRemaining) /
                                         static_cast<float>(stageSamplesTotal);

        if (curveType == CurveType::Linear || strength <= 0.0f)
            return linearProgress;

        // strength = 0.5 -> exponent = 2；strength = 0.75 -> exponent = 4。
        if (strength >= 1.0f)
            return curveType == CurveType::EaseIn ? 0.0f : 1.0f;

        const float exponent = 1.0f / (1.0f - strength);
        if (curveType == CurveType::EaseIn)
            return std::pow(linearProgress, exponent);

        return 1.0f - std::pow(1.0f - linearProgress, exponent);
    }

    void beginDecay()
    {
        currentVolume = 1.0f;
        stage = Stage::Decay;
        stageSamplesRemaining = static_cast<int>(decayTime * sampleRate);
        stageSamplesTotal = stageSamplesRemaining;
        if (stageSamplesRemaining <= 0)
        {
            currentVolume = sustainLevel;
            stage = Stage::Sustain;
        }
    }

    Stage stage = Stage::Attack;
    int stageSamplesRemaining = 0;
    int stageSamplesTotal = 0;
    float currentVolume = 0.0f;
    float releaseStartValue = 0.0f;
    bool finished = true;
};
