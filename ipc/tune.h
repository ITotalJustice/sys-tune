#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <switch.h>

enum { TuneModule = 420 };

typedef enum {
    TuneResult_InvalidArgument  = MAKERESULT(TuneModule, 1),
    TuneResult_InvalidPath      = MAKERESULT(TuneModule, 2),
    TuneResult_FileNotFound     = MAKERESULT(TuneModule, 3),
    TuneResult_QueueEmpty       = MAKERESULT(TuneModule, 10),
    TuneResult_NotPlaying       = MAKERESULT(TuneModule, 11),
    TuneResult_OutOfRange       = MAKERESULT(TuneModule, 12),
    TuneResult_FileOpenFailure  = MAKERESULT(TuneModule, 20),
    TuneResult_VoiceInitFailure = MAKERESULT(TuneModule, 21),
    TuneResult_PlaybackFailure  = MAKERESULT(TuneModule, 22),
    TuneResult_NoAudioBuffer    = MAKERESULT(TuneModule, 23),
    TuneResult_OutOfMemory      = MAKERESULT(TuneModule, 30),
    TuneResult_Generic          = MAKERESULT(TuneModule, 40),
} TuneResult;

typedef enum {
    TuneShuffleMode_Off,
    TuneShuffleMode_On,

    TuneShuffleMode_Count,
} TuneShuffleMode;

typedef enum {
    TuneRepeatMode_Off,
    TuneRepeatMode_One,
    TuneRepeatMode_All,

    TuneRepeatMode_Count,
} TuneRepeatMode;

typedef enum {
    TuneEnqueueType_Front,
    TuneEnqueueType_Back,

    TuneEnqueueType_Count,
} TuneEnqueueType;

typedef struct {
    u32 sample_rate;
    u32 current_frame;
    u32 total_frames;
} TuneCurrentStats;

Result tuneInitialize();

void tuneExit();

/**
 * @brief Get the current status of playback.
 * @param[out] status \ref AudioOutState
 */
Result tuneGetStatus(bool *status);

Result tunePlay();
Result tunePause();
Result tuneNext();
Result tunePrev();

/**
 * @brief Get the current playback volume.
 * @note On FW lower than [6.0.0] this will set the decode volume.
 * @param[out] out volume value (linear factor).
 */
Result tuneGetVolume(float *out);

/**
 * @brief Set the playback volume.
 * @note On FW lower than [6.0.0] this will return the decode volume.
 * @param[in] volume volume value (linear factor).
 */
Result tuneSetVolume(float volume);

/**
 * @brief Get the default playing mode for all titles.
 * @param[out] out set if the default mode is play.
 */
Result tuneGetDefaultTitlePlay(bool* out);

/**
 * @brief Set the default playing mode for all titles.
 * @param[play] play set to enable playing by default.
 */
Result tuneSetDefaultTitlePlay(bool play);

/**
 * @brief Get the default volume of all titles
 * @param[out] out volume value (linear factor).
 */
Result tuneGetDefaultTitleVolume(float *out);

/**
 * @brief Set the default volume of all titles
 * @param[in] volume volume value (linear factor).
 */
Result tuneSetDefaultTitleVolume(float volume);

/**
 * @brief Get the current loop status.
 * @param[out] state \ref TuneRepeatMode
 */
Result tuneGetRepeatMode(TuneRepeatMode *state);

/**
 * @brief Set repeat mode.
 * @param[in] state \ref TuneRepeatMode
 */
Result tuneSetRepeatMode(TuneRepeatMode state);

Result tuneGetShuffleMode(TuneShuffleMode *state);
Result tuneSetShuffleMode(TuneShuffleMode state);

/**
 * @brief Get the current queue size.
 * @param[out] count remaining tracks after current.
 */
Result tuneGetPlaylistSize(u32 *count);

