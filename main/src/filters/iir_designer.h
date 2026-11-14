#pragma once

#include "iir_biquad_filter.h"

#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

class IIRDesigner
{
public:
    enum class BiquadType
    {
        Lowpass,
        Highpass,
        Bandpass,
        Notch
    };

    static BiquadCoefficients Biquad(
        BiquadType type,
        double cutoffHz,
        double sampleRate,
        double resonanceQ)
    {
        if (!std::isfinite(sampleRate) || sampleRate <= 0.0)
            throw std::invalid_argument("Sample rate must be finite and positive");
        if (!std::isfinite(cutoffHz) || cutoffHz <= 0.0 || cutoffHz >= sampleRate / 2.0)
            throw std::invalid_argument("Cutoff must be between zero and Nyquist");
        if (!std::isfinite(resonanceQ) || resonanceQ <= 0.0)
            throw std::invalid_argument("Resonance Q must be finite and positive");

        constexpr double pi = 3.14159265358979323846;
        const double omega = 2.0 * pi * cutoffHz / sampleRate;
        const double sine = std::sin(omega);
        const double cosine = std::cos(omega);
        const double alpha = sine / (2.0 * resonanceQ);
        const double a0 = 1.0 + alpha;

        BiquadCoefficients c;
        switch (type)
        {
        case BiquadType::Lowpass:
            c = {(1.0 - cosine) / 2.0, 1.0 - cosine, (1.0 - cosine) / 2.0,
                 -2.0 * cosine, 1.0 - alpha};
            break;
        case BiquadType::Highpass:
            c = {(1.0 + cosine) / 2.0, -(1.0 + cosine), (1.0 + cosine) / 2.0,
                 -2.0 * cosine, 1.0 - alpha};
            break;
        case BiquadType::Bandpass:
            c = {alpha, 0.0, -alpha, -2.0 * cosine, 1.0 - alpha};
            break;
        case BiquadType::Notch:
            c = {1.0, -2.0 * cosine, 1.0, -2.0 * cosine, 1.0 - alpha};
            break;
        }

        c.b0 /= a0;
        c.b1 /= a0;
        c.b2 /= a0;
        c.a1 /= a0;
        c.a2 /= a0;
        if (!(1.0 + c.a1 + c.a2 > 0.0) ||
            !(1.0 - c.a1 + c.a2 > 0.0) ||
            !(1.0 - c.a2 > 0.0))
        {
            throw std::invalid_argument("Biquad parameters produce an unstable filter");
        }
        return c;
    }

    // RBJ Audio EQ Cookbook 二阶低通；resonanceQ 为无量纲 Q 值。
    static BiquadCoefficients BiquadLowpass(
        double cutoffHz, double sampleRate, double resonanceQ)
    {
        return Biquad(BiquadType::Lowpass, cutoffHz, sampleRate, resonanceQ);
    }

    // 设计一阶 IIR 低通滤波器，直流增益为 1。
    // cutoffHz 和 sampleRate 的单位为 Hz，截止频率处的增益为 -3 dB。
    // 返回的系数用于 y[n] = c[0] * x[n] + c[1] * y[n-1]。
    static std::array<double, 2> lowpassCoefficients(
        double cutoffHz,
        double sampleRate)
    {
        if (!std::isfinite(sampleRate) || sampleRate <= 0.0f)
            throw std::invalid_argument("Sample rate must be finite and positive");
        if (!std::isfinite(cutoffHz) || cutoffHz <= 0.0 ||
            cutoffHz >= sampleRate / 2.0)
            throw std::invalid_argument("Cutoff must be between zero and Nyquist");

        constexpr double pi = 3.14159265358979323846;
        const double sine = std::sin(pi * (cutoffHz / sampleRate));
        const double alpha = 2.0 * sine / (std::sqrt(1.0 + sine * sine) + sine);
        const double feedback = 1.0 - alpha;
        if (feedback >= 1.0)
            throw std::invalid_argument("Cutoff is too low for double coefficient precision");
        return {alpha, feedback};
    }

    static std::vector<float> OnePoleLowpass(float cutoffHz, float sampleRate)
    {
        const auto coefficients = lowpassCoefficients(cutoffHz, sampleRate);
        const float feedback = static_cast<float>(coefficients[1]);
        if (feedback >= 1.0f)
            throw std::invalid_argument("Cutoff is too low for float coefficient precision");

        return {static_cast<float>(coefficients[0]), feedback};
    }

    static std::vector<double> BiquadLowpass(double cutoffHz, double sampleRate)
    {
        if (!std::isfinite(sampleRate) || sampleRate <= 0.0f)
            throw std::invalid_argument("Sample rate must be finite and positive");
        if (!std::isfinite(cutoffHz) || cutoffHz <= 0.0 ||
            cutoffHz >= sampleRate / 2.0)
            throw std::invalid_argument("Cutoff must be between zero and Nyquist");

        constexpr double pi = 3.14159265358979323846;
        const double omega = 2.0 * pi * (cutoffHz / sampleRate);
        const double sine = std::sin(omega);
        const double cosine = std::cos(omega);
        const double alpha = sine / (2.0 * std::sqrt(2.0));
        const double a0 = 1.0 + alpha;
        return {
            (1.0 - cosine) / (2.0 * a0),
            (1.0 - cosine) / a0,
            (1.0 - cosine) / (2.0 * a0),
            (-2.0 * cosine) / a0,
            (1.0 - alpha) / a0};
    }
};
