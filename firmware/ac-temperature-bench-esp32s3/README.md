# A/C temperature bench firmware

`v0.2.0-dev.1` is a non-release bench profile that adds one real temperature
source without weakening the `EMPTY_RECOVERY` image.

## Implemented now

- Reads the ESP32-S3 internal die-temperature sensor at 1 Hz using the official
  ESP-IDF 5.5.3 `esp_driver_tsens` component.
- Uses the ESP32-S3 driver's documented `-10..80 C` range, whose selected
  attribute reports a maximum driver error of 1 C.
- Labels the observation `BOARD_DIE_HEALTH_ONLY` and `SENSOR_UNVERIFIED`. It is
  never presented as ambient, refrigerant-line, cabin-return, or vent temperature.
- Emits monotonic, versioned JSON evidence over USB serial.
- Keeps high-line, low-line, ambient, cabin-return, and center-vent channels
  explicit as `HARDWARE_NOT_SELECTED` with null values.
- Keeps Wi-Fi and BLE unlinked and uninitialized, preserves NVS, and retains
  A/B rollback. A pending image is accepted only if the platform and die-temp
  driver initialize successfully.

## External probes remain gated

The focused specification says to select two or three automotive-rated contact
probes but does not freeze an orderable part, electrical interface, pin map, or
calibration. This target therefore contains no generic thermistor equation,
OneWire driver, RTD driver, or fake reading. External drivers may be added only
after the selection record in `docs/development/TEMPERATURE-SENSORS.md` is complete.

## Build

```bash
docker run --rm \
  -v "$PWD:/project" \
  -w /project/firmware/ac-temperature-bench-esp32s3 \
  espressif/idf:v5.5.3 \
  bash -lc 'idf.py set-target esp32s3 && idf.py build'
```

This profile has not been physically flashed or promoted as a release.
