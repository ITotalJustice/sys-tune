#include "tune.h"

#include "ipc_cmd.h"

#include <string.h>

Service g_tune;

Result tuneInitialize() {
    Handle handle = 0;
    Result rc = svcConnectToNamedPort(&handle, "tune");
    if (R_SUCCEEDED(rc))
        serviceCreate(&g_tune, handle);
    return rc;
}

void tuneExit() {
    serviceClose(&g_tune);
}

Result tuneGetStatus(bool *status) {
    u8 tmp=0;
    Result rc = serviceDispatchOut(&g_tune, TuneIpcCmd_GetStatus, tmp);
    if (R_SUCCEEDED(rc) && status) *status = tmp & 1;
    return rc;
}

Result tunePlay() {
    return serviceDispatch(&g_tune, TuneIpcCmd_Play);
}

Result tunePause() {
    return serviceDispatch(&g_tune, TuneIpcCmd_Pause);
}

Result tuneNext() {
    return serviceDispatch(&g_tune, TuneIpcCmd_Next);
}

Result tunePrev() {
    return serviceDispatch(&g_tune, TuneIpcCmd_Prev);
}

Result tuneGetVolume(float *out) {
    return serviceDispatchOut(&g_tune, TuneIpcCmd_GetVolume, *out);
}

Result tuneSetVolume(float volume) {
    return serviceDispatchIn(&g_tune, TuneIpcCmd_SetVolume, volume);
}

Result tuneGetDefaultTitlePlay(bool* out) {
    return serviceDispatchOut(&g_tune, TuneIpcCmd_GetDefaultTitlePlay, *out);
}

Result tuneSetDefaultTitlePlay(bool play) {
    return serviceDispatchIn(&g_tune, TuneIpcCmd_SetDefaultTitlePlay, play);
}

Result tuneGetDefaultTitleVolume(float *out) {
    return serviceDispatchOut(&g_tune, TuneIpcCmd_GetDefaultTitleVolume, *out);
}

Result tuneSetDefaultTitleVolume(float volume) {
    return serviceDispatchIn(&g_tune, TuneIpcCmd_SetDefaultTitleVolume, volume);
}

Result tuneGetRepeatMode(TuneRepeatMode *state) {
    u8 out = 0;
    Result rc = serviceDispatchOut(&g_tune, TuneIpcCmd_GetRepeatMode, out);
    if (R_SUCCEEDED(rc) && state)
        *state = out;
    return rc;
}

Result tuneSetRepeatMode(TuneRepeatMode state) {
    u8 tmp = state;
    return serviceDispatchIn(&g_tune, TuneIpcCmd_SetRepeatMode, tmp);
}

Result tuneGetShuffleMode(TuneShuffleMode *state) {
    u8 out = 0;
    Result rc = serviceDispatchOut(&g_tune, TuneIpcCmd_GetShuffleMode, out);
    if (R_SUCCEEDED(rc) && state)
        *state = out;
    return rc;
}
Result tuneSetShuffleMode(TuneShuffleMode state) {
    u8 tmp = state;
    return serviceDispatchIn(&g_tune, TuneIpcCmd_SetShuffleMode, tmp);
}

Result tuneGetPlaylistSize(u32 *count) {
    return serviceDispatchOut(&g_tune, TuneIpcCmd_GetPlaylistSize, *count);
}

Result tuneGetPlaylistItem(u32 index, char *out_path, size_t out_path_length) {
    return serviceDispatchIn(&g_tune, TuneIpcCmd_GetPlaylistItem, index,
                              .buffer_attrs = {SfBufferAttr_Out | SfBufferAttr_HipcMapAlias},
                              .buffers = {{out_path, out_path_length}}, );
}

Result tuneGetCurrentQueueItem(char *out_path, size_t out_path_length, TuneCurrentStats *out) {
    return serviceDispatchOut(&g_tune, TuneIpcCmd_GetCurrentQueueItem, *out,
                              .buffer_attrs = {SfBufferAttr_Out | SfBufferAttr_HipcMapAlias},
                              .buffers = {{out_path, out_path_length}}, );
}

Result tuneClearQueue() {
    return serviceDispatch(&g_tune, TuneIpcCmd_ClearQueue);
}

Result tuneMoveQueueItem(u32 src, u32 dst) {
    const struct {
        u32 src;
        u32 dst;
    } in = {src, dst};
    return serviceDispatchIn(&g_tune, TuneIpcCmd_MoveQueueItem, in);
}

Result tuneSelect(u32 index) {
    return serviceDispatchIn(&g_tune, TuneIpcCmd_Select, index);
}

Result tuneSeek(u32 position) {
    return serviceDispatchIn(&g_tune, TuneIpcCmd_Seek, position);
}

Result tuneEnqueue(const char *path, TuneEnqueueType type) {
    u8 tmp = type;
    size_t path_length = strlen(path) + 1;
    return serviceDispatchIn(&g_tune, TuneIpcCmd_Enqueue, tmp,
                             .buffer_attrs = {SfBufferAttr_In | SfBufferAttr_HipcMapAlias},
                             .buffers = {{path, path_length}}, );
}

