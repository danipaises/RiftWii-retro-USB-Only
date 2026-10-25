# RiftWii USB-Only: experimental development build

Upstream: https://github.com/KakarottoCake/Riftwii

Base inspected: `21172fe07123e8facb2f488332f5c0b174989997` (main, fetched on 2026-10-25).
Development branch: `codex/usb-only`. Nothing was pushed or published.

This is a real modified RiftWii build, not a separate launcher or an SD
emulator. Its first target is the original menu and Wii WBFS/ISO games
with an empty SD slot. There has been no physical Wii test. A compiled DOL,
host tests and a Dolphin menu smoke test do not establish USB or d2x
compatibility on a console.

## What changed

- `include/riftwii/storagepaths.hpp` selects one application device before
  worker threads begin. Launching from USB chooses a mounted FAT USB
  volume even when SD is present. Launching from SD keeps the SD layout;
  missing SD falls back to USB. Application path accessors are evaluated
  when used, not during global initialization.
- `wii/main.cpp` mounts USB before reading application settings and admits
  the original menu with either application volume. `MountStartupSd()`
  remains for optional SD. `StartedFromUsb()` still reads the Homebrew
  Channel's launch argument. `MENU_NEEDS_SD` is reached only when the
  application volume is unavailable; a USB failure has its own recovery
  text, in all six translations.
- Settings, choices, covers, banners, font and theme files, music, history,
  profiles, downloaded names/cheats, report files and ordinary logs use
  the primary volume. Background downloads still finish before launch.
  Menu screenshots are stored there without importing/deleting NAND files.
- `wii/storage/usbcatalog.cpp` exposes its existing USB mount at startup,
  without loading settings before device selection. Raw game scans keep
  their actual device, sector reader and fragment lists. SD game scans
  produce no missing-card error in USB-only mode. Menu USB raw reads and
  libfat writes share a lock around the driver's transfer buffers.
- `wii/storage/primary.cpp` supplies libfat with a separate d2x USB
  interface after an IOS reload. The menu's libogc driver is closed first.
  The existing `/dev/usb2` handle serves both libfat and the game's USB
  paths; unmounting libfat does not close that handle. Only a 512-byte
  sector device is accepted by this new application mount.
- `umsdev.cpp` adds bounded writes using the existing RiftWii UMS protocol:
  aligned 32 KiB MEM2 bounce buffer, 64 sectors per transfer, the existing
  ten-second request deadline and permanent refusal after an unanswered
  request. Read and write reject wrapped sector ranges. No image file is
  deliberately opened for writing.
- `ios_reload.cpp` closes the log, flushes/unmounts the USB application
  volume and releases libogc before forgetting d2x's handle. Menu reloads,
  failed launches and warm restarts use the same ownership rules. An
  optional SD present at startup is remounted through its own driver.
- USB-only mode keeps the normal menu on IOS58. Per-game cIOS selection
  and existing game reload-block logic remain. The original custom menu
  IOS setting is ignored in USB mode; disc/font operations can still
  temporarily request a cIOS.
- The SD hotplug watcher does not discard a USB-backed menu's files when
  the SD slot is empty. In this version, restart to recognize an SD added
  later while using USB as primary storage.
- Two declarations of `udelay` were replaced by local clock-based waits
  for compatibility with libogc 3.2.1, which moved that helper inline.

No literal hardware path was changed into a different device. In
particular, `/dev/sdio/slot0`, `/dev/sdio/sdhc`, raw SD sector lookups,
resident saves and the GameCube card backend still describe real SD.

## Remaining physical-SD dependencies

