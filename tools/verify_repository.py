#!/usr/bin/env python3
"""Verify the standalone A/C node targets and checked-in recovery release."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RECOVERY_FIRMWARE = ROOT / "firmware" / "ac-sensor-node-esp32s3"
TEMPERATURE_FIRMWARE = ROOT / "firmware" / "ac-temperature-bench-esp32s3"
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


def source_text(firmware_dir: Path) -> str:
    return "\n".join(
        path.read_text(encoding="utf-8")
        for path in sorted((firmware_dir / "main").glob("*"))
        if path.suffix in {".c", ".h"}
    )


def require_radio_off_source(source: str, profile: str) -> None:
    forbidden = {
        "nvs_flash_erase": "destructive NVS recovery",
        "esp_wifi_init": "Wi-Fi initialization",
        "esp_netif_init": "network initialization",
        "esp_event_loop_create_default": "network event loop",
        "nimble_port_init": "NimBLE initialization",
        "esp_bt_controller_init": "Bluetooth initialization",
        "twai_transmit": "vehicle-bus transmission",
    }
    for fragment, authority in forbidden.items():
        require(fragment not in source, f"{profile} contains {authority}")


def verify_source_boundary() -> None:
    source = source_text(RECOVERY_FIRMWARE)
    for fragment in (
        "EMPTY_RECOVERY",
        '\\"wifi_state\\":\\"DISABLED\\"',
        '\\"ble_state\\":\\"DISABLED\\"',
        "initialize_nvs_non_destructive",
        "esp_ota_mark_app_valid_cancel_rollback",
        '\\"sensor_data_state\\":\\"UNAVAILABLE\\"',
    ):
        require(fragment in source, f"recovery source is missing {fragment}")

    require_radio_off_source(source, "EMPTY_RECOVERY")
    forbidden = {
        "adc_oneshot_new_unit": "ADC initialization",
        "spi_bus_initialize": "SPI initialization",
        "sdmmc_host_init": "SD initialization",
    }
    for fragment, authority in forbidden.items():
        require(fragment not in source, f"EMPTY_RECOVERY contains {authority}")


def verify_temperature_source_boundary() -> None:
    source = source_text(TEMPERATURE_FIRMWARE)
    required = (
        "TEMPERATURE_BENCH",
        "temperature_sensor_install",
        "temperature_sensor_enable",
        "temperature_sensor_get_celsius",
        "TEMPERATURE_SENSOR_CONFIG_DEFAULT",
        "VHOS_BOARD_TEMP_RANGE_MIN_C (-10)",
        "VHOS_BOARD_TEMP_RANGE_MAX_C 80",
        "ESP32_S3_INTERNAL_DIE",
        "BOARD_DIE_HEALTH_ONLY",
        "SENSOR_UNVERIFIED",
        "HARDWARE_NOT_SELECTED",
        "UNCONFIGURED",
        "esp-idf.esp32s3.tsens/5.5.3",
        "esp_ota_mark_app_valid_cancel_rollback",
    )
    for fragment in required:
        require(fragment in source, f"temperature bench source is missing {fragment}")
    require_radio_off_source(source, "TEMPERATURE_BENCH")
    forbidden_external_assumptions = {
        "adc_oneshot_new_unit": "analog temperature input",
        "onewire": "OneWire temperature input",
        "ds18": "DS18 temperature input",
        "max318": "MAX318 temperature input",
        "thermistor": "thermistor conversion",
        "spi_bus_initialize": "SPI temperature input",
        "i2c_master_bus_add_device": "I2C temperature input",
    }
    lowered = source.lower()
    for fragment, authority in forbidden_external_assumptions.items():
        require(fragment.lower() not in lowered, f"temperature bench contains unselected {authority}")
    require('\\"calibration\\":{\\"configured\\":false' in source,
            "temperature bench must retain unconfigured calibration evidence")


def verify_partitions(firmware_dir: Path, profile: str) -> None:
    partitions: dict[str, tuple[int, int]] = {}
    with (firmware_dir / "partitions.csv").open(encoding="utf-8", newline="") as source:
        rows = (line for line in source if not line.lstrip().startswith("#"))
        for row in csv.reader(rows):
            if not row or not row[0].strip():
                continue
            name, _, _, offset, size, *_ = [value.strip() for value in row]
            multiplier = 1024 * 1024 if size.upper().endswith("M") else 1024 if size.upper().endswith("K") else 1
            normalized_size = size[:-1] if multiplier != 1 else size
            partitions[name] = (int(offset, 0), int(normalized_size, 0) * multiplier)

    require(partitions["nvs"] == (EXPECTED_NVS["address"], EXPECTED_NVS["byteCount"]), f"{profile} NVS range changed")
    require(partitions["otadata"] == (0xD000, 0x2000), f"{profile} OTA-data range changed")
    require(partitions["ota_0"] == (0x10000, 0x300000), f"{profile} primary OTA slot changed")
    require(partitions["ota_1"] == (0x310000, 0x300000), f"{profile} secondary OTA slot changed")


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
    temperature_schema = read_json(
        ROOT / "contracts" / "jsonschema" / "v1" /
        "temperature-bench-observation.schema.json"
    )
    require(
        temperature_schema["properties"]["software_profile"]["const"] ==
        "TEMPERATURE_BENCH",
        "temperature contract profile changed",
    )
    channels = temperature_schema["properties"]["channels"]["properties"]
    require(
        set(channels) == {
            "board", "high_line", "low_line", "ambient", "cabin_return", "center_vent"
        },
        "temperature contract channel roles changed",
    )
    require(
        temperature_schema["$defs"]["boardChannel"]["properties"]
        ["semantic_role"]["const"] == "BOARD_DIE_HEALTH_ONLY",
        "board die temperature role was overstated",
    )
    require(
        temperature_schema["$defs"]["unconfiguredExternalChannel"]
        ["properties"]["state"]["const"] == "HARDWARE_NOT_SELECTED",
        "external temperature hardware gate was removed",
    )
    temperature_notes = (
        ROOT / "docs" / "development" / "TEMPERATURE-SENSORS.md"
    ).read_text(encoding="utf-8")
    for fragment in (
        "BOARD_DIE_HEALTH_ONLY",
        "HARDWARE_NOT_SELECTED",
        "superheat",
        "subcooling",
    ):
        require(
            fragment.lower() in temperature_notes.lower(),
            f"temperature notes are missing {fragment}",
        )


def verify_build(
    build_dir: Path,
    app_name: str,
    profile: str,
    require_temperature_driver: bool,
) -> None:
    sdkconfig = (build_dir / "config" / "sdkconfig.h").read_text(encoding="utf-8")
    require('#define CONFIG_IDF_TARGET "esp32s3"' in sdkconfig, "compiled target is not ESP32-S3")
    require("#define CONFIG_ESPTOOLPY_FLASHSIZE_16MB 1" in sdkconfig, "compiled flash size is not 16 MB")
    require("#define CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE 1" in sdkconfig, "compiled rollback is disabled")
    require("#define CONFIG_BT_ENABLED 1" not in sdkconfig, f"Bluetooth is compiled into {profile}")

    project = read_json(build_dir / "project_description.json")
    components = set(project.get("build_components", []))
    forbidden = {"bt", "esp_netif", "esp_wifi", "lwip", "wifi_provisioning"}
    require(
        not components.intersection(forbidden),
        f"radio or network components are linked into {profile}",
    )
    if require_temperature_driver:
        require("esp_driver_tsens" in components, "temperature driver is not linked into TEMPERATURE_BENCH")
    else:
        require("esp_driver_tsens" not in components, "temperature driver leaked into EMPTY_RECOVERY")
    application = build_dir / f"{app_name}.bin"
    require(application.is_file(), f"compiled {profile} application binary is missing")
    require(application.stat().st_size <= 0x300000, f"compiled {profile} application exceeds an OTA slot")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--recovery-build-dir", type=Path)
    parser.add_argument("--temperature-build-dir", type=Path)
    args = parser.parse_args()
    verify_source_boundary()
    verify_temperature_source_boundary()
    verify_partitions(RECOVERY_FIRMWARE, "EMPTY_RECOVERY")
    verify_partitions(TEMPERATURE_FIRMWARE, "TEMPERATURE_BENCH")
    verify_release()
    verify_documents()
    if args.recovery_build_dir is not None:
        verify_build(
            args.recovery_build_dir.resolve(),
            "vhos_ac_sensor_node",
            "EMPTY_RECOVERY",
            False,
        )
    if args.temperature_build_dir is not None:
        verify_build(
            args.temperature_build_dir.resolve(),
            "vhos_ac_temperature_bench",
            "TEMPERATURE_BENCH",
            True,
        )
    print(
        "verified: recovery and temperature source boundaries, partitions, "
        "release bytes, contracts, equations, and specifications"
    )


if __name__ == "__main__":
    main()
