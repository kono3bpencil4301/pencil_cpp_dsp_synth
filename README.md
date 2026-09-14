> # MiniAudioDSP Lab
>
> 博客链接：https://kono3bpencil4301.github.io/kono3bpencil4301AudioLab.github.io

一个基于 C++ 的数字音频处理与合成器开发实验项目。

本项目不依赖任何高级音频框架，从底层理解计算机如何存储、处理并生成声音。通过逐步实现数字合成器的核心模块，探索：

- C++ 工程开发
- 数字信号处理（DSP）
- 音频工程
- 电子音乐制作
- 声音设计

---

# Project Introduction

声音是物理世界中的连续信号，而计算机只能处理离散的数据。

那么：

> 空气中的振动，究竟如何转换为计算机中的数字？
> 一个合成器，又是如何通过数学模型创造声音？

MiniAudioDSP Lab 通过 C++ 从零构建一个数字合成器系统，逐步探索声音从物理振动、模拟信号、数字采样、DSP 算法到声音输出的完整过程。

本项目已实现：

- PCM 数据处理与 WAV 文件生成
- 基于傅里叶级数的加法合成器
- 波形表（Wavetable）驱动的多种波形振荡器
- ADSR 包络（状态机 + 线性 / 非线性插值）
- Voice 架构（单音合成器单元）
- PolySynth 复音合成器（和弦与琶音）

---

# Project Motivation

作为一个电子音乐爱好者，我长期关注：

- 合成器（Synthesizer）
- 音色设计（Sound Design）
- 数字音频技术（Digital Audio）

在学习编程过程中，我逐渐发现：很多软件开发停留在应用层，而音频工程连接了数学、信号处理、编程语言、硬件接口与艺术创作。

因此，我希望通过 C++ 从底层重新理解声音，建立一个**从数学模型到声音创造之间的完整技术链路**。

---

# Core Concept

一个数字合成器本质上是：

```
数学模型 → 数字信号 → DSP 算法 → 声音输出
```

例如，正弦函数 $x(t) = A\sin(2\pi ft)$ 可以成为一个数字振荡器。

傅里叶分析可以解释：**为什么复杂声音可以由多个简单波形组成。**

ADSR 包络可以控制：**声音随时间的音量变化，让声音拥有"生命"。**

复音合成器可以管理：**多个独立 Voice 同时演奏和弦与琶音。**

因此，电子音乐中的音色，本质上也是数字信号处理的结果。

---

# Development Roadmap

## Phase 1：Digital Audio Foundation

状态：✅ Completed

目标：理解计算机如何存储声音。

内容：
- PCM（Pulse Code Modulation）
- Sample Rate / Bit Depth / Channel Layout
- WAV / RIFF Format
- Little Endian

成果：
- 手写 WAV 文件生成器
- C++ PCM 数据写入

教程：
- 《让 C++ 发出第一声：从 PCM 样本到手写 WAV 文件》

---

## Phase 2：Additive Synthesis & Digital Oscillator

状态：✅ Completed

目标：通过傅里叶级数实现加法合成器，用波形表驱动多种波形振荡器。

内容：
- Fourier Series（傅里叶级数）
- Harmonics（谐波）与 Partial（泛音分量）
- Phase Accumulator（相位累加器）
- Anti-Aliasing（抗混叠）
- Sine / Square / Sawtooth / Triangle 波形表
- AdditiveWavetableFactory 波形表工厂

成果：
- 加法合成器 `AdditiveSynth`
- 波形表工厂 `AdditiveWavetableFactory`
- 带相位累加器的 wavetable 振荡器 `Oscillator`

教程：
- 《让 C++ 学会合成声音：从傅里叶级数到第一台软件合成器》

---

## Phase 3：ADSR Envelope

状态：✅ Completed

目标：实现 ADSR 包络，让声音拥有音量随时间变化的"生命感"。

内容：
- ADSR 状态机（Attack → Decay → Sustain → Release）
- 线性插值
- 非线性插值（幂函数曲线：EaseIn / EaseOut）
- 曲线强度（Curve Strength）控制

成果：
- 完整的 ADSR 包络 `EnvelopeADSR`
- 支持线性与非线性插值，可独立配置各阶段曲线类型与强度

教程：
- 《让声音活起来：从状态机到线性插值，C++ 手搓 ADSR 包络（上）》
- 《让声音活起来：线性插值到非线性插值，C++ 手搓 ADSR 包络（中）》

---

## Phase 4：Voice Architecture & PolySynth

状态：✅ Completed

目标：构建 Voice 架构，实现多音符复音演奏（和弦与琶音）。

内容：
- Voice 单音合成器单元（Note + Oscillator + EnvelopeADSR）
- PolySynth 复音合成器（Voice 池管理）
- 和弦（Chord）：多 Voice 同时发声
- 琶音（Arpeggio）：多 Voice 错开时间依次进入
- `startTimes` 延迟触发机制

成果：
- `Voice` 类：组合振荡器 + 包络 + 音符参数
- `PolySynth` 类：管理 Voice 池，支持和弦与琶音
- 输出 `poly_chord.wav`（C 大调三和弦）与 `poly_arpeggio.wav`（C 大调琶音）

