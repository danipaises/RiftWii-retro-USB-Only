// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
#include <vector>
namespace riftwii::retro {
struct Platform { const char* id; const char* title; const char* emulator; const char* extensions; };
const std::vector<Platform>& platforms();
std::vector<std::string> roots(const Platform&);
std::string stable_key(const std::string& path, bool rom);
bool valid_library_key(const std::string&);
bool is_emulator(const std::string&);
// A ROM remains a game even when it launches the same DOL as an emulator app.
bool is_library_game(const std::string& rom);
bool included_in_all(const std::string& platform, const std::string& selection);
bool accepts(const Platform&, const std::string& path);
struct Launch { std::string program; std::vector<std::string> arguments; std::string error; };
Launch prepare(const Platform&, const std::string& rom, const std::string& n64 = "gln64");
}
