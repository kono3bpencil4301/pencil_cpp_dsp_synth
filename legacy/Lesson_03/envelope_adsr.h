#pragma once

class EnvelopeADSR
{
    float attackTime = 0.0f;
    float decayTime = 0.0f;
    float sustainLevel = 0.0f;
    float releaseTime = 0.0f;
    float sampleRate = 44100.0f;

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
    }

    void noteOn()
    {
        stage = Stage::Attack;
    }

    void noteOff()
    {
        releaseStartValue = currentVolume;
        stage = Stage::Release;
    }
    void setAttackTime(float attackTime)
    {
        this->attackTime = attackTime;
    }
    void setDecayTime(float decayTime)
    {
        this->decayTime = decayTime;
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
        this->releaseTime = releaseTime;
    }
    void setSampleRate(float sampleRate)
    {
        this->sampleRate = sampleRate;
    }
    float getReleaseTime() const
    {
        return releaseTime;
    }

    void setADSRAll(float attackTime, float decayTime, float sustainLevel, float releaseTime)
    {
        this->attackTime = attackTime;
        this->decayTime = decayTime;
        this->sustainLevel = sustainLevel;
        this->releaseTime = releaseTime;
    }

    double process()
    {
        if (finished)
            return 0.0;

        switch (stage)
        {
        case Stage::Attack:
        {
            float step = (attackTime > 0.0f) ? (1.0f - sustainLevel) / (attackTime * sampleRate) : (1.0f - sustainLevel);
            currentVolume += step;
            if (currentVolume >= 1.0f)
            {
                currentVolume = 1.0f;
                stage = Stage::Decay;
            }
            break;
        }
        case Stage::Decay:
        {
            float step = (decayTime > 0.0f) ? (1.0f - sustainLevel) / (decayTime * sampleRate) : (1.0f - sustainLevel);
            currentVolume -= step;
            if (currentVolume <= sustainLevel)
            {
                currentVolume = sustainLevel;
                stage = Stage::Sustain;
            }
            break;
        }
        case Stage::Sustain:
            // 保持持续音量，直到 noteOff 将 stage 切换为 Release
            currentVolume = sustainLevel;
            break;
        case Stage::Release:
        {
            float step = (releaseTime > 0.0f) ? releaseStartValue / (releaseTime * sampleRate) : 1.0f;
            currentVolume -= step;
            if (currentVolume <= 0.0f)
            {
                currentVolume = 0.0f;
                finished = true;
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
    Stage stage = Stage::Attack;
    float currentVolume = 0.0f;
    float releaseStartValue = 0.0f;
    bool finished = true;
};
