#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 RiftWii contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Validate release layout without executing Wii software."""
import hashlib,json,pathlib,sys,xml.etree.ElementTree as ET,zipfile
APPS=['riftwii','fceugx','snes9xgx','vbagx','genplusgx','wii64-gln64','wii64-rice']
def validate(path):
    with zipfile.ZipFile(path) as z:
        names=z.namelist()
        if len(names)!=len(set(names)):raise ValueError('Duplicate ZIP entry')
        for name in names:
            p=pathlib.PurePosixPath(name)
            if p.is_absolute() or '..' in p.parts or '\\' in name:raise ValueError('Unsafe ZIP path')
            if p.suffix.lower() in {'.wad','.nes','.fds','.unf','.unif','.sfc','.smc','.gb','.gbc','.gba','.z64','.n64','.v64','.iso','.wbfs','.cue','.chd','.gg','.sms','.32x','.smd','.sg'} or p.name.startswith('bios_CD_'):
                raise ValueError('Forbidden ROM, BIOS or installer: '+name)
        for app in APPS:
            dol=z.read('apps/'+app+'/boot.dol')
            if len(dol)<256:raise ValueError('Missing DOL: '+app)
        meta=ET.fromstring(z.read('apps/riftwii/meta.xml'))
        for field in ['name','coder','version','release_date','short_description','long_description']:
            if not meta.findtext(field):raise ValueError('Missing HBC metadata: '+field)
        if meta.tag!='app' or meta.find('ahb_access') is None or meta.find('no_ios_reload') is None:raise ValueError('Invalid HBC flags')
        if not z.read('apps/riftwii/icon.png').startswith(b'\x89PNG\r\n\x1a\n'):raise ValueError('Invalid HBC icon')
        for p in ['apps/riftwii/font.ttf','apps/riftwii/music.ogg','sources/riftwii-source.tar.gz','sources/emulators.lock.json','riftwii/installed-components.json']:
            if not z.read(p):raise ValueError('Empty required resource: '+p)
        if not any(n.startswith('apps/riftwii/plugins/gamecube.') for n in names):raise ValueError('Missing GameCube plugin')
        state=json.loads(z.read('riftwii/installed-components.json'))
        for name,sha in state['files'].items():
            if hashlib.sha256(z.read(name)).hexdigest()!=sha:raise ValueError('Factory record mismatch: '+name)
        print('USB kit validated:',len(state['files']),'managed files;',len(names),'ZIP entries')
if __name__=='__main__':validate(pathlib.Path(sys.argv[1]))
