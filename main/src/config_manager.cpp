#include "config_manager.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace
{
    json MakeDefaultJson()
    {
        return {
            {"poly_chord", {{"note", {{"noteNumber", {60, 64, 67}}, {"noteVelocity", {0.8, 0.7, 0.6}}, {"holdTimes", {3.0, 3.0, 3.0}}, {"startTime", {0.0, 0.0, 0.0}}}}, {"ADSR_config", {0.05, 0.3, 0.3, 1.5}}, {"wavetable", "sawtooth"}, {"curveType", "linear"}}},
            {"poly_arpeggio", {{"note", {{"noteNumber", {60, 64, 67, 72}}, {"noteVelocity", {0.8, 0.7, 0.6, 0.5}}, {"holdTimes", {2.0, 2.0, 2.0, 2.0}}, {"startTime", {0.0, 0.8, 1.6, 2.4}}}}, {"ADSR_config", {0.05, 0.3, 0.3, 1.5}}, {"wavetable", "sine"}, {"curveType", "linear"}}}};
    }

    void ValidateTrack(const TrackConfig &track)
    {
        if (track.notes.empty())
            throw std::runtime_error("Track has no notes: " + track.name);

        auto validTime = [](float value)
        {
            return std::isfinite(value) && value >= 0.0f;
        };

        if (!validTime(track.adsr.attack) ||
            !validTime(track.adsr.decay) ||
            !validTime(track.adsr.release) ||
            !std::isfinite(track.adsr.sustain) ||
            track.adsr.sustain < 0.0f || track.adsr.sustain > 1.0f)
        {
            throw std::runtime_error("Invalid ADSR in track: " + track.name);
        }

        // 输出文件名来自音轨名，禁止路径分隔符等危险字符。
        if (track.name.empty() ||
            track.name.find_first_not_of(
                "abcdefghijklmnopqrstuvwxyz"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "0123456789_-") != std::string::npos)
        {
            throw std::runtime_error("Invalid track name: " + track.name);
        }

        for (const auto &note : track.notes)
        {
            if (note.number < 0 || note.number > 127 ||
                !std::isfinite(note.velocity) ||
                note.velocity < 0.0f || note.velocity > 1.0f ||
                !validTime(note.holdTime) ||
                !validTime(note.startTime))
            {
                throw std::runtime_error("Invalid note in track: " + track.name);
            }
        }
    }

    TrackConfig ParseTrack(const std::string &name, const json &j)
    {
        TrackConfig track;
        track.name = name;
        track.wavetable = j.at("wavetable").get<std::string>();
        track.curveType = j.at("curveType").get<std::string>();

        const auto &adsr = j.at("ADSR_config");
        if (!adsr.is_array() || adsr.size() != 4)
            throw std::runtime_error("ADSR_config needs 4 values: " + name);

        track.adsr = {
            adsr.at(0).get<float>(),
            adsr.at(1).get<float>(),
            adsr.at(2).get<float>(),
            adsr.at(3).get<float>()};

        const auto &n = j.at("note");

        const auto numbers = n.at("noteNumber").get<std::vector<int>>();
        const auto velocities = n.at("noteVelocity").get<std::vector<float>>();
        const auto holds = n.at("holdTimes").get<std::vector<float>>();
        const auto starts = n.at("startTime").get<std::vector<float>>();

        if (numbers.size() != velocities.size() ||
            numbers.size() != holds.size() ||
            numbers.size() != starts.size())
        {
            throw std::runtime_error("Note array size mismatch: " + name);
        }

        track.notes.reserve(numbers.size());
        for (size_t i = 0; i < numbers.size(); ++i)
        {
            track.notes.push_back({numbers[i], velocities[i], holds[i], starts[i]});
        }

        ValidateTrack(track);
        return track;
    }
} // namespace

void EnsureDefaultConfig(const std::filesystem::path &path)
{
    if (std::filesystem::exists(path))
        return;

    if (!path.parent_path().empty())
        std::filesystem::create_directories(path.parent_path());

    std::ofstream output(path);
    if (!output)
        throw std::runtime_error("Cannot create JSON: " + path.string());

    output << MakeDefaultJson().dump(4) << '\n';
    if (!output)
        throw std::runtime_error("Cannot write JSON: " + path.string());
}

std::vector<TrackConfig> LoadTracks(const std::filesystem::path &path)
{
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("Cannot open JSON: " + path.string());

    json root;
    input >> root;

    if (!root.is_object() || root.empty())
        throw std::runtime_error("JSON root must be a nonempty object");

    std::vector<TrackConfig> tracks;
    tracks.reserve(root.size());

    for (auto it = root.begin(); it != root.end(); ++it)
    {
        tracks.push_back(ParseTrack(it.key(), it.value()));
    }

    return tracks;
}