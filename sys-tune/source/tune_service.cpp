#include "tune_service.hpp"

#include "impl/music_player.hpp"
#include "ipc_cmd.h"
#include "tune_result.hpp"
#include "tune_types.hpp"

#include <nxExt.h>

#define GET_SINGLE(type, expr)            \
    ({                                    \
        *out_dataSize     = sizeof(type); \
        *(type *)out_data = expr();       \
        return 0;                         \
    })

#define SET_SINGLE(type, expr)              \
    ({                                      \
        if (r->data.size >= sizeof(type)) { \
            expr(*(type *)r->data.ptr);     \
            return 0;                       \
        }                                   \
        break;                              \
    })

namespace tune {

    namespace {

        template<typename Arg1, typename Func>
        Result GetDataHelper(const IpcServerRequest *r, u8 *out_data, size_t *out_dataSize, Func func) {
            struct Data {
                Arg1 arg1;
                bool has;
            };

            auto id = *(const u64 *)r->data.ptr;
            auto out = (Data *)out_data;
            *out_dataSize = sizeof(*out);
            func(id, &out->arg1, &out->has);
            return 0;
        }

        template<typename Arg1, typename Func>
        Result SetDataHelper(const IpcServerRequest *r, u8 *out_data, size_t *out_dataSize, Func func) {
            struct Data {
                u64 id;
                Arg1 arg1;
                bool reset;
            };

            auto in = (const Data *)r->data.ptr;
            func(in->id, in->arg1, in->reset);
            return 0;
        }

        IpcServer g_server;
        bool running = true;

