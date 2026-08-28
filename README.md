# 4Runner A/C Telemetry Node

Standalone hardware, firmware, contract, equation, and release repository for
the 2005 Toyota 4Runner engine-bay A/C telemetry node.

The checked-in release is `v0.1.0-dev.1` with the `EMPTY_RECOVERY` software
profile. A separate `v0.2.0-dev.1` `TEMPERATURE_BENCH` target adds the real
ESP32-S3 internal die-temperature sensor as a board-health observation while
keeping every unselected external probe explicit and unavailable. Both targets
use A/B rollback, preserve NVS, and omit Wi-Fi, BLE, storage, pressure, and
vehicle-bus paths.

## Current safety boundary

- Wi-Fi and BLE are absent from the recovery build, not merely hidden in the UI.
- The temperature bench target also omits Wi-Fi and BLE and never labels the
  internal die sensor as ambient, refrigerant-line, cabin, or vent temperature.
- A/C probe pressure and temperature values remain unavailable until exact sensor parts,
  transfer functions, interfaces, and calibration provenance are frozen.
- The release installer preserves NVS at `0x9000..0xCFFF`.
- A/B OTA slots and bootloader rollback remain available for recovery.
- The checked-in release is byte-verified but is not labeled physically
  flash-and-boot validated. That unresolved gate is machine-readable in the
  release validation record.
- Nothing in this repository authorizes refrigerant recovery, charging,
  evacuation, venting, or permanent fitting changes.

## Repository map

- `firmware/ac-sensor-node-esp32s3/` — ESP-IDF 5.5.3 firmware and release builder.
- `firmware/ac-temperature-bench-esp32s3/` — real board die-temperature bench target.
- `releases/v0.1.0-dev.1/` — exact installer binaries, manifest, checksums, and validation evidence.
- `contracts/jsonschema/v1/` — versioned POST and telemetry contracts.
- `equations/v1/` — pressure lift, absolute pressure ratio, and vent delta-T definitions.
- `docs/prd/` — focused specification text export, source record, and earlier master DOCX.
- `docs/development/` — empty-recovery authority and physical validation gate.
- `docs/PROVENANCE.md` — extraction history and upstream ESP32 design decisions.

## Verify the repository

```bash
python3 tools/verify_repository.py
```

This verifies release hashes and sizes, reconstructs every merged-image segment,
checks the protected NVS range, rejects radio/sensor/vehicle-bus initialization
in the recovery source, and parses every checked-in schema and equation.

## Build

From the repository root:

```bash
docker run --rm \
  -v "$PWD:/project" \
  -w /project/firmware/ac-sensor-node-esp32s3 \
  espressif/idf:v5.5.3 \
  bash -lc 'idf.py set-target esp32s3 && idf.py build'
```

Or use `make build` to compile both the recovery and temperature-bench targets.
A recovery release build requires a clean worktree:

```bash
make release VERSION=v0.1.0-dev.1
```

The temperature-bench target is development software, not a published flash
release. See `docs/development/TEMPERATURE-SENSORS.md` for the external-probe
selection and calibration gate.

## Recovery installation

Use the VHOS Device Provisioner with the exact target **ESP32-S3 + A/C Sensor
Node — Wi-Fi-off recovery**. The browser must detect ESP32-S3 and exactly 16 MB
of flash, save a complete backup, verify every segment, and receive explicit
hardware confirmation before writing.

This repository is extracted from the public
[`4runner-vehicle-health-os`](https://github.com/IsaiahDupree/4runner-vehicle-health-os)
project. See `docs/PROVENANCE.md` for exact source commits and evidence lineage.

## License

Repository source is published under the GNU General Public License v3.0. See
`LICENSE`.
