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

// 将 PCM 正弦波音频数据写入 WAV 文件。
// 参数：
//   fileName   - 输出文件路径
//   frequency  - 正弦波频率 (Hz)，如 440.0 表示标准音 A4
//   duration   - 持续时间 (秒)
//   sampleRate - 采样率 (Hz)，如 44100.0
//   channels   - 声道数，如 2 表示立体声
//   amplitude  - 振幅，范围 [0.0, 1.0]
void WriteSineWave(const char *fileName, double frequency, double duration, double sampleRate, int channels, double amplitude)
{
    if (frequency <= 0 || duration <= 0 || sampleRate <= 0 || channels <= 0 || amplitude <= 0)
        throw std::invalid_argument("invalid parameter");

    // 固定使用 16-bit PCM 格式 (format_tag = 0x0001)
    constexpr uint16_t format_tag = 0x0001;
    constexpr uint16_t bits_per_sample = 16;
    constexpr uint32_t fmt_chunk_size = 16; // PCM 格式描述固定为 16 字节
    constexpr double PI = 3.14159265358979323846;

    // 计算音频数据的关键参数
    const uint16_t block_align = channels * bits_per_sample / 8;         // 每个采样帧的字节数
    const uint32_t byte_per_second = sampleRate * block_align;           // 每秒数据量
    const uint32_t frame_count = static_cast<uint32_t>(duration * sampleRate); // 总采样帧数
    const uint32_t data_chunk_size = frame_count * block_align;          // PCM 数据总字节数
    // RIFF chunk 大小 = "WAVE"(4) + fmt子块(8+16) + data子块头(8) + 数据
    const uint32_t riff_size = 4 + 8 + fmt_chunk_size + 8 + data_chunk_size;

    // ---- 写入 WAV 文件头 ----
    std::ofstream out(fileName, std::ios::binary);
    if (!out)
        throw std::runtime_error("failed to open file");

    // RIFF 块头："RIFF" + 后续总大小 + "WAVE" 类型标识
    WriteFourCC(out, "RIFF");
    WriteU32LE(out, riff_size);
    WriteFourCC(out, "WAVE");

    // fmt 子块：格式描述
    WriteFourCC(out, "fmt ");
    WriteU32LE(out, fmt_chunk_size);
    WriteU16LE(out, format_tag);      // 0x0001 = PCM
    WriteU16LE(out, channels);        // 声道数
    WriteU32LE(out, sampleRate);      // 采样率
    WriteU32LE(out, byte_per_second); // 每秒字节数
    WriteU16LE(out, block_align);     // 块对齐
    WriteU16LE(out, bits_per_sample); // 位深

    // data 子块头：标识 + 数据大小（实际数据在后面写入）
    WriteFourCC(out, "data");
    WriteU32LE(out, data_chunk_size);

    // ---- 生成并写入正弦波 PCM 采样数据 ----
    // 使用相位累加器生成正弦波，避免对每帧都调用 sin() 时参数过大导致精度下降
    double phase = 0.0;
    // 每帧的相位增量 = 2π × 频率 / 采样率
    const double phase_step = 2.0 * PI * frequency / sampleRate;
    for (uint32_t i = 0; i < frame_count; ++i)
    {
        // 计算当前帧的正弦波幅值（浮点，范围 [-amplitude, +amplitude]）
        double value = amplitude * std::sin(phase);

        // 将浮点幅值映射到 16-bit 有符号整数范围 [-32768, 32767]
        const long integer_value = std::lround(value * 32767.0);
        const auto sample = static_cast<int16_t>(
            std::clamp<long>(integer_value, -32768, 32767));

        // 同一帧的采样数据写入所有声道（即各声道数据相同，单声道信号）
        for (uint16_t channel = 0; channel < channels; ++channel)
        {
            WriteU16LE(out, static_cast<uint16_t>(sample));
        }

        // 累加相位，保持在 [0, 2π) 范围内防止浮点精度随时间劣化
        phase += phase_step;
        if (phase > 2.0 * PI)
            phase -= 2.0 * PI;
    }
    if (!out)
        throw std::runtime_error("failed to write file");
}

// 程序入口：生成一个 440Hz（标准音 A4）、3 秒、立体声、50% 振幅的正弦波 WAV 文件
int main()
{
    try
    {
        WriteSineWave("sine_wave.wav", 440.0, 3.0, 44100.0, 2, 0.5);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
