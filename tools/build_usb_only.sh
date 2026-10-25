#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 RiftWii contributors
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
cd "$(dirname "$0")/.."
export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
export DEVKITPPC="${DEVKITPPC:-$DEVKITPRO/devkitPPC}"
export PATH="$DEVKITPPC/bin:$DEVKITPRO/tools/bin:$PATH"
cmake -S . -B build-host -G Ninja
cmake --build build-host -j"${JOBS:-4}"
ctest --test-dir build-host -j"${JOBS:-4}" --output-on-failure
build_args=()
if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    if [[ ! -f SOURCE-INFO.json ]]; then
        echo 'An exported source tree needs SOURCE-INFO.json for its version.' >&2
        exit 1
    fi
    mapfile -t build_args < <(python3 - <<'PY'
import json
with open('SOURCE-INFO.json', encoding='utf-8') as file:
    info = json.load(file)
for key, field in [('RIFTWII_COMMITS', 'commits_since_base_tag'), ('RIFTWII_VERSION', 'version'),
                   ('RIFTWII_COMMIT', 'commit_short'), ('RIFTWII_BUILD_STAMP', 'build_stamp')]:
    print(key + '=' + str(info[field]))
PY
)
fi
make -f Makefile.wii -j"${JOBS:-4}" "${build_args[@]}"
python3 tools/make_plugin.py check --dol build_wii/riftwii-unpacked.dol build-plugins/plugins/*.rwp
