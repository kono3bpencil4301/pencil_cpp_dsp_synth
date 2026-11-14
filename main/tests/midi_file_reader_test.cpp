#include "midi/midi_file_reader.h"
#include "midi/midi_json_writer.h"
#include "config/config_manager.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
    void appendBE16(std::string &bytes, std::uint16_t value)
    {
        bytes.push_back(static_cast<char>(value >> 8));
        bytes.push_back(static_cast<char>(value));
    }

    void appendBE32(std::string &bytes, std::uint32_t value)
    {
        bytes.push_back(static_cast<char>(value >> 24));
        bytes.push_back(static_cast<char>(value >> 16));
        bytes.push_back(static_cast<char>(value >> 8));
        bytes.push_back(static_cast<char>(value));
    }

    std::string makeMidi()
    {
        const std::string track{
            '\x00', '\xFF', '\x51', '\x03', '\x07', '\xA1', '\x20',
            '\x00', '\x90', '\x3C', '\x64',
            '\x81', '\x00', '\x80', '\x3C', '\x00',
            '\x00', '\x90', '\x40', '\x00',
            '\x00', '\xFF', '\x2F', '\x00'};

        std::string midi{"MThd"};
        appendBE32(midi, 6);
        appendBE16(midi, 0);
        appendBE16(midi, 1);
        appendBE16(midi, 96);
        midi += "MTrk";
        appendBE32(midi, static_cast<std::uint32_t>(track.size()));
        midi += track;
        return midi;
    }
}

int main()
{
    std::istringstream input(makeMidi());
    const auto sequence = miniaudiodsp::MidiFileReader::parse(input);

    assert(sequence.tempos.size() == 1);
    assert(sequence.format == 0);
    assert(sequence.ticksPerQuarter == 96);
    assert(sequence.tracks.size() == 1);
    assert(sequence.tracks[0].events.size() == 3);
    assert(sequence.tracks[0].events[0].type == miniaudiodsp::MidiEventType::NoteOn);
    assert(sequence.tracks[0].events[1].type == miniaudiodsp::MidiEventType::NoteOff);
    assert(sequence.tracks[0].events[2].type == miniaudiodsp::MidiEventType::NoteOff);
    assert(sequence.tracks[0].events[1].tick == 128);
    assert(sequence.tracks[0].endTick == 128);

    const auto tempPath =
        std::filesystem::temp_directory_path() / "miniaudiodsp_midi_file_reader_test.mid";
    {
        std::ofstream output(tempPath, std::ios::binary);
        output << makeMidi();
    }
    const auto loaded = miniaudiodsp::MidiFileReader::load(tempPath);
    std::filesystem::remove(tempPath);
    assert(loaded.tracks.size() == 1);
    assert(loaded.tracks[0].endTick == 128);

    const auto jsonPath =
        std::filesystem::temp_directory_path() / "miniaudiodsp_midi_tracks_test.json";
    miniaudiodsp::WriteMidiNotesJson(loaded, jsonPath);
    nlohmann::json json;
    {
        std::ifstream jsonInput(jsonPath);
        json = nlohmann::json::parse(jsonInput);
    }
    std::filesystem::remove(jsonPath);
    assert(json.at("midi_track_0").at("noteNumber").size() == 1);
    assert(json.at("midi_track_0").at("noteNumber")[0] == 60);
    assert(json.at("midi_track_0").at("holdTimes")[0].get<float>() > 0.0f);

    bool rejectedMissingEnd = false;
    try
    {
        std::string invalid = makeMidi();
        invalid.resize(invalid.size() - 4);
        std::istringstream invalidInput(invalid);
        miniaudiodsp::MidiFileReader::parse(invalidInput);
    }
    catch (const std::runtime_error &)
    {
        rejectedMissingEnd = true;
    }
    assert(rejectedMissingEnd);
}