| Subsystem | Relevant files | USB-only status |
| --- | --- | --- |
| Ordinary Wii WBFS/ISO games | `usbcatalog.cpp`, `launchrun.cpp`, `boot.cpp` | Implemented; requires physical test with existing d2x |
| Standard Riivolution file/folder and memory replacements on USB | `modplan.cpp`, `umsdev.cpp`, resident `RT_KIND_USB` | Existing USB engine retained; untested on Wii here |
| A mod's `<savegame>` or separate/fresh/shared saves | `modplan.cpp`, `d2xsave.cpp`, `d2xnandemu.cpp`, `sdfile.cpp`, `rtfs.c` | Still SD; no-SD launch is refused, without moving the save |
| GameCube games on USB | `gclaunch.cpp`, `plugins/gamecube/gcnative.cpp`, `gc_rt.c`, plugin SD handover API | Still SD for writable memory cards and other runtime state; explicitly refused without SD |
| Wii RVZ on either drive | `prepare_rvz_launch`, `rvz_resident_options` in `usbcatalog.cpp` | Still SD for the virtual disc stub and runtime group table; use WBFS/ISO |
| Native SD-reading code builds (Project+/REX/etc.) | `codebuilds.cpp`, `frontend.cpp` | Plain files on USB retain the upstream refusal |
| Existing virtual-card images on USB | `vsdimage.cpp`, `vsdhook.cpp`, `runtime/vsd` | Upstream implementation retained, not expanded or tested; not a general SD emulator |
| Creating virtual-card images, GC patch preparation, channel emulated NAND | `vsdmake.cpp`, `gcpatch.cpp`, `sharednandfind.cpp`, `nandchannels.cpp` | Original SD-specific paths retained; not promised USB-only |
| Card failure log and pack-aware Reset | `boot.cpp`, `resident.cpp`, `cardlog.bin`, `restart.txt` | Raw SD; pack-aware Reset disabled for primary USB |
| In-game screenshots, crash recorder, Wii Remote recording, Message Board play log | hooks and `boot.cpp` | NAND-writing features disabled for primary USB |
| Channel installer | `channel.cpp`, `OfferChannelOnce` | Disabled for primary USB; no WAD installer is bundled |
| Official in-app executable update | `online.cpp` | Refused for primary USB: it would replace this fork with the SD-dependent official executable |
| Cover/name/cheat and community-theme downloads | menu/online sources | Redirected to USB; network services not verified end to end |

The GameCube USB reader already exists, but simply changing its card-file
paths would still give its runtime SD sector numbers and an SDIO handle.
Its plugin API, card cache/writeback and crash recovery need a separate
USB card backend. Wii RVZ similarly needs a USB-capable stub/fragment and
group-table handoff. These are unfinished work, not proven hardware
impossibilities.

A new USB save backend would likewise need consistent FAT ownership and
write serialization between the game and the loader. This build does not
pretend the current SD save engine has acquired those properties.

## Storage layout

Copy the `apps` and `riftwii` folders from the installation ZIP to the root
of an existing FAT32 USB drive. Prefer one FAT32 partition and one storage
device for the first test. Do not format or repartition a drive to follow
these instructions. Copy only missing folders/files if settings, saves or
themes already exist.

```
apps/riftwii/boot.dol
apps/riftwii/meta.xml
apps/riftwii/icon.png
apps/riftwii/font.ttf
apps/riftwii/music.ogg
apps/riftwii/plugins/*.rwp
riftwii/settings.txt          created by RiftWii, not supplied over yours
riftwii/choices/
riftwii/themes/
riftwii/covers/
riftwii/banners/
riftwii/logs/                 archived logs
riftwii/session.log           current menu session
riftwii/boot.log              current launch, ending before game handoff
riftwii/history.txt
riftwii/saves/                reserved; redirected game saves still need SD
wbfs/
games/
riivolution/
```

The matching plugins and full font must accompany this DOL. The original
build stamp/API check is preserved. This ZIP includes no games, console
keys, Nintendo assets, cIOS installers or channel installer.

## First physical test

1. Back up the existing `apps/riftwii` folder on your computer, if present.
   Keep existing settings and save files. Do not replace them with empty
   copies. Copy this build's application files and its matching plugins.
2. Leave the Wii's SD slot empty. Connect the FAT32 USB drive before
   starting the Homebrew Channel, preferably in USB port 0. Select the
   USB device in the Homebrew Channel (its device menu is opened with 1)
   and start **RiftWii USB-Only (experimental)**.
3. Verify that the original Home screen opens. If it shows only games
   with mods, press 1 to select all games. First test a normal Wii game
   without mods. Set the menu language to Portuguese, exit cleanly and
   start again; confirm the choice persisted on the pendrive.
4. Check that `riftwii/session.log` exists on the pendrive. Its beginning
   records the launch path, primary storage and IOS. Change the theme and
   view, then restart once more to check file reads and writes.
5. Place your own Wii backup in the normal upstream layout, for example
   `wbfs/Game name [ID6]/ID6.wbfs`, with split `.wbf1` beside it when used.
   FAT32 cannot hold one file of 4 GiB or more. Existing split WBFS support
   is preserved; the ZIP does not convert or supply game images.
