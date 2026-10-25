// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#define _CPU_ISR_Disable(level) do { (level) = 0; } while (0)
#define _CPU_ISR_Restore(level) do { (void)(level); } while (0)
inline void DCFlushRange(const void*, unsigned) {}
inline void DCInvalidateRange(void*, unsigned) {}
