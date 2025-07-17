#include "music_player.hpp"

#include "../tune_result.hpp"
#include "../tune_service.hpp"
#include "../config/config.hpp"
#include "sdmc/sdmc.hpp"
#include "pm/pm.hpp"
#include "aud_wrapper.h"
#include "source.hpp"

#include <cstring>
#include <nxExt.h>

namespace tune::impl {

    namespace {
        constexpr float VOLUME_MAX = 1.f;
        constexpr auto PLAYLIST_ENTRY_MAX = 300; // 75k
        constexpr auto PATH_SIZE_MAX = 256;

        struct PlaylistID {
            u32 id{UINT32_MAX};

            bool IsValid() const {
                return id != UINT32_MAX;
            }

            void Reset() {
                id = UINT32_MAX;
            }
        };

        class PlayList {
        public:
            void Init() {
                Clear();
                m_playlist.reserve(PLAYLIST_ENTRY_MAX);
                m_shuffle_playlist.reserve(PLAYLIST_ENTRY_MAX);
            }

            bool Add(const char* path, EnqueueType type) {
                u32 index;
                if (!FindNextFreeEntry(index)) {
                    return false;
                }

                if (!m_entries[index].Add(path)) {
                    return false;
                }

                if (type == EnqueueType::Front) {
                    m_playlist.emplace(m_playlist.cbegin(), index);
                } else {
                    m_playlist.emplace_back(index);
                }

                // add new entry id to shuffle_playlist_list
                const auto shuffle_playlist_size = m_shuffle_playlist.size() + 1;
                const auto shuffle_index = randomGet64() % shuffle_playlist_size;
                m_shuffle_playlist.emplace(m_shuffle_playlist.cbegin() + shuffle_index, index);

                return true;
            }

            bool Remove(u32 index, ShuffleMode shuffle) {
                const auto entry = Get(index, shuffle);
                R_UNLESS(entry.IsValid(), false);

                // remove entry.
                m_entries[entry.id].Remove();

                // remove from both playlists.
                if (shuffle == ShuffleMode::On) {
                    m_playlist.erase(m_playlist.begin() + GetIndexFromID(entry, ShuffleMode::Off));
                    m_shuffle_playlist.erase(m_shuffle_playlist.begin() + index);
                } else {
                    m_playlist.erase(m_playlist.begin() + index);
                    m_shuffle_playlist.erase(m_shuffle_playlist.begin() + GetIndexFromID(entry, ShuffleMode::On));
                }

                return true;
            }

            bool Swap(u32 src, u32 dst, ShuffleMode shuffle) {
                if (src >= Size() || dst >+ Size()) {
                    return false;
                }

                if (shuffle == ShuffleMode::On) {
                    std::swap(m_shuffle_playlist[src], m_shuffle_playlist[dst]);
                } else {
                    std::swap(m_playlist[src], m_playlist[dst]);
                }

                return true;
            }

            void Shuffle() {
                const auto size = m_shuffle_playlist.size();
                if (!size) {
                    return;
                }

                for (auto& e : m_shuffle_playlist) {
                    const auto index = randomGet64() % size;
                    std::swap(e, m_shuffle_playlist[index]);
                }
            }

            const char* GetPath(u32 index, ShuffleMode shuffle) const {
                return GetPath(Get(index, shuffle));
            }

            const char* GetPath(const PlaylistID& entry) const {
                R_UNLESS(entry.IsValid(), nullptr);

                return m_entries[entry.id].GetPath();
            }

            void Clear() {
                for (u32 i = 0; i < m_entries.size(); i++) {
                    m_entries[i].Remove();
                }

                m_playlist.clear();
                m_shuffle_playlist.clear();
            }

            u32 Size() const {
                return m_playlist.size();
            }

            PlaylistID Get(u32 index, ShuffleMode shuffle) const {
                if (index >= Size()) {
                    return {};
                }

                if (shuffle == ShuffleMode::On) {
                    return m_shuffle_playlist[index];
                } else {
                    return m_playlist[index];
                }
            }

