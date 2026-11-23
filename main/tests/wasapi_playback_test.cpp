
#define NOMINMAX
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <ksmedia.h>
#include <wrl/client.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <memory>

using Microsoft::WRL::ComPtr;

constexpr double PI = 3.14159265358979323846;

void Check(HRESULT hr, const char *message)
{
    if (FAILED(hr))
    {
        throw std::runtime_error(
            std::string(message) +
            " HRESULT=" + std::to_string(hr));
    }
}

int main()
{
    // 初始化 COM
    HRESULT hr = CoInitializeEx(
        nullptr,
        COINIT_MULTITHREADED);

    if (FAILED(hr))
    {
        std::cerr << "COM initialization failed\n";
        return 1;
    }

    try
    {
        ComPtr<IMMDeviceEnumerator> enumerator;
        ComPtr<IMMDevice> device;
        ComPtr<IAudioClient> audioClient;
        ComPtr<IAudioRenderClient> renderClient;

        // 1. 创建设备枚举器
        Check(
            CoCreateInstance(
                __uuidof(MMDeviceEnumerator),
                nullptr,
                CLSCTX_ALL,
                IID_PPV_ARGS(&enumerator)),
            "Create enumerator");

        // 2. 获取默认扬声器
        Check(
            enumerator->GetDefaultAudioEndpoint(
                eRender,
                eConsole,
                &device),
            "Get default device");

        // 3. 激活音频客户端
        Check(
            device->Activate(
                __uuidof(IAudioClient),
                CLSCTX_ALL,
                nullptr,
                reinterpret_cast<void **>(
                    audioClient.GetAddressOf())),
            "Activate audio client");

        // 4. 获取系统混音格式
        WAVEFORMATEX *rawFormat = nullptr;

        Check(
            audioClient->GetMixFormat(&rawFormat),
            "Get mix format");

        std::unique_ptr<
            WAVEFORMATEX,
            decltype(&CoTaskMemFree)>
            format(rawFormat, &CoTaskMemFree);

        // 本教程先支持 32-bit Float PCM
        bool isFloat = false;

        if (format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
        {
            isFloat = true;
        }
        else if (
            format->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
            format->cbSize >=
                sizeof(WAVEFORMATEXTENSIBLE) -
                    sizeof(WAVEFORMATEX))
        {
            auto *ext =
                reinterpret_cast<WAVEFORMATEXTENSIBLE *>(
                    format.get());

            isFloat = IsEqualGUID(
                ext->SubFormat,
                KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
        }

        if (!isFloat || format->wBitsPerSample != 32)
        {
            throw std::runtime_error(
                "This demo requires Float32 mix format");
        }

        const UINT32 channels = format->nChannels;
        const double sampleRate =
            static_cast<double>(format->nSamplesPerSec);

        if (channels == 0 || sampleRate <= 0.0)
            throw std::runtime_error("Invalid audio format");

        std::cout << "Sample rate: "
                  << sampleRate << "\n";
        std::cout << "Channels: "
                  << channels << "\n";

        // 5. 创建自动重置事件
        HANDLE audioEvent = CreateEventW(
            nullptr, FALSE, FALSE, nullptr);

        if (!audioEvent)
            throw std::runtime_error("CreateEvent failed");

        // 用 RAII 管理事件句柄
        struct HandleCloser
        {
            void operator()(void *h) const
            {
                if (h)
                    CloseHandle(h);
            }
        };

        std::unique_ptr<void, HandleCloser>
            eventGuard(audioEvent);

        // 6. 初始化共享模式事件驱动流
        Check(
            audioClient->Initialize(
                AUDCLNT_SHAREMODE_SHARED,
                AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                1000000, // 请求约 100ms 缓冲
                0,
                format.get(),
                nullptr),
            "Initialize WASAPI");

        Check(
            audioClient->SetEventHandle(audioEvent),
            "Set event handle");

        UINT32 bufferFrames = 0;
        Check(
            audioClient->GetBufferSize(&bufferFrames),
            "Get buffer size");

        Check(
            audioClient->GetService(
                IID_PPV_ARGS(&renderClient)),
            "Get render client");

        std::cout << "Buffer frames: "
                  << bufferFrames << "\n";

        double phase = 0.0;
        const double frequency = 440.0;
        const double phaseStep =
            2.0 * PI * frequency / sampleRate;

        // 7. 填充一批音频
        auto FillAudio = [&](UINT32 frames)
        {
            if (frames == 0)
                return;

            BYTE *rawBuffer = nullptr;

            Check(
                renderClient->GetBuffer(
                    frames, &rawBuffer),
                "Get audio buffer");

            auto *output =
                reinterpret_cast<float *>(rawBuffer);

            for (UINT32 i = 0; i < frames; ++i)
            {
                float sample = static_cast<float>(
                    0.15 * std::sin(phase));

                phase += phaseStep;
                if (phase >= 2.0 * PI)
                    phase -= 2.0 * PI;

                // 将单声道信号复制到所有输出声道
                for (UINT32 ch = 0; ch < channels; ++ch)
                {
                    output[i * channels + ch] = sample;
                }
            }

            Check(
                renderClient->ReleaseBuffer(frames, 0),
                "Release audio buffer");
        };

        // 8. 启动前预填充，避免初始缓冲为空
        FillAudio(bufferFrames);

        Check(
            audioClient->Start(),
            "Start playback");

        std::cout << "Playing 440Hz...\n";

        // 9. 在本线程中运行事件驱动播放循环
        ULONGLONG startTime = GetTickCount64();

        while (GetTickCount64() - startTime < 5000)
        {
            DWORD result = WaitForSingleObject(
                audioEvent, 2000);

            if (result != WAIT_OBJECT_0)
                throw std::runtime_error(
                    "Audio event wait failed or timed out");

            UINT32 padding = 0;

            Check(
                audioClient->GetCurrentPadding(&padding),
                "Get current padding");

            if (padding > bufferFrames)
                throw std::runtime_error(
                    "Invalid buffer padding");

            UINT32 availableFrames =
                bufferFrames - padding;

            FillAudio(availableFrames);
        }

        // 10. 停止播放
        Check(
            audioClient->Stop(),
            "Stop playback");

        std::cout << "Playback finished\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: "
                  << e.what() << "\n";

        CoUninitialize();
        return 1;
    }

    CoUninitialize();
    return 0;
}
