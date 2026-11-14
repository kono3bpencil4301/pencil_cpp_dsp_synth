// ============================================================================
// WAV 文件与 PCM 音频
// ============================================================================
//
// WAV 是 RIFF (Resource Interchange File Format) 容器格式的一种，
// 用于存储未压缩的 PCM (Pulse Code Modulation, 脉冲编码调制) 音频数据。
//
// WAV 文件整体结构如下：
//   [RIFF chunk]          文件头，标识这是一个 RIFF 文件
//     ├─ "RIFF"           4 字节块标识
//     ├─ chunk_size       4 字节，后续数据的总大小（不含前 8 字节）
//     ├─ "WAVE"           4 字节，RIFF 类型标识
//     ├─ [fmt  subchunk]  音频格式描述（采样率、声道数、位深等）
//     └─ [data subchunk]  实际的 PCM 采样数据
//
// 所有整数字段均使用小端序 (Little-Endian) 存储。
// ============================================================================

#include "wav_and_pcm.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>

// 低层写入工具函数：将各种类型以小端序写入输出流。
// WAV 文件要求所有整数以小端序 (Little-Endian) 存储，
// 即低位字节在前、高位字节在后。

// 写入 4 字节的 FOURCC 标识（直接按字节写入，无需考虑大小端）
void WriteFourCC(std::ostream &out, std::string_view text)
{
    if (text.size() != 4)
        throw std::invalid_argument("text must be 4 characters long");

    out.write(text.data(), 4);
}

// 写入 16 位无符号整数（小端序：低字节在前）
void WriteU16LE(std::ostream &out, uint16_t value)
{
    const char byte[2] = {static_cast<char>(value & 0xFF), static_cast<char>(value >> 8)};
    out.write(byte, 2);
}

// 写入 32 位无符号整数（小端序：低字节在前）
void WriteU32LE(std::ostream &out, uint32_t value)
{
    const char byte[4] = {static_cast<char>(value & 0xFF), static_cast<char>(value >> 8), static_cast<char>(value >> 16), static_cast<char>(value >> 24)};
    out.write(byte, 4);
}

// 将 WAV 文件头写入输出流。
// 参数：
//   out        - 输出流（已打开的二进制文件）
//   sampleRate - 采样率 (Hz)，如 44100.0
//   channels   - 声道数，如 2 表示立体声
//   frameCount - 总采样帧数
// 返回：PCM 数据区域的总字节数（data_chunk_size）
uint32_t WriteWavHeader(std::ostream &out, double sampleRate, int channels, uint32_t frameCount)
{
    // 固定使用 16-bit PCM 格式 (format_tag = 0x0001)
    constexpr uint16_t format_tag = 0x0001;
    constexpr uint16_t bits_per_sample = 16;
    constexpr uint32_t fmt_chunk_size = 16; // PCM 格式描述固定为 16 字节

    // 计算音频数据的关键参数
    const uint16_t block_align = channels * bits_per_sample / 8;                // 每个采样帧的字节数
    const uint32_t byte_per_second = static_cast<uint32_t>(sampleRate) * block_align; // 每秒数据量
    const uint32_t data_chunk_size = frameCount * block_align;                  // PCM 数据总字节数
    // RIFF chunk 大小 = "WAVE"(4) + fmt子块(8+16) + data子块头(8) + 数据
    const uint32_t riff_size = 4 + 8 + fmt_chunk_size + 8 + data_chunk_size;

    // RIFF 块头："RIFF" + 后续总大小 + "WAVE" 类型标识
    WriteFourCC(out, "RIFF");
    WriteU32LE(out, riff_size);
    WriteFourCC(out, "WAVE");

    // fmt 子块：格式描述
    WriteFourCC(out, "fmt ");
    WriteU32LE(out, fmt_chunk_size);
    WriteU16LE(out, format_tag);                // 0x0001 = PCM
    WriteU16LE(out, static_cast<uint16_t>(channels)); // 声道数
    WriteU32LE(out, static_cast<uint32_t>(sampleRate)); // 采样率
    WriteU32LE(out, byte_per_second);           // 每秒字节数
    WriteU16LE(out, block_align);               // 块对齐
    WriteU16LE(out, bits_per_sample);           // 位深

    // data 子块头：标识 + 数据大小（实际数据在后续写入）
    WriteFourCC(out, "data");
    WriteU32LE(out, data_chunk_size);

    return data_chunk_size;
}