#pragma once

#include "../tune_types.hpp"
#include <string>
#include <vector>

namespace tune::impl {

    Result Initialize();
    void Exit();

    void TuneThreadFunc(void *);
    void GpioThreadFunc(void *ptr);
    void PmdmntThreadFunc(void *ptr);

    bool GetStatus();
    void Play();
    void Pause();
    void Next();
    void Prev();

    float GetVolume();
    void SetVolume(float volume);

    bool GetDefaultTitlePlay();
    void SetDefaultTitlePlay(bool play);
    float GetDefaultTitleVolume();
    void SetDefaultTitleVolume(float volume);

    RepeatMode GetRepeatMode();
    void SetRepeatMode(RepeatMode mode);
    ShuffleMode GetShuffleMode();
    void SetShuffleMode(ShuffleMode mode);

    u32 GetPlaylistSize();
    u32 GetPlaylistItem(u32 index, char* buffer, size_t buffer_size);
    Result GetCurrentQueueItem(CurrentStats *out, char* buffer, size_t buffer_size);
    void ClearQueue();
    void MoveQueueItem(u32 src, u32 dst);
    void Select(u32 index);
    void Seek(u32 position);

    Result Enqueue(const char* buffer, EnqueueType type);
    Result Remove(u32 index);

    void GetTunePlayOverride(u64 id, bool *out, bool* has);
    void SetTunePlayOverride(u64 id, bool play, bool reset);
    void GetTuneVolumeOverride(u64 id, float *out, bool* has);
    void SetTuneVolumeOverride(u64 id, float volume, bool reset);
    void GetTitleVolumeOverride(u64 id, float *out, bool* has);
    void SetTitleVolumeOverride(u64 id, float volume, bool reset);
    void GetTitleMusicPathOverride(u64 id, char *out, size_t out_length, bool* has);
    void SetTitleMusicPathOverride(u64 id, const char* path, bool reset);
    bool HasOverride(u64 id);
    void ResetOverride(u64 id);
    void ResetAllOverride();

    void GetAutoPlayPath(char *out, size_t out_length);
    void SetAutoPlayPath(const char *path);
}