            u32 GetIndexFromID(const PlaylistID& entry, ShuffleMode shuffle) const {
                if (!entry.IsValid()) {
                    return 0;
                }

                std::span list{m_playlist};
                if (shuffle == ShuffleMode::On) {
                    list = m_shuffle_playlist;
                }

                for (u32 i = 0; i < list.size(); i++) {
                    if (list[i].id == entry.id) {
                        return i;
                    }
                }

                return 0;
            }

        private:
            bool FindNextFreeEntry(u32& index) const {
                for (u32 i = 0; i < m_entries.size(); i++) {
                    if (m_entries[i].IsEmpty()) {
                        index = i;
                        return true;
                    }
                }

                return false;
            }

        private:
            struct PlayListNameEntry {
            public:
                // in most cases, the path will not exceed 256 bytes,
                // so this is a reasonable max rather than 0x301.
                bool Add(const char* path) {
                    if (!IsEmpty()) {
                        return false;
                    }

                    if (std::strlen(path) >= sizeof(m_path)) {
                        return false;
                    }

                    std::strcpy(m_path, path);
                    return true;
                }

                bool Remove() {
                    m_path[0] = '\0';
                    return true;
                }

                bool IsEmpty() const {
                    return m_path[0] == '\0';
                }

                const char* GetPath() const {
                    return m_path;
                }

            private:
                char m_path[PATH_SIZE_MAX]{};
            };

        private:
            std::vector<PlaylistID> m_playlist{};
            std::vector<PlaylistID> m_shuffle_playlist{};
            std::array<PlayListNameEntry, PLAYLIST_ENTRY_MAX> m_entries{};
        };

        PlayList g_playlist;

        struct LastSong {
            public:
                struct LastSongEntry {
                    std::string path;
                    u32 offset;
                };

                void Reset() {
                    m_entry.reset();
                }

                void Push(const std::string& path, u32 offset) {
                    m_entry.emplace(path, offset);
                }

                auto Pop() -> std::optional<LastSongEntry> {
                    auto result = m_entry;
                    m_entry.reset();
                    return result;
                }

            private:
                std::optional<LastSongEntry> m_entry;
        };

        struct CurrentSong final : PlaylistID {
            std::string path;

            void Load(const std::string& _path) {
                path = _path;
                id = UINT32_MAX;
            }

            void Load(PlaylistID _id) {
                path = g_playlist.GetPath(_id);
                id = _id.id;
            }

            bool IsValid() const {
                return id != UINT32_MAX || !path.empty();
            }

            void Reset() {
                id = UINT32_MAX;
                path.clear();
            }
        };

        CurrentSong g_current;
        LastSong g_last_song;
        u32 g_queue_position;

        LockableMutex g_mutex;

        RepeatMode g_repeat   = RepeatMode::All;
        ShuffleMode g_shuffle = ShuffleMode::Off;
        PlayerStatus g_status = PlayerStatus::FetchNext;
        Source *g_source = nullptr;

        u64 g_current_tid = 0; // set in pmdnmt thread.

        float g_default_title_volume = 1.f;
        float g_default_tune_volume = 1.f;
        std::optional<float> g_tune_volume_override;
        std::optional<float> g_title_volume_override;
        std::string g_title_music_override;

        constexpr auto AUDIO_FREQ          = 48000;
        constexpr auto AUDIO_CHANNEL_COUNT = 2;
        constexpr auto AUDIO_BUFFER_COUNT  = 2;
        constexpr auto AUDIO_LATENCY_MS    = 42;
        constexpr auto AUDIO_BUFFER_SIZE   = AUDIO_FREQ / 1000 * AUDIO_LATENCY_MS * AUDIO_CHANNEL_COUNT;

        AudioOutBuffer g_audout_buffer[AUDIO_BUFFER_COUNT];
        alignas(0x1000) s16 AudioMemoryPool[AUDIO_BUFFER_COUNT][(AUDIO_BUFFER_SIZE + 0xFFF) & ~0xFFF];
        static_assert((sizeof(AudioMemoryPool[0]) % 0x2000) == 0, "Audio Memory pool needs to be page aligned!");

