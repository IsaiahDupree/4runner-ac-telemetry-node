# A/C carrier discovery and mounting reference

Status: **BLOCKED_REQUIREMENTS**. This package carries the recent enclosure work
into the A/C electronics repository. It contains an editable KiCad mounting
reference and a proposed electrical architecture. It does not contain a wired
schematic, populated PCB, manufacturing BOM, Gerbers or fabrication approval.

Open [the visual review](review.html) for the mounting pattern, planned circuit
blocks and unresolved decisions. [design-inputs.json](design-inputs.json) is the
versioned input record; null values represent genuinely unresolved contracts.

The [build and test plan](BUILD-AND-TEST.md) defines the board, software work and
acceptance evidence. Its [typed matrix](build-test-plan.json) connects every
active requirement and significant contract to work and tests. The
[software-generated review](product-build-review/index.html) retains the actual
inputs and unresolved tasks; its input checks do not authorize circuit creation.

The [source reconciliation](source-reconciliation.md) recovers controller,
sensor, power, storage and mounting decisions from the parent project, live
Drive specs, CAD implementation and the owner's earlier request. Its
[retained evidence](evidence/source-research-2026-10-12.json) includes exact
source excerpts and hashes. The fourteen incomplete contracts are composite
engineering inputs, not fourteen topics with no known requirements.

[boardspec.json](boardspec.json) uses the existing PCB agent's canonical
`BoardSpec` model and passes its content-hash validation and serialization
roundtrip. All fourteen significant contract fields remain `unknown`; the
mounting axes are a derived, non-authorizing reference. The retained
[model review](boardspec-review.json) binds the exact snapshot bytes.

The existing deterministic intake flags this automotive/radio scope outside
the current automatic generator's supported capability and gives a conservative
Tier C floor. [risk-intake.json](risk-intake.json) preserves that result;
it is not an approved formal risk assessment. A functional carrier needs
guided electrical authoring, qualified assets and a dedicated Orion native
verification profile. The current bounded LED generator cannot synthesize it.

## Reconciled source authority

The focused August electrical spec explicitly freezes **Arduino Nano ESP32
with headers** for V0/V1 and controls sensor/calibration behavior and its Nano
power path. A fresh Drive text readback matches the retained text byte for byte.
The owner later requested separate Pi and Arduino mounting planes, which the
current cassette follows. Its CAD source links the photographed UNO-format
ESP32-S3 to a likely **DIYables ESP32-S3-WROOM-1-N16R8** listing and identifies
**Waveshare UPS HAT (D)**. The original component photos were not re-inspected;
the source's candidate match does not verify the owned board revision, header
pinout, supply limits or the host Pi model. The electrical migration and new
sensor-PCB role remain to be resolved.

The older parent repository's V0 BOM names Honeywell PX2AF1XX200PSAAX /
PX2AF1XX600PSAAX and Adafruit PT1000/MAX31865 development assemblies. They are
historical alternatives, not selected parts for this later Sensata baseline.
The current focused spec and temperature gate still leave the orderable
pressure variants and external temperature technology open.

The parent firmware record documents an August ESP32-S3 observation with 16 MB
flash and 8 MB PSRAM; the current recovery release requires exactly 16 MB flash.
That historical observation does not prove which board SKU was used or link it
to the later UNO-format candidate. Verify the actual module, flash capacity and
board revision before selecting firmware or flashing.

The retained common-fastener CAD release is shell `mdl_c29e657682e440af`, slide
`mdl_d17c5f5215d74f07` and cross-fit `mdl_5d10914d0a9e4648`, with a maximum
installed plastic envelope of 264.7 × 92 × 97 mm. Unmodified source receipts,
the actual slide measurements and its manifest are copied under `sources/`.
The original cross-fit audit SHA-256 is
`9dbe71295dab05902e480e225961b4674007b43e5df301f634ce3c3788ba6597`;
the complete 18 MB audit remains in the existing common-fastener fit kit.

## Meaning of the editable board

[mounting-reference.kicad_pcb](mounting-reference.kicad_pcb) is a **rectangular
mounting reference**, 68.58 × 53.34 × 1.6 mm, with four 3.2 mm NPTH holes.
Its asymmetric axes use the installed 180° frame from the actual CAD slide;
it is not mirrored. Drawings show the Ø8.4 mm underside support bosses.
Those circles are reference drawings, not implemented electrical keepout rules.
The reference rectangle is not the original UNO's detailed perimeter.

The existing blue platform already holds the controller. This file does not
establish room for a second PCB, a shield stack, new connectors or a replacement
controller. The exact new assembly must receive a fresh populated-envelope,
STEP cross-fit and physical mounting check. The Pi/UPS geometry is a bench
reference; its lithium battery is prohibited underhood.

The file has no nets beyond the empty net, tracks, vias, copper zones or
electrical components. Native reopening/ERC/DRC have not been run. Do not
manufacture this reference as the functional A/C board.

