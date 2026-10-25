// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <gccore.h>
constexpr s32 IPC_EINVAL = -4;
constexpr s32 IPC_ENOMEM = -22;
s32 IOS_IoctlvAsync(s32, s32, s32, s32, ioctlv*, ipccallback, void*);
