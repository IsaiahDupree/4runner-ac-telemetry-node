# H2 and R9 integrated-board commissioning

This is a real ESP-IDF bring-up program for the custom, single-PCB designs.
Select **H2 or R9 explicitly**. The H2 ADS1115 is at 0x49, avoiding its
TSC2007 at 0x48; the R9 ADC is at 0x48. Both use the MAX17260 at 0x36.
The older AC_R8 battery firmware remains a separate target.

USB Serial/JTAG reports the compiled source hash, physical MAC, board profile,
real per-register gauge I/O, raw ADC counts/input voltage for both mux banks,
SD card-detect and pressure-switch fault levels. Every failed read has `null`
data plus its error. ADC input voltage is not calibrated sensor voltage or
pressure. Sensor supplies stay off because the actual probes are unidentified.
The fuel gauge is read-only: no guessed pack model is written, and raw gauge
registers are marked unqualified rather than presented as valid battery status.

H2 drives the HX8357D screen, debounces all four keys in a separate task and
reports raw TSC2007 touch coordinates. MENU changes the commissioning page.
The screen keeps missing probe/pack calibration explicit. It does not provide
the finished operating UI, calibrated touch or a tested mobile link. SD storage
read/write, selected probe drivers and the calibrated pack/BLE integration are
still separate unfinished work. SPI transaction success is not visual proof.

The current hardware charges in OFF/CHARGE with the MCU powered down. It has
no software charger register or direct VBUS-detect input; USB serial activity
does not prove charging current. This firmware cannot report live charging
statistics while the MCU is off. Selecting charge-while-running would require
a reviewed power-path hardware revision before making that product claim.

Build on **owned Orion only**, with the installed offline ESP-IDF v5.5.3 image,
two build jobs and a source manifest hash. Invoke `idf.py -D AC_BOARD=H2
-D AC_SOURCE_SHA256=<64 hex source seal> reconfigure` (or R9), then the bounded
native build. The source-pinned controller retains binaries, ELF, maps, SDK
configuration, partitions and toolchain identity. It never flashes hardware.

`python3 tools/verify_integrated_commissioning.py` exercises production math
across the full ADC domain, byte order, invalid channel rejection and the
compiled physical-board profiles. Repository verification and the required
EMPTY_RECOVERY build remain necessary. These checks are not a hardware pass.

Before flashing, identify the exact assembled board, inspect the unresolved
PCB routes, qualify the pack/probes, back up flash/NVS, verify the bin hash and
apply the existing preservation/restore procedure. Never erase NVS or choose
the first available serial port. No custom board was physically attached or
flashed during source/build verification.

Display initialization data derives from the pinned Adafruit HX8357 driver;
the BSD notice and source identities are in `third_party/`.