        bool g_should_play       = true;
        bool g_should_run        = true;
        bool g_music_path_current = {};


        Result PlayTrack(const char* path, bool loaded_from_playlist) {
            /* Open file and allocate */
            auto source = OpenFile(path);
            R_UNLESS(source != nullptr, TuneResult_FileOpenFailure);
            R_UNLESS(source->IsOpen(), TuneResult_FileOpenFailure);
            R_UNLESS(source->SetupResampler(audoutGetChannelCount(), audoutGetSampleRate()), TuneResult_VoiceInitFailure);

            AudioOutState state;
            R_TRY(audoutGetAudioOutState(&state));
            if (state == AudioOutState_Stopped) {
                R_TRY(audoutStartAudioOut());
            }

            // check if we are loading back a previous song that was overriden.
            if (loaded_from_playlist) {
                if (const auto last = g_last_song.Pop()) {
                    if (last->path == path) {
                        source->Seek(last->offset);
                    }
                }
            }

            // keep track of this song being set via title override.
            if (!g_title_music_override.empty()) {
                g_music_path_current = true;
            }

            g_title_music_override.clear();
            g_source = source.get();
            ON_SCOPE_EXIT( g_source = nullptr );

            // for the first buffer, use very small buffer sizes to reduce latency between songs.
            int first = 1;

            // keep track of current pause state.
            bool should_play = g_should_play;

            while (g_should_run && g_status == PlayerStatus::Playing) {
                // stop audout of pause changes so that we don't drop samples.
                if (should_play != g_should_play) {
                    should_play = g_should_play;
                    if (should_play) {
                        audoutStartAudioOut();
                    } else {
                        audoutStopAudioOut();
                    }
                }

                if (!should_play) {
                    svcSleepThread(17'000'000);
                    continue;
                }

                /* Check if have a buffer that's not yet been submitted or has been released. */
                AudioOutBuffer* buffer = NULL;
                for (int i = 0; i < AUDIO_BUFFER_COUNT; i++) {
                    bool has_buffer = false;
                    R_TRY(audoutContainsAudioOutBuffer(&g_audout_buffer[i], &has_buffer));
                    if (!has_buffer) {
                        buffer = &g_audout_buffer[i];
                        break;
                    }
                }

                /* If we don't have a buffer free, wait until one of the pending buffers has finished. */
                if (!buffer) {
                    u32 released_count;
                    R_TRY(audoutWaitPlayFinish(&buffer, &released_count, UINT64_MAX));
                }

                /* This should never happen, however just in case, guard against this becoming a spinloop. */
                R_UNLESS(buffer, TuneResult_NoAudioBuffer);

                /* Update the default buffer size if this is the first audio buffer, reduces latency between songs. */
                auto buffer_size = AUDIO_BUFFER_SIZE * sizeof(s16);
                if (first) {
                    first--;
                    buffer_size = std::min(512 * sizeof(s16), buffer_size);
                }

                /* Checking if the source has finished is handled below, so returning <= 0 is always an error. */
                const auto nSamples = source->Resample((u8*)buffer->buffer, buffer_size);
                R_UNLESS(nSamples > 0, TuneResult_PlaybackFailure);

                /* Submit the audio buffer. */
                buffer->data_size = nSamples;
                R_TRY(audoutAppendAudioOutBuffer(buffer));

                /* If we have finished, check if we should loop or move onto the next song. */
                if (source->Done()) {
                    if (g_repeat == RepeatMode::One && !g_music_path_current) {
                        if (source->Seek(0)) {
                            continue;
                        }
                    } else if (g_repeat != RepeatMode::One && !g_music_path_current) {
                        Next();
                    }
                    break;
                }
            }

            // if we are loading a song override, save the path and offset to resume from.
            if (loaded_from_playlist && !g_title_music_override.empty() && g_status == PlayerStatus::FetchNext && !source->Done()) {
                g_last_song.Push(path, source->Tell().first);
            }

            return 0;
        }

    }

