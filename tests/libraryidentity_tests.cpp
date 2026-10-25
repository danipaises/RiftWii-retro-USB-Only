// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "riftwii/retrolibrary.hpp"
#include "riftwii/playhistory.hpp"
#include "riftwii/settingsfile.hpp"
#include "riftwii/profiles.hpp"
#include <iostream>
int main() {
    const auto first = riftwii::retro::stable_key("usb:/retro/nes/Ação.nes", true);
    const auto other = riftwii::retro::stable_key("usb:/retro/gba/Ação.gba", true);
    riftwii::PlayHistory history;
    history.record(first, 42); history.record(other, 50);
    riftwii::PlayHistory reloaded; reloaded.parse(history.serialize());
    if (!riftwii::valid_tile_key(first) || reloaded.size() != 2 || reloaded.recent()[0] != other) return 1;
    riftwii::LoaderSettings settings;
    settings.favorites.insert(first); settings.favorites.insert(other);
    settings.other["view"] = "all"; settings.other["platform"] = "nes";
    riftwii::LoaderSettings loaded; loaded.parse(settings.serialize());
    if (loaded.favorites != settings.favorites || loaded.other["platform"] != "nes") return 3;
    // Missing files must be filtered by the catalog, never removed from history.
    if (!reloaded.find(first) || first == other) return 2;
    std::cout << "Library identities survive a serialized history and distinguish platforms.\n";
}