教程：
- 《让多个音符共同演奏，C++ 手搓 ADSR 收尾以及 PolySynth》

---

## Phase 5：Digital Filter

状态：📌 Planned

目标：实现数字滤波器，塑造声音的频率结构。

内容：
- Low Pass Filter
- High Pass Filter
- Band Pass Filter
- Resonance
- Filter Envelope

---

## Phase 6：MIDI System

状态：📌 Planned

目标：让合成器从实验程序变成可演奏乐器。

内容：
- MIDI Message
- Note Event / Velocity
- Pitch Bend / Controller

```
Keyboard → MIDI Event → Synth Engine → Audio Output
```

---

## Phase 7：Realtime Audio Engine

状态：📌 Planned

目标：实现实时声音生成。

内容：
- Audio Callback / Buffer
- Latency / Threading
- Audio Driver
- Real-time Processing

---

# Technology Stack

## Programming Language

- C++17

## Build System

- MSVC (cl.exe) / g++ (MSYS2)
- VS Code

## Audio Technology

- PCM
- WAV / RIFF
- DSP Algorithm
- Additive Synthesis
- ADSR Envelope

## Mathematics

- Fourier Series
- Signal Processing
- Numerical Computing

## Development Tools

- Git
- Audacity
- Python (Audio Analysis)

---

# Project Structure

```text
pencil_mini_audio_lab/
├── main/                                # 当前工作目录（最新代码）
│   ├── wav_and_pcm.h / .cpp             # PCM 数据处理与 WAV 文件写入
│   ├── additive_synth.h                 # Oscillator / Partial / AdditiveSynth
│   ├── additive_wavetable_factory.h     # 波形表工厂（Sine/Square/Saw/Triangle）
│   ├── note.h                           # Note 结构体（MIDI 音符号 → 频率）
│   ├── envelope_adsr.h                  # ADSR 包络（状态机 + 线性/非线性插值）
│   ├── voice.h                          # Voice（Note + Oscillator + Envelope）
│   ├── poly_synth.h                     # PolySynth 复音合成器
│   └── main.cpp                         # Demo：和弦 & 琶音
├── legacy/                              # 历史课程代码存档
│   ├── Lesson_01/                       # PCM & WAV
│   ├── Lesson_02/                       # 加法合成器
│   └── Lesson_03/                       # ADSR 包络（线性）
├── note/                                # 教程笔记（Markdown）
│   ├── lesson_01/                       # 第 1 期：PCM & WAV
│   ├── lesson_02/                       # 第 2 期：傅里叶级数 & 加法合成
│   ├── lesson_03/                       # 第 3 期：ADSR 状态机 & 线性插值
│   ├── lesson_04/                       # 第 4 期：非线性插值
│   └── lesson_05/                       # 第 5 期：PolySynth 复音
└── README.md
```

---

# Technical Articles

| 期数  | 标题                                                         | 核心内容                                                |
| :---: | ------------------------------------------------------------ | ------------------------------------------------------- |
|  01   | 让 C++ 发出第一声：从 PCM 样本到手写 WAV 文件                | 数字音频基础、PCM、WAV 格式、二进制文件写入、正弦波生成 |
|  02   | 让 C++ 学会合成声音：从傅里叶级数到第一台软件合成器          | Fourier Series、Harmonics、加法合成器、波形表           |
|  03   | 让声音活起来：从状态机到线性插值，C++ 手搓 ADSR 包络（上）   | ADSR 状态机、线性插值、Attack/Decay/Sustain/Release     |
|  04   | 让声音活起来：线性插值到非线性插值，C++ 手搓 ADSR 包络（中） | 幂函数曲线、EaseIn/EaseOut、Curve Strength              |
|  05   | 让多个音符共同演奏，C++ 手搓 ADSR 收尾以及 PolySynth         | Voice 架构、PolySynth、和弦、琶音                       |

---

# Current Progress

## Completed

✅ WAV Writer（手写 RIFF/WAV 文件生成）

✅ PCM Generator（PCM 采样点生成与写入）

✅ Additive Synthesis（基于傅里叶级数的加法合成器）

✅ Wavetable Oscillator（波形表驱动的振荡器，支持 4 种波形）

✅ ADSR Envelope（状态机 + 线性 / 非线性插值）

✅ Voice Architecture（单音合成器单元）

✅ PolySynth（复音合成器，和弦与琶音）

## Future

📌 Digital Filter（数字滤波器）

📌 MIDI Controller Support

📌 Real-time Synthesizer

📌 Complete Software Synthesizer

---

# Long-Term Vision

未来三年，希望逐步完成一个完整的 C++ 数字合成器框架。

最终目标不是简单复刻商业合成器，而是通过工程实践深入理解：

- 数字信号处理
- 音频软件架构
- 声音生成机制

希望通过这个项目建立：

```
Mathematics → Signal Processing → Programming → Electronic Music
```

之间的连接。

---

# License

MIT License
