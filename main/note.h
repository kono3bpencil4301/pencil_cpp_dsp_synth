#pragma once

#include <cmath>

struct Note
{
    int noteNumber = 60;
    float velocity = 1.0f;
    double getFrequency() const
    {
        return 440.0 * std::pow(2.0, (noteNumber - 69) / 12.0);
    }
};

enum class NoteEventType
{
    NoteOn,
    NoteOff
};

struct NoteEvent
{
    Note note;
    NoteEventType type;
};