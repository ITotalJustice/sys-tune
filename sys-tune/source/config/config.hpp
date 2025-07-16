#pragma once

#include <switch.h>

namespace config {

// tune shuffle
auto get_shuffle() -> bool;
void set_shuffle(bool value);

// tune repeat
auto get_repeat() -> int;
void set_repeat(int value);

// tune volume
auto get_volume() -> float;
void set_volume(float value);

// default for tune for every title
auto get_title_play_default() -> bool;
void set_title_play_default(bool value);

// default volume for every title
auto get_default_title_volume() -> float;
void set_default_title_volume(float value);

// returns the length of the string
auto get_load_path(char* out, int max_len) -> int;
void set_load_path(const char* path);

// per title tune enable
auto has_tune_play_override(u64 tid) -> bool;
auto get_tune_play_override(u64 tid) -> bool;
void set_tune_play_override(u64 tid, bool value, bool reset);

// per title tune volume
auto has_tune_volume_override(u64 tid) -> bool;
auto get_tune_volume_override(u64 tid) -> float;
void set_tune_volume_override(u64 tid, float value, bool reset);

// per title title volume
auto has_title_volume_override(u64 tid) -> bool;
auto get_title_volume_override(u64 tid) -> float;
void set_title_volume_override(u64 tid, float value, bool reset);

// per title music path
auto has_title_music_override(u64 tid) -> bool;
auto get_title_music_override(u64 tid, char* out, int max_len) -> int;
void set_title_music_override(u64 tid, const char* value, bool reset);

auto has_override(u64 tid) -> bool;
void reset_override(u64 tid);
void reset_all_override();

}
