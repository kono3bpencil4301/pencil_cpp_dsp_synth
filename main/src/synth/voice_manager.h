#include "voice.h"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

class voice_manager
{
private:
    std::vector<Voice> voices;

public:
    explicit voice_manager(int numVoices = 8)
    {
        initVoices(numVoices);
    }

    void initVoices(int numVoices)
    {
        if (numVoices < 0)
            throw std::invalid_argument("numVoices must not be negative");

        voices.assign(static_cast<std::size_t>(numVoices), Voice{});
    }

    void setSampleRate(float sampleRate)
    {
        if (sampleRate <= 0.0f)
            throw std::invalid_argument("sampleRate must be positive");

        for (auto &voice : voices)
            voice.setSampleRate(sampleRate);
    }

    std::size_t getVoiceCount() const
    {
        return voices.size();
    }

    bool noteOn(int note, float velocity, uint8_t channel)
    {
        for (auto &voice : voices)
        {
            if (!voice.isActive())
            {
                voice.noteOn(note, velocity, channel);
                return true;
            }
        }

        return false;
    }

    void noteOff(int note, uint8_t channel)
    {
        for (auto &voice : voices)
        {
            if (voice.matchesNote(note, channel))
            {
                voice.noteOff();
            }
        }
    }

    double process()
    {
        double output = 0.0;
        for (auto &voice : voices)
            output += voice.process();

        return output;
    }
};
