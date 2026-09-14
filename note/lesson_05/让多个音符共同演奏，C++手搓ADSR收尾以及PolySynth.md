# 让多个音符共同演奏：C++ 手搓 ADSR 收尾以及 PolySynth

在上一期节目当中，我们把原来的线性 ADSR 改造成了可以调节曲率的非线性包络。到这里，一个 Voice 已经能够比较完整地描述一个音符从 NoteOn 到 NoteOff 的生命周期。

但是，我们的合成器仍然存在一个明显限制：

一次只能维护一个正在演奏的音符。

如果想弹奏一个最简单的 C Major 和弦，我们就不能继续让多个音符共享同一个 Oscillator 和 Envelope，因为 C4、E4、G4 不仅频率不同，它们的振荡器相位、力度以及 ADSR 状态也可能完全不同。

因此，本期我们将正式引入 Polyphony（复音），为每个正在演奏的音符准备一个独立的 Voice，再通过 PolySynth 对多个 Voice 进行统一调度和混音。

到这一期结束以后，我们的程序将第一次能够真正同时演奏多个音符，并进一步实现柱式和弦以及简单的琶音。

## 什么是Polyphony？
Polyphony（多音）是指多个声音共同演奏，即同时播放多个音符。单音的话，你就在同一时刻只能听到一个音符了。但是，通过Polyphony，你可以同时听到多个音符，从而实现更丰富的音乐效果。比如一个C Major柱式和弦，你就可以同时听到C，E和G多个音符，从而实现更丰富的音乐效果。

在这个程序当中，复音并不是让一个振荡器同时拥有三个频率，而是同时运行多个独立的 Voice。因此，我们可以通过创建多个 Voice 来实现复音。

## 为什么一个音符必须拥有独立 Voice？

我们在之前的章节中，大费周章地写Voice这一数据结构，其实是为现在这个章节铺底——就是为了实现Polyphony。

```
Voice
├── Note
├── Oscillator
└── Envelope
```

为了实现 PolySynth，我们需要同时维护多个独立的 Voice。每个 Voice 都拥有自己的 Oscillator 和 Envelope，因此不同音符能够拥有独立的频率、相位以及包络状态。当前版本中，我们预先创建若干 Voice 并注册到 PolySynth，当某个 Voice 的 Envelope 完成 Release 后，这个 Voice 会被标记为 inactive，不再产生声音。真正根据 NoteOn 动态分配、回收 Voice 的机制，会在后面的 MIDI 和 Voice Allocation 中继续实现。在后续章节中，Voice数据结构将会支持更多的功能。

```
PolySynth
│
├── Voice 0
│   ├── Note C4
│   ├── Oscillator
│   └── ADSR
│
├── Voice 1
│   ├── Note E4
│   ├── Oscillator
│   └── ADSR
│
└── Voice 2
    ├── Note G4
    ├── Oscillator
    └── ADSR    
```

我们就是通过创建多个 Voice 来实现复音的，这里每个 Voice 必须拥有自己的：

- Frequency
- Phase
- Velocity
- ADSR Stage
- ADSR 当前值
- NoteOn / NoteOff 状态

比如 C4 可能已经进入 Sustain，但 G4 才刚刚 Attack。这样，每个音符都可以独立地播放，从而实现复音。

## PolySynth：从“一个声音”升级成“声音管理器”
上述我们在理论上探讨了PolySynth的实现原理，现在我们来实现PolySynth。

```c++
class PolySynth
{
private:
    std::vector<Voice *> voices;
};
```

PolySynth 类维护一个 Voice 指针的向量，每个 Voice 都拥有自己的频率、相位、速度、ADSR 状态和 NoteOn/NoteOff 状态。这样，每个音符都可以独立地播放，从而实现复音。

之前我们是让voice拥有振荡器和包络，现在我们让PolySynth管理多个voice。PolySynth的功能就是让它拥有多个 Voice的调度器与混音器。

```c++
void addVoice(Voice *voice)
{
    voices.push_back(voice);
}
```

我们利用这个Vector来存储多个Voice，然后通过添加多个Voice来实现PolySynth的多音符演奏。

既然我们已经有了多个 Voice，那么我们就可以通过遍历这个向量来实现PolySynth的多音符演奏。

```c++
double process()
{
    double mix = 0.0;

    for (auto *voice : voices)
    {
        mix += voice->process();
    }

    return mix;
}
```

