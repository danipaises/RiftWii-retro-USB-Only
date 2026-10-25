// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "riftwii/retrolibrary.hpp"
#include "riftwii/sha1.hpp"
#include <algorithm>
#include <cctype>
namespace riftwii::retro {
const std::vector<Platform>& platforms() {
    static const std::vector<Platform> value = {
        {"nes", "NES", "fceugx", ".nes|.fds|.unf|.unif|.zip"},
        {"snes", "SNES", "snes9xgx", ".sfc|.smc|.zip"},
        {"gb", "Game Boy", "vbagx", ".gb|.zip"},
        {"gbc", "Game Boy Color", "vbagx", ".gbc|.zip"},
        {"gba", "Game Boy Advance", "vbagx", ".gba|.zip"},
        {"n64", "Nintendo 64", "wii64", ".z64|.n64|.v64"},
        {"master-system", "Master System", "genplusgx", ".sms"},
        {"mega-drive", "Mega Drive", "genplusgx", ".md|.gen|.bin|.smd"},
        {"game-gear", "Game Gear", "genplusgx", ".gg"},
        {"sg1000", "SG-1000", "genplusgx", ".sg"},
        {"sega-cd", "Sega CD", "genplusgx", ".cue|.chd|.iso"},
    };
    return value;
}
std::vector<std::string> roots(const Platform& p) {
    std::vector<std::string> out{std::string("usb:/retro/") + p.id};
    const std::string id = p.id;
    if (id == "nes") out.push_back("usb:/fceugx/roms");
    if (id == "snes") out.push_back("usb:/snes9xgx/roms");
    if (id == "gb" || id == "gbc" || id == "gba") out.push_back("usb:/vbagx/roms");
    if (id == "n64") out.push_back("usb:/wii64/roms");
    return out;
}
std::string stable_key(const std::string& path, bool rom) {
    std::string canonical = path;
    for (char& c : canonical) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    const auto digest = sha1(reinterpret_cast<const std::uint8_t*>(canonical.data()), canonical.size());
    const char* hex = "0123456789abcdef";
    std::string key = rom ? "retro:" : "app:";
    for (auto byte : digest) { key += hex[byte >> 4]; key += hex[byte & 15]; }
    return key;
}
bool valid_library_key(const std::string& key) {
    const std::size_t prefix = key.compare(0, 6, "retro:") == 0 ? 6 : key.compare(0, 4, "app:") == 0 ? 4 : 0;
    if (!prefix || key.size() != prefix + 40) return false;
    return key.find_first_not_of("0123456789abcdef", prefix) == std::string::npos;
}
bool is_emulator(const std::string& path) {
    std::string folded = path;
    for (char& c : folded) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    for (const char* app : {"fceugx", "snes9xgx", "vbagx", "genplusgx", "wii64", "wii64-gln64", "wii64-rice", "not64"})
        if (folded == std::string("usb:/apps/") + app + "/boot.dol") return true;
    return false;
}
bool accepts(const Platform& platform, const std::string& path) {
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    const std::string extensions = std::string("|") + platform.extensions + "|";
    return extensions.find("|" + ext + "|") != std::string::npos;
}
Launch prepare(const Platform& platform, const std::string& rom, const std::string& n64) {
    Launch out;
    bool root_ok = false;
    for (const auto& root : roots(platform)) {
        const std::string prefix = root + "/";
        if (rom.compare(0, prefix.size(), prefix) == 0) root_ok = true;
    }
    if (!root_ok || rom.find('\0') != std::string::npos || rom.find('\\') != std::string::npos ||
        rom.find("/../") != std::string::npos || rom.find("/./") != std::string::npos || rom.size() > 220 ||
        !accepts(platform, rom)) {
        out.error = "ROM path is invalid for this USB platform";
        return out;
    }
    out.program = std::string("usb:/apps/") + platform.emulator + "/boot.dol";
    if (std::string(platform.id) == "n64") {
        if (n64 != "gln64" && n64 != "rice") {
            out.error = "N64 autoboot adapter is not verified for this emulator";
            return out;
        }
        out.program = "usb:/apps/wii64-" + n64 + "/boot.dol";
        out.arguments = {rom, "NativeDevice=1", "StatesDevice=1", "AutoSave=1"};
    } else {
        const auto slash = rom.find_last_of('/');
        // GX loaders read directory and filename separately. These strings
        // are NUL-separated by app_command_line, never shell-quoted.
        out.arguments = {rom.substr(0, slash + 1), rom.substr(slash + 1)};
        if (std::string(platform.emulator) == "genplusgx") out.arguments.push_back("riftwii");
    }
    return out;
}
}