    Result Initialize() {
        for (int i = 0; i < AUDIO_BUFFER_COUNT; i++) {
            g_audout_buffer[i].buffer = AudioMemoryPool[i];
            g_audout_buffer[i].buffer_size = sizeof(AudioMemoryPool[i]);
        }

        R_TRY(audoutInitialize());
        SetVolume(config::get_volume());

        /* Fetch values from config, sanitize the return value */
        if (auto c = config::get_repeat(); c <= 2 && c >= 0) {
            SetRepeatMode(static_cast<RepeatMode>(c));
        }

        SetShuffleMode(static_cast<ShuffleMode>(config::get_shuffle()));
        SetDefaultTitleVolume(config::get_default_title_volume());

        // reserves memory so that we don't allocate later on.
        g_playlist.Init();

        return 0;

    }

    void Exit() {
        g_should_run = false;
    }

    void TuneThreadFunc(void *) {
        {
            char load_path[PATH_SIZE_MAX];
            if (config::get_load_path(load_path, sizeof(load_path))) {
                // check if the path is a file or folder.
                FsDirEntryType type;
                if (R_SUCCEEDED(sdmc::GetType(load_path, &type))) {
                    if (type == FsDirEntryType_File) {
                        // path is a file, load single entry.
                        if (GetSourceType(load_path) != SourceType::NONE) {
                            Enqueue(load_path, EnqueueType::Back);
                        }
                    } else {
                        // path is a folder, load all entries.
                        FsDir dir;
                        if (R_SUCCEEDED(sdmc::OpenDir(&dir, load_path, FsDirOpenMode_ReadFiles|FsDirOpenMode_NoFileSize))) {
                            // during init, we have a lot of memory to work with.
                            std::vector<FsDirectoryEntry> entries(std::min(64, PLAYLIST_ENTRY_MAX));

                            s64 total;
                            char full_path[PATH_SIZE_MAX];
                            Result rc = 0;

                            while (R_SUCCEEDED(fsDirRead(&dir, &total, entries.size(), entries.data())) && total) {
                                for (s64 i = 0; i < total; i++) {
                                    if (GetSourceType(entries[i].name) != SourceType::NONE) {
                                        std::snprintf(full_path, sizeof(full_path), "%s/%s", load_path, entries[i].name);
                                        rc = Enqueue(full_path, EnqueueType::Back);
                                        if (rc == TuneResult_OutOfMemory) {
                                            break;
                                        }
                                    }
                                }

                                if (rc == TuneResult_OutOfMemory) {
                                    break;
                                }
                            }

                            fsDirClose(&dir);
                        }
                    }
                }
            }
        }

        /* Run as long as we aren't stopped and no error has been encountered. */
        while (g_should_run) {
            g_current.Reset();

            if (!g_title_music_override.empty() || g_status == PlayerStatus::FetchNext) {
                g_music_path_current = false;
            }

            // sleep if we are waiting for tid to change.
            if (g_music_path_current) {
                svcSleepThread(100'000'000ul);
                continue;
            }

            bool loaded_from_playlist;

            if (!g_title_music_override.empty()) {
                g_current.Load(g_title_music_override);
                loaded_from_playlist = false;
            } else {
                std::scoped_lock lk(g_mutex);
                g_music_path_current = false;

                const auto queue_size = g_playlist.Size();
                if (queue_size == 0) {
                    g_current.Reset();
                } else if (g_queue_position >= queue_size) {
                    g_queue_position = queue_size - 1;
                    continue;
                } else {
                    g_current.Load(g_playlist.Get(g_queue_position, g_shuffle));
                    loaded_from_playlist = true;
                }
            }

            /* Sleep if queue is empty. */
            if (!g_current.IsValid()) {
                svcSleepThread(100'000'000ul);
                continue;
            }

            g_status = PlayerStatus::Playing;
            /* Only play if playing and we have a track queued. */
            Result rc = PlayTrack(g_current.path.c_str(), loaded_from_playlist);

            /* Log error. */
            if (R_FAILED(rc)) {
                /* Remove track if something went wrong. */
                Remove(g_queue_position);
            }
        }

        audoutStopAudioOut();
        audoutExit();
    }

