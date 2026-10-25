// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
enum { CONF_LANG_JAPANESE, CONF_LANG_ENGLISH, CONF_LANG_GERMAN, CONF_LANG_FRENCH,
       CONF_LANG_SPANISH, CONF_LANG_ITALIAN, CONF_LANG_DUTCH, CONF_LANG_SIMP_CHINESE,
       CONF_LANG_TRAD_CHINESE, CONF_LANG_KOREAN };
constexpr int CONF_ASPECT_16_9 = 1;
inline int CONF_GetLanguage() { return CONF_LANG_ENGLISH; }
inline int CONF_GetAspectRatio() { return 0; }
