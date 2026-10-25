// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "retroscan.hpp"
#include "riftwii/retrolibrary.hpp"
#include "textfile.hpp"
#include "log.hpp"
#include <dirent.h>
#include <sys/stat.h>
namespace riftwii::wii {
namespace {
bool file(const std::string& path) {
    struct stat st {};
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0;
}
}
void AppendRetroApps(std::vector<HomebrewApp>& apps) {
    std::string n64 = "gln64", selected;
    if (ReadTextFile("usb:/retro/n64/emulator.txt", selected)) {
        while (!selected.empty() && (selected.back() == '\n' || selected.back() == '\r')) selected.pop_back();
        n64 = selected;
    }
    for (const auto& platform : retro::platforms()) {
        const std::string folder = std::string("usb:/retro/") + platform.id;
        DIR* dir = opendir(folder.c_str());
        if (!dir) continue;
        unsigned count = 0;
        while (dirent* entry = readdir(dir)) {
            const std::string name = entry->d_name;
            const std::string rom = folder + "/" + name;
            if (name.empty() || name[0] == '.' || !retro::accepts(platform, name) || !file(rom)) continue;
            if (apps.size() >= 512) break;
            const auto launch = retro::prepare(platform, rom, n64);
            HomebrewApp app;
            app.name = std::string("[") + platform.title + "] " + name.substr(0, name.find_last_of('.'));
            app.folder = rom; // Stable per-ROM key for existing favorite/search UI and cover cache.
            app.path = app.program = launch.program;
            app.device = platform.title;
            app.arguments = launch.arguments;
            app.retro_rom = rom;
            app.problem = launch.error;
            if (app.problem.empty() && !file(app.path)) app.problem = "The emulator is missing from USB";
            if (std::string(platform.id) == "sega-cd" &&
                (!file("usb:/genplus/bios/bios_CD_U.bin") || !file("usb:/genplus/bios/bios_CD_E.bin") ||
                 !file("usb:/genplus/bios/bios_CD_J.bin")))
                app.problem = "Sega CD requires user-provided regional BIOS files in usb:/genplus/bios";
            apps.push_back(std::move(app));
            ++count;
        }
        closedir(dir);
        logf("Retro: %s has %u ROM(s); total catalog capped at 512 entries\n", platform.title, count);
    }
}
}
