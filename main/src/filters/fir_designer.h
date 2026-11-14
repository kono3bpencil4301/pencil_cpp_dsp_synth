#pragma once

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

class FIRDesigner
{
public:
    // Hamming 窗加窗 Sinc 低通 FIR，直流增益约为 1。
    // cutoffHz 为理想低通的设计截止频率。
    // 在足够长、远离频率边界的滤波器中，
    // 该频率附近的增益通常约为 -6 dB。
    // taps 为奇数且 >= 3，具有线性相位特性。
    // 群延迟为 (taps - 1) / 2 个采样点。
    static std::vector<float> lowpass(
        float cutoffHz,
        float sampleRate,
        std::size_t taps)
    {
        if (!std::isfinite(sampleRate) || sampleRate <= 0.0f)
            throw std::invalid_argument("Sample rate must be finite and positive");
        if (!std::isfinite(cutoffHz) || cutoffHz <= 0.0f ||
            static_cast<double>(cutoffHz) >= static_cast<double>(sampleRate) / 2.0)
            throw std::invalid_argument("Cutoff must be between zero and Nyquist");
        if (taps < 3 || taps % 2 == 0)
            throw std::invalid_argument("FIR taps must be odd and at least 3");

        constexpr double pi = 3.14159265358979323846;
        const double frequency = static_cast<double>(cutoffHz) / sampleRate;
        const std::size_t center = (taps - 1) / 2;
        std::vector<double> kernel(taps);
        double sum = 0.0;

        for (std::size_t i = 0; i < taps; ++i)
        {
            const double offset = static_cast<double>(i) - static_cast<double>(center);
            const double sinc = i == center
                                    ? 2.0 * frequency
                                    : std::sin(2.0 * pi * frequency * offset) / (pi * offset);
            const double window = 0.54 - 0.46 * std::cos(
                                                    2.0 * pi * static_cast<double>(i) / static_cast<double>(taps - 1));
            kernel[i] = sinc * window;
            sum += kernel[i];
        }

        if (!std::isfinite(sum) || sum <= 0.0)
            throw std::runtime_error("Cannot normalize FIR coefficients");

        std::vector<float> coefficients(taps);
        for (std::size_t i = 0; i < taps; ++i)
            coefficients[i] = static_cast<float>(kernel[i] / sum);
        return coefficients;
    }
};
