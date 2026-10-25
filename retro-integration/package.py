# SPDX-FileCopyrightText: 2026 RiftWii contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a noncommercial alpha kit; never overwrite user saves/settings."""
import hashlib,json,os,pathlib,shutil,subprocess,tempfile,urllib.request,zipfile
import py7zr
ROOT=pathlib.Path(__file__).resolve().parents[1]
OUT=ROOT/'dist';OUT.mkdir(exist_ok=True)
LOCK=json.loads((ROOT/'retro-integration/emulators.lock.json').read_text())
def download(url,path):
    if not path.exists():
        with urllib.request.urlopen(url,timeout=120) as r,open(path,'wb') as f:shutil.copyfileobj(r,f)
    return path
with tempfile.TemporaryDirectory() as work:
    stage=pathlib.Path(work)/'usb';stage.mkdir()
    cache=ROOT/'build-downloads';cache.mkdir(exist_ok=True)
    for name in ['nes','snes','gb','gbc','gba','n64','master-system','mega-drive','game-gear','sg1000','sega-cd','sega-32x','covers']:
        d=stage/'retro'/name;d.mkdir(parents=True)
        (d/'README.txt').write_text('Use somente seus arquivos legalmente obtidos. Diretório de ROMs vazio.\n')
    for item in LOCK:
        asset=download(item['asset'],cache/item['asset'].split('/')[-1])
        if hashlib.sha256(asset.read_bytes()).hexdigest()!=item['sha256']:raise ValueError('Asset checksum mismatch: '+item['repo'])
        extracted=pathlib.Path(work)/item['repo'].split('/')[-1];extracted.mkdir()
        if asset.suffix=='.zip':
            with zipfile.ZipFile(asset) as z:
                for n in z.namelist():
                    p=pathlib.PurePosixPath(n)
                    if p.is_absolute() or '..' in p.parts:raise ValueError('Unsafe asset path')
                z.extractall(extracted)
        elif asset.suffix=='.7z':
            with py7zr.SevenZipFile(asset) as z:
                if any(pathlib.PurePosixPath(n).is_absolute() or '..' in pathlib.PurePosixPath(n).parts for n in z.getnames()):raise ValueError('Unsafe archive')
                z.extractall(extracted)
        else:
            d=extracted/'apps/genplusgx';d.mkdir(parents=True);shutil.copy2(asset,d/'boot.dol')
        # Only normal Wii applications; exclude installers and WiiVC builds.
        for app in (extracted/'apps').glob('*'):
            if app.name.endswith('_wiivc'):continue
            target={'wii64':'wii64-gln64','wii64 Rice':'wii64-rice'}.get(app.name,app.name)
            if target not in ['fceugx','snes9xgx','vbagx','genplusgx','wii64-gln64','wii64-rice']:continue
            for f in app.iterdir():
                if f.name not in ['boot.dol','icon.png','meta.xml']:continue
                dest=stage/'apps'/target/f.name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,dest)
        for f in (extracted/'wii64').glob('*'):
            if f.is_file() and f.name in ['boxart.bin','RiceVideoLinux.ini']:
                d=stage/'wii64';d.mkdir(exist_ok=True);shutil.copy2(f,d/f.name)
        # Corresponding full source accompanies binaries, with upstream licenses.
        sources=stage/'sources';sources.mkdir(exist_ok=True)
        archive=download('https://codeload.github.com/'+item['repo']+'/tar.gz/'+item['commit'],cache/(item['repo'].split('/')[-1]+'-'+item['commit']+'.tar.gz'))
        shutil.copy2(archive,sources/archive.name)
    for folder in ['fceugx/saves','snes9xgx/saves','vbagx/saves','wii64/saves','genplus/bios','genplus/saves/md','genplus/saves/ms','genplus/saves/gg','genplus/saves/sg','genplus/saves/cd','riftwii/logs','wbfs','games']:
        (stage/folder).mkdir(parents=True,exist_ok=True)
    (stage/'genplus/bios/README.txt').write_text('BIOS proprietárias não são incluídas. Forneça legalmente bios_CD_U.bin, bios_CD_E.bin e bios_CD_J.bin. Sega CD ficará bloqueado na ausência delas.\n')
    # Template has a different name: updates cannot overwrite existing settings.
    (stage/'wii64/settings-USB.example.cfg').write_text('NativeDevice = 1\nAutoSave = 1\n')
    (stage/'retro/n64/emulator.example.txt').write_text('gln64\n')
    app=stage/'apps/riftwii';app.mkdir(parents=True,exist_ok=True)
    shutil.copy2(ROOT/'riftwii.dol',app/'boot.dol')
    for f in (ROOT/'build-plugins/plugins').glob('*.rwp'):
        d=app/'plugins';d.mkdir(exist_ok=True);shutil.copy2(f,d/f.name)
    shutil.copy2(ROOT/'wii/font/rounded.ttf',app/'font.ttf')
    shutil.copy2(ROOT/'hbc/music.ogg',app/'music.ogg')
    for f in ['icon.png','meta.xml']:
        source=ROOT/'hbc'/f
        if f=='icon.png':shutil.copy2(source,app/f)
    (app/'meta.xml').write_text('<app version="1"><name>RiftWii Retro USB alpha</name><version>0.1.0-alpha</version><ahb_access /></app>')
    for f in ['README.md','LICENSE','NOTICE.md'] :shutil.copy2(ROOT/f,stage/f)
    shutil.copy2(ROOT/'retro-integration/emulators.lock.json',stage/'sources/emulators.lock.json')
    subprocess.run(['git','archive','--format=tar.gz','-o',str(stage/'sources/riftwii-source.tar.gz'),'HEAD'],cwd=ROOT,check=True)
    required=['riftwii','fceugx','snes9xgx','vbagx','genplusgx','wii64-gln64','wii64-rice']
    for name in required:
        f=stage/'apps'/name/'boot.dol'
        if not f.is_file() or f.stat().st_size<256:raise ValueError('Missing emulator: '+name)
    for f in stage.rglob('*'):
        if not f.is_file():continue
        if (f.suffix.lower() == '.md' and f.relative_to(stage).parts[0] == 'retro') or f.suffix.lower() in ['.wad','.nes','.sfc','.smc','.gb','.gbc','.gba','.z64','.n64','.v64','.iso','.wbfs','.cue','.gg','.sms','.32x'] or f.name.startswith('bios_CD_'):raise ValueError('Forbidden ROM/BIOS/installer: '+str(f))
    manifest=''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.relative_to(stage).as_posix()+'\n' for f in sorted(stage.rglob('*')) if f.is_file())
    (stage/'SHA256SUMS.txt').write_text(manifest)
    target=OUT/'RiftWii-Retro-USB-Only-v0.1.0-alpha-Kit.zip'
    with open(str(target)+'.tmp','wb') as raw:
        with zipfile.ZipFile(raw,'w',zipfile.ZIP_DEFLATED) as z:
            for f in sorted(stage.rglob('*')):z.write(f,f.relative_to(stage).as_posix())
        raw.flush();os.fsync(raw.fileno())
    os.replace(str(target)+'.tmp',target)
    (OUT/'SHA256SUMS.txt').write_text(hashlib.sha256(target.read_bytes()).hexdigest()+'  '+target.name+'\n')
    print(target)
