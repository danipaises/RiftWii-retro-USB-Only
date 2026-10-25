# USB-only channel return recovery

This is a focused change over the USB-only source exported from
`58d9ac5ad499aee289c14a7d5131d36ec168d057`. Normal USB startup and game launches
were reported working on the user's Wii. The reported failure is a readable
storage error after leaving a game for the RiftWii channel.

## Evidence and scope

* `wii/launch/boot.cpp`, `return_title` and `apply_return_to`, send the game
  back to the installed channel through ES. They do not call `WarmRestart`.
* `channel/forwarder/main.c`, `read_dol`, unmounts FAT and calls the selected
  storage interface's shutdown. `main` then calls `dolboot_run(g_dol, g_path)`.
  `channel/common/dolboot.c` passes the DOL path, with no restart note or IOS
  reload. The channel TMD requests IOS58, but the handoff itself does not
  refresh the IOS USB service between forwarder and application.
* The previous USB-only `wii/main.cpp` tried USB immediately. Its call to
  `StartMenuIos` and its later recovery mount used `RestartKind != None` as
  their fresh-start condition. A normal channel launch has no such note.
* `wii/launch/menuios.cpp`, `StartMenuIos`, returns without reloading when
  slot is zero and `fresh` is false. USB menu storage deliberately uses
  slot zero, meaning IOS58. A channel return therefore skipped this reset,
  even after a failed initial USB mount.
* libogc v3.2.1, commit `959871c08ed80d1a63b258bc207218623ea1b0e5`, was
  inspected as a dependency reference. `__IOS_LoadStartupIOS` in
  `libogc/ios.c` is a no-op. Storage startup in `libogc/usbstorage.c`
  initializes USB/storage; it does not reload IOS. Storage shutdown closes
  the device, and is not equivalent to restarting IOS's USB host.

The confirmed code defect is the missing channel-entry recovery branch.
The user's failing IOS number, exact rejected IPC request and descriptor
state have not been captured. An inherited cIOS, or IOS58 with USB service
state surviving the forwarder handoff, are possibilities, not measured
facts. No particular stale numeric handle or mount-delay cause is claimed.

## Change

`StartedByRiftWiiChannel` checks the running ES title against the existing
UFTW and legacy RFTW IDs, before any reload. `MountStartupUsb` forces a fresh
IOS58 for USB channel entries and USB internal restarts, before mounting SD
or USB, opening settings or reading the catalog. It also handles the
no-arguments/no-SD fallback. HBC entries have no additional forced reload.

The existing `reload_ios` performs the cleanup: close logging, flush/unmount
USB, release the menu storage driver, forget the d2x UMS descriptor, close DI,
stop other IOS clients, shut down IOS subsystems, launch IOS58, reinitialize
IPC and initialize subsystems. No second USB driver runs alongside d2x.
A refused reload is retried once; a failed reset is not accepted solely
because IOS_GetVersion still reads 58. A terminal IPC failure halts without
another storage call. No arbitrary sleep or longer FAT retry was added.

Startup diagnostics retain origin, IOS/revision before/after reload, reload
result/detail and FAT mount result in RAM until session.log opens. Low-level
USB setup and later app mounts also log IOS/revision, initialization and
device-list results. If USB remains completely unavailable, it cannot save
a file there; the diagnostic buffer is not persistent across power loss.

The forwarder, its installed channel and IOS/cIOS installations are unchanged.
The new path reloads only IOS58. Hardware access is preserved using the
existing routine only when entering under IOS58; the new path never patches
an inherited custom IOS such as IOS246. The UI, game-launch policy, themes,
mods, saves and runtime are unchanged. No channel installer is distributed
with the fix. The temporary channel build is a compilation check only.

## Validation

See the delivered Portuguese test report and original logs. Host tests use
the real startup coordinator with simulated IOS/USB functions. A dirty IOS58
channel return without a restart note fails the old mount-only sequence and
passes the reset-before-mount sequence. Three simulated return cycles include
inherited IOS249, IOS246 and IOS58. Reload refusal, same-number failure,
terminal IPC failure and real mount failure are covered.

These tests prove ordering and error handling in the covered code, not Wii
hardware behavior. Dolphin cannot validate d2x or the user's USB drive.
The full three-game sequence remains an acceptance test on the physical Wii.
