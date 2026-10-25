#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 RiftWii contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Opt-in Linux update/rollback. Wii-side installation remains disabled."""
import stat,argparse,hashlib,json,os,pathlib,shutil,tempfile,time,urllib.parse,urllib.request,zipfile
REPO='danipaises/RiftWii-retro-USB-Only'
APPS={'riftwii','fceugx','snes9xgx','vbagx','genplusgx','wii64-gln64','wii64-rice'}
def digest(path):
    h=hashlib.sha256()
    with open(path,'rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
    return h.hexdigest()
def allowed(name):
    p=pathlib.PurePosixPath(name)
    if p.as_posix()!=name or p.is_absolute() or '..' in p.parts or '.' in p.parts or '\\' in name or ':' in name:return False
    if len(p.parts)==3 and p.parts[0]=='apps' and p.parts[1] in APPS:
        return p.name in {'boot.dol','meta.xml','icon.png'} or (p.parts[1]=='riftwii' and p.name in {'font.ttf','music.ogg'})
    return (len(p.parts)==4 and p.parts[:3]==('apps','riftwii','plugins') and
            p.name.startswith(('gamecube.','selftest.')) and p.name.endswith('.rwp'))
def destination(root,name):
    if not allowed(name):raise ValueError('Protected or unknown destination: '+name)
    p=root
    for part in pathlib.PurePosixPath(name).parts:
        p=p/part
        if p.is_symlink():raise ValueError('Symlink destination refused')
    return p
def atomic(path,data):
    path.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=path.parent,delete=False) as f:
        temporary=pathlib.Path(f.name);f.write(data);f.flush();os.fsync(f.fileno())
    os.chmod(temporary,stat.S_IMODE(path.stat().st_mode) if path.exists() else 0o644)
    os.replace(temporary,path)
    fd=os.open(path.parent,os.O_DIRECTORY)
    try:os.fsync(fd)
    finally:os.close(fd)
def trusted(url):
    p=urllib.parse.urlsplit(url)
    return p.scheme=='https' and p.netloc=='github.com' and p.path.startswith('/'+REPO+'/releases/download/') and not p.fragment and not p.query and '..' not in p.path and '%' not in p.path
class Redirects(urllib.request.HTTPRedirectHandler):
    def redirect_request(self,req,fp,code,msg,headers,newurl):
        p=urllib.parse.urlsplit(newurl)
        if p.scheme!='https' or not (p.netloc=='github.com' or p.netloc.endswith('.githubusercontent.com')):
            raise ValueError('Untrusted redirect')
        return super().redirect_request(req,fp,code,msg,headers,newurl)
def fetch(url,maximum):
    if not trusted(url):raise ValueError('Updates must originate in the fork release assets')
    with urllib.request.build_opener(Redirects()).open(url,timeout=45) as f:
        data=f.read(maximum+1)
    if len(data)>maximum:raise ValueError('Download exceeds manifest bound')
    return data
def validate(manifest):
    if manifest.get('schema')!=1 or manifest.get('repository')!=REPO or manifest.get('approved') is not True:
        raise ValueError('Manifest is not an approved fork combination')
    seen=set()
    for f in manifest['files']:
        if not allowed(f['path']) or f['path'] in seen:raise ValueError('Invalid or duplicate file')
        if len(f['sha256'])!=64 or any(c not in '0123456789abcdef' for c in f['sha256']):raise ValueError('Invalid SHA256')
        if not 0 < f['size'] <= 32*1024*1024:raise ValueError('Invalid file size')
        seen.add(f['path'])
    if len(manifest['package']['sha256'])!=64 or any(c not in '0123456789abcdef' for c in manifest['package']['sha256']):raise ValueError('Invalid package SHA256')
    if not trusted(manifest['package']['url']):raise ValueError('Untrusted package origin')
    if not 0 < manifest['package']['size'] <= 128*1024*1024:raise ValueError('Package too large')
def state_path(root):
    p=root/'riftwii/installed-components.json'
    if p.is_symlink() or (root/'riftwii').is_symlink():raise ValueError('Symlink state path')
    return p
