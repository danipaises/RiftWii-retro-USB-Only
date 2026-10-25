// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <gccore.h>
#include <ctime>
using u64 = std::uint64_t;
using lwpq_t = unsigned;
using syswd_t = unsigned;
constexpr lwpq_t LWP_TQUEUE_NULL = 0;
constexpr syswd_t SYS_WD_NULL = 0;
s32 LWP_InitQueue(lwpq_t*);
void LWP_CloseQueue(lwpq_t);
void LWP_ThreadSignal(lwpq_t);
void LWP_ThreadSleep(lwpq_t);
s32 SYS_CreateAlarm(syswd_t*);
void SYS_SetAlarm(syswd_t, const timespec*, void (*)(syswd_t, void*), void*);
void SYS_CancelAlarm(syswd_t);
u64 gettime();
inline u64 ticks_to_millisecs(u64 ticks) { return ticks; }
