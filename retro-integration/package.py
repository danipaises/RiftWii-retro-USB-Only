# SPDX-FileCopyrightText: 2026 RiftWii contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a noncommercial alpha kit; never overwrite user saves/settings."""
import hashlib,json,os,pathlib,shutil,subprocess,tempfile,urllib.request,zipfile
import py7zr
import sys,datetime,io,re,tarfile,xml.etree.ElementTree as ET
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/"tools"))
from usb_update import allowed
from validate_usb_kit import validate as validate_kit
ROOT=pathlib.Path(__file__).resolve().parents[1]
OUT=ROOT/'dist';OUT.mkdir(exist_ok=True)
VERSION=(ROOT/'retro-integration/VERSION').read_text().strip()
if not VERSION.startswith('v0.') or any(c not in '0123456789abcdefghijklmnopqrstuvwxyz.-' for c in VERSION):raise ValueError('Only experimental version identifiers are accepted')
LOCK=json.loads((ROOT/'retro-integration/emulators.lock.json').read_text())
def download(url,path):
    if not path.exists():
        with urllib.request.urlopen(url,timeout=120) as r,open(str(path)+'.part','wb') as f:
            shutil.copyfileobj(r,f);f.flush();os.fsync(f.fileno())
        os.replace(str(path)+'.part',path)
    return path
