#pragma once

#include <algorithm>
#include <vector>
#include <cstddef>
#include <stdexcept>

class FIRFilter
{
private:
    std::vector<float> coefficients_;
    std::vector<float> history_;

    std::size_t writeIndex_ = 0;

public:
    explicit FIRFilter(
        const std::vector<float> &coefficients)
        : coefficients_(coefficients), history_(coefficients.size(), 0.0f)
    {
        if (coefficients_.empty())
            throw std::invalid_argument("Empty FIR coefficients");
    }

    float process(float input)
    {
        history_[writeIndex_] = input;

        float output = 0.0f;
        std::size_t readIndex = writeIndex_;
        for (float coefficient : coefficients_)
        {
            output += coefficient * history_[readIndex];
            readIndex = readIndex == 0 ? history_.size() - 1 : readIndex - 1;
        }

        if (++writeIndex_ == history_.size())
            writeIndex_ = 0;

        return output;
    }

    void reset()
    {
        std::fill(history_.begin(), history_.end(), 0.0f);
        writeIndex_ = 0;
    }
};

inline std::vector<float> firFilter(
    const std::vector<float> &x,
    const std::vector<float> &c)
{
    if (c.empty())
        throw std::invalid_argument("Empty FIR coefficients");

    std::vector<float> y(x.size(), 0.0f);

    for (std::size_t i = 0; i < y.size(); ++i)
    {
        for (std::size_t j = 0;
             j < c.size() && j <= i; ++j)
        {
            y[i] += x[i - j] * c[j];
        }
    }

    return y;
}
