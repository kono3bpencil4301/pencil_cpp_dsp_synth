#include "iir_biquad_filter.h"
#include "iir_biquad_filter.h"
#include "iir_designer.h"

#include <cmath>
#include <complex>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
    void require(bool condition)
    {
        if (!condition)
            throw std::runtime_error("Biquad test failed");
    }

    double gain(const BiquadCoefficients &c, double omega)
    {
        const auto z = std::polar(1.0, -omega);
        return std::abs((c.b0 + c.b1 * z + c.b2 * z * z) /
                        (1.0 + c.a1 * z + c.a2 * z * z));
    }

    void expectInvalid(double cutoff, double sampleRate, double q)
    {
        try
        {
            IIRDesigner::BiquadLowpass(cutoff, sampleRate, q);
        }
        catch (const std::invalid_argument &)
        {
            return;
        }
        throw std::runtime_error("Invalid biquad parameters accepted");
    }
}

int main()
{
    constexpr double pi = 3.14159265358979323846;
    constexpr double sampleRate = 48000.0;
    constexpr double cutoff = 3000.0;
    const double omega = 2.0 * pi * cutoff / sampleRate;

    for (double q : {0.5, 1.0 / std::sqrt(2.0), 3.0, 10.0})
    {
        const auto c = IIRDesigner::BiquadLowpass(cutoff, sampleRate, q);
        require(std::abs(gain(c, 0.0) - 1.0) < 1e-12);
        require(std::abs(gain(c, omega) - q) < 1e-12);
        require(gain(c, pi) < 1e-12);

        BiquadFilter whole, chunks;
        whole.setCoefficients(c);
        chunks.setCoefficients(c);
        const std::vector<float> input{1.0f, 0.0f, -0.5f, 0.2f, 0.0f};
        const auto expected = whole.process(input);
        auto actual = chunks.process(std::vector<float>{1.0f, 0.0f});
        const auto tail = chunks.process(std::vector<float>{-0.5f, 0.2f, 0.0f});
        actual.insert(actual.end(), tail.begin(), tail.end());
        require(actual == expected);
        chunks.reset();
        require(chunks.process(input) == expected);
        require(std::abs(expected[0] - c.b0) < 1e-7);
    }

    const double inf = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    for (double q : {0.0, -1.0, inf, nan})
        expectInvalid(cutoff, sampleRate, q);
    for (double f : {0.0, -1.0, sampleRate / 2.0, inf, nan})
        expectInvalid(f, sampleRate, 1.0);
    for (double rate : {0.0, -1.0, inf, nan})
        expectInvalid(cutoff, rate, 1.0);
}
