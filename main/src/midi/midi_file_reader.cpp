#include "midi_file_reader.h"

#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>

namespace miniaudiodsp
{
    namespace
    {
        class ByteSpan
        {
        public:
            ByteSpan(const std::uint8_t *data, std::size_t size) : data_(data), size_(size) {}

            const std::uint8_t *begin() const { return data_; }
            const std::uint8_t *end() const { return data_ + size_; }
            std::size_t size() const { return size_; }
            const std::uint8_t &operator[](std::size_t index) const { return data_[index]; }

            ByteSpan subspan(std::size_t offset, std::size_t count) const
            {
                return ByteSpan(data_ + offset, count);
            }

        private:
            const std::uint8_t *data_;
            std::size_t size_;
        };

        class ByteCursor
        {
        public:
            explicit ByteCursor(ByteSpan bytes) : bytes_(bytes) {}

            bool empty() const { return position_ == bytes_.size(); }
            std::size_t remaining() const { return bytes_.size() - position_; }
            std::size_t position() const { return position_; }

            ByteSpan take(std::size_t count)
            {
                if (count > remaining())
                    throw std::runtime_error(
                        "Truncated MIDI file or event exceeds chunk boundary at offset " +
                        std::to_string(position_) + " (requested " +
                        std::to_string(count) + ", remaining " +
                        std::to_string(remaining()) + ")");
                auto result = bytes_.subspan(position_, count);
                position_ += count;
                return result;
            }

            std::uint8_t peek() const
            {
                if (empty())
                    throw std::runtime_error("Unexpected end of MIDI chunk");
                return bytes_[position_];
            }

            std::uint8_t readU8() { return take(1)[0]; }

            std::uint8_t readDataByte()
            {
                const auto b = readU8();
                if ((b & 0x80U) != 0)
                    throw std::runtime_error("Expected a MIDI data byte (0x00..0x7F)");
                return b;
            }

            std::uint16_t readBE16()
            {
                const auto a = readU8();
                const auto b = readU8();
                return static_cast<std::uint16_t>((static_cast<std::uint16_t>(a) << 8) | b);
            }

            std::uint32_t readBE32()
            {
                const auto a = readU8();
                const auto b = readU8();
                const auto c = readU8();
                const auto d = readU8();
                return (static_cast<std::uint32_t>(a) << 24) |
                       (static_cast<std::uint32_t>(b) << 16) |
                       (static_cast<std::uint32_t>(c) << 8) |
                       static_cast<std::uint32_t>(d);
            }

            std::string readFourCC()
            {
                const auto s = take(4);
                return std::string(s.begin(), s.end());
            }

            // Standard MIDI VLQ is limited to four bytes (up to 0x0FFFFFFF).
            std::uint32_t readVLQ()
            {
                std::uint32_t result = 0;
                for (int i = 0; i < 4; ++i)
                {
                    const auto b = readU8();
                    result = (result << 7) | (b & 0x7FU);
                    if ((b & 0x80U) == 0)
                        return result;
                }
                throw std::runtime_error("Invalid MIDI VLQ: exceeds 4 bytes");
            }

