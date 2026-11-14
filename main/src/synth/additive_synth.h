#ifndef ADDITIVE_SYNTH_H
#define ADDITIVE_SYNTH_H

#include <cmath>
#include <cstdint>
#include <vector>

class AdditiveWavetableFactory; // 前置声明，实现在 additive_wavetable_factory.h

// 波形类型枚举
enum class OscillatorType
{
    Sine,
    Square,
    Sawtooth,
    Triangle
};

namespace
{
    constexpr float PI = 3.14159265358979323846f;
}

// ============================================================================
// Oscillator —— 带相位累加器的正弦波振荡器
// ============================================================================
// process() 每调用一次，输出一个采样点，同时自动推进相位。
class Oscillator
{
private:
    float frequency = 0.0f;    // 频率 (Hz)
    float amplitude = 0.0f;    // 振幅 [0, 1]
    float initialPhase = 0.0f; // 初始相位 (rad)
    float phase = 0.0f;        // 当前累积相位 (rad)
    float sampleRate = 44100.0f;

    // 波形表相关
    std::vector<float> wavetable;
    float wavetablePos = 0.0f;

public:
    void setFrequency(float freq) { frequency = freq; }
    void setAmplitude(float amp) { amplitude = amp; }
    void setInitialPhase(float rad) { initialPhase = rad; }
    void setSampleRate(float sr) { sampleRate = sr; }

    // 重置相位（开始新一轮合成时调用）
    void reset()
    {
        phase = initialPhase;
        wavetablePos = 0.0f;
    }

    // 设置波形表（实现位于 additive_wavetable_factory.h，避免循环依赖）
    void setWavetable(OscillatorType type);

    // 输出一个采样点，并推进相位
    float process()
    {
        if (!wavetable.empty())
        {
            float output = wavetable[static_cast<int>(wavetablePos) % wavetable.size()];
            wavetablePos += static_cast<float>(wavetable.size()) * frequency / sampleRate;
            if (wavetablePos >= static_cast<float>(wavetable.size()))
                wavetablePos -= static_cast<float>(wavetable.size());
            phase += 2.0f * PI * frequency / sampleRate;
            if (phase >= 2.0f * PI)
                phase -= 2.0f * PI;
            return amplitude * output;
        }

        float value = amplitude * std::sin(phase);
        phase += 2.0f * PI * frequency / sampleRate;
        // 保持在 [0, 2π) 防止浮点精度劣化
        if (phase >= 2.0f * PI)
            phase -= 2.0f * PI;
        return value;
    }
};

// ============================================================================
// Partial —— 一个泛音分量（振幅 + 初相 + 振荡器）
// ============================================================================
class Partial
{
private:
    Oscillator oscillator;

public:
    void setFrequency(float freq) { oscillator.setFrequency(freq); }
    void setAmplitude(float amp) { oscillator.setAmplitude(amp); }
    void setInitialPhase(float rad) { oscillator.setInitialPhase(rad); }
    void setSampleRate(float sr) { oscillator.setSampleRate(sr); }
    void reset() { oscillator.reset(); }

    float process() { return oscillator.process(); }
};

// ============================================================================
// AdditiveSynth —— 加法合成器
// ============================================================================
// 用法示例：
//   AdditiveSynth synth;
//   synth.setSampleRate(44100);
//   synth.setBaseConfig(440, 0.5, 0)
//        .add(0.4, 0)
//        .add(0.2, 60);
//   // 然后逐帧调用 synth.process() 生成采样点
class AdditiveSynth
{
private:
    std::vector<Partial> partials;
    float baseFrequency = 0.0f;
    float baseAmplitude = 0.0f;
    float basePhaseDeg = 0.0f;
    float sampleRate = 44100.0f;
    int nextHarmonic = 2; // 下一个谐波序号（基频为第 1 次谐波）

public:
    static constexpr float kDefaultDuration = 3.0f;

    void setSampleRate(float sr) { sampleRate = sr; }

    // 设置基频 / 基振幅 / 基初相，并应用到所有已有 partial
    AdditiveSynth &setBaseConfig(float frequency, float amplitude, float initialPhaseDeg)
    {
        baseFrequency = frequency;
        baseAmplitude = amplitude;
        basePhaseDeg = initialPhaseDeg;
        nextHarmonic = 2; // 重置谐波计数器

        float phaseRad = initialPhaseDeg * PI / 180.0f;
        for (auto &p : partials)
        {
            p.setFrequency(frequency);
            p.setAmplitude(amplitude);
            p.setInitialPhase(phaseRad);
            p.setSampleRate(sampleRate);
        }
        return *this;
    }

    // 追加一个泛音分量（振幅, 初相角度），频率自动为基频的下一个谐波倍数
    AdditiveSynth &add(float amplitude, float initialPhaseDeg)
    {
        Partial p;
        p.setFrequency(baseFrequency * nextHarmonic++);
        p.setAmplitude(amplitude);
        p.setInitialPhase(initialPhaseDeg * PI / 180.0f);
        p.setSampleRate(sampleRate);
        partials.push_back(p);
        return *this;
    }

    // 重置所有振荡器的相位（开始生成音频前调用）
    void reset()
    {
        for (auto &p : partials)
            p.reset();
    }

    // 生成指定帧数的音频缓冲区（frameCount 为 0 时按默认时长自动计算）
    std::vector<float> generate(uint32_t frameCount = 0)
    {
        if (frameCount == 0)
            frameCount = static_cast<uint32_t>(kDefaultDuration * sampleRate);
        reset();
        std::vector<float> buffer(frameCount);
        for (uint32_t i = 0; i < frameCount; ++i)
            buffer[i] = process();
        return buffer;
    }

    // 输出当前帧的合成采样值（所有泛音叠加）
    float process()
    {
        float output = 0.0f;
        for (auto &p : partials)
            output += p.process();
        return output;
    }
};

#endif // ADDTIVE_SYNTH_H
