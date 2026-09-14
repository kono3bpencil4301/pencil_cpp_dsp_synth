#pragma once
#include "additive_synth.h"
#include <cmath>
#include <cstdint>
#include <vector>

// ============================================================================
// AdditiveWavetableFactory —— 用加法合成生成标准波形的音频缓冲区
// ============================================================================
// 根据频率、采样率和时长，生成完整的 PCM 浮点缓冲区。
// 所有波形均通过傅里叶级数（加法合成）生成，谐波数量可指定。
// 输出范围归一化到 [-1, 1]。
// ============================================================================

class AdditiveWavetableFactory
{
public:
    // 默认采样率
    static constexpr double kDefaultSampleRate = 44100.0;
    // 默认时长 (秒)
    static constexpr double kDefaultDuration = 3.0;

    // -----------------------------------------------------------------------
    // 正弦波 (Sine Wave)
    // -----------------------------------------------------------------------
    // 傅里叶级数：f(θ) = sin(θ)
    // 只有基频，无谐波。
    // 参数：
    //   frequency      - 频率 (Hz)，如 440.0
    //   amplitude      - 振幅 [0, 1]，默认 1.0
    //   initialPhaseDeg - 初相 (角度制)，默认 0
    //   sampleRate     - 采样率 (Hz)，默认 44100.0
    //   duration       - 时长 (秒)，默认 3.0
    //   harmonics      - 谐波数量，默认 1
    static std::vector<float> CreateSine(double frequency,
                                         double amplitude = 1.0,
                                         double initialPhaseDeg = 0.0,
                                         double sampleRate = kDefaultSampleRate,
                                         double duration = kDefaultDuration,
                                         uint32_t harmonics = 1)
    {
        uint32_t frameCount = static_cast<uint32_t>(duration * sampleRate);
        std::vector<float> buffer(frameCount);
        float phaseRad = static_cast<float>(initialPhaseDeg) * PI / 180.0f;
        for (uint32_t i = 0; i < frameCount; ++i)
        {
            float theta = 2.0f * PI * static_cast<float>(i) * frequency / static_cast<float>(sampleRate) + phaseRad;
            float value = 0.0f;
            for (uint32_t n = 1; n <= harmonics; ++n)
            {
                value += std::sin(n * theta);
            }
            buffer[i] = static_cast<float>(amplitude) * value / harmonics; // 归一化
        }
        return buffer;
    }

    // -----------------------------------------------------------------------
    // 方波 (Square Wave)
    // -----------------------------------------------------------------------
    // 傅里叶级数：f(θ) = (4/π) Σ sin((2k-1)θ) / (2k-1)
    // 只含奇次谐波，振幅按 1/n 衰减。
    //   = (4/π)[sin(θ) + sin(3θ)/3 + sin(5θ)/5 + ...]
    static std::vector<float> CreateSquare(double frequency,
                                           double amplitude = 1.0,
                                           double initialPhaseDeg = 0.0,
                                           double sampleRate = kDefaultSampleRate,
                                           double duration = kDefaultDuration,
                                           uint32_t harmonics = 16)
    {
        uint32_t frameCount = static_cast<uint32_t>(duration * sampleRate);
        std::vector<float> buffer(frameCount);
        float phaseRad = static_cast<float>(initialPhaseDeg) * PI / 180.0f;
        for (uint32_t i = 0; i < frameCount; ++i)
        {
            float theta = 2.0f * PI * static_cast<float>(i) * frequency / static_cast<float>(sampleRate) + phaseRad;
            float value = 0.0f;
            for (uint32_t k = 1; k <= harmonics; ++k)
            {
                uint32_t n = 2 * k - 1; // 奇次谐波：1, 3, 5, 7...

                // 抗混叠：采样定理规定可表达的最高频率为奈奎斯特频率 (sampleRate / 2)，
                // 超过的谐波会折叠回来产生混叠噪声，因此提前截断。
                float harmonicsFreq = n * frequency;
                if (harmonicsFreq >= sampleRate / 2.0)
                {
                    break;
                }
                value += std::sin(n * theta) / static_cast<float>(n);
            }
            buffer[i] = static_cast<float>(amplitude) * value * (4.0f / PI);
        }
        return buffer;
    }

