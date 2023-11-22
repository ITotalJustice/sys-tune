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
auto get_bass() -> double;
void set_bass(double value);

// tune volume
auto get_volume() -> double;
void set_volume(double value);

// per title tune enable
auto has_title_enabled(u64 tid) -> bool;
auto get_title_enabled(u64 tid) -> bool;
void set_title_enabled(u64 tid, bool value);

// default for tune for every title
auto get_title_enabled_default() -> bool;
void set_title_enabled_default(bool value);

// per title volume
auto has_title_volume(u64 tid) -> bool;
auto get_title_volume(u64 tid) -> double;
void set_title_volume(u64 tid, double value);

// default volume for every title
auto get_default_title_volume() -> double;
void set_default_title_volume(double value);

// returns true if title causes a fatal on launch
auto get_title_blacklist(u64 tid) -> bool;
void set_title_blacklist(u64 tid, bool value);

}
