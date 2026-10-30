// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
namespace riftwii {
struct UsbExitGuard { std::uint32_t magic, image, check; };
inline bool claim_usb_exit(UsbExitGuard& guard, std::uint32_t image) {
    constexpr std::uint32_t magic = 0x52575545;
    if (guard.magic == magic && guard.image == image && guard.check == ~image) return false;
    guard = {magic, image, ~image};
    return true;
}
}
