#pragma once

#include "iir_designer.h"

#include <vector>

class OnePoleFilter
{
private:
    double sampleRate;
    double cutoffFrequency;
    double alpha;
    double feedback;
    double prevOutput = 0.0;

    void setParameters(double rate, double frequency)
    {
        // 先验证并计算，再更新状态，保证无效参数不会破坏现有配置。
        const auto coefficients = IIRDesigner::lowpassCoefficients(frequency, rate);
        sampleRate = rate;
        cutoffFrequency = frequency;
        alpha = coefficients[0];
        feedback = coefficients[1];
    }

public:
    explicit OnePoleFilter(double rate = 44100.0, double frequency = 1000.0)
    {
        setParameters(rate, frequency);
    }

    void setSampleRate(double rate)
    {
        setParameters(rate, cutoffFrequency);
    }

    void setCutoffFrequency(double frequency)
    {
        setParameters(sampleRate, frequency);
    }

    double process(double input)
    {
        prevOutput = alpha * input + feedback * prevOutput;
        return prevOutput;
    }

    // 每次调用都延续之前的滤波状态；需要重新开始时调用 reset()。
    std::vector<float> process(const std::vector<float> &input)
    {
        std::vector<float> output;
        output.reserve(input.size());
        for (float sample : input)
            output.push_back(static_cast<float>(process(static_cast<double>(sample))));
        return output;
    }

    void reset()
    {
        prevOutput = 0.0;
    }
};
