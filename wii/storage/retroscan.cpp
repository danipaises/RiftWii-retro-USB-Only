// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "retroscan.hpp"
#include "riftwii/retrolibrary.hpp"
#include "riftwii/checkedalloc.hpp"
#include "textfile.hpp"
#include "log.hpp"
#include <dirent.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <strings.h>
#include <set>
namespace riftwii::wii {
namespace {
bool file(const std::string& path) {
    struct stat st {};
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0;
}
void scan_folder(const retro::Platform& platform, const std::string& folder, unsigned depth,
                 const std::string& n64, std::set<std::string>& seen, std::vector<HomebrewApp>& apps,
                 std::size_t& platform_count, std::size_t& platform_bytes) {
    constexpr std::size_t kPlatformLimit = 2048;
    constexpr std::size_t kPlatformBytes = 2 * 1024 * 1024;
    if (platform_count >= kPlatformLimit || platform_bytes >= kPlatformBytes) return;
    DIR* dir = opendir(folder.c_str());
    if (!dir) {
        logf("Retro scan: %s: %s (missing optional folders are allowed)\n", folder.c_str(), std::strerror(errno));
        return;
    }
    unsigned found = 0;
    while (dirent* entry = readdir(dir)) {
        const std::string name = entry->d_name;
        if (name.empty() || name[0] == '.') continue;
        const std::string rom = folder + "/" + name;
        struct stat st {};
        if (lstat(rom.c_str(), &st) != 0 || S_ISLNK(st.st_mode)) continue;
        if (S_ISDIR(st.st_mode)) {
            if (depth < 6 && rom.size() < 200) scan_folder(platform, rom, depth + 1, n64, seen, apps, platform_count, platform_bytes);
            continue;
        }
        if (!S_ISREG(st.st_mode) || st.st_size <= 0 || !retro::accepts(platform, name)) continue;
        // A ZIP in a shared VBA folder cannot identify GB/GBC/GBA reliably.
        if (rom.compare(0, 16, "usb:/vbagx/roms/") == 0 && name.size() >= 4 &&
            strcasecmp(name.c_str() + name.size() - 4, ".zip") == 0) continue;
        const auto key = retro::stable_key(rom, true);
        if (!seen.insert(key).second) continue;
        // Separate budgets prevent a large NES collection from starving SNES/GBA.
        // Estimate strings, arguments and allocator overhead as well as the row.
        const std::size_t bytes = sizeof(HomebrewApp) + 1024 + rom.size() * 6 + name.size() * 2;
        if (platform_count >= kPlatformLimit || bytes > kPlatformBytes - platform_bytes) {
            logf("Retro scan: %s reached its per-console budget; other consoles will still be read\n", platform.id);
            platform_bytes = kPlatformBytes;
            break;
        }
        if (apps.size() == apps.capacity() &&
            (!riftwii::have_room(4 * 1024 * 1024) || !riftwii::try_reserve(apps, apps.size() + 32))) {
            logf("Retro scan: not enough memory for %s; retaining existing entries\n", platform.id);
            break;
        }
        const auto launch = retro::prepare(platform, rom, n64);
        HomebrewApp app;
        app.name = name.substr(0, name.find_last_of('.'));
        app.folder = app.retro_rom = rom;
        app.library_key = key;
        app.platform = platform.id;
        app.path = app.program = launch.program;
        app.device = platform.title;
        app.arguments = launch.arguments;
        app.problem = launch.error;
        if (app.problem.empty() && !file(app.path)) app.problem = "The emulator is missing from USB";
        if (app.platform == "sega-cd" && (!file("usb:/genplus/bios/bios_CD_U.bin") ||
            !file("usb:/genplus/bios/bios_CD_E.bin") || !file("usb:/genplus/bios/bios_CD_J.bin")))
            app.problem = "Sega CD requires user-provided regional BIOS files in usb:/genplus/bios";
        logf("Retro scan: %s: %s; %s\n", platform.id, rom.c_str(), app.problem.empty() ? "ready for launch" : app.problem.c_str());
        apps.push_back(std::move(app));
        ++platform_count;
        platform_bytes += bytes;
        ++found;
    }
    closedir(dir);
    logf("Retro scan: %s: %u direct ROM(s)\n", folder.c_str(), found);
}
}
void AppendRetroApps(std::vector<HomebrewApp>& apps) {
    std::string n64 = "gln64", selected;
    if (ReadTextFile("usb:/retro/n64/emulator.txt", selected)) {
        while (!selected.empty() && (selected.back() == '\n' || selected.back() == '\r')) selected.pop_back();
        n64 = selected;
    }
    std::set<std::string> seen;
    for (const auto& platform : retro::platforms()) {
        std::size_t count = 0, bytes = 0;
        for (const auto& root : retro::roots(platform)) scan_folder(platform, root, 0, n64, seen, apps, count, bytes);
        logf("Retro scan: %s total %u ROMs, estimated %u KiB of its budget\n", platform.id,
             static_cast<unsigned>(count), static_cast<unsigned>(bytes / 1024));
    }
}
}
