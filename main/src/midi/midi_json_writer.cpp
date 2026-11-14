#include "midi_json_writer.h"
#include "config/config_manager.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace miniaudiodsp
{
    namespace
    {
        using json = nlohmann::json;

        struct OpenNote
        {
            std::uint64_t tick;
            std::uint8_t velocity;
        };

        double TickToSeconds(
            std::uint64_t targetTick,
            std::uint16_t ticksPerQuarter,
            std::vector<MidiTempoChange> tempos)
        {
            if (ticksPerQuarter == 0)
                throw std::runtime_error("MIDI ticks per quarter must be positive");

            std::sort(tempos.begin(), tempos.end(),
                      [](const auto &left, const auto &right)
                      {
                          return left.tick < right.tick;
                      });

            double seconds = 0.0;
            std::uint64_t currentTick = 0;
            std::uint32_t microsecondsPerQuarter = 500000;

            for (const auto &tempo : tempos)
            {
                if (tempo.tick > targetTick)
                    break;

                if (tempo.tick > currentTick)
                {
                    seconds += static_cast<double>(tempo.tick - currentTick) *
                               microsecondsPerQuarter /
                               (static_cast<double>(ticksPerQuarter) * 1000000.0);
                    currentTick = tempo.tick;
                }
                microsecondsPerQuarter = tempo.microsecondsPerQuarter;
            }

            seconds += static_cast<double>(targetTick - currentTick) *
                       microsecondsPerQuarter /
                       (static_cast<double>(ticksPerQuarter) * 1000000.0);
            return seconds;
        }

        json MakeNotesJson(
            const MidiSequence &sequence,
            const MidiTrack &track)
        {
            std::map<std::pair<std::uint8_t, std::uint8_t>, std::vector<OpenNote>> openNotes;
            std::vector<NoteConfig> notes;

            for (const auto &event : track.events)
            {
                const auto key = std::make_pair(event.channel, event.data1);
                if (event.type == MidiEventType::NoteOn && event.data2 != 0)
                {
                    openNotes[key].push_back({event.tick, event.data2});
                }
                else if (event.type == MidiEventType::NoteOff)
                {
                    auto it = openNotes.find(key);
                    if (it == openNotes.end() || it->second.empty())
                        continue;

                    const auto opened = it->second.front();
                    it->second.erase(it->second.begin());

                    const double start = TickToSeconds(
                        opened.tick, sequence.ticksPerQuarter, sequence.tempos);
                    const double end = TickToSeconds(
                        event.tick, sequence.ticksPerQuarter, sequence.tempos);
                    notes.push_back({
                        event.data1,
                        static_cast<float>(opened.velocity / 127.0),
                        static_cast<float>(std::max(0.0, end - start)),
                        static_cast<float>(start)});
                }
            }

            for (const auto &[key, pending] : openNotes)
            {
                for (const auto &opened : pending)
                {
                    const double start = TickToSeconds(
                        opened.tick, sequence.ticksPerQuarter, sequence.tempos);
                    const double end = TickToSeconds(
                        track.endTick, sequence.ticksPerQuarter, sequence.tempos);
                    notes.push_back({
                        key.second,
                        static_cast<float>(opened.velocity / 127.0),
                        static_cast<float>(std::max(0.0, end - start)),
                        static_cast<float>(start)});
                }
            }

            std::sort(notes.begin(), notes.end(),
                      [](const auto &left, const auto &right)
                      {
                          return left.startTime < right.startTime;
                      });

            json noteNumbers = json::array();
            json velocities = json::array();
            json holdTimes = json::array();
            json startTimes = json::array();
            for (const auto &note : notes)
            {
                noteNumbers.push_back(note.number);
                velocities.push_back(note.velocity);
                holdTimes.push_back(note.holdTime);
                startTimes.push_back(note.startTime);
            }

            return {
                {"noteNumber", noteNumbers},
                {"noteVelocity", velocities},
                {"holdTimes", holdTimes},
                {"startTime", startTimes}};
        }
    }

    void WriteMidiNotesJson(
        const MidiSequence &sequence,
        const std::filesystem::path &filename)
    {
        if (sequence.tracks.empty())
            throw std::runtime_error("MIDI sequence has no tracks");

        json root = json::object();
        for (std::size_t index = 0; index < sequence.tracks.size(); ++index)
        {
            const auto trackJson = MakeNotesJson(sequence, sequence.tracks[index]);
            if (!trackJson.at("noteNumber").empty())
                root["midi_track_" + std::to_string(index)] = trackJson;
        }

        if (root.empty())
            throw std::runtime_error("MIDI sequence contains no note events");

        if (!filename.parent_path().empty())
            std::filesystem::create_directories(filename.parent_path());

        std::ofstream output(filename);
        if (!output)
            throw std::runtime_error("Cannot create JSON: " + filename.string());

        output << root.dump(4) << '\n';
        if (!output)
            throw std::runtime_error("Cannot write JSON: " + filename.string());
    }

    void WriteDefaultSynthConfigJson(
        const MidiSequence &sequence,
        const std::filesystem::path &filename)
    {
        json root = {
            {"midiFile", ""},
            {"channels", 2},
            {"tracks", json::object()}};

        for (std::size_t index = 0; index < sequence.tracks.size(); ++index)
        {
            const auto notes = MakeNotesJson(sequence, sequence.tracks[index]);
            if (!notes.at("noteNumber").empty())
            {
                root["tracks"]["midi_track_" + std::to_string(index)] = {
                    {"ADSR_config", {0.05, 0.3, 0.3, 1.5}},
                    {"wavetable", "sine"},
                    {"curveType", "linear"},
                    {"cutoff", 2000.0},
                    {"resonance", 0.70710678},
                    {"type", "lowpass"},
                    {"voiceFilter", {
                        {"enabled", false},
                        {"amount", 2.0},
                        {"curveType", "linear"},
                        {"ADSR_config", {0.05, 0.3, 0.3, 1.5}}}}};
            }
        }

        if (!filename.parent_path().empty())
            std::filesystem::create_directories(filename.parent_path());
        std::ofstream output(filename);
        if (!output)
            throw std::runtime_error("Cannot create JSON: " + filename.string());
        output << root.dump(4) << '\n';
        if (!output)
            throw std::runtime_error("Cannot write JSON: " + filename.string());
    }
}
