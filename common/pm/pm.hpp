#pragma once

#include <switch.h>
#include <span>

namespace pm {

enum SystemAppletId : u64 {
    SystemAppletId_qlaunch = 0x0100000000001000,
    SystemAppletId_auth = 0x0100000000001001,
    SystemAppletId_cabinet = 0x0100000000001002,
    SystemAppletId_controller = 0x0100000000001003,
    SystemAppletId_dataErase = 0x0100000000001004,
    SystemAppletId_error = 0x0100000000001005,
    SystemAppletId_netConnect = 0x0100000000001006,
    SystemAppletId_playerSelect = 0x0100000000001007,
    SystemAppletId_swkbd = 0x0100000000001008,
    SystemAppletId_miiEdit = 0x0100000000001009,
    SystemAppletId_LibAppletWeb = 0x010000000000100A,
    SystemAppletId_LibAppletShop = 0x010000000000100B,
    SystemAppletId_overlayDisp = 0x010000000000100C,
    SystemAppletId_photoViewer = 0x010000000000100D,
    SystemAppletId_LibAppletOff = 0x010000000000100F,
    SystemAppletId_LibAppletLns = 0x0100000000001010,
    SystemAppletId_LibAppletAuth = 0x0100000000001011,
    SystemAppletId_starter = 0x0100000000001012,
    SystemAppletId_myPage = 0x0100000000001013,
    SystemAppletId_maintenance = 0x0100000000001015,
    SystemAppletId_splay = 0x0100000000001048,
};

struct SystemAppletEntry {
    const char* name;
    u64 id;
    bool hidden;
};

auto Initialize() -> Result;
void Exit();
void getCurrentPidTid(u64* pid_out, u64* tid_out);
auto PollCurrentPidTid(u64* pid_out, u64* tid_out) -> bool;

auto GetSystemAppletList() -> std::span<const SystemAppletEntry>;

}
