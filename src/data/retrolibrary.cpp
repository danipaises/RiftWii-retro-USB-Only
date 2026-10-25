// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "riftwii/retrolibrary.hpp"
#include <algorithm>
#include <cctype>
namespace riftwii::retro {
const std::vector<Platform>& platforms() {
    static const std::vector<Platform> value = {
        {"nes", "NES", "fceugx", ".nes|.fds"},
        {"snes", "SNES", "snes9xgx", ".sfc|.smc"},
        {"gb", "Game Boy", "vbagx", ".gb"},
        {"gbc", "Game Boy Color", "vbagx", ".gbc"},
        {"gba", "Game Boy Advance", "vbagx", ".gba"},
        {"n64", "Nintendo 64", "wii64", ".z64|.n64|.v64"},
        {"master-system", "Master System", "genplusgx", ".sms"},
        {"mega-drive", "Mega Drive", "genplusgx", ".md|.gen|.bin|.smd"},
        {"game-gear", "Game Gear", "genplusgx", ".gg"},
        {"sg1000", "SG-1000", "genplusgx", ".sg"},
        {"sega-cd", "Sega CD", "genplusgx", ".cue|.chd|.iso"},
    };
    return value;
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
    const std::string prefix = std::string("usb:/retro/") + platform.id + "/";
    if (rom.compare(0, prefix.size(), prefix) != 0 || rom.find('\0') != std::string::npos ||
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
