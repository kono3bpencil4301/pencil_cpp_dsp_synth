#pragma once

#include "additive_wavetable_factory.h"
#include <array>
#include <vector>
#include <stdexcept>

class WavetableBank
{
private:
    std::array<
        std::array<std::vector<float>, 128>, 4>
        tables;

public:
    WavetableBank();
    explicit WavetableBank(double sampleRate)
    {
        for (int note = 0; note < 128; ++note)
        {
            double freq = 440.0 * std::pow(2.0, (note - 69) / 12.0);

            if (freq > sampleRate / 2.0)
                continue; // Skip frequencies above Nyquist
            tables[0][note] = AdditiveWavetableFactory::createSineTable(freq, sampleRate);
            tables[1][note] = AdditiveWavetableFactory::createSquareTable(freq, sampleRate);
            tables[2][note] = AdditiveWavetableFactory::createSawtoothTable(freq, sampleRate);
            tables[3][note] = AdditiveWavetableFactory::createTriangleTable(freq, sampleRate);
        }
    }
    const std::vector<float> &get(OscillatorType type, int note) const
    {
        if (note < 0 || note >= 128)
            throw std::out_of_range("Note must be in the range [0, 127]");
        return tables.at(static_cast<std::size_t>(type)).at(note);
    }
};