    void GpioThreadFunc(void *ptr) {
        GpioPadSession *session = static_cast<GpioPadSession *>(ptr);

        bool pre_unplug_pause = false;

        /* [0] Low == plugged in; [1] High == not plugged in. */
        GpioValue old_value = GpioValue_High;
        gpioPadGetValue(session, &old_value);

        // TODO(TJ): pausing on headphone change should be a config option.
        while (g_should_run) {
            /* Fetch current gpio value. */
            GpioValue value;
            if (R_SUCCEEDED(gpioPadGetValue(session, &value))) {
                if (old_value == GpioValue_Low && value == GpioValue_High) {
                    pre_unplug_pause = g_should_play;
                    g_should_play     = true;
                } else if (old_value == GpioValue_High && value == GpioValue_Low) {
                    if (!pre_unplug_pause)
                        g_should_play = false;
                }
                old_value = value;
            }
            svcSleepThread(10'000'000);
        }
    }

    void PmdmntThreadFunc(void *ptr) {
        float tune_volume = 1.f;;

        while (g_should_run) {
            u64 pid{}, new_tid{};
            if (pm::PollCurrentPidTid(&pid, &new_tid)) {
                g_current_tid = new_tid;
                g_tune_volume_override.reset();
                g_title_volume_override.reset();
                g_title_music_override.clear();

                if (config::has_tune_play_override(new_tid)) {
                    g_should_play = config::get_tune_play_override(new_tid);
                } else {
                    g_should_play = config::get_title_play_default();
                }

                if (config::has_tune_volume_override(new_tid)) {
                    g_tune_volume_override = config::get_tune_volume_override(new_tid);
                }

                if (config::has_title_volume_override(new_tid)) {
                    g_title_volume_override = config::get_title_volume_override(new_tid);
                }

                if (config::has_title_music_override(new_tid)) {
                    char path[256];
                    if (config::get_title_music_override(new_tid, path, sizeof(path))) {
                        g_title_music_override = path;
                    }
                }

                char path[256];
                if (config::get_title_music_override(new_tid, path, sizeof(path))) {
                    g_title_music_override = path;
                    g_status = PlayerStatus::FetchNext;
                } else if (g_music_path_current) {
                    g_status = PlayerStatus::FetchNext;
                }
            }

            // sadly, we can't simply apply auda when the title changes
            // as it seems to apply to quickly, before the title opens audio
            // services, so the changes don't apply.
            // best option is to repeatdly set the out :/
            float title_volume;
            if (pid && R_SUCCEEDED(audWrapperGetProcessMasterVolume(pid, &title_volume))) {
                const auto v = g_title_volume_override.value_or(g_default_title_volume);
                if (v != title_volume) {
                    // fade the change over 100ms.
                    audWrapperSetProcessMasterVolume(pid, 1e+8, v);
                }
            }

            if (tune_volume != g_tune_volume_override.value_or(g_default_tune_volume)) {
                tune_volume = g_tune_volume_override.value_or(g_default_tune_volume);
                audoutSetAudioOutVolume(tune_volume);
            }

            svcSleepThread(1e+8);
        }
    }

    bool GetStatus() {
        return g_should_play;
    }

    void Play() {
        g_should_play = true;
    }

    void Pause() {
        g_should_play = false;
    }

    void Next() {
        bool play = true;
        {
            std::scoped_lock lk(g_mutex);

            if (g_queue_position < g_playlist.Size() - 1) {
                g_queue_position++;
            } else {
                g_queue_position = 0;
                if (g_repeat == RepeatMode::Off)
                    play = false;
            }
        }
        g_status     = PlayerStatus::FetchNext;
        g_should_play = play;
    }

