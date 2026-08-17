#!/usr/bin/env python3
"""Verify the standalone A/C node source and checked-in recovery release."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
FIRMWARE = ROOT / "firmware" / "ac-sensor-node-esp32s3"
RELEASE = ROOT / "releases" / "v0.1.0-dev.1"
EXPECTED_FLASH_BYTES = 16 * 1024 * 1024
EXPECTED_NVS = {"address": 0x9000, "byteCount": 0x4000}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def read_json(path: Path) -> dict:
    value = json.loads(path.read_text(encoding="utf-8"))
    require(isinstance(value, dict), f"{path.relative_to(ROOT)} must contain an object")
    return value


def verify_source_boundary() -> None:
    source = "\n".join(
        path.read_text(encoding="utf-8")
        for path in sorted((FIRMWARE / "main").glob("*"))
        if path.suffix in {".c", ".h"}
    )
    for fragment in (
        "EMPTY_RECOVERY",
        '\\"wifi_state\\":\\"DISABLED\\"',
        '\\"ble_state\\":\\"DISABLED\\"',
        "initialize_nvs_non_destructive",
        "esp_ota_mark_app_valid_cancel_rollback",
        '\\"sensor_data_state\\":\\"UNAVAILABLE\\"',
    ):
        require(fragment in source, f"recovery source is missing {fragment}")

    forbidden = {
        "nvs_flash_erase": "destructive NVS recovery",
        "esp_wifi_init": "Wi-Fi initialization",
        "esp_netif_init": "network initialization",
        "esp_event_loop_create_default": "network event loop",
        "nimble_port_init": "NimBLE initialization",
        "esp_bt_controller_init": "Bluetooth initialization",
        "adc_oneshot_new_unit": "ADC initialization",
        "spi_bus_initialize": "SPI initialization",
        "sdmmc_host_init": "SD initialization",
        "twai_transmit": "vehicle-bus transmission",
    }
    for fragment, authority in forbidden.items():
        require(fragment not in source, f"EMPTY_RECOVERY contains {authority}")


def verify_partitions() -> None:
    partitions: dict[str, tuple[int, int]] = {}
    with (FIRMWARE / "partitions.csv").open(encoding="utf-8", newline="") as source:
        rows = (line for line in source if not line.lstrip().startswith("#"))
        for row in csv.reader(rows):
            if not row or not row[0].strip():
                continue
            name, _, _, offset, size, *_ = [value.strip() for value in row]
            multiplier = 1024 * 1024 if size.upper().endswith("M") else 1024 if size.upper().endswith("K") else 1
            normalized_size = size[:-1] if multiplier != 1 else size
            partitions[name] = (int(offset, 0), int(normalized_size, 0) * multiplier)

    require(partitions["nvs"] == (EXPECTED_NVS["address"], EXPECTED_NVS["byteCount"]), "NVS range changed")
    require(partitions["otadata"] == (0xD000, 0x2000), "OTA-data range changed")
    require(partitions["ota_0"] == (0x10000, 0x300000), "primary OTA slot changed")
    require(partitions["ota_1"] == (0x310000, 0x300000), "secondary OTA slot changed")


def verify_release() -> None:
    manifest = read_json(RELEASE / "manifest-ac-sensor-node-esp32s3.json")
    validation = read_json(RELEASE / "recovery-validation-ac-sensor-node-esp32s3.json")
    require(manifest["release"] == "v0.1.0-dev.1", "unexpected release version")
    require(manifest["softwareProfile"] == "EMPTY_RECOVERY", "release is not EMPTY_RECOVERY")
    require(manifest["runtimeRadios"] == {"wifi": "not-initialized", "ble": "not-initialized"}, "radio state changed")
    require(validation["expectedFlashBytes"] == EXPECTED_FLASH_BYTES, "flash capacity gate changed")
    require(validation["checks"]["physicalFlashAndBoot"] == "not-run-by-release-builder", "physical gate was overstated")
    protected = manifest["protectedRanges"]
    require(len(protected) == 1, "release must publish exactly one protected NVS range")
    require({key: protected[0][key] for key in EXPECTED_NVS} == EXPECTED_NVS, "manifest NVS range changed")

    checksum_lines = (RELEASE / "SHA256SUMS-ac-sensor-node-esp32s3").read_text(encoding="utf-8").splitlines()
    expected_checksums = {}
    for line in checksum_lines:
        digest, filename = line.split(None, 1)
        expected_checksums[filename.strip()] = digest
    for filename, digest in expected_checksums.items():
        path = RELEASE / filename
        require(path.is_file(), f"release artifact is missing: {filename}")
        require(sha256(path) == digest, f"checksum mismatch: {filename}")

    merged_path = RELEASE / Path(manifest["artifact"]["url"]).name
    merged = merged_path.read_bytes()
    require(len(merged) == manifest["artifact"]["byteCount"], "merged byte count differs from manifest")
    require(sha256(merged_path) == manifest["artifact"]["sha256"], "merged checksum differs from manifest")
    nvs_start = EXPECTED_NVS["address"]
    nvs_end = nvs_start + EXPECTED_NVS["byteCount"]
    for segment in manifest["segments"]:
        path = RELEASE / Path(segment["url"]).name
        data = path.read_bytes()
        address = segment["address"]
        require(len(data) == segment["byteCount"], f"segment size mismatch: {path.name}")
        require(sha256(path) == segment["sha256"], f"segment checksum mismatch: {path.name}")
        require(merged[address : address + len(data)] == data, f"merged segment mismatch: {path.name}")
        require(address + len(data) <= nvs_start or address >= nvs_end, f"segment overlaps NVS: {path.name}")


def verify_documents() -> None:
    for path in sorted((ROOT / "contracts" / "jsonschema" / "v1").glob("*.json")):
        read_json(path)
    equations = sorted((ROOT / "equations" / "v1").glob("ac-*.json"))
    require(len(equations) == 3, "expected exactly three versioned A/C equations")
    for path in equations:
        equation = read_json(path)
        require(equation["validation_status"] == "EXPERIMENTAL", f"{path.name} overstates validation")
    spec_text = ROOT / "docs" / "prd" / "2005_Toyota_4Runner_AC_Engine_Bay_Telemetry_Node_Spec_v1.0.txt"
    require(spec_text.stat().st_size > 20_000, "focused specification text export is missing or truncated")
    master = ROOT / "docs" / "prd" / "4Runner_Telemetry_Build_Master_Spec.docx"
    require(master.is_file(), "historical master DOCX is missing")


def verify_build(build_dir: Path) -> None:
    sdkconfig = (build_dir / "config" / "sdkconfig.h").read_text(encoding="utf-8")
    require('#define CONFIG_IDF_TARGET "esp32s3"' in sdkconfig, "compiled target is not ESP32-S3")
    require("#define CONFIG_ESPTOOLPY_FLASHSIZE_16MB 1" in sdkconfig, "compiled flash size is not 16 MB")
    require("#define CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE 1" in sdkconfig, "compiled rollback is disabled")
    require("#define CONFIG_BT_ENABLED 1" not in sdkconfig, "Bluetooth is compiled into EMPTY_RECOVERY")

    project = read_json(build_dir / "project_description.json")
    components = set(project.get("build_components", []))
    forbidden = {"bt", "esp_netif", "esp_wifi", "lwip", "wifi_provisioning"}
    require(not components.intersection(forbidden), "radio or network components are linked into EMPTY_RECOVERY")
    application = build_dir / "vhos_ac_sensor_node.bin"
    require(application.is_file(), "compiled application binary is missing")
    require(application.stat().st_size <= 0x300000, "compiled application exceeds an OTA slot")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path)
    args = parser.parse_args()
    verify_source_boundary()
    verify_partitions()
    verify_release()
    verify_documents()
    if args.build_dir is not None:
        verify_build(args.build_dir.resolve())
    print("verified: EMPTY_RECOVERY source boundary, partitions, release bytes, contracts, equations, and specifications")


if __name__ == "__main__":
    main()
