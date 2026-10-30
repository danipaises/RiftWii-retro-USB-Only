// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "startupusb.hpp"
#include "riftwii/usbexitguard.hpp"
#include "ios_reload.hpp"

#include <gccore.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

namespace {
int failures = 0;
#define CHECK(c) do { if (!(c)) { std::cerr << __LINE__ << ": " #c "\n"; ++failures; } } while (0)
int ios = 58;
bool dirty = false;
bool mount_ok = true;
unsigned reloads = 0;
int refuse_reloads = 0;
bool terminal = false;
std::vector<std::string> events;
std::string detail = "IPC and USB initialized";
}

s32 IOS_GetVersion() { return ios; }
s32 IOS_GetRevision() { return ios == 58 ? 6176 : 65535; }

namespace riftwii::wii {
ReloadResult reload_ios(int target, std::string& error, bool force) {
    events.push_back("reload " + std::to_string(target));
    ++reloads;
    CHECK(target == 58 && force);
    if (terminal) return ReloadResult::Terminal;
    if (refuse_reloads-- > 0) {
        error = "simulated ES failure";
        return ReloadResult::Failed;
    }
    ios = target;
    dirty = false;
    return ReloadResult::Ok;
}
const std::string& last_reload_detail() { return detail; }
[[noreturn]] void halt_after_terminal_reload() { _exit(42); }
bool mount_usb_app(std::string& error) {
    events.push_back("mount");
    CHECK(!terminal);
    if (ios != 58 || dirty || !mount_ok) {
        error = "simulated unreadable USB service";
        return false;
    }
    return true;
}
}

int main() {
    using namespace riftwii::wii;
    riftwii::UsbExitGuard guard{};
    CHECK(riftwii::claim_usb_exit(guard, 100));
    CHECK(!riftwii::claim_usb_exit(guard, 100));
    guard.magic = 0; // USB has mounted: a later return gets its own attempt.
    CHECK(riftwii::claim_usb_exit(guard, 100));
    CHECK(!riftwii::claim_usb_exit(guard, 100));
    CHECK(riftwii::claim_usb_exit(guard, 101));
    // The already-working HBC path stays on its current IOS, with one
    // mount attempt and no extra reload or sleep.
    StartupUsb start = MountStartupUsb(false, false);
    CHECK(start.mounted && !start.ios_refreshed && reloads == 0);
    CHECK(events == std::vector<std::string>{"mount"});

    // Reproduce the old cold-channel path: no restart marker, IOS58 by
    // number, but its USB service still belongs to the previous handoff.
    dirty = true;
    std::string error;
    CHECK(!mount_usb_app(error));
    events.clear();
    start = MountStartupUsb(true, false);
    CHECK(start.mounted && start.ios_refreshed);
    CHECK((events == std::vector<std::string>{"reload 58", "mount"}));
    CHECK(start.diagnostics.find("RiftWii channel, no internal restart note") != std::string::npos);
    CHECK(start.diagnostics.find("mount attempt 1 on IOS58 rev 6176 succeeded") != std::string::npos);

    // Three game/return cycles: every channel start gets a new service.
    // Include the user's Fakemote slot as the inherited IOS. Nothing
    // selects, installs or patches that slot; the only reload target is 58.
    for (int inherited : {249, 246, 58}) {
        ios = inherited;
        dirty = true;
        events.clear();
        start = MountStartupUsb(true, false);
        CHECK(start.mounted && start.ios_refreshed && ios == 58);
        CHECK((events == std::vector<std::string>{"reload 58", "mount"}));
        CHECK(start.diagnostics.find("IOS" + std::to_string(inherited)) != std::string::npos);
    }
    // The internal restart also resets before mounting, even without a
    // channel title. main must not force the same reload a second time.
    ios = 249;
    events.clear();
    start = MountStartupUsb(false, true);
    CHECK(start.mounted && start.ios_refreshed);
    CHECK((events == std::vector<std::string>{"reload 58", "mount"}));

    // A reload that was refused while IOS58 still reads as the active
    // version is not success. Retry the reload, not the FAT mount.
    refuse_reloads = 1;
    dirty = true;
    events.clear();
    start = MountStartupUsb(true, false);
    CHECK(start.mounted && start.ios_refreshed);
    CHECK((events == std::vector<std::string>{"reload 58", "reload 58", "mount"}));
    refuse_reloads = 2;
    dirty = true;
    events.clear();
    start = MountStartupUsb(true, false);
    CHECK(!start.mounted && !start.ios_refreshed);
    CHECK((events == std::vector<std::string>{"reload 58", "reload 58"}));
    CHECK(start.diagnostics.find("no mount attempted") != std::string::npos);

    // A mount failure after a successful reset stays a real mount failure.
    refuse_reloads = 0;
    mount_ok = false;
    events.clear();
    start = MountStartupUsb(true, false);
    CHECK(!start.mounted && start.ios_refreshed);
    CHECK((events == std::vector<std::string>{"reload 58", "mount"}));
    CHECK(start.diagnostics.find("simulated unreadable USB service") != std::string::npos);
    mount_ok = true;

    // Terminal IPC failure must halt without another IOS or storage call.
    const pid_t child = fork();
    CHECK(child >= 0);
    if (child == 0) {
        terminal = true;
        MountStartupUsb(true, false);
        _exit(1);
    }
    if (child > 0) {
        int status = 0;
        CHECK(waitpid(child, &status, 0) == child);
        CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 42);
    }
    std::cout << "startupusb: " << failures << " failure(s)\n";
    return failures ? 1 : 0;
}
