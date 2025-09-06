#include <xaudio2.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl.h>
#include <string>
#include <stdexcept>
#include <thread>
#include <vector>
#include "../../BH.h"
#include "AudioPlayer.h"

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

using Microsoft::WRL::ComPtr;


AudioPlayer::AudioPlayer() {
    // TODO remove after testing
    PrintText(0xFFFFA500, "init audio player");
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) throw std::runtime_error("Failed to initialize COM");

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    if (FAILED(hr)) throw std::runtime_error("Failed to initialize Media Foundation");

    hr = XAudio2Create(&xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) throw std::runtime_error("Failed to create XAudio2 engine");

    hr = xaudio2->CreateMasteringVoice(&masterVoice);
    if (FAILED(hr)) throw std::runtime_error("Failed to create mastering voice");
}

AudioPlayer::~AudioPlayer() {
    if (masterVoice) masterVoice->DestroyVoice();
    if (xaudio2) xaudio2.Reset();
    MFShutdown();
    CoUninitialize();
}

void AudioPlayer::PlaySoundAsync(const std::wstring& filePath, float volume) {
    std::thread([this, filePath, volume]() {
        try {
            PlaySoundInternal(filePath, volume);
        }
        catch (const std::exception& e) {
            // TODO after testing we need to remove this print. Silently fail.
            PrintText(0xFFFFA500, const_cast<char*>(e.what()));
        }
        }).detach();
}

ComPtr<IXAudio2> xaudio2;
IXAudio2MasteringVoice* masterVoice = nullptr;

