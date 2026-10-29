// SPDX-FileCopyrightText: 2026 RiftWii contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "riftwii/retrolibrary.hpp"
#include "riftwii/appmeta.hpp"
#include <iostream>
int main() {
    using namespace riftwii::retro;
    int failures = 0;
    const auto check = [&](bool ok) { if (!ok) ++failures; };
    check(platforms().size() == 11);
    for (const auto& p : platforms()) {
        const std::string extensions=p.extensions;
        const auto ext=extensions.substr(0,extensions.find('|'));
        const std::string rom=std::string("usb:/retro/")+p.id+"/Ação & espaço ' teste"+ext;
        check(accepts(p,rom));
        auto launch=prepare(p,rom);
        check(launch.error.empty());
        check(launch.program.find("usb:/apps/")==0);
        const auto line=riftwii::app_command_line(launch.program,launch.arguments);
        check(!line.empty());
        check(line.find("Ação & espaço ' teste")!=std::string::npos);
        check(!prepare(p,"sd:/retro/game"+ext).error.empty());
        check(!prepare(p,std::string("usb:/retro/")+p.id+"/../bad"+ext).error.empty());
        check(!prepare(p,rom+std::string(1,'\0')).error.empty());
    }
    check(prepare(platforms()[5],"usb:/retro/n64/a.v64","rice").program=="usb:/apps/wii64-rice/boot.dol");
    check(!prepare(platforms()[5],"usb:/retro/n64/a.z64","not64").error.empty());
    check(accepts(platforms()[0],"a.ZIP"));
    check(is_emulator("USB:/APPS/FCEUGX/BOOT.DOL"));
    check(!is_emulator("usb:/apps/myhomebrew/boot.dol"));
    // Regression: ROM and emulator share a DOL, but only the ROM is a game.
    for (const auto& p : platforms()) {
        const std::string rom = std::string("usb:/retro/") + p.id + "/Test" + std::string(p.extensions).substr(0, std::string(p.extensions).find('|'));
        const auto launch = prepare(p, rom);
        check(is_emulator(launch.program));
        check(is_library_game(rom));
        check(!is_library_game(""));
        check(included_in_all(p.id, "all"));
        check(!included_in_all(p.id, "gamecube"));
    }
    check(included_in_all("gamecube", "gamecube"));
    check(!included_in_all("wii", "gamecube"));
    check(included_in_all("nes", "nes,snes"));
    check(!included_in_all("snes", "nes"));
    check(!included_in_all("homebrew", "all"));
    check(!included_in_all("unknown", "all"));
    check(!included_in_all("gamecube", "none"));
    check(included_in_all("wii", ""));
    const auto key = stable_key("usb:/retro/nes/Ação.nes", true);
    check(valid_library_key(key));
    check(key != stable_key("usb:/retro/nes/My Ação.nes", true));
    check(key == stable_key("USB:/RETRO/NES/Ação.NES", true));
    check(prepare(platforms()[0],"usb:/fceugx/roms/sub/test.nes").error.empty());
    check(!prepare(platforms()[0],"usb:/retro/nes/..\\bad.nes").error.empty());
    std::cout<<"retrolibrary: "<<failures<<" failure(s)\n";
    return failures?1:0;
}