6. Use the already-installed d2x cIOS. RiftWii retains automatic selection
   among the available slots; a per-game slot can still be selected. This
   build installs and modifies no IOS/cIOS permanently.
7. Start the game with its normal save setting. Confirm title screen,
   controller response and several minutes of play. The game itself can
   use and update its ordinary NAND save as it always does; this build
   neither migrates nor clears it. Strictly zero NAND writes during normal
   gameplay is not a promise a loader can make for the game or IOS.
8. Exit normally. Check play history and reopen RiftWii with the SD slot
   still empty. If there is a black screen at launch, record the last
   boot.log lines, game ID/format, drive model, USB port and cIOS slot/base.
   Do not repeatedly unplug the drive during an active write.
9. Only after an unmodified game passes, try a USB Riivolution pack. If it
   requires SD save redirection, disable that pack option or keep using
   SD for that feature. Do not treat the refusal as permission to rewrite
   the pack's save engine or move existing saves.

No physical test was performed during development. USB timing, disk
compatibility, the actual cIOS transition, real game launch and actual
save behavior remain acceptance tests for the console owner.

## Build and host verification

The environment used devkitPPC GCC 16.2.0, libogc 3.2.1, libfat-ogc 2.1.0,
and the official devkitPro PowerPC portlibs. See the package's toolchain
manifest and logs for exact package revisions. GNU host C++, CMake, Ninja,
zlib and zstd are also required. The tree's original pinned CI image is
unchanged.

Use a devkitPro installation from https://devkitpro.org/ and install
`wii-dev ppc-freetype ppc-libogg ppc-libvorbisidec ppc-zlib ppc-bzip2`.
Then, from the source root:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC="$DEVKITPRO/devkitPPC"
export PATH="$DEVKITPPC/bin:$DEVKITPRO/tools/bin:$PATH"
cmake -S . -B build-host -G Ninja
cmake --build build-host -j4
ctest --test-dir build-host -j4 --output-on-failure
make -f Makefile.wii -j4
python3 tools/make_plugin.py check --dol build_wii/riftwii-unpacked.dol build-plugins/plugins/*.rwp
```

The supplied source snapshot includes `SOURCE-INFO.json`; its
`tools/build_usb_only.sh` also supports building that exported tree without
Git history, by supplying the original version and stamp to Make.

Apply the patch to a clean checkout of the exact base, on your own branch:

```sh
git clone https://github.com/KakarottoCake/Riftwii.git RiftWii-usb
cd RiftWii-usb
git checkout -b usb-only 21172fe07123e8facb2f488332f5c0b174989997
git apply --check /path/to/riftwii-usb-only.patch
git apply /path/to/riftwii-usb-only.patch
```

Commit locally before building a distinct build stamp. There is no need to
push, publish or modify the official repository.

## Evidence and limits

- 117 host suites pass, including four added USB-related checks: the real
  UMS client's chunked writes/deadlines with fake IOS, primary mount driver
  transitions, actual settings persistence/failure recovery, and the
  existing GameTDB cache-corruption test repeated with USB paths.
- The five relevant host suites also pass with AddressSanitizer and UBSan.
  Leak detection could not run in this environment because LeakSanitizer
  cannot inspect `/proc/<pid>/task`; it was disabled for that retry. The
  initial LeakSanitizer failures are retained in the evidence.
- The release DOL builds and its matching plugins pass the built-in stamp,
  API and size checks. Existing expected vendor warnings are documented
  by upstream. Neither the release nor its plugins are copied from an
  official binary: they were built from this modified source.
- Dolphin 2609-119 boots the DOL with no virtual SD and no USB device. The
  original renderer reaches the new no-readable-storage recovery screen,
  and exits normally. The screenshot is included. This is a startup/error
  screen check, not proof of the USB Home screen or a USB game launch.
- Dolphin's libusb initialization is unavailable in this environment. No
  real USB drive was passed through, no game/console dump was supplied,
  and no d2x USB launch was run in Dolphin.
- Translations, project rules, Git whitespace and patch application are
  checked separately. Full results, SHA-256 and source commit are in the
  package manifest and test report.

Some unchanged help text still says `sd:/` for application folders. In
USB mode the application-owned I/O above uses `usb:/`; genuine SD-only
features in this table remain SD-only. USB hot removal, multiple USB
partitions and custom menu cIOS are outside the first physical test.
