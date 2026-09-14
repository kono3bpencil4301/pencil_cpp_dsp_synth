#ifndef WAV_AND_PCM_H
#define WAV_AND_PCM_H

#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>

using std::make_shared;
using std::shared_ptr;

// FOURCC (Four Character Code) 是用 4 个 ASCII 字符来标识数据块类型的编码方式，
// 在 WAV/RIFF 文件中广泛用于标识 "RIFF"、"WAVE"、"fmt "、"data" 等块。
// 这里将 FOURCC 定义为 uint32_t，4 个字符各占 1 个字节，按小端序排列。
#define FOURCC uint32_t
// 将 4 个字符组合成一个 FOURCC 值，例如 MAKE_FOURCC('R','I','F','F')
#define MAKE_FOURCC(a, b, c, d) ((FOURCC)(a | (b << 8) | (c << 16) | (d << 24)))
// 编译期生成 FOURCC 常量的模板，用法：MakeFourCC<'R','I','F','F'>::value
template <char ch0, char ch1, char ch2, char ch3>
struct MakeFourCC
{
    static const FOURCC value = MAKE_FOURCC(ch0, ch1, ch2, ch3);
};

// RIFF 文件中的基本数据块（chunk）。
// 每个 chunk 由一个 4 字节的 FOURCC 标识和一个 4 字节的大小字段组成。
struct BaseChunk
{
    FOURCC fcc;        // 块标识，如 'fmt ' 或 'data'
    uint32_t cb_size;  // 数据域的字节数（不含标识和大小字段本身）

    BaseChunk(FOURCC fourcc)
        : fcc(fourcc)
    {
        cb_size = 0;
    }
};

// PCM 音频格式描述结构，对应 WAV 文件中 "fmt " 子块的数据内容。
// 对于最常见的 PCM 格式 (format_tag = 0x0001)，该结构体恰好为 16 字节。
struct WaveFormat
{
    uint16_t wFormatTag;     // 音频格式标签，0x0001 表示 PCM（未压缩）
    uint16_t nChannels;      // 声道数：1=单声道, 2=立体声
    uint32_t nSamplesPerSec; // 采样率，如 44100 Hz（CD 音质）
    uint32_t nBytesPerSec;   // 每秒数据量 = 采样率 × 块对齐
    uint16_t nBlockAlign;    // 块对齐 = 声道数 × 每样本字节数，即一个采样帧的字节数
    uint16_t wBitsPerSample; // 每个采样的位深，如 16 bit
    uint16_t cExtensionSize; // 格式扩展区域大小，PCM 格式下为 0

    WaveFormat()
    {
        wFormatTag = 1;
        cExtensionSize = 0;
        nChannels = 0;
        nSamplesPerSec = 0;
        nBytesPerSec = 0;
        nBlockAlign = 0;
        wBitsPerSample = 0;
    }

    WaveFormat(uint16_t format_tag, uint16_t channels, uint32_t samples_per_sec, uint32_t bytes_per_sec, uint16_t block_align, uint16_t bits_per_sample)
    {
        wFormatTag = format_tag;
        nChannels = channels;
        nSamplesPerSec = samples_per_sec;
        nBytesPerSec = channels * samples_per_sec * bits_per_sample / 8;
        nBlockAlign = channels * bits_per_sample / 8;
        wBitsPerSample = bits_per_sample;
        cExtensionSize = 0;
    }
};

// WAV 文件头的完整结构，包含 RIFF 块、WAVE 标识、fmt 子块和 data 子块。
// 使用 shared_ptr 管理子块，方便在内存中组装完整的 WAV 头。
struct WaveHeader
{
    shared_ptr<BaseChunk> riff;    // RIFF 块标识
    FOURCC wave_fcc;               // "WAVE" 类型标识
    shared_ptr<BaseChunk> fmt;     // "fmt " 子块标识
    shared_ptr<WaveFormat> format; // 音频格式参数
    shared_ptr<BaseChunk> data;    // "data" 子块标识

    WaveHeader(uint16_t format_tag, uint16_t channels, uint32_t samples_per_sec, uint32_t bytes_per_sec, uint16_t block_align, uint16_t bits_per_sample)
    {
        riff = make_shared<BaseChunk>(MakeFourCC<'R', 'I', 'F', 'F'>::value);
        wave_fcc = MakeFourCC<'W', 'A', 'V', 'E'>::value;
        fmt = make_shared<BaseChunk>(MakeFourCC<'f', 'm', 't', ' '>::value);
        format = make_shared<WaveFormat>(format_tag, channels, samples_per_sec, bytes_per_sec, block_align, bits_per_sample);
        data = make_shared<BaseChunk>(MakeFourCC<'d', 'a', 't', 'a'>::value);
    }

    WaveHeader()
    {
        riff = nullptr;
        fmt = nullptr;
        data = nullptr;
        format = nullptr;
    }
};

// ---------- 工具函数声明 ----------

// 写入 4 字节的 FOURCC 标识
void WriteFourCC(std::ostream &out, std::string_view text);
// 写入 16 位无符号整数（小端序）
void WriteU16LE(std::ostream &out, uint16_t value);
// 写入 32 位无符号整数（小端序）
void WriteU32LE(std::ostream &out, uint32_t value);
// 写入 WAV 文件头，返回 PCM 数据区域的总字节数
uint32_t WriteWavHeader(std::ostream &out, double sampleRate, int channels, uint32_t frameCount);

#endif // WAV_AND_PCM_H
