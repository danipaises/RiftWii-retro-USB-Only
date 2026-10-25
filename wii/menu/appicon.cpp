// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "appicon.hpp"
#include "riftwii/retrolibrary.hpp"

#include <gccore.h>
#include <sys/stat.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <set>
#include <vector>

#include "covers.hpp"
#include "log.hpp"
#include "riftwii/canvas.hpp"
#include "riftwii/modart.hpp"
#include "usbcatalog.hpp"

namespace riftwii::wii {
namespace {

constexpr int kIconW = 128;
constexpr int kIconH = 48;
constexpr int kSlots = 24;
constexpr long kMaxPng = 1 << 20;

struct Slot {
    std::string folder;
    u8* data = nullptr;
    unsigned used = 0;
    int w = kIconW, h = kIconH;
    bool ready = false;
};

Slot g_slots[kSlots];
unsigned g_clock = 0;
std::set<std::string> g_failed;

bool read_png(const std::string& path, std::vector<std::uint8_t>& out) {
    if (path.compare(0, 5, "usb:/") == 0) {
        std::uint64_t size = 0;
        std::uint8_t probe;
        if (!usb_volume_ready() || !read_usb_range(path, 0, &probe, 0, size) || size == 0 ||
            size > static_cast<std::uint64_t>(kMaxPng))
            return false;
        out.resize(static_cast<std::size_t>(size));
        return read_usb_range(path, 0, out.data(), out.size(), size);
    }
    struct stat st;
    if (stat(path.c_str(), &st) != 0 || S_ISDIR(st.st_mode) || st.st_size <= 0 || st.st_size > kMaxPng) return false;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) return false;
    out.resize(static_cast<std::size_t>(st.st_size));
    const bool ok = std::fread(out.data(), 1, out.size(), f) == out.size();
    std::fclose(f);
    return ok;
}

}

bool AppIcon(const std::string& folder, skin::Tex& out) {
    out = skin::Tex{};
    if (folder.empty()) return false;
    for (Slot& slot : g_slots) {
        if (slot.ready && slot.folder == folder) {
            slot.used = ++g_clock;
            out.data = slot.data;
            out.w = slot.w;
            out.h = slot.h;
            return true;
        }
    }
    return false;
}

bool AppIconWanted(const std::string& folder) {
    if (folder.empty() || g_failed.count(folder)) return false;
    for (const Slot& slot : g_slots)
        if (slot.ready && slot.folder == folder) return false;
    return true;
}

void LoadAppIcon(const std::string& folder) {
    if (!AppIconWanted(folder)) return;
    Slot* victim = &g_slots[0];
    for (Slot& slot : g_slots)
        if (slot.used < victim->used) victim = &slot;
    g_failed.insert(folder);
    std::vector<std::uint8_t> png, rgba;
    int w = 0, h = 0;
    std::string error;
    const bool rom = folder.find("/apps/") == std::string::npos;
    // Stable-ID covers also work for native emulator ROM directories.
    std::string picture = rom ? "usb:/retro/covers/" + retro::stable_key(folder, true).substr(6) + ".png" : folder + "/icon.png";
    if (!read_png(picture, png) && rom && folder.compare(0, 11, "usb:/retro/") == 0)
        read_png("usb:/retro/covers/" + folder.substr(11) + ".png", png);
    if (png.empty() || !DecodePngRgba(png, rgba, w, h, error)) {
        if (!error.empty()) logf("Homebrew: %s/icon.png: %s\n", folder.c_str(), error.c_str());
        return;
    }
    std::vector<std::uint8_t>().swap(png);
    const int boxW = rom ? 96 : kIconW, boxH = rom ? 128 : kIconH;
    const std::vector<std::uint8_t> box = riftwii::fit_art(rgba.data(), w, h, boxW, boxH);
    std::vector<std::uint8_t>().swap(rgba);
    if (!victim->data) victim->data = skin::Mem2Alloc(static_cast<std::size_t>(96) * 128 * 4);
    if (victim->data == nullptr || !riftwii::to_gx_rgba8(box.data(), boxW, boxH, victim->data)) {
        victim->ready = false;
        return;
    }
    DCFlushRange(victim->data, static_cast<std::size_t>(96) * 128 * 4);
    victim->w = boxW; victim->h = boxH;
    victim->folder = folder;
    victim->used = ++g_clock;
    victim->ready = true;
    g_failed.erase(folder);
}

void ForgetAppIcons() {
    for (Slot& slot : g_slots) {
        slot.folder.clear();
        slot.used = 0;
        slot.ready = false;
    }
    g_clock = 0;
    g_failed.clear();
}

}
