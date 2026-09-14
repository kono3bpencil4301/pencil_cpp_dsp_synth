# 让声音活起来：从状态机到线性插值，C++ 手搓 ADSR 包络（上）

在第二期的节目当中,我们系统讲解了傅里叶级数以及加法合成，也讲解了相对应的C++实现，然而现在生成的波形还是一直维持在一个音量级别，但真正的乐器却存在音量的变化。
所以，本期节目想要讨论的问题是，一个振荡器怎样知道什么时候应该响，什么时候应该停，又该怎样让音量平滑变化？
在合成器中，我们经常通过各种调制器让原本静止的参数随时间发生变化，例如 ADSR 包络、LFO 低频振荡器以及 Macro 宏控制。而其中 ADSR 最重要的用途之一，就是描述一次 Note 从触发到结束的振幅变化过程。

在本期节目当中，我们将讲解合成器ADSR的C++实现，这次我将分为三集，第一集我们先实现ADSR状态机, Note事件, Voice和线性插值，而第二集则实现非线性插值，第三集则实现复音合成。

## 如果只有开和关，声音会怎么样？

```
if (pressed)
    gain = 1.0f;
else
    gain = 0.0f;
```

这段伪代码描述了声音是如何通过幅度突变来触发的。然而这里就会产生两个问题:

• 听感上非常生硬
• 在音频开始和结束处会产生Click音。

于是，我们希望音量不能瞬间从 A 跳到 B，而应该在一段时间内逐渐变化。

在经典的合成器架构当中，单个音符通过ADSR来管理生命周期，即

• Attack(time s)
• Decay(time s)
• Sustain(level %)
• Release(time s)

其映射的图像大致如下：

![ADSR包络](.\env_ADSR.png)

这里Sustain并不是时间，而是代表着一个振幅等级，代表着该音频在NoteOff信号触发之前要一直奏响。

## ADSR可以用有限状态机描述

在ADSR的生命周期架构设计环节当中，我将通过状态机来实现ADSR的控制。那么什么是状态机呢？

状态机是一种计算模型，它通过状态转移来实现计算。状态机由状态、输入、输出和状态转移函数组成。状态机可以根据输入来改变状态，并根据状态和输入来产生输出。状态机可以通过枚举，展示有多个状态，但每次只能处于一个状态。状态机的状态转移是确定的，即对于给定的状态和输入，状态机总是产生相同的输出和下一个状态。

因此，我们将使用状态机来实现ADSR的控制。ADSR有四个阶段，即Attack、Decay、Sustain和Release。在Attack阶段，音量从0线性增加到1，在Decay阶段，音量从1线性减少到Sustain水平，在Sustain阶段，音量保持在Sustain水平，在Release阶段，音量从用户松开按键的那一刻的音量大小水平线性减少到0。

其实现的代码如下：

```c++
enum class Stage
{
    Attack,
    Decay,
    Sustain,
    Release
};

```

不过，只定义ADSR这四个状态还不够，程序还需要知道，什么时候从一个状态进入到另一个状态：

这里就引申出两个事件：Note On 和 Note Off，分别用来模拟现实中用户按下和松开按键的行为

```c++
enum class NoteEventType
{
    NoteOn,
    NoteOff
};
```

当然，Attack状态到Decay 并不是因为用户发送了事件，而是经过上面代码当中的时间判断，自动从Attack状态进入Decay状态。这里我们先定义 NoteOn 和 NoteOff 两类事件的概念，当前离线 WAV Demo 仍然直接调用 Voice::noteOn() 和 Voice::noteOff()；真正的事件队列会在后续实时音频 / MIDI 章节中继续实现。

```C++
if (currentVolume >= 1.0f)
           {
               currentVolume = 1.0f;
               stage = Stage::Decay;
           }
```

## 状态机只负责音频走在哪，插值负责怎么走

到这里，状态机已经能够告诉程序当前应该处于 Attack 还是 Decay，但还有一个问题没有解决：如果 Attack 设置为 0.5s，计算机怎么知道这 0.5s 内每一个采样点到底应该增加多少音量？

这个时候，就开始讲插值了，简而言之，插值就是一个函数，描述两点之间的连接关系，而ADSR，在程序语言可以用状态机描述，在数学角度的描述，则是性质比较特殊的函数（因为要考虑Sustain的条件关系）。

在这里，我们设定了一个采样率fs，时间T，走过的采样点用N表示，那么三者的关系如下：
$$
N=T×fs
$$
我们假设，在音频波形a当中，Ta的时间为0.5s，采样率为44100Hz，那么走过的采样点数量为22050。这就意味着，在0.5秒内，存在22050个采样点中，从0一点点走向1.

而线性插值的通用公式如下
$$
\Delta V = \frac{V_1 - V_0}{N}
$$
这公式的意思是，在一段时间内，V1代表的是时间段结尾代表的数值，而V0则是该时间段刚开始的时候所代表的数值，而N则是该时间段的离散值。

在离散音频的ADSR插值当中，V0和V1通常代表它所在的时间点的音量（或者调制参数），而N则是时间段内，采样点的数量。因此，线性插值有点类似一次函数，呈现出**稳定斜率的数值变化**（这点和非线性插值是不同的）。

对于Attack, Decay和Release，我们有
$$
\Delta Attack = \frac{1}{T_Af_s}
$$

$$
\Delta Decay = \frac{1-V_{\text{SustainLevel}}}{T_Df_s}
$$

