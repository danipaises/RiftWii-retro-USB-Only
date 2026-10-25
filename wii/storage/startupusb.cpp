// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "startupusb.hpp"

#include <gccore.h>

#include "ios_reload.hpp"
#include "usbcatalog.hpp"

namespace riftwii::wii {

StartupUsb MountStartupUsb(bool from_channel, bool warm_restart) {
    StartupUsb out;
    out.diagnostics = "USB startup: " + std::string(from_channel ? "RiftWii channel" : "loader") +
                      (warm_restart ? ", internal restart" : ", no internal restart note") +
                      "; IOS" + std::to_string(IOS_GetVersion()) + " rev " +
                      std::to_string(IOS_GetRevision()) + ".\n";
    if (from_channel || warm_restart) {
        for (int attempt = 1; attempt <= 2; ++attempt) {
            std::string error;
            // A real reload, not a version check or a mount delay. The
            // shared reload routine releases PPC handles before restarting
            // IOS and reinitializes IPC and the USB host afterwards.
            const ReloadResult result = reload_ios(58, error, true);
            if (result == ReloadResult::Terminal) halt_after_terminal_reload();
            out.diagnostics += "USB startup: forced IOS58 reload " + std::to_string(attempt) +
                               " returned " + std::to_string(static_cast<int>(result)) +
                               "; active IOS" + std::to_string(IOS_GetVersion()) + " rev " +
                               std::to_string(IOS_GetRevision()) + "; " + last_reload_detail() +
                               (error.empty() ? "" : "; " + error) + ".\n";
            if (result == ReloadResult::Ok && IOS_GetVersion() == 58) {
                out.ios_refreshed = true;
                break;
            }
        }
        if (!out.ios_refreshed) {
            out.diagnostics += "USB startup: no mount attempted because a fresh IOS58 could not be confirmed.\n";
            return out;
        }
    }
    std::string error;
    out.mounted = mount_usb_app(error);
    out.diagnostics += "USB startup: mount attempt 1 on IOS" + std::to_string(IOS_GetVersion()) +
                       " rev " + std::to_string(IOS_GetRevision()) +
                       (out.mounted ? " succeeded" : " failed") +
                       (error.empty() ? "" : "; " + error) + ".\n";
    return out;
}

}  // namespace riftwii::wii