自然而然，复音过多了，很容易出现削波，我们需要对PolySynth进行离线峰值归一化处理。

现在我的教程当中，WAV 写入阶段用了峰值归一化：如果 peak 大于 1，就用 1 / peak 作为增益。以后我们讲效果器的时候，会用到这个方法。

## 让我们演奏C Major柱式和弦

在之前的节目中，我们实现了一个音符的播放，并且用Midi音符序号表示音符的高低。因此，我们可以通过创建多个 Voice 来实现C Major柱式和弦的播放。

```c++
std::vector<int> noteNums = {60, 64, 67};
std::vector<float> velocities = {0.8f, 0.7f, 0.6f};
std::vector<float> holdTimes = {2.0f, 2.0f, 2.0f};
std::vector<float> startTimes = {0.0f, 0.0f, 0.0f};

auto buffer = synth.generate(noteNums, velocities, holdTimes, startTimes);
GenerateWavFile("poly_chord.wav", buffer, sampleRate);
```

程序写入各个voice的Generate函数的参数，分别是音符序号、力度、hold时间和Start时间。

60对应C4，64对应E4，67对应G4。Hold时间，用来模拟现实操作键盘，按键的时间，而start时间，用来模拟现实操作键盘开始按键的时刻。我们直接设置Hold时间和Start时间，其实是临时过渡的，在后续章节中，我们会使用Midi文件的音符数据，来加载音符的Hold时间和Start时间。

我们还可以改startTimes，来模拟琶音的进行。比如，我们可以设置startTimes为{0.0f, 0.5f, 1.0f}，来模拟C4、E4、G4依次播放的效果。这样synth.generate函数会生成一个C4、E4、G4依次播放的音符序列。代表着C4、E4、G4依次在0.0秒、0.5秒、1.0秒开始播放。

具体涉及的代码如下：
```c++
int startFrame =static_cast<int>(startT * sampleRate);

if (frame == startFrame)
    noteOn(...);
```

这里我们顺便回顾下上期的公式，在上期节目中，我们概述了音符播放时间与采样率之间关系的公式：

$$
N=tf_s​
$$

如果琶音的间隔为0.5秒，那么C4、E4、G4依次在0.0秒、0.5秒、1.0秒开始播放，对应的frame为0，22050和44100。

当然，如果秒数乘以采样率，结果可能不是整数，比如0.125秒乘以44100采样率，结果是5512.5，显然不是整数，但是 Sample 本身是离散的，并不存在“第 5512.5 个 Sample”。因此，当时间无法恰好落在 Sample Grid 上时，我们必须把它映射到某一个整数采样点。在这里使用 round()，将时间映射到距离目标时刻最近的 Sample。

需要注意的是，多个 Voice 并不是各自拥有一条不同的全局时间轴。整个 PolySynth 仍然沿着同一个 Sample Frame 循环向前推进，只是每个 Voice 拥有不同的 startFrame、offFrame、振荡器相位以及 ADSR 状态。因此在同一个 Frame 上，C4 可能已经进入 Sustain，而 G4 可能刚刚触发 Attack。


## 我们终于做成了......了吗

```
                  PolySynth
                     │
        ┌────────────┼────────────┐
        ↓            ↓            ↓
      Voice        Voice        Voice
        │            │            │
   ┌────┼────┐  ┌────┼────┐  ┌────┼────┐
 Note Osc Env  Note Osc Env  Note Osc Env
        │            │            │
        └────────────┼────────────┘
                     ↓
                    Mix
                     ↓
               Float Buffer
                     ↓
               PCM Conversion
                     ↓
                    WAV
```


我们终于做成了PolySynth，可以播放多个音符了。第二期把多个 Oscillator 加起来形成 Timbre，第五期把多个 Voice 加起来形成 Music。但目前为止，距离一款完整的简单合成器，我们依然还有很多功能没有实现，比如滤波器，效果器，Midi文件的加载，音符的播放控制等等。这些功能将在后续章节中逐一实现。

在下一期节目当中，我们先实现简单的低通，带通和高通滤波器。同样也分为三期，第一期我们实现一阶低通/高通，第二期实现Biquad滤波，第三期我们讲ADSR接入到滤波器当中。滤波器将用于实现更复杂的音色，并且可以模拟现实中的乐器声音。