#pragma once

#include <cstdint>
#include <filesystem>
#include <istream>
#include <vector>

namespace miniaudiodsp
{
    enum class MidiEventType
    {
        NoteOff,
        NoteOn,
        PolyAftertouch,
        ControlChange,
        ProgramChange,
        ChannelAftertouch,
        PitchBend
    };

    struct MidiEvent
    {
        uint64_t tick = 0;
        uint16_t trackIndex = 0;
        MidiEventType type;
        uint8_t channel = 0;
        uint8_t data1 = 0;
        uint8_t data2 = 0;
    };

    struct MidiTempoChange
    {
        std::uint64_t tick = 0;
        std::uint16_t trackIndex = 0;
        std::uint32_t microsecondsPerQuarter = 500000;
    };

    struct MidiTrack
    {
        std::vector<MidiEvent> events;
        std::uint64_t endTick = 0;
    };

    struct MidiSequence
    {
        std::uint16_t format = 0;          // SMF format 0 or 1
        std::uint16_t ticksPerQuarter = 0; // PPQ only in v0.1
        std::vector<MidiTrack> tracks;
        std::vector<MidiTempoChange> tempos; // raw tempo changes, not yet converted to seconds
    };

    class MidiFileReader
    {
    public:
        static MidiSequence load(const std::filesystem::path &filename);
        static MidiSequence parse(std::istream &input);
    };
}