    void Prev() {
        {
            std::scoped_lock lk(g_mutex);

            if (g_queue_position > 0) {
                g_queue_position--;
            } else {
                g_queue_position = g_playlist.Size() - 1;
            }
        }
        g_status     = PlayerStatus::FetchNext;
        g_should_play = true;
    }

    float GetVolume() {
        return g_default_tune_volume;
    }

    void SetVolume(float volume) {
        volume = std::clamp(volume, 0.f, VOLUME_MAX);
        g_default_tune_volume = volume;
        config::set_volume(volume);
    }

    bool GetDefaultTitlePlay() {
        return config::get_title_play_default();
    }

    void SetDefaultTitlePlay(bool play) {
        if (!config::has_tune_play_override(g_current_tid)) {
            g_should_play = play;
        }

        config::set_title_play_default(play);
    }

    float GetDefaultTitleVolume() {
        return g_default_title_volume;
    }

    void SetDefaultTitleVolume(float volume) {
        volume = std::clamp(volume, 0.f, VOLUME_MAX);
        g_default_title_volume = volume;
        config::set_default_title_volume(volume);
    }

    RepeatMode GetRepeatMode() {
        return g_repeat;
    }

    void SetRepeatMode(RepeatMode mode) {
        g_repeat = mode;
        config::set_repeat((int)mode);
    }

    ShuffleMode GetShuffleMode() {
        return g_shuffle;
    }

    void SetShuffleMode(ShuffleMode mode) {
        std::scoped_lock lk(g_mutex);

        // if we just enabled shuffle mode, re-shuffle the playlist.
        if (g_shuffle == ShuffleMode::Off && mode == ShuffleMode::On) {
            g_playlist.Shuffle();
        }

        g_shuffle = mode;
        config::set_shuffle((int)mode);
    }

    u32 GetPlaylistSize() {
        std::scoped_lock lk(g_mutex);

        return g_playlist.Size();
    }

    Result GetPlaylistItem(u32 index, char *buffer, size_t buffer_size) {
        std::scoped_lock lk(g_mutex);

        const auto path = g_playlist.GetPath(index, g_shuffle);
        R_UNLESS(path, TuneResult_OutOfRange);

        std::snprintf(buffer, buffer_size, "%s", path);

        return 0;
    }

    Result GetCurrentQueueItem(CurrentStats *out, char *buffer, size_t buffer_size) {
        R_UNLESS(g_source != nullptr, TuneResult_NotPlaying);
        R_UNLESS(g_source->IsOpen(), TuneResult_NotPlaying);

        {
            std::scoped_lock lk(g_mutex);

            R_UNLESS(!g_current.path.empty(), TuneResult_NotPlaying);
            std::snprintf(buffer, buffer_size, "%s", g_current.path.c_str());
        }

        auto [current, total] = g_source->Tell();
        int sample_rate       = g_source->GetSampleRate();

        out->sample_rate   = sample_rate;
        out->current_frame = current;
        out->total_frames  = total;

        return 0;
    }

    void ClearQueue() {
        {
            std::scoped_lock lk(g_mutex);

            g_playlist.Clear();
            g_last_song.Reset();
            g_queue_position = 0;
        }
        g_status = PlayerStatus::FetchNext;
    }

    // currently unused (and untested).
    void MoveQueueItem(u32 src, u32 dst) {
        std::scoped_lock lk(g_mutex);

        if (!g_playlist.Swap(src, dst, g_shuffle)) {
            return;
        }

        if (g_queue_position == src) {
            g_queue_position = dst;
        }
    }

    void Select(u32 index) {
        {
            std::scoped_lock lk(g_mutex);

            const auto size = g_playlist.Size();
            if (!size) {
                return;
            }

            g_queue_position = std::min(index, size - 1);
        }
        g_status     = PlayerStatus::FetchNext;
        g_should_play = true;
    }

    void Seek(u32 position) {
        if (g_source != nullptr && g_source->IsOpen())
            g_source->Seek(position);
    }

