// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
// The real UMS client against fake IOS: writes must use aligned MEM2,
// bounded chunks and d2x's vector direction, and stop after a timeout.
#include "umsdev.hpp"
#include <gccore.h>
#include <ogc/lwp_watchdog.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

namespace {
int failures = 0;
#define CHECK(c) do { if (!(c)) { std::cerr << __LINE__ << ": " #c "\n"; ++failures; } } while (0)
constexpr s32 kBase = 0x554D5300;
std::vector<std::uint8_t> disk(1024 * 512, 0);
unsigned writes = 0, queued = 0;
bool fail_write = false, timeout_write = false;
void (*alarm_callback)(syswd_t, void*) = nullptr;
void* alarm_user = nullptr;
ipccallback pending_callback = nullptr;
void* pending_user = nullptr;
std::uint8_t* bounce = nullptr;
}
s32 LWP_InitQueue(lwpq_t* q) { *q = 1; return 0; }
void LWP_CloseQueue(lwpq_t) {}
void LWP_ThreadSignal(lwpq_t) {}
void LWP_ThreadSleep(lwpq_t) { CHECK(alarm_callback); alarm_callback(1, alarm_user); }
s32 SYS_CreateAlarm(syswd_t* w) { *w = 1; return 0; }
void SYS_SetAlarm(syswd_t, const timespec* t, void (*cb)(syswd_t, void*), void* user) {
    CHECK(t->tv_sec == 10);
    alarm_callback = cb;
    alarm_user = user;
}
void SYS_CancelAlarm(syswd_t) {}
u64 gettime() { return 1; }
s32 IOS_GetVersion() { return 249; }
s32 IOS_OpenAsync(const char* path, u32, ipccallback cb, void* user) {
    CHECK(std::strcmp(path, "/dev/usb2") == 0);
    cb(12, user);
    return 0;
}
s32 IOS_Close(s32 fd) { CHECK(fd == 12); return 0; }
s32 IOS_IoctlvAsync(s32 fd, s32 command, s32 in, s32 out, ioctlv* vec, ipccallback cb, void* user) {
    ++queued;
    CHECK(fd == 12);
    s32 result = 0;
    if (command == kBase + 2) {
        CHECK(in == 0 && out == 1);
        *static_cast<u32*>(vec[0].data) = 512;
        result = 1024;
    } else if (command == kBase + 3 || command == kBase + 4) {
        CHECK(in == 2 && out == 1);
        CHECK(vec[0].len == 4 && vec[1].len == 4);
        CHECK(reinterpret_cast<std::uintptr_t>(vec[2].data) % 32 == 0);
        CHECK(vec[2].data == bounce);
        const auto sector = *static_cast<const u32*>(vec[0].data);
        const auto count = *static_cast<const u32*>(vec[1].data);
        CHECK(count > 0 && count <= 64 && vec[2].len == count * 512);
        CHECK(sector < 1024 && count <= 1024 - sector);
        if (command == kBase + 4) {
            ++writes;
            if (timeout_write) { pending_callback = cb; pending_user = user; return 0; }
            if (fail_write) result = -5;
            else std::memcpy(disk.data() + sector * 512, vec[2].data, count * 512);
        } else {
            std::memcpy(vec[2].data, disk.data() + sector * 512, count * 512);
        }
    } else {
        CHECK(command == kBase + 1 || command == kBase + 0x83);
    }
    cb(result, user);
    return 0;
}
namespace riftwii::wii::skin {
std::uint8_t* Mem2Alloc(std::size_t bytes) {
    void* memory = nullptr;
    if (posix_memalign(&memory, 32, bytes) != 0) return nullptr;
    bounce = static_cast<std::uint8_t*>(memory);
    return bounce;
}
}
namespace riftwii::wii {
void logf(const char*, ...) {}
void release_usb_driver() {}
bool usb_picked_drive(std::uint32_t&, std::uint64_t&) { return false; }
std::uint64_t usb_sector0_hash(const std::uint8_t*, std::size_t) { return 1; }
std::vector<std::string> usb_wanted_folders() { return {}; }
}

int main() {
    using namespace riftwii::wii;
    disk[510] = 0x55; disk[511] = 0xAA;
    std::string error;
    CHECK(ums::Open(error));
    std::vector<std::uint8_t> data(129 * 512 + 1);
    for (std::size_t i = 1; i < data.size(); ++i) data[i] = static_cast<std::uint8_t>(i * 7);
    CHECK(ums::Write(5, 129, data.data() + 1));
    CHECK(writes == 3);
    CHECK(std::equal(data.begin() + 1, data.end(), disk.begin() + 5 * 512));
    CHECK(disk[4 * 512] == 0 && disk[134 * 512] == 0);
    std::vector<std::uint8_t> back(data.size());
    CHECK(ums::Read(5, 129, back.data() + 1));
    CHECK(data == back);
    const unsigned before_invalid = queued;
    CHECK(!ums::Write(0x100000000ull, 1, data.data()));
    CHECK(!ums::Write(UINT32_MAX - 1, 3, data.data()));
    CHECK(queued == before_invalid);
    fail_write = true;
    CHECK(!ums::Write(5, 1, data.data()));
    fail_write = false;
    timeout_write = true;
    CHECK(!ums::Write(5, 1, data.data()));
    CHECK(ums::Stuck());
    const auto kept = std::vector<std::uint8_t>(bounce, bounce + 512);
    const unsigned before_stuck = queued;
    CHECK(!ums::Write(6, 1, data.data() + 1));
    CHECK(!ums::Read(6, 1, back.data()));
    CHECK(queued == before_stuck);
    CHECK(std::equal(kept.begin(), kept.end(), bounce));  // late IOS reply still owns this buffer
    CHECK(pending_callback != nullptr);
    pending_callback(0, pending_user);
    CHECK(ums::Stuck());
    std::cout << "umswrite: " << failures << " failure(s)\n";
    return failures ? 1 : 0;
}
