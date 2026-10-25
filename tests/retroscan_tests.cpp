// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "retroscan.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
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
    fs::current_path(before);fs::remove_all(root);
    std::cout<<"retroscan: "<<failed<<" failure(s)\n";return failed?1:0;
}
