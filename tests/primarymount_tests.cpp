// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "riftwii/storagepaths.hpp"
#include "d2xsd.hpp"
#include <fat.h>
#include <gccore.h>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {
int failures = 0;
#define CHECK(c) do { if (!(c)) { std::cerr << __LINE__ << ": " #c "\n"; ++failures; } } while (0)
int ios = 58;
int fd = -1;
bool open_ok = true, stuck = false;
unsigned sector_bytes = 512;
unsigned opens = 0, sd_mounts = 0, menu_mounts = 0, reads = 0, writes = 0;
std::vector<std::string> events;
const DISC_INTERFACE* mounted_io = nullptr;
DISC_INTERFACE sd_io{};
}
s32 IOS_GetVersion() { return ios; }
bool fatMountSimple(const char* name, const DISC_INTERFACE* interface) {
    events.push_back(std::string("mount ") + name);
    if (std::strcmp(name, "sd") == 0) ++sd_mounts;
    mounted_io = interface;
    return true;
}
void fatUnmount(const char* name) { events.push_back(std::string("flush ") + name); mounted_io = nullptr; }
namespace riftwii::wii {
bool libogc_sd_start() { return true; }
const DISC_INTERFACE* sd_interface() { return &sd_io; }
bool mount_usb_app(std::string&) { ++menu_mounts; events.push_back("menu USB"); return true; }
void unmount_usb_games() { events.push_back("release menu volume"); }
}
namespace riftwii::wii::ums {
int Fd() { return fd; }
bool Stuck() { return stuck; }
std::uint32_t SectorBytes() { return sector_bytes; }
bool OpenWaiting(std::string&, int seconds) {
    CHECK(seconds == 20);
    events.push_back("open d2x");
    ++opens;
    if (open_ok) fd = 10 + static_cast<int>(opens);
    return open_ok;
}
bool Read(std::uint64_t, std::uint32_t, std::uint8_t*) { ++reads; return fd >= 0; }
bool Write(std::uint64_t, std::uint32_t, const std::uint8_t*) { ++writes; return fd >= 0; }
}

int main() {
    using namespace riftwii;
    using namespace riftwii::wii;
    for (int from_usb = 0; from_usb < 2; ++from_usb) {
        for (int sd = 0; sd < 2; ++sd) {
            for (int usb = 0; usb < 2; ++usb) {
                const PrimaryDevice choice = choose_primary_device(from_usb, sd, usb);
                CHECK(choice == (usb && (from_usb || !sd) ? PrimaryDevice::Usb : sd ? PrimaryDevice::Sd : PrimaryDevice::None));
            }
        }
    }
    SelectPrimaryStorage(PrimaryDevice::Usb);
    CHECK(std::strcmp(RIFTWII_PATH("riftwii/settings.txt"), "usb:/riftwii/settings.txt") == 0);
    CHECK(std::strcmp(RIFTWII_PATH("apps/riftwii/font.ttf"), "usb:/apps/riftwii/font.ttf") == 0);
    CHECK(MountPrimaryForMenu());
    CHECK(menu_mounts == 1 && opens == 0 && sd_mounts == 0);
    UnmountPrimaryUsb();
    ios = 249;
    events.clear();
    CHECK(MountPrimaryAfterReload());
    CHECK(events.size() == 2 && events[0] == "open d2x" && events[1] == "mount usb");
    CHECK(sd_mounts == 0 && mounted_io != nullptr);
    const int first_fd = fd;
    std::uint8_t sector[512]{};
    CHECK(mounted_io->readSectors(3, 1, sector));
    CHECK(mounted_io->writeSectors(4, 1, sector));
    CHECK(reads == 1 && writes == 1);
    CHECK(MountPrimaryAfterReload() && opens == 1);  // keep the shared handle
    stuck = true;
    CHECK(!mounted_io->writeSectors(4, 1, sector));
    CHECK(writes == 1);
    stuck = false;
    events.clear();
    UnmountPrimaryUsb();
    CHECK(events.front() == "flush usb:");
    CHECK(fd == first_fd);  // runtime/disc handle must survive a libfat unmount
    // What the IOS reload does after the flush: descriptors disappear.
    fd = -1;
    CHECK(MountPrimaryAfterReload());
    CHECK(fd != first_fd && sd_mounts == 0);
    UnmountPrimaryUsb();
    open_ok = false;
    events.clear();
    CHECK(!MountPrimaryAfterReload());
    CHECK(events.size() == 1 && mounted_io == nullptr);
    open_ok = true;
    sector_bytes = 2048;
    CHECK(!MountPrimaryAfterReload());
    CHECK(mounted_io == nullptr);
    // Starting from USB with an SD inserted keeps the two devices
    // separate: application paths still point at USB.
    sector_bytes = 512;
    SelectPrimaryStorage(PrimaryDevice::Usb, true);
    CHECK(MountPrimaryAfterReload() && sd_mounts == 1);
    CHECK(std::strcmp(RIFTWII_PATH("riftwii/settings.txt"), "usb:/riftwii/settings.txt") == 0);
    UnmountPrimaryUsb();
    SelectPrimaryStorage(PrimaryDevice::Sd);
    CHECK(std::strcmp(RIFTWII_PATH("riftwii/settings.txt"), "sd:/riftwii/settings.txt") == 0);
    CHECK(MountPrimaryForMenu() && sd_mounts == 2);
    std::cout << "primarymount: " << failures << " failure(s)\n";
    return failures ? 1 : 0;
}