Result tuneRemove(u32 index) {
    return serviceDispatchIn(&g_tune, TuneIpcCmd_Remove, index);
}

Result tuneQuit() {
    return serviceDispatch(&g_tune, TuneIpcCmd_QuitServer);
}

Result tuneGetApiVersion(u32 *version) {
    return serviceDispatchOut(&g_tune, TuneIpcCmd_GetApiVersion, *version);
}

Result tuneGetTunePlayOverride(u64 id, bool *out, bool* has) {
    struct {
        bool out;
        bool has;
    } out_data;

    const Result rc = serviceDispatchInOut(&g_tune, TuneIpcCmd_GetTunePlayOverride, id, out_data);
    if (R_SUCCEEDED(rc) && out)
        *out = out_data.out;
    if (R_SUCCEEDED(rc) && has)
        *has = out_data.has;

    return rc;
}

Result tuneSetTunePlayOverride(u64 id, bool play, bool reset) {
    const struct {
        u64 id;
        bool play;
        bool reset;
    } in_data = { id, play, reset };

    return serviceDispatchIn(&g_tune, TuneIpcCmd_SetTunePlayOverride, in_data);
}

Result tuneGetTuneVolumeOverride(u64 id, float *out, bool* has) {
    struct {
        float out;
        bool has;
    } out_data;

    const Result rc = serviceDispatchInOut(&g_tune, TuneIpcCmd_GetTuneVolumeOverride, id, out_data);
    if (R_SUCCEEDED(rc) && out)
        *out = out_data.out;
    if (R_SUCCEEDED(rc) && has)
        *has = out_data.has;

    return rc;
}

Result tuneSetTuneVolumeOverride(u64 id, float volume, bool reset) {
    const struct {
        u64 id;
        float volume;
        bool reset;
    } in_data = { id, volume, reset };

    return serviceDispatchIn(&g_tune, TuneIpcCmd_SetTuneVolumeOverride, in_data);
}

Result tuneGetTitleVolumeOverride(u64 id, float *out, bool* has) {
    struct {
        float out;
        bool has;
    } out_data;

    const Result rc = serviceDispatchInOut(&g_tune, TuneIpcCmd_GetTitleVolumeOverride, id, out_data);
    if (R_SUCCEEDED(rc) && out)
        *out = out_data.out;
    if (R_SUCCEEDED(rc) && has)
        *has = out_data.has;

    return rc;
}

Result tuneSetTitleVolumeOverride(u64 id, float volume, bool reset) {
    const struct {
        u64 id;
        float volume;
        bool reset;
    } in_data = { id, volume, reset };

    return serviceDispatchIn(&g_tune, TuneIpcCmd_SetTitleVolumeOverride, in_data);
}

Result tuneGetTitleMusicPathOverride(u64 id, char *out, size_t out_length, bool* has) {
    return serviceDispatchInOut(&g_tune, TuneIpcCmd_GetTitleMusicPathOverride, id, *has,
                              .buffer_attrs = {SfBufferAttr_Out | SfBufferAttr_HipcMapAlias},
                              .buffers = {{out, out_length}}, );
}

Result tuneSetTitleMusicPathOverride(u64 id, const char* path, bool reset) {
    const struct {
        u64 id;
        bool reset;
    } in_data = { id, reset };

    const size_t path_length = strlen(path) + 1;
    return serviceDispatchIn(&g_tune, TuneIpcCmd_SetTitleMusicPathOverride, in_data,
                             .buffer_attrs = {SfBufferAttr_In | SfBufferAttr_HipcMapAlias},
                             .buffers = {{path, path_length}}, );
}

Result tuneHasOverride(u64 id, bool* out) {
    return serviceDispatchInOut(&g_tune, TuneIpcCmd_HasOverride, id, *out);
}

Result tuneResetOverride(u64 id) {
    return serviceDispatchIn(&g_tune, TuneIpcCmd_ResetOverride, id);
}

Result tuneResetAllOverride(void) {
    return serviceDispatch(&g_tune, TuneIpcCmd_ResetAllOverride);
}

Result tuneGetAutoPlayPath(char *out, size_t out_length) {
    return serviceDispatch(&g_tune, TuneIpcCmd_GetAutoPlayPath,
                              .buffer_attrs = {SfBufferAttr_Out | SfBufferAttr_HipcMapAlias},
                              .buffers = {{out, out_length}}, );
}

Result tuneSetAutoPlayPath(const char *path) {
    const size_t path_length = strlen(path) + 1;
    return serviceDispatch(&g_tune, TuneIpcCmd_SetAutoPlayPath,
                              .buffer_attrs = {SfBufferAttr_In | SfBufferAttr_HipcMapAlias},
                              .buffers = {{path, path_length}}, );
}
