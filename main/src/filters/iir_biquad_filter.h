#pragma once

#include <vector>

// 分母已归一化为 1 + a1*z^-1 + a2*z^-2。
struct BiquadCoefficients
{
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;

    double a1 = 0.0;
    double a2 = 0.0;
};

class BiquadFilter
{
private:
    BiquadCoefficients coefficients;

    double x1 = 0.0;
    double x2 = 0.0;

    double y1 = 0.0;
    double y2 = 0.0;

public:
    void setCoefficients(const BiquadCoefficients &c)
    {
        coefficients = c;
    }

    double process(double input)
    {
        double output = coefficients.b0 * input +
                        coefficients.b1 * x1 +
                        coefficients.b2 * x2 -
                        coefficients.a1 * y1 -
                        coefficients.a2 * y2;

        x2 = x1;
        x1 = input;

        y2 = y1;
        y1 = output;

        return output;
    }

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
        x1 = x2 = y1 = y2 = 0.0;
    }
};