    Result Enqueue(const char *buffer, EnqueueType type) {
        if (GetSourceType(buffer) == SourceType::NONE)
            return TuneResult_InvalidPath;

        /* Ensure file exists. */
        if (!sdmc::FileExists(buffer))
            return TuneResult_InvalidPath;

        std::scoped_lock lk(g_mutex);

        if (!g_playlist.Add(buffer, type)) {
            return TuneResult_OutOfMemory;
        }

        // check if the current position still points to the same entry, update if not.
        if (g_current.IsValid() && g_current.id != g_playlist.Get(g_queue_position, g_shuffle).id) {
            g_queue_position = g_playlist.GetIndexFromID(g_current, g_shuffle);
            g_current.Load(g_playlist.Get(g_queue_position, g_shuffle));
        }

        return 0;
    }

    Result Remove(u32 index) {
        std::scoped_lock lk(g_mutex);

        /* Ensure we don't operate out of bounds. */
        R_UNLESS(g_playlist.Size(), TuneResult_QueueEmpty);

        if (!g_playlist.Remove(index, g_shuffle)) {
            return TuneResult_OutOfRange;
        }

        /* Fetch a new track if we deleted the current song. */
        const bool fetch_new = g_queue_position == index;

        /* Lower current position if needed. */
        if (g_queue_position > index) {
            g_queue_position--;
        }

        if (fetch_new)
            g_status = PlayerStatus::FetchNext;

        return 0;
    }

    void GetTunePlayOverride(u64 id, bool *out, bool* has) {
        *has = config::has_tune_play_override(id);
        if (*has) {
            *out = config::get_tune_play_override(id);
        } else {
            *out = true;
        }
    }

    void SetTunePlayOverride(u64 id, bool value, bool reset) {
        if (id == g_current_tid) {
            if (reset) {
                g_should_play = config::get_title_play_default();
            } else {
                g_should_play = value;
            }
        }

        config::set_tune_play_override(id, value, reset);
    }

    void GetTuneVolumeOverride(u64 id, float *out, bool* has) {
        *has = config::has_tune_volume_override(id);
        if (*has) {
            *out = config::get_tune_volume_override(id);
        } else {
            *out = 1.f;
        }
    }

    void SetTuneVolumeOverride(u64 id, float value, bool reset) {
        value = std::clamp(value, 0.f, 1.f);

        if (id == g_current_tid) {
            if (reset) {
                g_tune_volume_override.reset();
            } else {
                g_tune_volume_override = value;
            }
        }

        config::set_tune_volume_override(id, value, reset);
    }

    void GetTitleVolumeOverride(u64 id, float *out, bool* has) {
        *has = config::has_title_volume_override(id);
        if (*has) {
            *out = config::get_title_volume_override(id);
        } else {
            *out = 1.f;
        }
    }

    void SetTitleVolumeOverride(u64 id, float value, bool reset) {
        value = std::clamp(value, 0.f, 1.f);

        if (id == g_current_tid) {
            if (reset) {
                g_title_volume_override.reset();
            } else {
                g_title_volume_override = value;
            }
        }

        config::set_title_volume_override(id, value, reset);
    }

    void GetTitleMusicPathOverride(u64 id, char *out, size_t out_length, bool* has) {
        *has = config::has_title_music_override(id);
        if (*has) {
            config::get_title_music_override(id, out, out_length);
        } else {
            memset(out, 0, out_length);
        }
    }

    void SetTitleMusicPathOverride(u64 id, const char* value, bool reset) {
        config::set_title_music_override(id, value, reset);
    }

    bool HasOverride(u64 id) {
        return config::has_override(id);
    }

    void ResetOverride(u64 id) {
        config::reset_override(id);
    }

    void ResetAllOverride() {
        config::reset_all_override();
    }

    void GetAutoPlayPath(char *out, size_t out_length) {
        config::get_load_path(out, out_length);
    }

    void SetAutoPlayPath(const char *path) {
        config::set_load_path(path);
    }
}
