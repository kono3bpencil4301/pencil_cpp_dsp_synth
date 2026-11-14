#pragma once

#include "midi_file_reader.h"

#include <filesystem>

namespace miniaudiodsp
{
    // 将 MIDI 音符事件写入独立的音符序列 JSON。
    void WriteMidiNotesJson(
        const MidiSequence &sequence,
        const std::filesystem::path &filename);

    // 为音符轨道创建可编辑的合成器参数 JSON。
    void WriteDefaultSynthConfigJson(
        const MidiSequence &sequence,
        const std::filesystem::path &filename);
}
