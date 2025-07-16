#include "pm.hpp"

namespace pm {
namespace {

u64 CURRENT_TITLE_ID{};
u64 CURRENT_PROCESS_ID{};
PdmPlayStatistics CURRENT_PLAY_STATS{};
PdmAppletEvent CURRENT_PLAY_EVENT{};
u64 LOST_FOCUS_EXPIRE_NS{};

constexpr SystemAppletEntry SYSTEM_APPLET_IDS[] = {
    { "Home menu", SystemAppletId_qlaunch, true },
    { "Eshop", SystemAppletId_LibAppletShop, false },
    { "Album", SystemAppletId_photoViewer, false },
    { "Error screen", SystemAppletId_error, false },
};

// array of ids to ignore when the application goes out of focus
// due to one of these applets being launched, ie the application
// launching the web browser should not start playing qlaunch music.
constexpr u64 IGNORE_APPLET_IDS[] = {
    SystemAppletId_auth,
    SystemAppletId_controller,
    SystemAppletId_netConnect,
    SystemAppletId_playerSelect,
    SystemAppletId_swkbd,
    SystemAppletId_miiEdit,
    SystemAppletId_LibAppletWeb,
};

}

auto Initialize() -> Result {
    Result rc;
    if (R_FAILED(rc = pdmqryInitialize())) {
        return rc;
    }

    if (R_FAILED(rc = pmdmntInitialize())) {
        return rc;
    }

    return pminfoInitialize();
}

void Exit() {
    pminfoExit();
    pmdmntExit();
    pdmqryExit();
}

void SetPidTidToQlaunch(u64* pid_out, u64* tid_out) {
    *tid_out = SystemAppletId_qlaunch;
    pmdmntGetProcessId(pid_out, SystemAppletId_qlaunch);
}

// SOURCE: https://github.com/retronx-team/sys-clk/blob/570f1e5fe10b253eff0c8fda1bb893bb620af052/sysmodule/src/process_management.cpp#L37
void getCurrentPidTid(u64* pid_out, u64* tid_out) {
    *tid_out = CURRENT_TITLE_ID;
    *pid_out = CURRENT_PROCESS_ID;

    // check if one of the system applets is active.
    for (auto& e : GetSystemAppletList()) {
        if (!e.hidden && R_SUCCEEDED(pmdmntGetProcessId(pid_out, e.id))) {
            *tid_out = e.id;
            return;
        }
    }

    Result rc{};
    if (R_SUCCEEDED(rc = pmdmntGetApplicationProcessId(pid_out))) {
        if (0x20f == pminfoGetProgramId(tid_out, *pid_out)) {
            SetPidTidToQlaunch(pid_out, tid_out);
        } else {
            // check if we have focus, if not, report as qlaunch.
            PdmPlayStatistics stats;
            if (R_SUCCEEDED(pdmqryQueryPlayStatisticsByApplicationId(*tid_out, true, &stats))) {
                if (stats.program_id != CURRENT_PLAY_STATS.program_id || stats.last_entry_index != CURRENT_PLAY_STATS.last_entry_index) {
                    CURRENT_PLAY_STATS = stats;

                    s32 total;
                    if (R_SUCCEEDED(pdmqryQueryAppletEvent(stats.last_entry_index, true, &CURRENT_PLAY_EVENT, 1, &total)) && total) {
                        if (CURRENT_PLAY_EVENT.event_type != PdmAppletEventType_InFocus) {
                            // check if we lost focus because of an applet being launched.
                            for (auto id : IGNORE_APPLET_IDS) {
                                u64 temp_pid;
                                if (R_SUCCEEDED(pmdmntGetProcessId(&temp_pid, id))) {
                                    CURRENT_PLAY_EVENT.event_type = PdmAppletEventType_InFocus;
                                    return;
                                }
                            }

                            // delay before reporting as qlaunch, workaround nro launching triggering events.
                            // todo: make configurable in config.
                            LOST_FOCUS_EXPIRE_NS = armTicksToNs(armGetSystemTick()) + 5e+8;
                        }
                    }
                } else if (CURRENT_PLAY_EVENT.event_type != PdmAppletEventType_InFocus) {
                    const auto now = armTicksToNs(armGetSystemTick());
                    if (now >= LOST_FOCUS_EXPIRE_NS) {
                        SetPidTidToQlaunch(pid_out, tid_out);
                    }
                }
            }
        }
    } else if (rc == 0x20f) {
        SetPidTidToQlaunch(pid_out, tid_out);
    } else {
        *tid_out = CURRENT_TITLE_ID;
        *pid_out = CURRENT_PROCESS_ID;
    }
}

auto PollCurrentPidTid(u64* pid_out, u64* tid_out) -> bool {
    getCurrentPidTid(pid_out, tid_out);

    if (*tid_out != CURRENT_TITLE_ID || *pid_out != CURRENT_PROCESS_ID) {
        CURRENT_TITLE_ID = *tid_out;
        CURRENT_PROCESS_ID = *pid_out;
        return true;
    }

    return false;
}

auto GetSystemAppletList() -> std::span<const SystemAppletEntry> {
    return SYSTEM_APPLET_IDS;
}

}