        private:
            ByteSpan bytes_;
            std::size_t position_ = 0;
        };
    }
    MidiEventType decodeType(std::uint8_t kind, std::uint8_t velocity)
    {
        switch (kind)
        {
        case 0x80:
            return MidiEventType::NoteOff;
        case 0x90:
            return velocity == 0 ? MidiEventType::NoteOff : MidiEventType::NoteOn;
        case 0xA0:
            return MidiEventType::PolyAftertouch;
        case 0xB0:
            return MidiEventType::ControlChange;
        case 0xC0:
            return MidiEventType::ProgramChange;
        case 0xD0:
            return MidiEventType::ChannelAftertouch;
        case 0xE0:
            return MidiEventType::PitchBend;
        default:
            throw std::runtime_error("Unknown MIDI event type");
        }
    }
    MidiTrack parseTrack(ByteCursor data, std::uint16_t trackIndex, MidiSequence &sequence)
    {
        // 读取Delta Time
        std::uint64_t tick = 0;
        MidiTrack track;
        std::uint8_t runningStatus = 0;

        while (!data.empty())
        {
            // 读取tick差
            const auto delta = data.readVLQ();

            if (tick > std::numeric_limits<std::uint64_t>::max() - delta)
                throw std::runtime_error("Tick value overflow");
            tick += delta;

            const auto first = data.peek();
            if (first == 0xFF)
            {
                // Meta Event
                data.readU8(); // consume 0xFF
                const auto metaType = data.readU8();
                const auto length = data.readVLQ();
                const auto payload = data.take(length);

                runningStatus = 0; // Meta events reset running status

                if (metaType == 0x51) // Set Tempo
                {
                    if (payload.size() != 3)
                        throw std::runtime_error("Invalid Set Tempo meta event length");
                    const std::uint32_t tempo = (static_cast<std::uint32_t>(payload[0]) << 16) |
                                                (static_cast<std::uint32_t>(payload[1]) << 8) |
                                                static_cast<std::uint32_t>(payload[2]);
                    if (tempo == 0)
                        throw std::runtime_error("Invalid Set Tempo: microseconds per quarter note cannot be zero");
                    sequence.tempos.push_back({tick, trackIndex, tempo});
                }
                else if (metaType == 0x2F) // End of Track
                {
                    if (length != 0 || !data.empty())
                        throw std::runtime_error("Invalid End of Track meta event");

                    track.endTick = tick;
                    return track;
                }
                continue;
            }

            // SysEx Event (0xF0 or 0xF7) or MIDI Event
            if (first == 0xF0 || first == 0xF7)
            {
                data.readU8(); // consume SysEx status byte
                const auto length = data.readVLQ();
                data.take(length); // consume SysEx data
                runningStatus = 0; // SysEx events reset running status
                continue;          // Skip SysEx events for now
            }
            else
            {
                std::uint8_t statusByte;
                if (first & 0x80)
                {
                    // New status byte
                    statusByte = data.readU8();
                    runningStatus = statusByte;
                }
                else
                {
                    // Running status
                    if (runningStatus == 0)
                        throw std::runtime_error("Running status used without a previous status byte");
                    statusByte = runningStatus;
                }

                if (statusByte < 0x80 || statusByte > 0xEF)
                    throw std::runtime_error("Unsupported MIDI channel status byte");

                const std::uint8_t kind = statusByte & 0xF0;
                const std::uint8_t channel = statusByte & 0x0F;

                std::uint8_t data1 = data.readDataByte();
                std::uint8_t data2 = 0;

                if (kind != 0xC0 && kind != 0xD0) // Program Change and Channel Aftertouch have only one data byte
                    data2 = data.readDataByte();

                MidiEventType eventType = decodeType(kind, data2);

                track.events.push_back({tick, trackIndex, eventType, channel, data1, data2});
            }
        }

        throw std::runtime_error("MIDI track is missing End of Track meta event");
    }

    MidiSequence parseBytes(ByteSpan bytes)
    {
        ByteCursor file(bytes);

        if (file.readFourCC() != "MThd")
            throw std::runtime_error("Invalid MIDI header chunk");

        const auto headerLength = file.readBE32();
        if (headerLength < 6)
            throw std::runtime_error("Invalid MIDI header length");

        MidiSequence sequence;
        sequence.format = file.readBE16();
        const auto trackCount = file.readBE16();
        sequence.ticksPerQuarter = file.readBE16();

        if (sequence.format > 1)
            throw std::runtime_error("Unsupported MIDI format");
        if ((sequence.ticksPerQuarter & 0x8000U) != 0 || sequence.ticksPerQuarter == 0)
            throw std::runtime_error("Unsupported MIDI time division");

        if (headerLength > 6)
            file.take(headerLength - 6);

        for (std::uint16_t trackIndex = 0; trackIndex < trackCount; ++trackIndex)
        {
            if (file.readFourCC() != "MTrk")
                throw std::runtime_error("Invalid MIDI track chunk");

            const auto trackLength = file.readBE32();
            sequence.tracks.push_back(
                parseTrack(ByteCursor(file.take(trackLength)), trackIndex, sequence));
        }

        if (!file.empty())
            throw std::runtime_error("Unexpected data after MIDI tracks");

        return sequence;
    }

    MidiSequence MidiFileReader::parse(std::istream &input)
    {
        const std::vector<std::uint8_t> bytes{
            std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        return parseBytes(ByteSpan(bytes.data(), bytes.size()));
    }

    MidiSequence MidiFileReader::load(const std::filesystem::path &filename)
    {
        std::ifstream input(filename, std::ios::binary);
        if (!input)
            throw std::runtime_error("Cannot open MIDI file: " + filename.string());

        const std::vector<std::uint8_t> bytes{
            std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        if (input.bad())
            throw std::runtime_error("Failed to read MIDI file: " + filename.string());

        return parseBytes(ByteSpan(bytes.data(), bytes.size()));
    }
}