import contextlib
import importlib.util
import io
from pathlib import Path
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('rockbox_install',ROOT/'ports/rockbox/install.py')
installer=importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)


class InstallerTests(unittest.TestCase):
    def checkout(self,root):
        (root/'apps/plugins').mkdir(parents=True)
        (root/'apps/plugin.h').write_text('/* fixture */\n')
        (root/'apps/plugins/SUBDIRS').write_text('existing\n')
        (root/'apps/plugins/CATEGORIES').write_text('existing,games\n')

    def test_registration_is_idempotent_and_preserves_existing_lines(self):
        with tempfile.TemporaryDirectory() as tmp,contextlib.redirect_stdout(io.StringIO()):
            root=Path(tmp); self.checkout(root)
            original=(root/'apps/plugins/SUBDIRS').read_bytes()
            installer.install(root)
            subdirs=(root/'apps/plugins/SUBDIRS').read_bytes()
            installer.install(root)
            self.assertEqual((root/'apps/plugins/SUBDIRS').read_bytes(),subdirs)
            self.assertTrue(subdirs.startswith(original))
            self.assertEqual((root/'apps/plugins/CATEGORIES').read_text(),'existing,games\nbloons,games\n')
            self.assertEqual((root/'apps/plugins/bloons/src/game.cpp').read_bytes(),(ROOT/'src/game.cpp').read_bytes())

    def test_unrelated_directory_is_not_overwritten(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp); self.checkout(root)
            destination=root/'apps/plugins/bloons'; destination.mkdir()
            (destination/'keep.txt').write_text('user content')
            with self.assertRaises(ValueError): installer.install(root)
            self.assertEqual((destination/'keep.txt').read_text(),'user content')

    def test_noncheckout_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(ValueError): installer.install(Path(tmp))


if __name__=='__main__': unittest.main()