        Result ServiceHandlerFunc(void *arg, const IpcServerRequest *r, u8 *out_data, size_t *out_dataSize) {
            switch (r->data.cmdId) {
                case TuneIpcCmd_GetStatus:
                    GET_SINGLE(bool, impl::GetStatus);

                case TuneIpcCmd_Play:
                    impl::Play();
                    return 0;

                case TuneIpcCmd_Pause:
                    impl::Pause();
                    return 0;

                case TuneIpcCmd_Next:
                    impl::Next();
                    return 0;

                case TuneIpcCmd_Prev:
                    impl::Prev();
                    return 0;

                case TuneIpcCmd_GetVolume:
                    GET_SINGLE(float, impl::GetVolume);

                case TuneIpcCmd_SetVolume:
                    SET_SINGLE(float, impl::SetVolume);

                case TuneIpcCmd_GetDefaultTitlePlay:
                    GET_SINGLE(bool, impl::GetDefaultTitlePlay);

                case TuneIpcCmd_SetDefaultTitlePlay:
                    SET_SINGLE(bool, impl::SetDefaultTitlePlay);

                case TuneIpcCmd_GetDefaultTitleVolume:
                    GET_SINGLE(float, impl::GetDefaultTitleVolume);

                case TuneIpcCmd_SetDefaultTitleVolume:
                    SET_SINGLE(float, impl::SetDefaultTitleVolume);

                case TuneIpcCmd_GetRepeatMode:
                    GET_SINGLE(RepeatMode, impl::GetRepeatMode);

                case TuneIpcCmd_SetRepeatMode:
                    SET_SINGLE(RepeatMode, impl::SetRepeatMode);

                case TuneIpcCmd_GetShuffleMode:
                    GET_SINGLE(ShuffleMode, impl::GetShuffleMode);

                case TuneIpcCmd_SetShuffleMode:
                    SET_SINGLE(ShuffleMode, impl::SetShuffleMode);

                case TuneIpcCmd_GetPlaylistSize:
                    GET_SINGLE(u32, impl::GetPlaylistSize);

                case TuneIpcCmd_GetPlaylistItem:
                    if (r->hipc.meta.num_recv_buffers >= 1 && r->data.size >= sizeof(u32)) {
                        return impl::GetPlaylistItem(
                            *(u32 *)r->data.ptr,
                            (char *)hipcGetBufferAddress(r->hipc.data.recv_buffers),
                            hipcGetBufferSize(r->hipc.data.recv_buffers));
                    }
                    return TuneResult_InvalidArgument;

                case TuneIpcCmd_GetCurrentQueueItem:
                    if (r->hipc.meta.num_recv_buffers >= 1) {
                        *out_dataSize = sizeof(CurrentStats);
                        return impl::GetCurrentQueueItem(
                            (CurrentStats *)out_data,
                            (char *)hipcGetBufferAddress(r->hipc.data.recv_buffers),
                            hipcGetBufferSize(r->hipc.data.recv_buffers));
                    }
                    return TuneResult_InvalidArgument;

                case TuneIpcCmd_ClearQueue:
                    impl::ClearQueue();
                    return 0;

                case TuneIpcCmd_MoveQueueItem:
                    if (r->data.size >= 2 * sizeof(u32)) {
                        u32 *data = (u32 *)r->data.ptr;
                        impl::MoveQueueItem(data[0], data[1]);
                        return 0;
                    }
                    return TuneResult_InvalidArgument;

                case TuneIpcCmd_Select:
                    SET_SINGLE(u32, impl::Select);

                case TuneIpcCmd_Seek:
                    SET_SINGLE(u32, impl::Seek);

                case TuneIpcCmd_Enqueue:
                    if (r->hipc.meta.num_send_buffers >= 1 && r->data.size >= sizeof(EnqueueType)) {
                        return impl::Enqueue(
                            (const char *)hipcGetBufferAddress(r->hipc.data.send_buffers),
                            *(EnqueueType *)r->data.ptr);
                    }
                    return TuneResult_InvalidArgument;

                case TuneIpcCmd_Remove:
                    SET_SINGLE(u32, impl::Remove);

                case TuneIpcCmd_QuitServer:
                    running = false;
                    return 0;

                case TuneIpcCmd_GetTunePlayOverride:
                    return GetDataHelper<bool>(r, out_data, out_dataSize, impl::GetTunePlayOverride);

                case TuneIpcCmd_SetTunePlayOverride:
                    return SetDataHelper<bool>(r, out_data, out_dataSize, impl::SetTunePlayOverride);

                case TuneIpcCmd_GetTuneVolumeOverride:
                    return GetDataHelper<float>(r, out_data, out_dataSize, impl::GetTuneVolumeOverride);

                case TuneIpcCmd_SetTuneVolumeOverride:
                    return SetDataHelper<float>(r, out_data, out_dataSize, impl::SetTuneVolumeOverride);

                case TuneIpcCmd_GetTitleVolumeOverride:
                    return GetDataHelper<float>(r, out_data, out_dataSize, impl::GetTitleVolumeOverride);

                case TuneIpcCmd_SetTitleVolumeOverride:
                    return SetDataHelper<float>(r, out_data, out_dataSize, impl::SetTitleVolumeOverride);

                case TuneIpcCmd_GetTitleMusicPathOverride:
                    if (r->hipc.meta.num_recv_buffers >= 1) {
                        *out_dataSize = sizeof(bool);
                        impl::GetTitleMusicPathOverride(
                            *(u64 *)r->data.ptr,
                            (char *)hipcGetBufferAddress(r->hipc.data.recv_buffers),
                            hipcGetBufferSize(r->hipc.data.recv_buffers),
                            (bool *)out_data
                        );

                        return 0;
                    }
                    return TuneResult_InvalidArgument;

                case TuneIpcCmd_SetTitleMusicPathOverride:
                    if (r->hipc.meta.num_send_buffers >= 1) {
                        struct Data {
                            u64 id;
                            bool reset;
                        };
                        auto in = (const Data *)r->data.ptr;

                        impl::SetTitleMusicPathOverride(
                            in->id,
                            (const char *)hipcGetBufferAddress(r->hipc.data.send_buffers),
                            in->reset);

                        return 0;
                    }
                    return TuneResult_InvalidArgument;

                case TuneIpcCmd_HasOverride:
                    *out_dataSize     = sizeof(bool);
                    *(bool *)out_data = impl::HasOverride(*(u64 *)r->data.ptr);
                    return 0;

                case TuneIpcCmd_ResetOverride:
                    SET_SINGLE(u64, impl::ResetOverride);

                case TuneIpcCmd_ResetAllOverride:
                    impl::ResetAllOverride();
                    return 0;

                case TuneIpcCmd_GetAutoPlayPath:
                    if (r->hipc.meta.num_recv_buffers >= 1) {
                        impl::GetAutoPlayPath(
                            (char *)hipcGetBufferAddress(r->hipc.data.recv_buffers),
                            hipcGetBufferSize(r->hipc.data.recv_buffers));
                        return 0;
                    }
                    return TuneResult_InvalidArgument;

                case TuneIpcCmd_SetAutoPlayPath:
                    if (r->hipc.meta.num_send_buffers >= 1) {
                        impl::SetAutoPlayPath(
                            (const char *)hipcGetBufferAddress(r->hipc.data.send_buffers));
                        return 0;
                    }
                    return TuneResult_InvalidArgument;

                case TuneIpcCmd_GetApiVersion:
                    *out_dataSize    = sizeof(u32);
                    *(u32 *)out_data = TUNE_API_VERSION;
                    return 0;
            }
            return TuneResult_Generic;
        }

    }

    Result InitializeServer() {
        return ipcServerInit(&g_server, "tune", 2);
    }

    Result ExitServer() {
        return ipcServerExit(&g_server);
    }

    void LoopProcess() {
        while (running) {
            if (ipcServerProcess(&g_server, ServiceHandlerFunc, nullptr) == KERNELRESULT(Cancelled))
                break;
        }
    }

}
