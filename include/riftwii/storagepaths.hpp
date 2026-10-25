// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace riftwii {

enum class PrimaryDevice { None, Sd, Usb };

// Keep the launch device when it is writable. Without launch arguments,
// retain the old SD layout, falling back to a FAT USB volume.
constexpr PrimaryDevice choose_primary_device(bool launched_from_usb, bool sd_mounted, bool usb_mounted) {
    if (launched_from_usb && usb_mounted) return PrimaryDevice::Usb;
    if (sd_mounted) return PrimaryDevice::Sd;
    if (usb_mounted) return PrimaryDevice::Usb;
    return PrimaryDevice::None;
}

}  // namespace riftwii

namespace riftwii::wii {

inline PrimaryDevice g_primary_device = PrimaryDevice::Sd;
inline bool g_secondary_sd_present = false;
// Selected once at startup, before the menu's worker threads begin.
inline void SelectPrimaryStorage(PrimaryDevice device, bool sd_present = false) {
    g_primary_device = device;
    g_secondary_sd_present = sd_present;
}
inline bool PrimaryOnUsb() { return g_primary_device == PrimaryDevice::Usb; }
bool PrimaryStorageMounted();
// Flush and close libfat before an IOS reload or a raw in-game reader.
// Does not close d2x's handle: its virtual disc/runtime still needs it.
void UnmountPrimaryUsb();
bool MountPrimaryAfterReload();
bool MountPrimaryForMenu();
void CreatePrimaryFolders();

}  // namespace riftwii::wii

// Literal paths only. No allocation, no shared scratch buffer and no
// mutable global path initialized before the device was selected. Use
// this for application files, never for a hardware-specific SD reader.
#define RIFTWII_PATH(relative) (::riftwii::wii::PrimaryOnUsb() ? "usb:/" relative : "sd:/" relative)
