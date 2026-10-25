// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
// Actual settings and stdio code, with separate filesystem roots standing
// in for libfat's mounts. Only the console's language/display is mocked.
#include "loadersettings.hpp"
#include "riftwii/storagepaths.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

namespace riftwii::wii { void logf(const char*, ...) {} }
namespace {
int failures = 0;
#define CHECK(c) do { if (!(c)) { std::cerr << __LINE__ << ": " #c "\n"; ++failures; } } while (0)
std::string read(const char* path) {
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
void write(const char* path, const char* text) { std::ofstream(path, std::ios::binary) << text; }
}
int main() {
    namespace fs = std::filesystem;
    using namespace riftwii;
    using namespace riftwii::wii;
    fs::create_directories("sd:/riftwii/saves");
    fs::create_directories("usb:/riftwii/saves");
    write("sd:/riftwii/settings.txt", "language = es\n");
    write("usb:/riftwii/settings.txt", "language = pt\n");
    write("usb:/riftwii/saves/keep.bin", "existing save");
    const std::string sd_settings = read("sd:/riftwii/settings.txt");
    SelectPrimaryStorage(PrimaryDevice::Sd);
    ReloadSettings();
    CHECK(Settings().language == "es");
    SelectPrimaryStorage(PrimaryDevice::Usb);
    ReloadSettings();
    CHECK(Settings().language == "pt");
    Settings().language = "fr";
    CHECK(SaveSettings());
    ReloadSettings();
    CHECK(Settings().language == "fr");
    CHECK(read("sd:/riftwii/settings.txt") == sd_settings);
    CHECK(read("usb:/riftwii/saves/keep.bin") == "existing save");
    CHECK(!fs::exists("usb:/riftwii/settings.txt.part"));
    const std::string usb_settings = read("usb:/riftwii/settings.txt");
    fs::create_directory("usb:/riftwii/settings.txt.part");
    Settings().language = "it";
    CHECK(!SaveSettings());
    CHECK(read("usb:/riftwii/settings.txt") == usb_settings);
    fs::remove("usb:/riftwii/settings.txt.part");
    fs::rename("usb:/riftwii/settings.txt", "usb:/riftwii/settings.txt.part");
    ReloadSettings();
    CHECK(Settings().language == "fr");  // recover an interrupted preference save
    CHECK(SaveSettings());
    // The actual USB-only case, with no SD directory at all.
    fs::remove_all("sd:");
    ReloadSettings();
    CHECK(Settings().language == "fr" && SaveSettings());
    CHECK(!fs::exists("sd:"));
    CHECK(read("usb:/riftwii/saves/keep.bin") == "existing save");
    fs::remove_all("usb:");
    std::cout << "primarysettings: " << failures << " failure(s)\n";
    return failures ? 1 : 0;
}
