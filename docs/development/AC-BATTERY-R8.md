# R8 A/C pack monitor

The 21700 cradle, strap and six-position Molex connection hold and connect the factory-terminated pack. The pack specification calls for fused positive, raw cell negative and two independent 10 kΩ thermistors. The MAX17260 fuel gauge belongs on the single main PCB. Its high-side 10 mΩ shunt monitors both charge and discharge. The pack-negative isolation FETs and the two hardware NTC guards retain their existing connections. The new NTC amplifier reads the run sensor through a 1 MΩ isolation resistor and feeds bank B of the existing ADS1115 AIN3 path. Bank A still reads the divided pack supply.

The R8 development target uses ESP32-S3 IO15 for SDA, IO16 for SCL and IO17 for the analog mux. The I²C addresses are 0x36 for MAX17260 and 0x48 for ADS1115. Firmware uses the gauge's power-on reset and model-ready bits, the 9.8 Ah engineering pack capacity, the 10 mΩ shunt and 100 mA nominal termination. The indicated pack temperature is an estimate using the specified 103AT-2 curve. The capture contains the raw gauge registers and ADC counts. Validate the final pack, empty voltage and charge termination on the bench before using the estimates to make operational decisions. [MAX17260 data sheet](https://www.analog.com/media/en/technical-documentation/data-sheets/max17260.pdf) · [ModelGauge startup guide](https://www.analog.com/media/en/technical-documentation/user-guides/modelgauge-m5-host-side-software-implementation-guide.pdf)

This target provides battery telemetry in RUN. In OFF/CHARGE, the hardware charger drives its status LED and the ESP32 powers down. The gauge can integrate charge while the pack powers it, after the model has been configured. The gauge's equivalent cycle count belongs to the currently powered gauge session. A pack replacement or a gauge power loss can reset learning. A separate power-path revision is needed for live app readings during charging.

## BLE and evidence

The R8 engineering firmware exposes a read-only BLE service, with two encrypted characteristics. The iPhone System Status screen opens the A/C battery connection. Android Start / Reacquire opens an independent A/C link and stores samples in its verified vehicle profile. Each app validates the service, CRC32C, protocol and the A/C identity before displaying a new reading. They retain the exact raw frame before showing derived metrics. They clear the reading on disconnect or a stale signal.

| Entry | UUID |
| --- | --- |
| A/C battery service | `CB09443D-4F7E-4529-A8E6-E84391864ACE` |
| Identity frame | `A81A38E3-C36C-4EC7-80CC-9199C922E624` |
| Battery sample frame | `9686641E-3831-428A-891F-BCFE4846B0D9` |

The 36-byte VHOS envelope uses protocol 1.0, flags 0 and CRC32C. Type 1 carries the `sensor.node.battery.handshake` 1.0.0 JSON claim with role `AC_SENSOR`, hardware `AC_R8`, the source and boot IDs, build digest and capabilities `battery.max17260` and `battery.pack_ntc`. Type 14 carries the 32-byte battery record. Multi-byte fields use little endian.

| Offset | Representation |
| ---: | --- |
| 0 | Battery payload version 1 |
| 1 | Validity flags, bit 0 for the gauge and bit 1 for the external pack thermistor |
| 2 | Result code 0 valid, 1 gauge I/O, 2 model, 3 range, 4 pack thermistor |
| 3 | Reserved zero |
| 4–13 | Five 16-bit words in order for gauge Status, VCell, Current, AvgCurrent and RepSOC |
| 14–25 | Six 16-bit words in order for RepCap, FullCapRep, Age, Cycles, TTE and TTF |
| 26–31 | Three signed 16-bit words for pack centi-degrees Celsius, bank A raw ADC and bank B raw ADC |

Current is signed and positive into the pack. Voltage scale is 78.125 μV per count, current is 156.25 μA per count at 10 mΩ, charge percentage and age use 1/256 percent and capacity uses 0.5 mAh per count. Remaining time uses 5.625 seconds per count. Display times only in the corresponding current direction and when the register differs from 0xffff.

## Verification

Use the separate `firmware/ac-battery-monitor-esp32s3` target with installed ESP-IDF 5.5.3 on Orion. Pass `-D VHOS_BUILD_ID=<hex-source-digest>` at configure time. The source target compiles separately from the empty recovery and temperature-only images. The C test vector in `tests/ac_battery_wire_vector.c` uses the production encoder. The Swift and Kotlin tests decode that same frame and exercise corruption and unavailable readings.

Physical validation needs a populated R8 fuel-gauge board and matched pack emulator. Compare current and voltage against a meter in both directions, temperature against known resistances, the charging state against the physical LED and both mobile displays against retained raw bytes. Check both USB-C plug orientations. Check iPhone and Android pairing, reconnect and two-client reads on the intended devices before release.
