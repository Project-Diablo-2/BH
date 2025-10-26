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
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        char errorMsg[128];
        snprintf(errorMsg, sizeof(errorMsg), "Failed to initialize COM (HRESULT: 0x%08X)", hr);
        throw std::runtime_error(errorMsg);
    }

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    if (FAILED(hr)) {
        char errorMsg[128];
        snprintf(errorMsg, sizeof(errorMsg), "Failed to initialize Media Foundation (HRESULT: 0x%08X)", hr);
        throw std::runtime_error(errorMsg);
    }

    hr = XAudio2Create(&xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        char errorMsg[128];
        snprintf(errorMsg, sizeof(errorMsg), "Failed to create XAudio2 (HRESULT: 0x%08X)", hr);
        throw std::runtime_error(errorMsg);
    }

    hr = xaudio2->CreateMasteringVoice(&masterVoice);
    if (FAILED(hr)) {
        char errorMsg[128];
        snprintf(errorMsg, sizeof(errorMsg), "Failed to create mastering voice (HRESULT: 0x%08X)", hr);
        throw std::runtime_error(errorMsg);
    }
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
            if (App.lootfilter.dropCustomSoundsPrintDebug.value) {
                PrintText(0xFFFFA500, const_cast<char*>(e.what()));
            }
        }
        }).detach();
}

ComPtr<IXAudio2> xaudio2;
IXAudio2MasteringVoice* masterVoice = nullptr;
void AudioPlayer::PlaySoundInternal(const std::wstring& filePath, float volume) {
    // Initialize Media Foundation source reader
    ComPtr<IMFSourceReader> sourceReader;
    HRESULT hr = MFCreateSourceReaderFromURL(filePath.c_str(), nullptr, &sourceReader);
    if (FAILED(hr)) {
        char errorMsg[128];
        snprintf(errorMsg, sizeof(errorMsg), "Failed to create source reader (HRESULT: 0x%08X)", hr);
        throw std::runtime_error(errorMsg);
    }

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

	// Restrict the duration of the sound to avoid long playback
	// Loot filter makers should keep sounds short anyway, but lets add this safety net.
    DWORD maxSamples = waveFormat->nSamplesPerSec * App.lootfilter.dropCustomSoundsMaxDurationSeconds.value;
    if (App.lootfilter.dropCustomSoundsPrintDebug.value) {
        PrintText(0xFFFFA500, "Max Samples: %lu at max duration seconds: %d", waveFormat->nSamplesPerSec * App.lootfilter.dropCustomSoundsMaxDurationSeconds.value, App.lootfilter.dropCustomSoundsMaxDurationSeconds.value);
    }
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
    DWORD totalSamples = 0;
    bool reachedLimit = false;

    while (!reachedLimit) {
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
                // Calculate how many samples are in this buffer
                DWORD bytesPerSample = (waveFormat->wBitsPerSample / 8) * waveFormat->nChannels;
                DWORD numSamples = bytesPerSample ? (dataLength / bytesPerSample) : 0;

                // If adding this buffer would exceed the limit, trim it
                if (totalSamples + numSamples > maxSamples) {
                    DWORD allowedSamples = maxSamples - totalSamples;
                    DWORD allowedBytes = allowedSamples * bytesPerSample;
                    if (allowedBytes > 0 && allowedBytes <= dataLength) {
                        size_t oldSize = audioData.size();
                        audioData.resize(oldSize + allowedBytes);
                        memcpy(audioData.data() + oldSize, audioDataPtr, allowedBytes);
                    }
                    reachedLimit = true;
                }
                else {
                    size_t oldSize = audioData.size();
                    audioData.resize(oldSize + dataLength);
                    memcpy(audioData.data() + oldSize, audioDataPtr, dataLength);
                    totalSamples += numSamples;
                }
                mediaBuffer->Unlock();
            }
        }
    }
    if (audioData.empty()) {
        sourceVoice->DestroyVoice();
        CoTaskMemFree(waveFormat);
        throw std::runtime_error("No audio data read");
    }

    XAUDIO2_BUFFER buffer = { 0 };
    buffer.AudioBytes = static_cast<UINT32>(audioData.size());
    buffer.pAudioData = audioData.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM; // Mark end of stream
    hr = sourceVoice->SubmitSourceBuffer(&buffer);
    if (FAILED(hr)) {
        sourceVoice->DestroyVoice();
        CoTaskMemFree(waveFormat);
        throw std::runtime_error("Failed to submit buffer");
    }

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