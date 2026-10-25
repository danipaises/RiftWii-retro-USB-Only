# SPDX-FileCopyrightText: 2026 RiftWii contributors
# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib,importlib.util,json,pathlib,tempfile,unittest,zipfile
from unittest import mock
spec=importlib.util.spec_from_file_location('updater',pathlib.Path(__file__).resolve().parents[1]/'tools/usb_update.py')
u=importlib.util.module_from_spec(spec);spec.loader.exec_module(u)
class UpdateTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=pathlib.Path(self.temp.name)/'usb';self.root.mkdir()
        self.name='apps/riftwii/boot.dol';self.old=b'old executable';self.new=b'new executable'
        p=self.root/self.name;p.parent.mkdir(parents=True);p.write_bytes(self.old)
        self.state={'version':'alpha2','files':{self.name:hashlib.sha256(self.old).hexdigest()}}
        (self.root/'riftwii').mkdir();(self.root/'riftwii/installed-components.json').write_text(json.dumps(self.state))
        self.archive=pathlib.Path(self.temp.name)/'update.zip'
        with zipfile.ZipFile(self.archive,'w') as z:z.writestr(self.name,self.new)
        self.manifest={'schema':1,'repository':u.REPO,'approved':True,'version':'alpha3',
            'package':{'url':'https://github.com/'+u.REPO+'/releases/download/v3/Update.zip','size':self.archive.stat().st_size,'sha256':u.digest(self.archive)},
            'files':[{'path':self.name,'size':len(self.new),'sha256':hashlib.sha256(self.new).hexdigest()}]}
    def test_allowlist(self):
        for path in ['retro/nes/game.nes','wbfs/a.wbfs','games/a/game.iso','vbagx/saves/a.sav','genplus/bios/bios_CD_U.bin','riftwii/settings.txt','apps/riftwii/../boot.dol','apps//riftwii/boot.dol','apps/riftwii/custom.theme','apps/unknown/boot.dol']:
            self.assertFalse(u.allowed(path),path)
    def test_source(self):
        self.assertFalse(u.trusted('http://github.com/'+u.REPO+'/releases/download/v3/a'))
        self.assertFalse(u.trusted('https://github.com/evil/repo/releases/download/v3/a'))
    def test_apply_and_rollback_preserve_personal_files(self):
        personal=self.root/'riftwii/settings.txt';personal.write_bytes(b'my favorites and settings')
        save=self.root/'vbagx/saves/a.sav';save.parent.mkdir(parents=True);save.write_bytes(b'my save')
        identifier=u.apply(self.root,self.manifest,self.archive)
        self.assertEqual((self.root/self.name).read_bytes(),self.new)
        self.assertEqual(save.read_bytes(),b'my save');self.assertEqual(personal.read_bytes(),b'my favorites and settings')
        u.rollback(self.root,identifier)
        self.assertEqual((self.root/self.name).read_bytes(),self.old)
        self.assertEqual(json.loads(u.state_path(self.root).read_text()),self.state)
    def test_personal_factory_file_refused(self):
        (self.root/self.name).write_bytes(b'custom modified executable')
        with self.assertRaises(ValueError):u.apply(self.root,self.manifest,self.archive)
    def test_bad_hash_changes_nothing(self):
        self.manifest['package']['sha256']='0'*64
        with self.assertRaises(ValueError):u.apply(self.root,self.manifest,self.archive)
        self.assertEqual((self.root/self.name).read_bytes(),self.old)
        self.assertEqual(json.loads(u.state_path(self.root).read_text()),self.state)
    def test_symlinks_refused(self):
        (self.root/self.name).unlink();(self.root/self.name).symlink_to(self.archive)
        with self.assertRaises(ValueError):u.apply(self.root,self.manifest,self.archive)
    def test_rollback_refuses_later_edits(self):
        identifier=u.apply(self.root,self.manifest,self.archive);(self.root/self.name).write_bytes(b'personal change after update')
        with self.assertRaises(ValueError):u.rollback(self.root,identifier)
    def test_interrupted_install_has_durable_rollback(self):
        original=u.atomic
        def interrupt(path,data):
            original(path,data)
            if path==self.root/self.name:raise OSError('simulated interruption')
        with mock.patch.object(u,'atomic',side_effect=interrupt):
            with self.assertRaises(OSError):u.apply(self.root,self.manifest,self.archive)
        identifier=next((self.root/'riftwii/update-backups').iterdir()).name
        u.rollback(self.root,identifier);self.assertEqual((self.root/self.name).read_bytes(),self.old)
    def test_insufficient_space(self):
        with mock.patch.object(u.shutil,'disk_usage',return_value=type('Usage',(),{'free':0})()):
            with self.assertRaises(ValueError):u.apply(self.root,self.manifest,self.archive)
if __name__=='__main__':unittest.main()
