# SPDX-FileCopyrightText: 2026 RiftWii contributors
# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib,importlib.util,json,pathlib,tempfile,unittest,zipfile
spec=importlib.util.spec_from_file_location('kit',pathlib.Path(__file__).resolve().parents[1]/'tools/validate_usb_kit.py')
k=importlib.util.module_from_spec(spec);spec.loader.exec_module(k)
class KitTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.path=pathlib.Path(self.tmp.name)/'kit.zip'
        self.files={'apps/'+app+'/boot.dol':b'\x00'*256 for app in k.APPS}
        self.files.update({'apps/riftwii/meta.xml':b'<app version="1"><name>RiftWii</name><coder>Contributors</coder><version>alpha3</version><release_date>20261025</release_date><short_description>USB</short_description><long_description>USB only</long_description><ahb_access/><no_ios_reload/></app>',
            'apps/riftwii/icon.png':b'\x89PNG\r\n\x1a\n', 'apps/riftwii/font.ttf':b'font','apps/riftwii/music.ogg':b'music',
            'apps/riftwii/plugins/gamecube.test.rwp':b'plugin','sources/riftwii-source.tar.gz':b'source','sources/emulators.lock.json':b'[]'})
        state={'files':{'apps/riftwii/boot.dol':hashlib.sha256(self.files['apps/riftwii/boot.dol']).hexdigest()}}
        self.files['riftwii/installed-components.json']=json.dumps(state).encode()
    def write(self):
        with zipfile.ZipFile(self.path,'w') as z:
            for name,data in self.files.items():z.writestr(name,data)
    def test_required_layout(self):self.write();k.validate(self.path)
    def test_missing_emulator(self):
        del self.files['apps/fceugx/boot.dol'];self.write()
        with self.assertRaises((KeyError,ValueError)):k.validate(self.path)
    def test_no_commercial_content(self):
        for name in ['retro/nes/game.nes','genplus/bios/bios_CD_U.bin','games/a/game.iso','wbfs/game.wbfs']:
            self.files[name]=b'not actual game content';self.write()
            with self.assertRaises(ValueError):k.validate(self.path)
            del self.files[name]
    def test_hbc_xml(self):
        self.files['apps/riftwii/meta.xml']=b'<app><name>missing metadata</name></app>';self.write()
        with self.assertRaises(ValueError):k.validate(self.path)
    def test_factory_hash(self):
        self.files['apps/riftwii/boot.dol']=b'X'*256;self.write()
        with self.assertRaises(ValueError):k.validate(self.path)
if __name__=='__main__':unittest.main()