$$
\Delta R = -\frac{V_{\text{releaseStart}}}{T_R f_s}
$$

## process函数

在程序代码当中，我运用了process函数，对应计算Sample对应的包络音量值。`EnvelopeADSR::process()` 每调用一次，这个函数就只负责计算**一个 Sample 对应的包络值**。

也就是说，对音频调用process()函数后，该函数先**查看当前的Stage**，然后在Stage的Case当中**计算Sample的变化量**，之后就通过currentVolume += step（Attack），以及currentVolume -= step（Decay和Release）来**更新currentVolume**，之后检查是否要进入下一个Stage，最后**返回currentVolume**。

```C++
double process(){
     if (finished)
            return 0.0;    
	switch (stage) 
    {
    case Stage::Attack:
    {
        float step = (attackTime > 0.0f) ? (1.0f - sustainLevel) / (attackTime * sampleRate) : (1.0f - sustainLevel);//检测Attack时间是否为0，如果为0则不进行Attack，否则进行Attack行为，即音频音量一开始从0线性增加到1
        currentVolume += step;
        if (currentVolume >= 1.0f)
        {
            currentVolume = 1.0f;
            stage = Stage::Decay;
        }
        break;
    }
    case Stage::Decay:
    {
        float step = (decayTime > 0.0f) ? (1.0f - sustainLevel) / (decayTime * sampleRate) : (1.0f - sustainLevel);//检测Decay时间是否为0，如果为0则不进行Decay，否则进行Decay行为，即音频音量从1线性减少到Sustain水平
        currentVolume -= step;
        if (currentVolume <= sustainLevel)
        {
            currentVolume = sustainLevel;
            stage = Stage::Sustain;
        }
        break;
    }
    case Stage::Sustain:
        // 保持持续音量，直到 NoteOff 将 stage 切换为 Release
        currentVolume = sustainLevel;
        break;
    case Stage::Release:
    {
        float step = (releaseTime > 0.0f) ? releaseStartValue / (releaseTime * sampleRate) : 1.0f;//检测Release时间是否为0，如果为0则不进行Release，否则进行Release行为，即音频音量从Sustain水平线性减少到0
        currentVolume -= step;
        if (currentVolume <= 0.0f)
        {
            currentVolume = 0.0f;
            finished = true;
        }
        break;
    }
    }

    return static_cast<double>(currentVolume);
}
```

## Note功能的实现

到此，ADSR就介绍得差不多了，Envelope也已经知道声音应该怎么变化了，但在实现声音变化之前，还有一个问题：我们需要怎么让机器判断它控制的是哪个音符？

在这里，我定义了一份数据结构体，Note，包含noteNumber（映射MIDI音符的音高）和velocity（力度）等。

```c++
struct Note
{
    int noteNumber = 60;
    float velocity = 1.0f;
};
```

我们引用一个公式，根据十二平均律，A4音高的基波频率和MIDI协议，我们可以得出音符的频率f，和音符的MIDI序列数n的关系为
$$
f = 440 \times 2^{\frac{n-69}{12}}
$$
翻译成C++的语言，则是：

```C++
double getFrequency() const
{
    return 440.0 *std::pow(2.0,(noteNumber - 69) / 12.0);
}
```

于是，我们便形成了这样的一层关系：用户输入C4音符，映射到60号音符，计算出基础频率约为261.63Hz，最后由振荡器链路输出音频。

## Voice：声音要素该如何组织

讲了这么多，我们可以确信，Note 已经描述了“弹哪个音”，Envelope 也描述了“这个音怎么出现和消失”，Oscillator 则负责真正生成波形。那三者该如何组织起来呢？

这个时候，我们就将三者合成一个概念，即Voice。首先明确，Voice和Note不同，Voice是正在发声的运行实例，而Note则是数据。在当前课程的程序架构当中，我们设计了如下的关系。

```
Voice
├── Note
├── Oscillator
└── EnvelopeADSR
```

这样，在 Voice::noteOn() 函数当中，传入音符音高和力度 noteOn(60, 0.8)，首先保存 Note 数据，然后根据 MIDI 音符号计算频率并设置给 Oscillator，最后触发 envelope.trigger() 进入 Attack 阶段。

noteOff() 模拟用户松开按键的行为，它调用 envelope.noteOff()，记录当前音量作为 releaseStartValue，并将状态机无条件切换到 Release 阶段。

generate() 函数将整个音符的生命周期打包：先调用 noteOn() 开始发声，按采样率逐点生成持音时段的音频，然后调用 noteOff() 进入释放阶段，继续生成直到包络结束（isFinished() 为 true），最终返回完整的音频缓冲区。

## 结尾

到这里，我们已经实现了一条完整的线性 ADSR 包络。但如果把 Attack、Decay 和 Release 的变化曲线画出来，会发现它们全部由直线构成，。真实合成器中的包络变化，真的也是这样的吗？虽然线性插值已经能够让振幅平滑变化，但数学上的“匀速变化”，在人耳听来并不一定具有同样均匀的变化感。许多合成器也会使用指数、幂函数等不同曲线来塑造 Attack、Decay 和 Release。

那么那么，数学上的直线和我们真正听到的“自然变化”，为什么会产生差异呢？我们下一期就来讲非线性插值，这一阶段可以让用户自己设置曲线的弯曲斜率，从而让声音稍微逼真一些。