void AudioPlayer::PlaySoundInternal(const std::wstring& filePath, float volume) {
    // Initialize Media Foundation source reader
    ComPtr<IMFSourceReader> sourceReader;
    HRESULT hr = MFCreateSourceReaderFromURL(filePath.c_str(), nullptr, &sourceReader);
    if (FAILED(hr)) throw std::runtime_error("Failed to create source reader");

    // Set output format to PCM
    ComPtr<IMFMediaType> mediaType;
    hr = MFCreateMediaType(&mediaType);
    if (FAILED(hr)) throw std::runtime_error("Failed to create media type");

    hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    hr = sourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, mediaType.Get());
    if (FAILED(hr)) throw std::runtime_error("Failed to set media type");

    // Get the audio format
    ComPtr<IMFMediaType> outputType;
    hr = sourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &outputType);
    if (FAILED(hr)) throw std::runtime_error("Failed to get output media type");

    WAVEFORMATEX* waveFormat = nullptr;
    UINT32 waveFormatSize;
    hr = MFCreateWaveFormatExFromMFMediaType(outputType.Get(), &waveFormat, &waveFormatSize);
    if (FAILED(hr)) throw std::runtime_error("Failed to get wave format");

    // Create source voice
    IXAudio2SourceVoice* sourceVoice = nullptr;
    hr = xaudio2->CreateSourceVoice(&sourceVoice, waveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr);
    if (FAILED(hr)) {
        CoTaskMemFree(waveFormat);
        throw std::runtime_error("Failed to create source voice");
    }

    // Set volume (0.0f to 1.0f)
    sourceVoice->SetVolume(volume);

    std::vector<BYTE> audioData;
    while (true) {
        ComPtr<IMFSample> sample;
        DWORD flags = 0;
        hr = sourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &sample);
        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) break;
        if (sample) {
            ComPtr<IMFMediaBuffer> mediaBuffer;
            hr = sample->ConvertToContiguousBuffer(&mediaBuffer);
            if (FAILED(hr)) continue;
            BYTE* audioDataPtr = nullptr;
            DWORD dataLength = 0;
            hr = mediaBuffer->Lock(&audioDataPtr, nullptr, &dataLength);
            if (SUCCEEDED(hr)) {
                audioData.resize(audioData.size() + dataLength);
                memcpy(audioData.data() + audioData.size() - dataLength, audioDataPtr, dataLength);
                mediaBuffer->Unlock();
            }
        }
    }
    if (audioData.empty()) throw std::runtime_error("No audio data read");
    XAUDIO2_BUFFER buffer = { 0 };
    buffer.AudioBytes = static_cast<UINT32>(audioData.size());
    buffer.pAudioData = audioData.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM; // Mark end of stream
    hr = sourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr)) throw std::runtime_error("Failed to submit buffer");

    // Start playback
    sourceVoice->Start(0);
    // Wait for playback to finish
    XAUDIO2_VOICE_STATE state;
    while (true) {
        sourceVoice->GetState(&state);
        if (state.BuffersQueued == 0) break;
        Sleep(10);
    }

    // Cleanup
    sourceVoice->Stop();
    sourceVoice->FlushSourceBuffers();
    sourceVoice->DestroyVoice();
    CoTaskMemFree(waveFormat);
}
/*
class AudioPlayer {
public:
    AudioPlayer() {
        // TODO remove after testing
        PrintText(0xFFFFA500, "init audio player");
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) throw std::runtime_error("Failed to initialize COM");

        hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
        if (FAILED(hr)) throw std::runtime_error("Failed to initialize Media Foundation");

        hr = XAudio2Create(&xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr)) throw std::runtime_error("Failed to create XAudio2 engine");

        hr = xaudio2->CreateMasteringVoice(&masterVoice);
        if (FAILED(hr)) throw std::runtime_error("Failed to create mastering voice");
    }

    ~AudioPlayer() {
        if (masterVoice) masterVoice->DestroyVoice();
        if (xaudio2) xaudio2.Reset();
        MFShutdown();
        CoUninitialize();
    }

    void PlaySoundAsync(const std::wstring& filePath, float volume) {
        std::thread([this, filePath, volume]() {
            try {
                PlaySoundInternal(filePath, volume);
            }
            catch (const std::exception& e) {
                // TODO after testing we need to remove this print. Silently fail.
                PrintText(0xFFFFA500, const_cast<char*>(e.what()));
            }
            }).detach();
    }

private:
    ComPtr<IXAudio2> xaudio2;
    IXAudio2MasteringVoice* masterVoice = nullptr;

    void PlaySoundInternal(const std::wstring& filePath, float volume) {
        // Initialize Media Foundation source reader
        ComPtr<IMFSourceReader> sourceReader;
        HRESULT hr = MFCreateSourceReaderFromURL(filePath.c_str(), nullptr, &sourceReader);
        if (FAILED(hr)) throw std::runtime_error("Failed to create source reader");

        // Set output format to PCM
        ComPtr<IMFMediaType> mediaType;
        hr = MFCreateMediaType(&mediaType);
        if (FAILED(hr)) throw std::runtime_error("Failed to create media type");

        hr = mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        hr = mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        hr = sourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, mediaType.Get());
        if (FAILED(hr)) throw std::runtime_error("Failed to set media type");

        // Get the audio format
        ComPtr<IMFMediaType> outputType;
        hr = sourceReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &outputType);
        if (FAILED(hr)) throw std::runtime_error("Failed to get output media type");

        WAVEFORMATEX* waveFormat = nullptr;
        UINT32 waveFormatSize;
        hr = MFCreateWaveFormatExFromMFMediaType(outputType.Get(), &waveFormat, &waveFormatSize);
        if (FAILED(hr)) throw std::runtime_error("Failed to get wave format");

        // Create source voice
        IXAudio2SourceVoice* sourceVoice = nullptr;
        hr = xaudio2->CreateSourceVoice(&sourceVoice, waveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr);
        if (FAILED(hr)) {
            CoTaskMemFree(waveFormat);
            throw std::runtime_error("Failed to create source voice");
        }

        // Set volume (0.0f to 1.0f)
        sourceVoice->SetVolume(volume);

        std::vector<BYTE> audioData;
        while (true) {
            ComPtr<IMFSample> sample;
            DWORD flags = 0;
            hr = sourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &sample);
            if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) break;
            if (sample) {
                ComPtr<IMFMediaBuffer> mediaBuffer;
                hr = sample->ConvertToContiguousBuffer(&mediaBuffer);
                if (FAILED(hr)) continue;
                BYTE* audioDataPtr = nullptr;
                DWORD dataLength = 0;
                hr = mediaBuffer->Lock(&audioDataPtr, nullptr, &dataLength);
                if (SUCCEEDED(hr)) {
                    audioData.resize(audioData.size() + dataLength);
                    memcpy(audioData.data() + audioData.size() - dataLength, audioDataPtr, dataLength);
                    mediaBuffer->Unlock();
                }
            }
        }
        if (audioData.empty()) throw std::runtime_error("No audio data read");
        XAUDIO2_BUFFER buffer = { 0 };
        buffer.AudioBytes = static_cast<UINT32>(audioData.size());
        buffer.pAudioData = audioData.data();
        buffer.Flags = XAUDIO2_END_OF_STREAM; // Mark end of stream
        hr = sourceVoice->SubmitSourceBuffer(&buffer);
        if (FAILED(hr)) throw std::runtime_error("Failed to submit buffer");

        // Start playback
        sourceVoice->Start(0);
        // Wait for playback to finish
        XAUDIO2_VOICE_STATE state;
        while (true) {
            sourceVoice->GetState(&state);
            if (state.BuffersQueued == 0) break;
            Sleep(10);
        }

        // Cleanup
        sourceVoice->Stop();
        sourceVoice->FlushSourceBuffers();
        sourceVoice->DestroyVoice();
        CoTaskMemFree(waveFormat);
    }
};
*/
//int main() {
//    try {
//        AudioPlayer player;
//        player.PlaySoundAsync(L"C:\\BHTesting2\\Diablo II\\ProjectD2\\filters\\sounds\\AlertSound1.mp3", 1.0f); // Volume: 50%
//        Sleep(5000);
//    }
//    catch (const std::exception& e) {
//        printf("Error: %s\n", e.what());
//        return 1;
//    }
//    return 0;
//}