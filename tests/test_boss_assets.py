"""Catch missing or corrupt local boss pictures before a release build."""
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class BossAssetsTest(unittest.TestCase):
    def test_every_boss_has_a_local_jpeg(self):
        bosses = json.loads((ROOT / "data/bosses.json").read_text(encoding="utf-8"))["bosses"]
        for boss in bosses:
            with self.subTest(boss=boss["id"]):
                path = ROOT / "assets/bosses" / (boss["id"] + ".jpg")
                self.assertTrue(path.is_file(), "Missing local boss image: " + boss["id"])
                data = path.read_bytes()
                self.assertTrue(data.startswith(b"\xff\xd8\xff"), "Not a JPEG")
                self.assertTrue(data.endswith(b"\xff\xd9"), "Truncated JPEG")


if __name__ == "__main__":
    unittest.main()