    // -----------------------------------------------------------------------
    // 锯齿波 (Sawtooth Wave)
    // -----------------------------------------------------------------------
    // 傅里叶级数：f(θ) = (2/π) Σ (-1)^(n+1) × sin(nθ) / n
    // 包含所有整数次谐波，振幅按 1/n 衰减，符号交替。
    //   = (2/π)[sin(θ) - sin(2θ)/2 + sin(3θ)/3 - sin(4θ)/4 + ...]
    static std::vector<float> CreateSawtooth(double frequency,
                                             double amplitude = 1.0,
                                             double initialPhaseDeg = 0.0,
                                             double sampleRate = kDefaultSampleRate,
                                             double duration = kDefaultDuration,
                                             uint32_t harmonics = 16)
    {
        uint32_t frameCount = static_cast<uint32_t>(duration * sampleRate);
        std::vector<float> buffer(frameCount);
        float phaseRad = static_cast<float>(initialPhaseDeg) * PI / 180.0f;
        for (uint32_t i = 0; i < frameCount; ++i)
        {
            float theta = 2.0f * PI * static_cast<float>(i) * frequency / static_cast<float>(sampleRate) + phaseRad;
            float value = 0.0f;
            for (uint32_t n = 1; n <= harmonics; ++n)
            {
                // 抗混叠：采样定理规定可表达的最高频率为奈奎斯特频率 (sampleRate / 2)，
                // 超过的谐波会折叠回来产生混叠噪声，因此提前截断。
                float harmonicsFreq = n * frequency;
                if (harmonicsFreq >= sampleRate / 2.0)
                {
                    break;
                }
                // (-1)^(n+1)：n 为奇数时 +1，偶数时 -1
                float sign = (n % 2 == 1) ? 1.0f : -1.0f;
                value += sign * std::sin(n * theta) / static_cast<float>(n);
            }
            buffer[i] = static_cast<float>(amplitude) * value * (2.0f / PI);
        }
        return buffer;
    }

    // -----------------------------------------------------------------------
    // 三角波 (Triangle Wave)
    // -----------------------------------------------------------------------
    // 傅里叶级数：f(θ) = (8/π²) Σ (-1)^k × sin((2k+1)θ) / (2k+1)²
    // 只含奇次谐波，振幅按 1/n² 衰减，符号交替。
    //   = (8/π²)[sin(θ) - sin(3θ)/9 + sin(5θ)/25 - ...]
    static std::vector<float> CreateTriangle(double frequency,
                                             double amplitude = 1.0,
                                             double initialPhaseDeg = 0.0,
                                             double sampleRate = kDefaultSampleRate,
                                             double duration = kDefaultDuration,
                                             uint32_t harmonics = 16)
    {
        uint32_t frameCount = static_cast<uint32_t>(duration * sampleRate);
        std::vector<float> buffer(frameCount);
        float phaseRad = static_cast<float>(initialPhaseDeg) * PI / 180.0f;
        for (uint32_t i = 0; i < frameCount; ++i)
        {
            float theta = 2.0f * PI * static_cast<float>(i) * frequency / static_cast<float>(sampleRate) + phaseRad;
            float value = 0.0f;
            for (uint32_t k = 0; k < harmonics; ++k)
            {
                uint32_t n = 2 * k + 1; // 奇次谐波：1, 3, 5, 7...
                // 抗混叠：采样定理规定可表达的最高频率为奈奎斯特频率 (sampleRate / 2)，
                // 超过的谐波会折叠回来产生混叠噪声，因此提前截断。
                float harmonicsFreq = n * frequency;
                if (harmonicsFreq >= sampleRate / 2.0)
                {
                    break;
                }
                float sign = (k % 2 == 0) ? 1.0f : -1.0f;
                value += sign * std::sin(n * theta) / static_cast<float>(n * n);
            }
            buffer[i] = static_cast<float>(amplitude) * value * (8.0f / (PI * PI));
        }
        return buffer;
    }
};
