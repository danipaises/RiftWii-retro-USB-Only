// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "riftwii/storagepaths.hpp"

// Unattended test driver, built only into test builds (make -f Makefile.wii
// TEST=1; AGENTS.md "Testing in Dolphin"). Source commands select the image
// before probe, xml, boot, or launch: `sd <path-or-id> [cios-slot]`,
// `usb <path-or-id> [cios-slot]`, and `disc`. The launch steps it shares with
// the menu are in launchrun.hpp.
namespace riftwii::wii {

inline const char* kAutorunPath() { return RIFTWII_PATH("riftwii/autorun.txt"); }
inline const char* kAutorunLogPath() { return RIFTWII_PATH("riftwii/autorun.log"); }

bool AutorunPresent();
void RunAutorun();

}  // namespace riftwii::wii