def apply(root,manifest,archive):
    validate(manifest)
    state=state_path(root)
    if not state.exists():raise ValueError('No installed factory record: automatic adoption is deliberately refused')
    old=json.loads(state.read_text())
    changes=[]
    for f in manifest['files']:
        p=destination(root,f['path'])
        if p.exists() and digest(p)==f['sha256']:continue
        if p.exists() and (f['path'] not in old.get('files',{}) or digest(p)!=old['files'][f['path']]):
            raise ValueError('Personal or unknown file will not be overwritten: '+f['path'])
        changes.append(f)
    need=sum(f['size'] for f in changes)*2+sum(destination(root,f['path']).stat().st_size for f in changes if destination(root,f['path']).exists())+archive.stat().st_size+2*1024*1024
    if shutil.disk_usage(root).free<need:raise ValueError('Insufficient space for staging and backup')
    if digest(archive)!=manifest['package']['sha256'] or archive.stat().st_size!=manifest['package']['size']:
        raise ValueError('Package hash or size mismatch')
    with zipfile.ZipFile(archive) as z:
        names=z.namelist()
        if len(names)!=len(set(names)) or set(names)!={f['path'] for f in manifest['files']}:raise ValueError('Unexpected archive content')
        # Validate every byte before touching a destination.
        for f in manifest['files']:
            info=z.getinfo(f['path'])
            if info.file_size!=f['size']:raise ValueError('Expanded size mismatch')
            if hashlib.sha256(z.read(info)).hexdigest()!=f['sha256']:raise ValueError('Expanded hash mismatch')
        managed=root/'riftwii/update-backups'
        if managed.is_symlink():raise ValueError('Symlink backup directory')
        managed.mkdir(parents=True,exist_ok=True)
        backup=pathlib.Path(tempfile.mkdtemp(prefix='update-',dir=managed))
        journal={'status':'prepared','old_state':old,'files':[]}
        for f in changes:
            p=destination(root,f['path']);save=backup/f['path']
            if p.exists():save.parent.mkdir(parents=True,exist_ok=True);atomic(save,p.read_bytes())
            journal['files'].append({'path':f['path'],'old':digest(p) if p.exists() else None,'new':f['sha256']})
        atomic(backup/'journal.json',json.dumps(journal,indent=2).encode())
        try:
            for f in changes:atomic(destination(root,f['path']),z.read(f['path']))
            installed={'version':manifest['version'],'components':manifest.get('components',[]),'files':dict(old.get('files',{}))}
            installed['files'].update({f['path']:f['sha256'] for f in manifest['files']})
            atomic(state,json.dumps(installed,indent=2).encode())
            journal['status']='complete';atomic(backup/'journal.json',json.dumps(journal,indent=2).encode())
        except BaseException:
            # Journal + backups were synced first, including for power loss.
            raise
    return backup.name
def rollback(root,identifier):
    if pathlib.Path(identifier).name!=identifier:raise ValueError('Invalid backup identifier')
    folder=root/'riftwii/update-backups'/identifier
    if folder.is_symlink() or folder.parent.is_symlink():raise ValueError('Symlink backup')
    state_path(root)
    journal=json.loads((folder/'journal.json').read_text())
    for f in journal['files']:
        p=destination(root,f['path'])
        current=digest(p) if p.exists() else None
        if current not in {f['old'],f['new']}:raise ValueError('File changed after update; rollback will not overwrite it')
        old=folder/f['path']
        if f['old'] and (not old.is_file() or old.is_symlink() or digest(old)!=f['old']):raise ValueError('Backup hash mismatch')
    for f in journal['files']:
        p=destination(root,f['path'])
        if f['old']:atomic(p,(folder/f['path']).read_bytes())
        elif p.exists():p.unlink()
    atomic(state_path(root),json.dumps(journal['old_state'],indent=2).encode())
    journal['status']='rolled-back';atomic(folder/'journal.json',json.dumps(journal,indent=2).encode())
def discover_manifest():
    # Fixed API origin, then strict release-asset URLs; includes experimental releases.
    with urllib.request.urlopen('https://api.github.com/repos/'+REPO+'/releases?per_page=20',timeout=45) as response:
        releases=json.loads(response.read(2*1024*1024))
    for release in releases:
        if release.get('draft'):continue
        for asset in release.get('assets',[]):
            url=asset.get('browser_download_url','')
            if asset.get('name')=='components.json' and trusted(url):return url
    raise ValueError('No compatible fork update manifest is published')
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--usb',required=True,type=pathlib.Path,help='Explicit mounted USB root; never formatted')
    parser.add_argument('--manifest',help='HTTPS components.json URL from our GitHub release')
    parser.add_argument('--apply',action='store_true');parser.add_argument('--yes',action='store_true')
    parser.add_argument('--rollback')
    args=parser.parse_args();root=args.usb.resolve(strict=True)
    if not root.is_dir() or root==pathlib.Path('/'):raise ValueError('Select a mounted USB directory')
    print('Selected USB:',root)
    if args.rollback:
        if not args.yes:raise ValueError('Rollback requires --yes')
        rollback(root,args.rollback);return
    if not args.manifest:args.manifest=discover_manifest()
    manifest=json.loads(fetch(args.manifest,2*1024*1024));validate(manifest)
    state=state_path(root);installed=json.loads(state.read_text()) if state.exists() else {}
    versions={c['component']:c['version'] for c in installed.get('components',[])}
    comparisons=[{'component':c['component'],'installed':versions.get(c['component'],'unknown'),'available':c['version']} for c in manifest.get('components',[])]
    print(json.dumps({'installed':installed.get('version','unknown'),'available':manifest['version'],
                      'experimental':manifest.get('experimental',True),'hardware_validated':manifest.get('hardware_validated',False),
                      'changes':manifest.get('changes',''),'components':comparisons,'download':manifest['package']['size']},indent=2))
    if args.apply:
        if not args.yes:raise ValueError('Installation requires explicit --apply --yes')
        # Linux owns this mounted volume; no Wii executable is updated in place while running.
        if shutil.disk_usage(root).free<manifest['package']['size']*4:raise ValueError('Insufficient free space')
        with tempfile.TemporaryDirectory() as work:
            archive=pathlib.Path(work)/'update.zip'
            archive.write_bytes(fetch(manifest['package']['url'],manifest['package']['size']))
            print('Rollback identifier:',apply(root,manifest,archive))
if __name__=='__main__':
    try:main()
    except (ValueError,OSError,KeyError,zipfile.BadZipFile) as error:raise SystemExit(str(error))