def source_archive(destination):
    info=json.loads((ROOT/'SOURCE-INFO.json').read_text())
    commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    stamp=re.search(rb'RiftWii plugin build stamp: ([A-Za-z0-9_-]+)\0',(ROOT/'build_wii/riftwii-unpacked.dol').read_bytes())
    if not stamp:raise ValueError('The built DOL has no reproducible source stamp')
    stamp=stamp.group(1).decode()
    info.update({'branch':'main','commit':commit,'commit_short':commit[:8],
                 'version':stamp.rsplit('-g',1)[0],'build_stamp':stamp,'release':VERSION,
                 'commits_since_base_tag':int(stamp.split('-')[1]),'hardware_tested':False,
                 'created':datetime.datetime.now(datetime.timezone.utc).date().isoformat()})
    with tempfile.TemporaryDirectory() as work:
        raw=pathlib.Path(work)/'source.tar'
        subprocess.run(['git','archive','--format=tar','-o',str(raw),'HEAD'],cwd=ROOT,check=True)
        with tarfile.open(raw) as source,tarfile.open(destination,'w:gz') as target:
            for member in source:
                if member.name=='SOURCE-INFO.json':
                    data=(json.dumps(info,indent=2)+'\n').encode();member.size=len(data)
                    target.addfile(member,io.BytesIO(data))
                else:target.addfile(member,source.extractfile(member) if member.isfile() else None)
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
    meta=ET.Element('app',version='1')
    for key,value in {'name':'RiftWii Retro USB-Only','coder':'RiftWii contributors; USB fork: danipaises',
                      'version':VERSION[1:],'release_date':datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d%H%M%S'),
                      'short_description':'Biblioteca Wii, GameCube e retro via USB',
                      'long_description':'Versão experimental USB-only. Sem SD. Leia as notas e faça backup antes de atualizar.'}.items():
        ET.SubElement(meta,key).text=value
    ET.SubElement(meta,'no_ios_reload');ET.SubElement(meta,'ahb_access')
    ET.ElementTree(meta).write(app/'meta.xml',encoding='utf-8',xml_declaration=True)
    for f in ['README.md','LICENSE','NOTICE.md'] :shutil.copy2(ROOT/f,stage/f)
    shutil.copy2(ROOT/'retro-integration/emulators.lock.json',stage/'sources/emulators.lock.json')
    shutil.copy2(ROOT/'retro-integration/STATUS.md',stage/'LEIA-ME-alpha3.md')
    shutil.copy2(ROOT/'retro-integration/HARDWARE-alpha3.md',stage/'TESTE-NO-WII.md')
    shutil.copy2(ROOT/'tools/usb_update.py',stage/'usb_update.py')
    source_archive(stage/'sources/riftwii-source.tar.gz')
    required=['riftwii','fceugx','snes9xgx','vbagx','genplusgx','wii64-gln64','wii64-rice']
    for name in required:
        f=stage/'apps'/name/'boot.dol'
        if not f.is_file() or f.stat().st_size<256:raise ValueError('Missing emulator: '+name)
    # Some binary-only upstream distributions have no HBC metadata/icon.
    for name in required:
        folder=stage/'apps'/name
        if not (folder/'meta.xml').exists():
            m=ET.Element('app',version='1')
            ET.SubElement(m,'name').text={'genplusgx':'Genesis Plus GX'}.get(name,name)
            ET.SubElement(m,'coder').text='See sources/emulators.lock.json and corresponding source licenses'
            ET.SubElement(m,'version').text='Pinned USB kit component'
            ET.SubElement(m,'ahb_access');ET.SubElement(m,'no_ios_reload')
            ET.ElementTree(m).write(folder/'meta.xml',encoding='utf-8',xml_declaration=True)
        if not (folder/'icon.png').exists():shutil.copy2(ROOT/'hbc/icon.png',folder/'icon.png')
    for f in stage.rglob('*'):
        if not f.is_file():continue
        if (f.suffix.lower() == '.md' and f.relative_to(stage).parts[0] == 'retro') or f.suffix.lower() in ['.wad','.nes','.sfc','.smc','.gb','.gbc','.gba','.z64','.n64','.v64','.iso','.wbfs','.cue','.gg','.sms','.32x'] or f.name.startswith('bios_CD_'):raise ValueError('Forbidden ROM/BIOS/installer: '+str(f))
    managed=[f for f in sorted(stage.rglob('*')) if f.is_file() and allowed(f.relative_to(stage).as_posix())]
    components=[{'component':item['repo'].split('/')[-1],'version':item.get('tag',item['commit']),
                 'commit':item['commit'],'asset_sha256':item['sha256']} for item in LOCK]
    components.insert(0,{'component':'riftwii','version':VERSION})
    state={'version':VERSION,'components':components,'files':{f.relative_to(stage).as_posix():hashlib.sha256(f.read_bytes()).hexdigest() for f in managed}}
    (stage/'riftwii/installed-components.json').write_text(json.dumps(state,indent=2)+'\n')
    incremental=OUT/('RiftWii-Retro-USB-Only-'+VERSION+'-Update.zip')
    with zipfile.ZipFile(incremental,'w',zipfile.ZIP_DEFLATED) as z:
        for f in managed:z.write(f,f.relative_to(stage).as_posix())
    update={'schema':1,'repository':'danipaises/RiftWii-retro-USB-Only','version':VERSION,'approved':True,
            'hardware_validated':False,'experimental':True,'components':components,
            'changes':'Scanner NES/subpastas/ZIP; handoff USB IOS58; biblioteca unificada. Validação física pendente.',
            'package':{'url':'https://github.com/danipaises/RiftWii-retro-USB-Only/releases/download/'+VERSION+'/'+incremental.name,
                       'size':incremental.stat().st_size,'sha256':hashlib.sha256(incremental.read_bytes()).hexdigest()},
            'files':[{'path':f.relative_to(stage).as_posix(),'size':f.stat().st_size,'sha256':state['files'][f.relative_to(stage).as_posix()]} for f in managed]}
    (OUT/'components.json').write_text(json.dumps(update,indent=2)+'\n')
    shutil.copy2(ROOT/'tools/usb_update.py',OUT/'usb_update.py')
    shutil.copy2(ROOT/'retro-integration/STATUS.md',OUT/'INSTRUCOES.md')
    shutil.copy2(stage/'sources/riftwii-source.tar.gz',OUT/('RiftWii-'+VERSION+'-Source.tar.gz'))
    manifest=''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.relative_to(stage).as_posix()+'\n' for f in sorted(stage.rglob('*')) if f.is_file())
    (stage/'SHA256SUMS.txt').write_text(manifest)
    target=OUT/('RiftWii-Retro-USB-Only-'+VERSION+'-Kit.zip')
    with open(str(target)+'.tmp','wb') as raw:
        with zipfile.ZipFile(raw,'w',zipfile.ZIP_DEFLATED) as z:
            for f in sorted(stage.rglob('*')):z.write(f,f.relative_to(stage).as_posix())
        raw.flush();os.fsync(raw.fileno())
    os.replace(str(target)+'.tmp',target)
    validate_kit(target)
    (OUT/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.name+'\n' for f in sorted(OUT.iterdir()) if f.is_file() and f.name!='SHA256SUMS.txt'))
    print(target)