/**
 * @brief Read queue.
 * @param[out] read Amount written to buffer.
 * @param[out] out_path Path array FS_MAX_PATH * n
 * @param[in] out_path_length Size of the supplied path array.
 */
Result tuneGetPlaylistItem(u32 index, char *out_path, size_t out_path_length);

/**
 * @brief Get current song.
 * @param[out] out_path Path to current playing song.
 * @param[in] out_path_length Size of the out_path buffer. Path of the current track needs to fit.
 * @param[out] out \ref MusicCurrentTune
 */
Result tuneGetCurrentQueueItem(char *out_path, size_t out_path_length, TuneCurrentStats *out);

/**
 * @brief Clear queue.
 */
Result tuneClearQueue();
Result tuneMoveQueueItem(u32 src, u32 dst);
Result tuneSelect(u32 index);
Result tuneSeek(u32 position);

/**
 * @brief Add track to queue.
 * @note Must not include leading mount name.
 * @note Must match ^(sdmc:/.*.mp3)$
 * @param[in] path Path to file on sdcard.
 */
Result tuneEnqueue(const char *path, TuneEnqueueType type);

Result tuneRemove(u32 index);

/**
 * @brief Get the volume of the tune via override.
 * @param[id] application id of the title.
 * @param[out] out set if the title should play when title is loaded.
 * @param[has] set if the entry has been set.
 */
Result tuneGetTunePlayOverride(u64 id, bool *out, bool* has);

/**
 * @brief Set the volume of the tune via override.
 * @param[id] application id of the title.
 * @param[play] set to true to play when title is loaded.
 * @param[reset] if set, the value is removed from override.
 */
Result tuneSetTunePlayOverride(u64 id, bool play, bool reset);

/**
 * @brief Get the volume of the tune via override.
 * @param[id] application id of the title.
 * @param[out] out volume value (linear factor).
 */
Result tuneGetTuneVolumeOverride(u64 id, float *out, bool* has);

/**
 * @brief Set the volume of the tune via override.
 * @param[id] application id of the title.
 * @param[in] volume volume value (linear factor).
 * @param[reset] if set, the value is removed from override.
 */
Result tuneSetTuneVolumeOverride(u64 id, float volume, bool reset);

/**
 * @brief Get the volume of the title via override.
 * @param[id] application id of the title.
 * @param[out] out volume value (linear factor).
 * @param[has] set if the entry has been set.
 */
Result tuneGetTitleVolumeOverride(u64 id, float *out, bool* has);

/**
 * @brief Set the volume of the title via override
 * @param[id] application id of the title.
 * @param[in] volume volume value (linear factor).
 * @param[reset] if set, the value is removed from override.
 */
Result tuneSetTitleVolumeOverride(u64 id, float volume, bool reset);

/**
 * @brief Get the music path for a title.
 * @param[id] application id of the title.
 * @param[out] the music path to load from when the title is loaded.
 * @param[out_length] size of out.
 * @param[has] set if the entry has been set.
 */
Result tuneGetTitleMusicPathOverride(u64 id, char *out, size_t out_length, bool* has);

/**
 * @brief Set the music path for a title.
 * @param[id] application id of the title.
 * @param[path] the music path to load from when the title is loaded.
 * @param[reset] if set, the value is removed from override.
 */
Result tuneSetTitleMusicPathOverride(u64 id, const char* path, bool reset);

/**
 * @brief Check if the title has any override values.
 * @param[id] application id of the title.
 * @param[out] out set if the title has anything overriden.
 */
Result tuneHasOverride(u64 id, bool* out);
Result tuneResetOverride(u64 id);
Result tuneResetAllOverride(void);

/**
 * @brief Auto load song / folder when tune starts.
 */
Result tuneGetAutoPlayPath(char *out, size_t out_length);
Result tuneSetAutoPlayPath(const char *path);


Result tuneQuit();

Result tuneGetApiVersion(u32 *version);

#ifdef __cplusplus
}
#endif
