"""Exercise real retained inputs and unsafe edits; no mock EDA/providers."""

import copy
import json
from pathlib import Path
import shutil
import tempfile
import unittest

from tools.verify_ac_carrier import PACKAGE, ROOT, verify


class CarrierDiscoveryTests(unittest.TestCase):
    def test_real_retained_axes_and_sources(self):
        result = verify()
        self.assertTrue(result["retained_slide_axes_match"])
        self.assertEqual(result["minimum_requested_analog_observations_sps"], 105)
        self.assertFalse(result["schematic_compilation_allowed"])
        self.assertFalse(result["fabrication_allowed"])

    def run_mutation(self, mutate, message):
        with tempfile.TemporaryDirectory() as temporary:
            target = Path(temporary)
            shutil.copytree(ROOT / "docs", target / "docs")
            shutil.copytree(ROOT / "hardware", target / "hardware")
            inputs = target / PACKAGE / "design-inputs.json"
            data = json.loads(inputs.read_text())
            mutate(data, target)
            inputs.write_text(json.dumps(data))
            with self.assertRaisesRegex(ValueError, message):
                verify(target)

    def test_silent_sensor_choice_is_rejected(self):
        def select(data, root):
            sensor = next(item for item in data["unresolved_contracts"] if item["id"] == "PRESSURE_HIGH")
            sensor.update(status="CONFIRMED", value={"family": "Sensata", "transfer": "0.5-4.5 V"})
        self.run_mutation(select, "silently selected")

    def test_deleting_a_missing_contract_is_rejected(self):
        self.run_mutation(lambda data, root: data["unresolved_contracts"].pop(), "contract count")

    def test_self_approval_is_rejected(self):
        self.run_mutation(lambda data, root: data["authorities"].update(fabrication_approved=True), "grant approval")

    def test_source_change_is_rejected(self):
        def change(data, root):
            path = root / "docs/development/TEMPERATURE-SENSORS.md"
            path.write_text(path.read_text() + "\nchanged authority\n")
        self.run_mutation(change, "source bytes changed")

    def test_mirrored_mounting_pattern_is_rejected(self):
        def mirror(data, root):
            raw = copy.deepcopy(data["mechanical_reference"]["raw_uno_hole_centres_mm"])
            data["mechanical_reference"]["installed_local_hole_centres_mm"] = [[68.58 - x, y] for x, y in raw]
        self.run_mutation(mirror, "rotated incorrectly")

    def test_wrong_case_origin_is_rejected_against_actual_slide(self):
        self.run_mutation(lambda data, root: data["mechanical_reference"]["installed_board_origin_mm"].__setitem__(0, 127.5),
                          "actual retained slide axes")

    def test_cad_fit_cannot_be_relabelled_as_populated_pcb_fit(self):
        self.run_mutation(lambda data, root: data["mechanical_reference"].update(new_populated_pcb_fit_verified=True),
                          "physical/native fit")

    def test_unperformed_drc_cannot_be_relabelled_as_pass(self):
        self.run_mutation(lambda data, root: data["native_verification"].update(drc="PASS"), "native verification")

    def test_changed_native_source_is_rejected(self):
        def change(data, root):
            path = root / PACKAGE / "mounting-reference.kicad_pcb"
            path.write_text(path.read_text().replace("102.54 145.72", "103.54 145.72"))
        self.run_mutation(change, "mounting source bytes changed")

    def test_stale_native_node_binding_is_rejected(self):
        def change(data, root):
            path = root / PACKAGE / "mounting-source-inventory.json"
            observation = json.loads(path.read_text())
            observation["inventory"]["provenance"]["node_end_byte"] -= 1
            path.write_text(json.dumps(observation))
        self.run_mutation(change, "node binding changed")

    def test_changed_canonical_boardspec_is_rejected(self):
        def change(data, root):
            path = root / PACKAGE / "boardspec.json"
            snapshot = json.loads(path.read_text())
            snapshot["fields"][0]["status"] = "confirmed"
            path.write_text(json.dumps(snapshot))
        self.run_mutation(change, "BoardSpec bytes changed")

    def test_unsupported_scope_cannot_be_silently_enabled(self):
        def change(data, root):
            path = root / PACKAGE / "risk-intake.json"
            intake = json.loads(path.read_text())
            intake["disposition"] = "HUMAN_REVIEW_REQUIRED"
            path.write_text(json.dumps(intake))
        self.run_mutation(change, "silently enabled")


if __name__ == "__main__":
    unittest.main()
