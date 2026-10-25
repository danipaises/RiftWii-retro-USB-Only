// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>

namespace riftwii::wii {

struct StartupUsb {
    bool mounted = false;
    bool ios_refreshed = false;
    // Kept in RAM until session.log can be opened on the mounted drive.
    std::string diagnostics;
};

// Call before mounting SD, opening files or starting menu workers. The
// channel handoff has no WarmRestart note, and closing its storage handle
// does not give this DOL a fresh IOS USB service. A channel entry therefore
// needs a forced IOS58 reload even when the version already reads 58.
StartupUsb MountStartupUsb(bool from_channel, bool warm_restart);

}  // namespace riftwii::wii
