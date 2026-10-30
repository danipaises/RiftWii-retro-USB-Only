// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "retroscan.hpp"
#include "riftwii/retrolibrary.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
namespace riftwii::wii { void logf(const char*, ...) {} }
int main() {
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path()/"riftwii-retroscan-fixture";
    fs::remove_all(root); fs::create_directories(root); const auto before=fs::current_path();fs::current_path(root);
    std::vector<riftwii::wii::HomebrewApp> apps;
    riftwii::wii::AppendRetroApps(apps);
    int failed=!apps.empty();
    fs::create_directories("usb:/retro/nes");
    std::ofstream("usb:/retro/nes/Teste com ação.nes")<<"synthetic scanner fixture";
    riftwii::wii::AppendRetroApps(apps);
    failed+=apps.size()!=1 || apps[0].problem.empty();
    apps.clear();fs::create_directories("usb:/apps/fceugx");std::ofstream("usb:/apps/fceugx/boot.dol")<<"fixture";
    riftwii::wii::AppendRetroApps(apps);
    failed+=apps.size()!=1 || !apps[0].problem.empty() || apps[0].arguments.size()!=2;
    const auto key = apps[0].library_key;
    fs::create_directories("usb:/retro/nes/subfolder");
    std::ofstream("usb:/retro/nes/subfolder/Novo.ZIP") << "synthetic zip detection fixture";
    fs::create_directories("usb:/fceugx/roms/collection");
    std::ofstream("usb:/fceugx/roms/collection/Legacy.NES") << "fixture";
    apps.clear();
    riftwii::wii::AppendRetroApps(apps);
    failed += apps.size() != 3;
    std::set<std::string> keys;
    for (const auto& app : apps) {
        keys.insert(app.library_key);
        // This is the actual catalog result: sharing FCEUGX's program must
        // not remove these ROMs from the games view.
        failed += !riftwii::retro::is_emulator(app.program);
        failed += !riftwii::retro::is_library_game(app.retro_rom);
    }
    failed += keys.size() != 3 || !keys.count(key);
    fs::remove("usb:/retro/nes/Teste com ação.nes");
    apps.clear();
    riftwii::wii::AppendRetroApps(apps);
    failed += apps.size() != 2;
    // Reproduce the hardware report: 26 apps used to leave exactly 486
    // NES slots and zero for SNES/GBA. Actual scanner, actual filesystem.
    for (int i = 0; i < 600; ++i)
        std::ofstream("usb:/retro/nes/Bulk" + std::to_string(i) + ".nes") << "synthetic fixture";
    fs::create_directories("usb:/retro/snes");
    fs::create_directories("usb:/retro/gba");
    std::ofstream("usb:/retro/snes/Test.sfc") << "synthetic fixture";
    std::ofstream("usb:/retro/gba/Test.gba") << "synthetic fixture";
    apps.assign(26, riftwii::wii::HomebrewApp{});
    riftwii::wii::AppendRetroApps(apps);
    unsigned nes = 0, snes = 0, gba = 0;
    for (const auto& app : apps) {
        nes += app.platform == "nes";
        snes += app.platform == "snes";
        gba += app.platform == "gba";
    }
    failed += nes != 602 || snes != 1 || gba != 1;
    // Even a per-console overflow must not hide the following consoles.
    for (int i = 600; i < 2200; ++i)
        std::ofstream("usb:/retro/nes/Bulk" + std::to_string(i) + ".nes") << "synthetic fixture";
    apps.assign(26, riftwii::wii::HomebrewApp{});
    riftwii::wii::AppendRetroApps(apps);
    snes = gba = 0;
    for (const auto& app : apps) { snes += app.platform == "snes"; gba += app.platform == "gba"; }
    failed += snes != 1 || gba != 1;
    fs::current_path(before);fs::remove_all(root);
    std::cout<<"retroscan: "<<failed<<" failure(s)\n";return failed?1:0;
}
