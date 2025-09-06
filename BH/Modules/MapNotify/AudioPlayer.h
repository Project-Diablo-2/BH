#pragma once

#include <xaudio2.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl.h>
#include <string>
#include <vector>
#include <thread>
#include <iostream>

// Global XAudio2 pointers
extern IXAudio2* g_pXAudio2;
extern IXAudio2MasteringVoice* g_pMasterVoice;

// SoundCallback class for handling XAudio2 voice callbacks
class SoundCallback : public IXAudio2VoiceCallback {
public:
    SoundCallback() : hBufferEndEvent(CreateEvent(nullptr, FALSE, FALSE, nullptr)) {}
    virtual ~SoundCallback() { if (hBufferEndEvent) CloseHandle(hBufferEndEvent); }

    // IXAudio2VoiceCallback interface methods
    void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 /*BytesRequired*/) override {}
    void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
    void STDMETHODCALLTYPE OnStreamEnd() override { SetEvent(hBufferEndEvent); }
    void STDMETHODCALLTYPE OnBufferStart(void* /*pBufferContext*/) override {}
    void STDMETHODCALLTYPE OnBufferEnd(void* /*pBufferContext*/) override { SetEvent(hBufferEndEvent); }
    void STDMETHODCALLTYPE OnLoopEnd(void* /*pBufferContext*/) override {}
    void STDMETHODCALLTYPE OnVoiceError(void* /*pBufferContext*/, HRESULT /*Error*/) override {}

    HANDLE hBufferEndEvent;
};

// AudioPlayer class for managing XAudio2 playback to play sounds
class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    void PlaySoundAsync(const std::wstring& filePath, float volume);

private:
    Microsoft::WRL::ComPtr<IXAudio2> xaudio2;
    IXAudio2MasteringVoice* masterVoice;

    void PlaySoundInternal(const std::wstring& filePath, float volume);
};