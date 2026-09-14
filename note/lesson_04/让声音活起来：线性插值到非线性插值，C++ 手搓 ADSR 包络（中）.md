# 让声音活起来：线性插值到非线性插值，C++ 手搓 ADSR 包络（中）

在上一期节目当中，我们已经通过状态机和线性插值，实现了第一条完整的 ADSR 包络，而对于 Attack 来说，音量从 0 逐渐增加到 1；Decay 从 1 下降到 Sustain Level；而 Release 则从用户松开按键那一瞬间的音量下降到 0。

但是，我们把上一期视频当中生成的ADSR画出来，会发现一个非常明显的问题：

所有的阶段，都是直线线性变化的。

因为上一期对于每一个 Sample 的处理方式，本质上都是增加或者减少一个固定数值：currentVolume += step；

只要每个采样点增加的数值始终相同，那么曲线的斜率就始终相同，最后得到的自然是一条直线。但观察真正成熟的合成器（如Serum，Spire），我们发现这些合成器的包络**并不是这样线性增长的**。我们希望 Attack 刚开始增长得比较慢，随后越来越快；或者反过来，希望刚开始迅速变化，然后缓慢地接近目标值。

所以这一期，我们要解决的问题就是：

> 如何让 ADSR 不再只能沿着一条直线移动，而是能够拥有不同的弯曲程度？

## 与其直接修改音量，不如先修改“进度”

这一期，我们换种 ADSR 的实现路径，假设 Attack 一共拥有 (N) 个采样点，那么在某一个时刻，我们可以计算它已经走完了整个阶段的多少比例。类似这种的公式
$$
t=1-\frac{N_{\text{remaining}}}{N_{\text{total}}}
$$
其中total为为该阶段一共包含的Sample数量，remaining则表示还剩下多少个Sample没处理，这样，变量t就永远处于0-1之间。对应代码如下：

```C++
const float linearProgress = 
    1.0f - static_cast<float>(stageSamplesRemaining) / static_cast<float>(stageSamplesTotal);
```

到这里，我们得到的仍然只是一个普通的线性进度。如果直接使用这个进度作为 Attack 的振幅，那么最终得到的依然是一条直线。

## 让我们来掰弯它

这里我们引入高中学到的的幂函数
$$
g(t)=t^p
$$
其中 t依然代表时间进度，区间。范围是0-1，而p则控制曲线弯曲的程度。

也就是说，当p>1时，函数一开始呈现的变化较慢，然后逐渐加速，最后逼近目标值，反之一开始呈现的变化比较快，最后缓慢趋于1。

于是我们便得到了两种不同的非线性变化方式。但需要注意的是，这里的曲线函数并没有直接计算最终的音量，它改变的是一个更加基础的东西：进度。

## 让曲率也成为参数

只拥有“线性”和“非线性”两种模式还不够，我们还希望用户自己决定曲线到底应该弯多少。因此，这里再加入一个范围位于 0 到 1 之间的 `strength` 参数，并令：
$$
p=\frac{1}{1-strength}
$$
把它转换为幂函数的指数。

Strength 越大，指数越大，最终曲线弯曲的程度也越明显。
$$
const float exponent = 1.0f / (1.0f - strength);
$$
然后根据不同的曲线类型：

```c++
return std::pow(linearProgress, exponent);
```

或者：

```c++
return 1.0f -
       std::pow(1.0f - linearProgress, exponent);
```

这样，我们就不需要为“轻微弯曲”“中等弯曲”“强烈弯曲”分别写三套公式，而只需要调整一个参数，就能够达到类似Serum那样自由操控包络曲线的效果。

## Attack到Decay

对于 Attack 来说事情最简单，因为它本身就是 0\rightarrow1 的曲线

所以曲线函数输出多少，当前音量就可以直接取多少：
$$
V_A(t)=g(t)
$$
对应代码如下

```C++
currentVolume = curveProgress( attackCurveType, attackCurveStrength );
```

如果采用线性曲线，那么 Attack 就均匀地从 0 上升到 1。

如果采用开始慢、结束快的曲线，那么 Attack 会先缓慢增长，最后快速接近峰值。

反过来，使用开始快、结束慢的曲线，就能够形成另外一种 Attack 手感。

## Decay到Sustain

Decay 是从1下降到sustain值，因此我们不能直接把 (g(t)) 当作音量，而应该进行一次映射：
$$
1-(1-S)g(t)
$$
对应代码如下

```c++
const float progress = curveProgress( decayCurveType, decayCurveStrength );
```

## Sustain（或其他阶段）到Release

Release 同样不能简单假定声音一定从 Sustain Level 开始下降。

因为现实演奏过程中，用户完全可能在 Attack 还没有结束的时候就松开按键，也可能在 Decay 进行到一半的时候松手。因此，在收到 Note Off 时，我们首先记录当前音量：
$$
releaseStartValue = currentVolume;
$$
然后从这个值开始进入 Release，因此 Release 可以写成：
$$
V_{\text{releaseStart}}
[1-g(t)]
]
$$
类似如下的代码

```C++
currentVolume = releaseStartValue * (1.0f - progress);
```

## 结语

我们实际上并没有改变 ADSR 的状态机。Attack 结束之后仍然进入 Decay，Decay 结束之后仍然进入 Sustain，Note Off 依然让包络进入 Release。真正发生改变的是状态内部的数值变化方式。结构类似这样

```
Stage → Linear Progress → Curve Mapping →  Volume
```

状态机只需要回答：

> **现在声音处于ADSR的哪个阶段？**

而 Curve Function 则需要回答。

> **在这个阶段里面，参数究竟应该怎么走？**

本章节很简单，以后实现 Filter Envelope、LFO、Automation Curve，甚至视觉系统中的 Position、Scale、Opacity 等随时间变化的参数时，我们依然可以复用完全相同的思路，之后看到类似代码，也不必赘述了。
$$
\text{Time}
\rightarrow
\text{Progress}
\rightarrow
\text{Curve}
\rightarrow
\text{Parameter}
$$
本章节很简单，这也是为什么一个看起来很简单的 ADSR 包络，实际上已经开始触碰合成器中非常重要的一类设计思想：

我们控制的不只是参数本身，还有参数的变化方式

下一期节目当中，我们将对ADSR进行收尾，并且借用Voice Architecture机制，尝试让合成器演奏和弦与琶音。