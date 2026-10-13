# Temperature-sensor software and hardware gate

Status: board-health channel implemented; external probes not selected

## What software can do now

The `TEMPERATURE_BENCH` profile reads the ESP32-S3 internal die-temperature
sensor once per second through ESP-IDF 5.5.3. This is a real local observation
for board-health development, but it is not ambient air temperature and cannot
substitute for a refrigerant-line, cabin-return, or vent probe.
Its contract semantic role is therefore fixed as `BOARD_DIE_HEALTH_ONLY`.

The driver configuration uses the ESP32-S3 attribute range `-10..80 C`. The
pinned ESP-IDF table associates that range with a maximum driver error of 1 C.
That manufacturer/framework claim is retained as driver metadata; it is not a
project calibration or a vehicle-validation claim.

Primary pinned sources:

- ESP-IDF v5.5.3 API header: <https://github.com/espressif/esp-idf/blob/v5.5.3/components/esp_driver_tsens/include/driver/temperature_sensor.h>
- ESP32-S3 range table: <https://github.com/espressif/esp-idf/blob/v5.5.3/components/soc/esp32s3/temperature_sensor_periph.c>
- Official example: <https://github.com/espressif/esp-idf/blob/v5.5.3/examples/peripherals/temperature_sensor/temp_sensor/main/temp_sensor_main.c>

## External channel authority

The focused A/C specification calls for two or three automotive-rated contact
probes and identifies these roles:

- suction/low refrigerant line;
- liquid/high refrigerant line;
- ambient air;
- optional cabin return/inlet; and
- optional center vent.

It does not select an orderable part or electrical interface. Consequently,
the software publishes those channels as null, `MISSING`, and
`HARDWARE_NOT_SELECTED`. It contains no generic thermistor curve, OneWire
assumption, RTD conversion, or hard-coded pin map.

## Required selection record before an external driver

Each enabled external channel must freeze and verify:

1. manufacturer and orderable part number;
2. datasheet revision and SHA-256;
3. sensing technology and electrical interface;
4. temperature range, accuracy, response time, and environmental rating;
5. contact attachment, insulation, strain relief, and harness/connector;
6. ESP32/carrier binding, voltage compatibility, protection, and fault behavior;
7. raw observation unit and exact transfer function;
8. calibration identity, revision, reference instrument, points, uncertainty, and validation state; and
9. open, short, stuck, out-of-range, disconnect, and recovery evidence.

Only then may the channel emit a Celsius value with a quality stronger than
`SENSOR_UNVERIFIED`. Superheat, subcooling, and vent delta-T remain unavailable
until all required inputs and equation applicability conditions are satisfied.
