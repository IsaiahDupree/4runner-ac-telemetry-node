#!/usr/bin/env python3
"""Check retained A/C discovery inputs; never grant design/release authority.

This verifies JSON source bytes and mechanical metadata only. It does not run
KiCad, parse native connectivity, qualify hardware, or select unknown parts.
"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
from decimal import Decimal
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PACKAGE = Path("hardware/ac-carrier-v0.1")
CONTRACT_IDS = {
    "CONTROLLER", "BOARD_ROLE", "PRESSURE_HIGH", "PRESSURE_LOW",
    "TEMPERATURE_INTERFACES", "POWER_ENVELOPE", "SENSOR_RAIL", "PI_BOUNDARY",
    "MEASUREMENT_ROUTING", "STORAGE_INTERFACE", "CONNECTORS_HARNESS",
    "FINAL_PCB_MECHANICS", "FABRICATION_RULES", "ENVIRONMENT_AND_TEST",
}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def decimal(value: object) -> Decimal:
    require(type(value) in {int, float}, "mechanical coordinate is not numeric")
    result = Decimal(str(value))
    require(result.is_finite(), "mechanical coordinate is not finite")
    return result


def close(actual: object, expected: object) -> bool:
    return abs(decimal(actual) - decimal(expected)) <= Decimal("0.00000001")


def read_json(path: Path) -> dict:
    require(path.stat().st_size <= 256 * 1024, "discovery JSON exceeds size bound")
    value = json.loads(path.read_text(encoding="utf-8"))
    require(isinstance(value, dict), "discovery JSON must be an object")
    return value


def verify(root: Path = ROOT) -> dict:
    root = root.resolve()
    inputs = root / PACKAGE / "design-inputs.json"
    data = read_json(inputs)
    require(data["schema_version"] == "ac-carrier-discovery/1", "unknown discovery version")
    require(data["revision"] == "ac-carrier-v0.1", "discovery revision changed")
    require(data["state"] == "BLOCKED_REQUIREMENTS", "discovery must remain blocked")
    require(data["artifact_role"] == "discovery_and_mounting_reference", "artifact role changed")
    require(data["authorities"] == {
        "requirements_approved": False, "architecture_approved": False,
        "schematic_approved": False, "layout_approved": False,
        "fabrication_approved": False, "vehicle_installation_approved": False,
    }, "discovery cannot grant approval")
    require(all(value is False for value in data["authorities"].values()), "approval must be literal false")
    require(data["release"] == {
        "state": "BLOCKED", "schematic_compilation_allowed": False,
        "fabrication_allowed": False, "ordering_allowed": False,
        "standards_compliance": "NOT_ESTABLISHED", "physical_validation": "NOT_RUN",
    }, "discovery release boundary changed")
    require(all(data["release"][key] is False for key in (
        "schematic_compilation_allowed", "fabrication_allowed", "ordering_allowed",
    )), "release authority must be literal false")
    require(data["native_verification"] == {
        "erc": "NOT_RUN", "drc": "NOT_RUN", "export_review": "NOT_RUN",
        "electrical_netlist_present": False,
    }, "native verification must remain unperformed")
    require(data["native_verification"]["electrical_netlist_present"] is False,
            "discovery cannot contain an electrical netlist")

    source_files = {}
    source_ids = set()
    for source in data["sources"]:
        require(source["id"] not in source_ids, "duplicate source identity")
        source_ids.add(source["id"])
        path = root / source["path"]
        require(path.resolve().is_relative_to(root), "source escapes repository")
        require(path.resolve().relative_to(root).as_posix() == source["path"], "noncanonical source path")
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        require(digest == source["sha256"], f"source bytes changed: {source['id']}")
        source_files[source["id"]] = path
    require(source_ids == {"focused-spec", "temperature-gate", "cad-release", "cad-slide", "cad-manifest"},
            "discovery source set changed")
    for requirement in data["inherited_requirements"]:
        require(requirement["source_id"] in source_ids, "requirement has no retained source")

    contracts = data["unresolved_contracts"]
    require(len(contracts) == len(CONTRACT_IDS), "unresolved contract count changed")
    require({item["id"] for item in contracts} == CONTRACT_IDS, "a necessary contract was removed")
    for item in contracts:
        require(item["status"] == "UNKNOWN" and item["value"] is None,
                f"unresolved contract was silently selected: {item['id']}")
        require("layout" in item["blocks"], "unresolved contract lost its layout gate")
        require(bool(item["needed"].strip()), "unresolved contract needs an actionable question")

    mechanical = data["mechanical_reference"]
    release = read_json(source_files["cad-release"])
    slide = read_json(source_files["cad-slide"])
    manifest = read_json(source_files["cad-manifest"])
    for role, field in (("shell", "shell_job_id"), ("slide", "slide_job_id"), ("cross", "cross_fit_job_id")):
        require(mechanical[field] == release["package"]["jobs"][role], "mismatched CAD job pairing")
    require(mechanical["cad_source_commit"] == release["package"]["source_commit"], "CAD source commit changed")
    require(mechanical["installed_case_envelope_mm"] == [
        release["maximum_case_width_mm"], manifest["parameters"]["case_height_mm"],
        manifest["parameters"]["case_depth_mm"] + 5.0,
    ], "case dimensions do not match retained revision")
    require(mechanical["reference_board_thickness_mm"] == manifest["fixed_dimensions"]["pcb_thickness_mm"],
            "board thickness differs from CAD")
    require(mechanical["board_support_boss_diameter_mm"] == manifest["fixed_dimensions"]["board_insert_boss_od_mm"],
            "support boss diameter differs from CAD")
    require(mechanical["reference_board_rectangle_mm"] == [68.58, 53.34], "UNO reference envelope changed")
    require(mechanical["installed_rotation_deg"] == 180, "mounting transform must be a rotation")
    raw = mechanical["raw_uno_hole_centres_mm"]
    local = mechanical["installed_local_hole_centres_mm"]
    actual = slide["arduino_board_hole_centers_xy_mm"]
    require(len(raw) == len(local) == len(actual) == 4, "four asymmetric mounting axes required")
    width, height = map(decimal, mechanical["reference_board_rectangle_mm"])
    origin = mechanical["installed_board_origin_mm"]
    for original, transformed, retained in zip(raw, local, actual):
        require(close(transformed[0], float(width - decimal(original[0]))) and
                close(transformed[1], float(height - decimal(original[1]))),
                "UNO mounting axes mirrored or rotated incorrectly")
        require(all(close(float(decimal(origin[i]) + decimal(transformed[i])), retained[i]) for i in (0, 1)),
                "mounting reference does not match actual retained slide axes")
    for key in ("actual_controller_hole_fit_verified", "new_populated_pcb_fit_verified",
                "extra_shield_stack_fit_verified", "native_kicad_reopen_verified"):
        require(mechanical[key] is False, "reference cannot claim new physical/native fit")

    # Consume the actual bounded observation made with the existing PCB agent.
    # Recheck every byte-range binding; this is not native syntax/EDA approval.
    raw_board = (root / PACKAGE / "mounting-reference.kicad_pcb").read_bytes()
    board_digest = hashlib.sha256(raw_board).hexdigest()
    observation = read_json(root / PACKAGE / "mounting-source-inventory.json")
    require(observation["scope"] == "bounded_source_metadata_only_native_acceptance_unperformed" and
            observation["release"] == "BLOCKED", "source inventory overstates its scope")

    def check_bindings(value):
        if isinstance(value, dict):
            if "source_sha256" in value:
                require(value["relative_path"] == "mounting-reference.kicad_pcb" and
                        value["source_sha256"] == board_digest, "mounting source bytes changed")
                start, end = value["node_start_byte"], value["node_end_byte"]
                require(type(start) is int and type(end) is int and 0 <= start < end <= len(raw_board),
                        "source inventory byte range invalid")
                require(hashlib.sha256(raw_board[start:end]).hexdigest() == value["node_sha256"],
                        "source inventory node binding changed")
            for child in value.values():
                check_bindings(child)
        elif isinstance(value, list):
            for child in value:
                check_bindings(child)

    check_bindings(observation)
    board = observation["inventory"]
    require([(net["code"], net["name"]) for net in board["nets"]] == [(0, "")] and
            board["segments"] == [], "mounting reference contains electrical routing")
    require(len(board["footprints"]) == 4, "mounting source must contain four holes")
    for index, (footprint, point) in enumerate(zip(board["footprints"], local), start=1):
        require(footprint["reference"] == f"H{index}" and len(footprint["pads"]) == 1,
                "mounting source footprint identity changed")
        require(all(close(float(footprint["at"][key]), 100 + point[i])
                    for i, key in enumerate(("x_mm", "y_mm"))),
                "native-source hole axes differ from CAD input")
        pad = footprint["pads"][0]
        require(pad["pad_type"] == "np_thru_hole" and pad["net_code"] is None and
                close(float(pad["drill"]["diameter_mm"]), mechanical["hole_diameter_mm"]),
                "mounting source hole type/diameter changed")

    budget = data["analog_signal_budget"]
    observations = budget["required_observations"]
    require(len(observations) == len(set(observations)) == 7, "analog observation budget changed")
    require(budget["single_ads1115_inputs"] == 4, "ADS1115 channel count changed")
    require(budget["additional_measurement_path_required"] is True and
            budget["topology_selected"] is False, "measurement shortage must remain unresolved")
    expected_rate = 2 * budget["pressure_rate_per_channel_hz"]["maximum"] + 5 * budget["health_rate_hz"]
    require(budget["pressure_plus_reference_and_four_rails_at_max_requested_rate_sps"] == expected_rate,
            "minimum observation-rate calculation disagrees")

    snapshot_path = root / PACKAGE / "boardspec.json"
    snapshot = read_json(snapshot_path)
    domain_review = read_json(root / PACKAGE / "boardspec-review.json")
    require(hashlib.sha256(snapshot_path.read_bytes()).hexdigest() == domain_review["boardspec_sha256"],
            "canonical BoardSpec bytes changed")
    require(snapshot["record_hash"] == domain_review["boardspec_record_hash"], "BoardSpec identity changed")
    require(snapshot["project_id"] == "4runner-ac-telemetry-node" and
            snapshot["project_mode"] == "COPILOT", "BoardSpec project/mode changed")
    significant = [field for field in snapshot["fields"] if field["significant"]]
    required_paths = {"contracts." + identity.lower() for identity in CONTRACT_IDS}
    require({field["path"] for field in significant} == required_paths and len(significant) == len(required_paths),
            "canonical BoardSpec lost a significant contract")
    require(all(field["status"] == "unknown" and field["value"] is None for field in significant),
            "canonical BoardSpec silently selected a contract")
    require(set(domain_review["blocking_schematic_paths"]) == required_paths and
            all(domain_review[key] is False for key in (
                "autopilot_allowed", "requirements_approval_present", "schematic_compilation_allowed", "fabrication_allowed")),
            "canonical BoardSpec review grants authority")
    risk = read_json(root / PACKAGE / "risk-intake.json")
    scope_bytes = (root / PACKAGE / "scope-input.txt").read_bytes()
    require(hashlib.sha256(scope_bytes).hexdigest() == risk["query_sha256"], "risk scope bytes changed")
    require(risk["disposition"] == "OUT_OF_SCOPE" and risk["risk_tier"] == snapshot["risk_tier"],
            "unsupported automotive/radio scope was silently enabled")
    require(all(risk[key] is False for key in (
        "establishes_confirmed_requirements", "reports_engineering_pass", "can_advance_lifecycle",
        "authorizes_fabrication", "authorizes_order")), "risk intake cannot grant design authority")

    return {
        "schema_version": "ac-carrier-input-review/1",
        "input_consistency": "PASS",
        "design_state": "BLOCKED_REQUIREMENTS",
        "unresolved_contracts": [item["id"] for item in contracts],
        "schematic_compilation_allowed": False,
        "fabrication_allowed": False,
        "ordering_allowed": False,
        "native_eda": "NOT_RUN",
        "electrical_validation": "INCOMPLETE",
        "physical_validation": "NOT_RUN",
        "verified_source_files": len(source_files),
        "retained_slide_axes_match": True,
        "mounting_source_byte_ranges_match": True,
        "canonical_boardspec_bytes_bound": True,
        "automatic_generation_capability": "OUT_OF_SCOPE",
        "minimum_requested_analog_observations_sps": expected_rate,
        "input_sha256": hashlib.sha256(inputs.read_bytes()).hexdigest(),
        "mounting_reference_sha256": board_digest,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write-report", action="store_true")
    args = parser.parse_args()
    result = verify()
    if args.write_report:
        result["observed_at"] = datetime.now(timezone.utc).isoformat()
        (ROOT / PACKAGE / "input-review.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
