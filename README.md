# 4Runner A/C Telemetry Node

Standalone hardware, firmware, contract, equation, and release repository for
the 2005 Toyota 4Runner engine-bay A/C telemetry node.

The current firmware is `v0.1.0-dev.1` with the `EMPTY_RECOVERY` software
profile. It is a deliberately minimal USB recovery image for the bench-verified
16 MB flash / 8 MB octal-PSRAM ESP32-S3 board. It reports identity and health,
uses A/B rollback, preserves NVS, and does not link or initialize Wi-Fi, BLE,
sensor, storage, or vehicle-bus paths.

## Current safety boundary

- Wi-Fi and BLE are absent from the recovery build, not merely hidden in the UI.
- Pressure and temperature values remain unavailable until exact sensor parts,
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

Or use `make build`. A release build requires a clean worktree:

```bash
make release VERSION=v0.1.0-dev.1
```

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
