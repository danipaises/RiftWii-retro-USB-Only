#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 RiftWii contributors
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
bootstrap="$PWD"
git init imported
cd imported
git config user.name riftwii
git config user.email riftwii@localhost
git remote add origin "https://github.com/danipaises/RiftWii-retro-USB-Only.git"
git remote add upstream "https://github.com/KakarottoCake/Riftwii.git"
git fetch origin main
git fetch upstream 21172fe07123e8facb2f488332f5c0b174989997
git checkout -b main FETCH_HEAD
git apply "$bootstrap/retro-bootstrap/base.patch"
git apply "$bootstrap/retro-bootstrap/return.patch"
cp "$bootstrap/retro-bootstrap/SOURCE-INFO.json" SOURCE-INFO.json
git add -u
git ls-files --others --exclude-standard -z | git update-index --add -z --stdin
git commit -m 'Recover the tested USB-only source over the original upstream history.'
git tag usb-only-recovered-base
# Keep the publication commit and all original author history as parents.
git merge --allow-unrelated-histories -X ours --no-edit origin/main
git apply --exclude=.github/workflows/retro.yml "$bootstrap/retro-bootstrap/retro.patch"
git add -u
git ls-files --others --exclude-standard -z | git update-index --add -z --stdin
git commit -m 'Add the experimental USB retro scanner and per-emulator launch adapters.'
git -c http.extraheader="AUTHORIZATION: basic $(printf 'x-access-token:%s' "$GH_TOKEN" | base64 -w0)" push origin main refs/tags/usb-only-recovered-base
