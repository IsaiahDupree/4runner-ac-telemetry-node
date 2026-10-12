"""Regression checks for real build metadata retained outside the checkout."""

import json
from pathlib import Path
import shutil
import tempfile
import unittest

from tools.verify_repository import RELEASE, read_json


class ExternalBuildMetadataTests(unittest.TestCase):
    def test_real_release_manifest_can_be_read_outside_checkout(self):
        source = RELEASE / "manifest-ac-sensor-node-esp32s3.json"
        with tempfile.TemporaryDirectory() as temporary:
            copied = Path(temporary) / source.name
            shutil.copyfile(source, copied)
            self.assertEqual(read_json(copied), json.loads(source.read_text()))

    def test_non_object_external_json_reports_the_contract_failure(self):
        with tempfile.TemporaryDirectory() as temporary:
            invalid = Path(temporary) / "invalid-metadata.json"
            invalid.write_text("[]\n", encoding="utf-8")
            with self.assertRaisesRegex(AssertionError, "must contain an object"):
                read_json(invalid)


if __name__ == "__main__":
    unittest.main()
