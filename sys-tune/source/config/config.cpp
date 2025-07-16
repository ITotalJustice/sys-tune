#include "config.hpp"
#include "sdmc/sdmc.hpp"
#include "minIni/minIni.h"
#include <cstdio>

namespace config {

namespace {

const char CONFIG_PATH[]{"/config/sys-tune/config.ini"};
const char OVERRIDE_PATH[]{"/config/sys-tune/override.ini"};

void create_config_dir() {
    /* Creating directory on every set call looks sus, but the user may delete the dir */
    /* whilst the sys-mod is running and then any changes made via the overlay */
    /* is lost, which sucks. */
    sdmc::CreateFolder("/config");
    sdmc::CreateFolder("/config/sys-tune");
}

auto get_tid_str(u64 tid) -> const char* {
    static char buf[21]{};
    std::sprintf(buf, "%016lX", tid);
    return buf;
}

}

auto get_shuffle() -> bool {
    return ini_getbool("config", "shuffle", false, CONFIG_PATH);
}

void set_shuffle(bool value) {
    create_config_dir();
    ini_putl("config", "shuffle", value, CONFIG_PATH);
}

auto get_repeat() -> int {
    return ini_getl("config", "repeat", 1, CONFIG_PATH);
}

void set_repeat(int value) {
    create_config_dir();
    ini_putl("config", "repeat", value, CONFIG_PATH);
}

auto get_volume() -> float {
    return ini_getf("config", "volume", 1.f, CONFIG_PATH);
}

void set_volume(float value) {
    create_config_dir();
    ini_putf("config", "volume", value, CONFIG_PATH);
}

auto get_title_play_default() -> bool {
    return ini_getbool("config", "play", true, CONFIG_PATH);
}

void set_title_play_default(bool value) {
    create_config_dir();
    ini_putl("config", "play", value, CONFIG_PATH);
}

auto get_default_title_volume() -> float {
    return ini_getf("config", "global_volume", 1.f, CONFIG_PATH);
}

void set_default_title_volume(float value) {
    create_config_dir();
    ini_putf("config", "global_volume", value, CONFIG_PATH);
}

auto get_load_path(char* out, int max_len) -> int {
    return ini_gets("config", "load_path", "", out, max_len, CONFIG_PATH);
}

void set_load_path(const char* path) {
    create_config_dir();
    ini_puts("config", "load_path", path, CONFIG_PATH);
}

/* Overrides  */
auto has_tune_play_override(u64 tid) -> bool {
    return ini_haskey(get_tid_str(tid), "play", OVERRIDE_PATH);
}

auto get_tune_play_override(u64 tid) -> bool {
    return ini_getbool(get_tid_str(tid), "play", true, OVERRIDE_PATH);
}

void set_tune_play_override(u64 tid, bool value, bool reset) {
    if (reset) {
        ini_puts(get_tid_str(tid), "play", NULL, OVERRIDE_PATH);
    } else {
        ini_putl(get_tid_str(tid), "play", value, OVERRIDE_PATH);
    }
}

auto has_tune_volume_override(u64 tid) -> bool {
    return ini_haskey(get_tid_str(tid), "tune_volume", OVERRIDE_PATH);
}

auto get_tune_volume_override(u64 tid) -> float {
    return ini_getf(get_tid_str(tid), "tune_volume", 1.f, OVERRIDE_PATH);
}

void set_tune_volume_override(u64 tid, float value, bool reset) {
    if (reset) {
        ini_puts(get_tid_str(tid), "tune_volume", NULL, OVERRIDE_PATH);
    } else {
        ini_putf(get_tid_str(tid), "tune_volume", value, OVERRIDE_PATH);
    }
}

auto has_title_volume_override(u64 tid) -> bool {
    return ini_haskey(get_tid_str(tid), "title_volume", OVERRIDE_PATH);
}

auto get_title_volume_override(u64 tid) -> float {
    return ini_getf(get_tid_str(tid), "title_volume", 1.f, OVERRIDE_PATH);
}

void set_title_volume_override(u64 tid, float value, bool reset) {
    if (reset) {
        ini_puts(get_tid_str(tid), "title_volume", NULL, OVERRIDE_PATH);
    } else {
        ini_putf(get_tid_str(tid), "title_volume", value, OVERRIDE_PATH);
    }
}

auto has_title_music_override(u64 tid) -> bool {
    return ini_haskey(get_tid_str(tid), "music", OVERRIDE_PATH);
}

auto get_title_music_override(u64 tid, char* out, int max_len) -> int {
    return ini_gets(get_tid_str(tid), "music", "", out, max_len, OVERRIDE_PATH);
}

void set_title_music_override(u64 tid, const char* value, bool reset) {
    if (reset) {
        ini_puts(get_tid_str(tid), "music", NULL, OVERRIDE_PATH);
    } else {
        ini_puts(get_tid_str(tid), "music", value, OVERRIDE_PATH);
    }
}

// todo: use ini_browse here because its much faster than query each entry
auto has_override(u64 tid) -> bool {
    return has_tune_play_override(tid) || has_tune_volume_override(tid) || has_title_volume_override(tid) || has_title_music_override(tid);
}

void reset_override(u64 tid) {
    ini_puts(get_tid_str(tid), NULL, NULL, OVERRIDE_PATH);
}

void reset_all_override() {
    sdmc::DeleteFile(OVERRIDE_PATH);
}

}
