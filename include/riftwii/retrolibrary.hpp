// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
#include <vector>
namespace riftwii::retro {
struct Platform { const char* id; const char* title; const char* emulator; const char* extensions; };
const std::vector<Platform>& platforms();
bool accepts(const Platform&, const std::string& path);
struct Launch { std::string program; std::vector<std::string> arguments; std::string error; };
Launch prepare(const Platform&, const std::string& rom, const std::string& n64 = "gln64");
}
