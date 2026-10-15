#pragma once
#include "additive_synth.h"
#include <cmath>
#include <cstdint>
#include <vector>

// ============================================================================
// AdditiveWavetableFactory —— 用加法合成生成单周期波形表
// ============================================================================
// 每个 create 方法生成一个完整周期的波形表（默认 2048 个采样点）。
// 所有波形均通过傅里叶级数（加法合成）生成，谐波数量可指定。
// frequency 仅用于抗混叠判断，不影响波形表的周期数。
// ============================================================================

class AdditiveWavetableFactory
{
public:
    // 默认采样率
    static constexpr double kDefaultSampleRate = 44100.0;
    // 默认波形表大小（单周期采样点数）
    static constexpr uint32_t kDefaultTableSize = 2048;

    // -----------------------------------------------------------------------
    // 正弦波 (Sine Wave)
    // -----------------------------------------------------------------------
    // 傅里叶级数：f(θ) = sin(θ)
    // 只有基频，无谐波。
    static std::vector<float> createSineTable(
        double frequency,
        double sampleRate = kDefaultSampleRate,
        uint32_t tableSize = kDefaultTableSize,
        uint32_t harmonics = 1)
    {
        std::vector<float> table(tableSize);
        for (uint32_t i = 0; i < tableSize; ++i)
        {
            const float theta =
                2.0f * PI * static_cast<float>(i) / static_cast<float>(tableSize);

            float value = 0.0f;
            for (uint32_t n = 1; n <= harmonics; ++n)
            {
                if (n * frequency >= sampleRate / 2.0)
                    break;
                value += std::sin(n * theta);
            }
            table[i] = value / static_cast<float>(harmonics); // 归一化
        }
        return table;
    }

    // -----------------------------------------------------------------------
    // 方波 (Square Wave)
    // -----------------------------------------------------------------------
    // 傅里叶级数：f(θ) = (4/π) Σ sin((2k-1)θ) / (2k-1)
    // 只含奇次谐波，振幅按 1/n 衰减。
    //   = (4/π)[sin(θ) + sin(3θ)/3 + sin(5θ)/5 + ...]
    static std::vector<float> createSquareTable(
        double frequency,
        double sampleRate = kDefaultSampleRate,
        uint32_t tableSize = kDefaultTableSize,
        uint32_t harmonics = 16)
    {
        std::vector<float> table(tableSize);
        for (uint32_t i = 0; i < tableSize; ++i)
        {
            const float theta =
                2.0f * PI * static_cast<float>(i) / static_cast<float>(tableSize);

            float value = 0.0f;
            for (uint32_t k = 1; k <= harmonics; ++k)
            {
                uint32_t n = 2 * k - 1; // 奇次谐波：1, 3, 5, 7...

                // 抗混叠：采样定理规定可表达的最高频率为奈奎斯特频率 (sampleRate / 2)
                if (n * frequency >= sampleRate / 2.0)
                    break;

                value += std::sin(n * theta) / static_cast<float>(n);
            }
            table[i] = value * (4.0f / PI);
        }
        return table;
    }

    // -----------------------------------------------------------------------
    // 锯齿波 (Sawtooth Wave)
    // -----------------------------------------------------------------------
    // 傅里叶级数：f(θ) = (2/π) Σ (-1)^(n+1) × sin(nθ) / n
    // 包含所有整数次谐波，振幅按 1/n 衰减，符号交替。
    //   = (2/π)[sin(θ) - sin(2θ)/2 + sin(3θ)/3 - sin(4θ)/4 + ...]
    static std::vector<float> createSawtoothTable(
        double frequency,
        double sampleRate = kDefaultSampleRate,
        uint32_t tableSize = kDefaultTableSize,
        uint32_t harmonics = 16)
    {
        std::vector<float> table(tableSize);
        for (uint32_t i = 0; i < tableSize; ++i)
        {
            const float theta =
                2.0f * PI * static_cast<float>(i) / static_cast<float>(tableSize);

            float value = 0.0f;
            for (uint32_t n = 1; n <= harmonics; ++n)
            {
                // 抗混叠：frequency 仅用于判断
                if (n * frequency >= sampleRate / 2.0)
                    break;

                // (-1)^(n+1)：n 为奇数时 +1，偶数时 -1
                const float sign = (n % 2 == 1) ? 1.0f : -1.0f;
                value += sign * std::sin(n * theta) / static_cast<float>(n);
            }
            table[i] = value * (2.0f / PI);
        }
        return table;
    }

    // -----------------------------------------------------------------------
    // 三角波 (Triangle Wave)
    // -----------------------------------------------------------------------
    // 傅里叶级数：f(θ) = (8/π²) Σ (-1)^k × sin((2k+1)θ) / (2k+1)²
    // 只含奇次谐波，振幅按 1/n² 衰减，符号交替。
    //   = (8/π²)[sin(θ) - sin(3θ)/9 + sin(5θ)/25 - ...]
    static std::vector<float> createTriangleTable(
        double frequency,
        double sampleRate = kDefaultSampleRate,
        uint32_t tableSize = kDefaultTableSize,
        uint32_t harmonics = 16)
    {
        std::vector<float> table(tableSize);
        for (uint32_t i = 0; i < tableSize; ++i)
        {
            const float theta =
                2.0f * PI * static_cast<float>(i) / static_cast<float>(tableSize);

            float value = 0.0f;
            for (uint32_t k = 0; k < harmonics; ++k)
            {
                uint32_t n = 2 * k + 1; // 奇次谐波：1, 3, 5, 7...

                // 抗混叠
                if (n * frequency >= sampleRate / 2.0)
                    break;

                const float sign = (k % 2 == 0) ? 1.0f : -1.0f;
                value += sign * std::sin(n * theta) / static_cast<float>(n * n);
            }
            table[i] = value * (8.0f / (PI * PI));
        }
        return table;
    }
};

// ============================================================================
// Oscillator::setWavetable 的实现（放在此处以避免循环依赖）
// ============================================================================
inline void Oscillator::setWavetable(OscillatorType type)
{
    const double freq = (frequency > 0.0f) ? static_cast<double>(frequency) : 440.0;
    const double sr = static_cast<double>(sampleRate);
    switch (type)
    {
    case OscillatorType::Sine:
        wavetable = AdditiveWavetableFactory::createSineTable(freq, sr);
        break;
    case OscillatorType::Square:
        wavetable = AdditiveWavetableFactory::createSquareTable(freq, sr);
        break;
    case OscillatorType::Sawtooth:
        wavetable = AdditiveWavetableFactory::createSawtoothTable(freq, sr);
        break;
    case OscillatorType::Triangle:
        wavetable = AdditiveWavetableFactory::createTriangleTable(freq, sr);
        break;
    }
    wavetablePos = 0.0f;
}
