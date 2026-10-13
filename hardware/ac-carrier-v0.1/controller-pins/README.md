# Draft controller pin reservations

Engineering **INCOMPLETE**. Release **BLOCKED**. Open the
[report and diagram](review/index.html), [readable report](review/review.md) or
[pictures](pictures/index.html). The [request](request.json) binds the real
BoardSpec, discovery, focused specification and controller source reconciliation.

Two unselected partial Nano ESP32 reference proposals use nine signals, or eleven
with optional ADC interrupt inputs. They cover default ADC I2C, SD SPI/CS, a
future custom-socket detect input and independent HIGH/LOW sensor enables. The
partial reservations fit without collision under the conservative default-bus,
UART-preservation and strapping-pin-exclusion policy. They do not complete the
controller assignment, select an ADC topology or implement firmware acquisition.

The printed Nano label **D2** maps to ESP32 **GPIO5** (`GPIO_NUM_5`), while the
default Arduino pin number is 2. The software keeps those namespaces distinct.
Manufacturer JP1/JP2 references identify drawing pins, not a qualified footprint.
D13/GPIO48 shares the on-board SCK LED; its loading still needs qualification.

The later UNO-format ESP32-S3 board remains a candidate whose owned identity,
electrical header map and power path are unverified. No Nano mapping is applied
to it and no controller migration or new mounting choice is made. Temperature
interfaces, Pi link, power, exact switches and boot-time enable behavior remain
unresolved. A reserved detect pin does not establish a detect capability on the
Adafruit #4682 breakout or select a final socket.

The [software guide](https://github.com/IsaiahDupree/pcb-agent/blob/main/docs/controller-pin-planner.md)
provides replay commands. The
[execution receipt](../evidence/controller-pin-test-2026-10-13.json) records
software verification. All fourteen significant BoardSpec contracts remain
unchanged and incomplete; no wired schematic, populated A/C PCB or hardware
qualification is produced by this review.

Reference pin facts come from the
[Arduino pinout](https://docs.arduino.cc/resources/pinouts/ABX00083-full-pinout.pdf),
[Arduino schematic](https://docs.arduino.cc/resources/schematics/ABX00083-schematics.pdf)
and [ESP-IDF 5.5.3 GPIO summary](https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32s3/api-reference/peripherals/gpio.html).
Arduino resources are © Arduino SA, [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/).
