// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "riftwii/storagepaths.hpp"

#include <fat.h>
#include <gccore.h>
#include <ogc/usbstorage.h>
#include <sys/stat.h>

#include "d2xsd.hpp"
#include "umsdev.hpp"
#include "usbcatalog.hpp"

namespace riftwii::wii {
namespace {
bool g_d2x_mounted = false;

bool ready() { return ums::Fd() >= 0 && !ums::Stuck() && ums::SectorBytes() == 512; }
bool read(sec_t sector, sec_t count, void* out) {
    return ready() && ums::Read(sector, count, static_cast<std::uint8_t*>(out));
}
bool write(sec_t sector, sec_t count, const void* data) {
    return ready() && ums::Write(sector, count, static_cast<const std::uint8_t*>(data));
}
bool nothing() { return true; }
const DISC_INTERFACE kD2xUsbIo = {
    DEVICE_TYPE_WII_USB, FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE | FEATURE_WII_USB,
    ready, ready, read, write, nothing, nothing,
};
}  // namespace

bool PrimaryStorageMounted() {
    if (g_primary_device == PrimaryDevice::None) return false;
    struct stat st;
    return stat(RIFTWII_PATH(""), &st) == 0 && S_ISDIR(st.st_mode);
}

void UnmountPrimaryUsb() {
    // Includes a menu mount: usbcatalog's flags must not retain a mount
    // whose IOS is about to disappear. ums::Forget runs only after this.
    if (g_d2x_mounted) fatUnmount("usb:");
    g_d2x_mounted = false;
    unmount_usb_games();
}

bool MountPrimaryAfterReload() {
    if (!PrimaryOnUsb()) return fatMountSimple("sd", sd_interface());
    if (g_d2x_mounted) return ready();
    // An optional SD with games or packs keeps its own real driver. Its
    // absence or a failed mount must never block the USB application.
    if (g_secondary_sd_present) fatMountSimple("sd", sd_interface());
    std::string error;
    if (!ums::OpenWaiting(error, 20) || ums::SectorBytes() != 512) return false;
    g_d2x_mounted = fatMountSimple("usb", &kD2xUsbIo);
    return g_d2x_mounted;
}

bool MountPrimaryForMenu() {
    if (!PrimaryOnUsb()) return libogc_sd_start() && fatMountSimple("sd", sd_interface());
    if (IOS_GetVersion() != 58) return MountPrimaryAfterReload();
    if (g_secondary_sd_present) fatMountSimple("sd", sd_interface());
    std::string error;
    return mount_usb_app(error);
}

void CreatePrimaryFolders() {
    mkdir(RIFTWII_PATH("riftwii"), 0777);
    for (const char* path : {RIFTWII_PATH("riftwii/themes"), RIFTWII_PATH("riftwii/covers"),
                            RIFTWII_PATH("riftwii/banners"), RIFTWII_PATH("riftwii/logs"),
                            RIFTWII_PATH("riftwii/saves"), RIFTWII_PATH("riftwii/choices")}) {
        mkdir(path, 0777);
    }
}
}  // namespace riftwii::wii