## Electrical findings carried into the design

- Seven distinct analog observations are required: HIGH pressure, LOW pressure,
  ADC reference, protected vehicle input, controller input, sensor 5 V and
  logic 3.3 V. One ADS1115 has four inputs. Add a qualified measurement path
  after the host and GPIO map are known; no topology is frozen here.
- At the maximum requested 50 Hz pressure rate, the minimum count is
  `2 × 50 + 1 + 4 = 105` observations/s. Actual ADC programming, MUX settling,
  filtering and fault scans require a measured schedule.
- LM5164-Q1 is a buck converter. A roughly 7 V controller rail cannot be
  maintained when its protected input falls below that output. Resolve the
  crank waveform and whether operation must continue or restart cleanly;
  do not claim a 100 V input rating proves crank/load-dump qualification.
- For the original Nano only, VIN is 6–21 V and I/O is 3.3 V. VIN-powered VBUS
  does not supply the pressure-sensor 5 V rail. This does not establish the
  unidentified UNO-format board's power path.
- TPS7B83-Q1 is a 150 mA LDO candidate. Its upstream node and thermal budget
  remain open. A conditional 40 mA two-sensor load from 14.4 V would dissipate
  `(14.4 − 5) × 0.04 = 0.376 W`, before quiescent and other loads. From 7 V
  it would dissipate 0.080 W. These are sensitivity examples, not measured
  loads, approved rail choices or junction-temperature passes.
- ADC full-scale selection does not make inputs tolerant of arbitrary voltages.
  Normal/fault voltages, ground offsets, unpowered inputs, clamp currents,
  divider loading and reference uncertainty must be checked against the exact
  parts. Independent sensor switches must also prevent signal back-powering.

Primary manufacturer references, checked 2026-10-12:
[Arduino Nano ESP32 manual](https://docs.arduino.cc/resources/datasheets/ABX00083-datasheet.pdf),
[TI ADS1115-Q1 Rev E](https://www.ti.com/lit/ds/symlink/ads1115-q1.pdf),
[TI LM5164-Q1 Rev C](https://www.ti.com/lit/ds/symlink/lm5164-q1.pdf),
[TI TPS7B83-Q1](https://www.ti.com/lit/ds/symlink/tps7b83-q1.pdf),
[Sensata A/C family](https://www.sensata.com/products/pressure-sensors/ac-pressure-sensor-0).
The links are research references; no exact component-document or purchasable
configuration is frozen by this discovery package.

## Completion gates

1. Resolve the controller/board role and exact pressure and temperature parts.
2. Freeze power, protection, switches, ADC/health allocation, GPIO and connectors
   with manufacturer pin maps and explicit normal/fault limits.
3. Produce the wired schematic and complete BOM; review calculations and fault
   cases before layout. The discovery gate blocks this step while inputs remain
   unresolved, following pcb-agent's four-sensor benchmark plan.
4. Lay out the selected board and recheck the exact enclosure/connector/harness
   assembly. Run native ERC/DRC and all applicable manufacturing/export checks
   on registered Orion jobs without exclusions or reduced severities.
5. Obtain independent engineering review, then the required fabrication approval.
   Record actual bring-up, power/ADC/sensor faults, power-loss logs, calibration,
   temperature, EMC and vehicle results against approved numeric test limits.

Public, transportation and military compliance are **not established**. AEC
component qualification, sensor family ratings, CAD clearance and firmware
builds do not qualify the complete board for any of those applications.

## Verification

From the repository root:

```bash
python3 tools/verify_ac_carrier.py
python3 -m unittest discover -s tests -v
python3 tools/verify_repository.py
```

The first command verifies source bytes, coordinate transforms, the actual
retained slide axes and the discovery gate. A successful consistency check is
not electrical, native EDA, physical or release approval. Revisions resolving
requirements must replace this discovery contract through review, rather than
flipping its authority flags or labeling family-level data as selected parts.

[Orion firmware evidence](evidence/orion-firmware-build-2026-10-12.json) records
successful ESP-IDF 5.5.3 builds of both existing profiles, the image digest,
source and compiled-artifact hashes, and bounded resource use. All eighteen
firmware source files match the build archive. Compiled configuration checks
passed for 16 MB flash, rollback, radio absence, the correct temperature-driver
boundary and application size. Fifteen current regression tests passed.

The Orion archive predates the canonical BoardSpec snapshots and the final
metadata-reader fix; its eleven discovery tests passed. Current checks ran
separately against the retrieved, hash-verified compiled files. The reader fix
allows build metadata outside the checkout without failing while formatting a
diagnostic. These results do not verify sensor acquisition, physical flash/boot,
a functional PCB or vehicle installation. Raw receipts and logs remain at the
local evidence path recorded in the JSON; they are not firmware release assets.
