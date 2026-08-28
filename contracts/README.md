# A/C node contracts

`jsonschema/v1/` contains immutable evidence contracts. Add a new contract or
version rather than changing a released meaning.

The `sensor.node.temperature-bench/0.1.0` observation is intentionally smaller
than installed-vehicle telemetry because the bench board has no validated wall
clock, vehicle assignment, or capture assignment. It distinguishes the ESP32-S3
internal die-temperature observation from every external A/C temperature role.
External channels remain null and `HARDWARE_NOT_SELECTED` until their exact
sensor, driver, binding, and calibration records exist.
