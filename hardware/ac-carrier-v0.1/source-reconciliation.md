# A/C PCB requirements recovered from the related projects

Research date: 2026-10-12. Scope: source reconciliation for `ac-carrier-v0.1`.

Most of the starting requirements already exist. The focused A/C specification
explicitly freezes a Nano ESP32 baseline. The later enclosure source identifies
a particular UNO-format ESP32-S3 candidate, and the owner's September request
establishes separate Pi and Arduino mounting planes. These are useful recovered
decisions, but the documents do not contain a completed electrical migration
between those controller boards. Exact Sensata configurations and final contact
temperature probes are explicitly left open in the focused specification.

## Recovered requirements and their scope

| Item | What the source actually establishes | What remains to complete the PCB input |
| --- | --- | --- |
| Original controller | **Arduino Nano ESP32 with headers**, ESP32-S3 / NORA-W106, marked **FREEZE V0/V1** in the focused spec, lines 171–174. | Reconcile its electrical design with the later UNO-format board and current cassette; do not apply Nano VIN or pin numbering to the other board. |
| Later controller candidate | CAD source names the owner-photographed **ESP32-S3 UNO-format** board and links the DIYables **ESP32-S3-WROOM-1-N16R8** listing. | Verify the owned board's maker/revision, schematic, header pin map, power path and measured outline. A likely listing is not a verified board identity. |
| Two mounting planes | The owner's 2026-09-20 message requests separate Pi and Arduino planes on a shared slide for bench access. The current CAD implements that direction. | The Pi computer model, its electrical role/link and the new sensor PCB's mounting/stacking arrangement are not specified by this request. |
| Pi-side CAD component | Retained CAD explicitly identifies **Waveshare UPS HAT (D)**. | A HAT identity does not identify its host Pi. The modeled lithium UPS remains a bench reference, excluded from underhood installation. |
| HIGH pressure | **Sensata Automotive A/C** analog family, **5 V option**, **0.6–35 barA** baseline; **FAMILY FREEZE; orderable PN TBD**. | Exact orderable configuration, output/transfer, connector pinout, faults, calibration and refrigerant fitting. |
| LOW pressure | Same family, separate **0.6–10.25 barA** baseline; **FAMILY FREEZE; orderable PN TBD**. | Its own exact configuration and calibration; neither HIGH calibration nor an older gauge-pressure sensor is interchangeable. |
| External temperature | Focused spec calls for **2–3 automotive-rated contact probes**, status **SELECT**. Current firmware contract distinguishes high line, low line and ambient roles. | Probe technology/part, interface, attachment and calibration. Exact older bench RTDs are available as references below. |
| Pressure acquisition | **ADS1115-Q1**, **RECOMMENDED V1/V2**, unless measured evidence justifies the controller ADC. Independent HIGH/LOW sensor power switching is required. | Input conditioning, fault behavior, GPIO and the additional path for seven analog observations versus four ADC inputs. |
| Vehicle power | **LM5164-Q1**, **FREEZE REFERENCE**, approximately **6.5–7 V into Nano VIN** after protection; independent sensor 5 V. **TPS7B83-Q1** is a **CANDIDATE** for that sensor rail. | Exact controller-compatible supply, crank/surge limits, switches, thermal/load budget and regulator selection. |
| Logging | **Adafruit microSD #4682 + 16–64 GB high-endurance card**, **FREEZE V0**. The spec calls for a socket/card-detect implementation on the custom PCB. | Selected socket, pin allocation, voltage domain, current and mechanical clearance. |
| Harness | Focused spec gives DTM/DT connector families, a 1 A branch fuse and wire-gauge guidance. | Exact mating parts, cavity assignments, cable terminations and fit. Family examples are not a complete harness BOM. |

The values above are recovered **project requirements and candidate identities**.
They are not fresh hardware measurements, manufacturer qualification of the
assembly, or evidence of a complete pin-level design.

## Exact parts found in older work

The parent project's [bench BOM](https://github.com/IsaiahDupree/4runner-vehicle-health-os/blob/c852f17e84ef6d274b74871488ad97e0c4a4942b/docs/hardware/RECOMMENDED-HARDWARE.md#L47)
does name exact parts. Its own scope is V0 development, with procurement and
installation checks still required:

| Earlier item | Recorded identity | Relationship to the current design |
| --- | --- | --- |
| Pressure sensors | Honeywell **PX2AF1XX200PSAAX** LOW / **PX2AF1XX600PSAAX** HIGH | Earlier sealed-gauge candidates; the focused baseline later uses Sensata absolute-pressure variants. Do not silently substitute them or reuse a transfer function. |
| Sensor pigtails | Honeywell **3685301** | Associated with the older Honeywell choice, not a Sensata pinout. |
| Service couplers | YELLOW JACKET **41337** HIGH / **41338** LOW | Earlier bench plumbing references; not frozen permanent fittings for the current assembly. |
| Contact temperature | **3 × Adafruit PT1000 #3984**, with **3 × MAX31865 #3648** | Exact bench probe/interface references, not selected final engine-bay probes. |
| Controller and ADC | ESP32-S3-DevKitC-1-**N8R8**, Adafruit ADS1115 **#1085** | Earlier development assembly. Its 8 MB controller entry does not establish compatibility with the current 16 MB recovery release. |
| Storage | SanDisk MAX ENDURANCE 32 GB **SDSQQVR-032G-GN6IA**, Adafruit **#4682** | Exact earlier card candidate and the breakout retained by the focused V0 baseline. |

The earlier enclosure script separately reserves **DFRobot DFR0975** and
**Adafruit SHT40 #4885** alongside microSD #4682 and UPS HAT (D). The later
slot-mount script introduces the photographed UNO-format candidate. The SHT40
temperature/humidity breakout is not a refrigerant-line contact probe. Its
presence in CAD does not close the line-temperature requirement, and an old
interface-board space reservation does not prove fit in the current cassette.

## Which controller evidence can be reused

The parent firmware README records an August 16 observation of an **ESP32-S3
revision 0.2 with 16 MB flash and 8 MB PSRAM**. This explains why the recovery
release targets 16 MB instead of the older N8R8 bench BOM. It does not establish
the board SKU or prove that the August board and September photographed board
are the same device. This audit did not interrogate or flash hardware.

The later CAD source retains four component-photo hashes, interpretations and
the likely [DIYables board listing](https://diyables.io/products/esp32-s3-development-board-with-esp32-s3-wroom-1-n16r8-wifi-bluetooth-uno-compatible-form-factor-works-with-arduino-ide).
That listing identifies an ESP32-S3-WROOM-1-N16R8 UNO-format product. The original
photo bytes were not located or inspected in this audit, so the source's visual
match remains a candidate. Seller pin labels, an UNO hole pattern and matching
flash capacity do not replace the exact owned-board electrical documentation.
The original Nano's manufacturer resources remain available from
[Arduino](https://docs.arduino.cc/hardware/nano-esp32).

The current enclosure direction is therefore already documented. The remaining
controller work is an **electrical reconciliation**, not a request to repeat the
owner's mechanical preference. Preserve the two-plane cassette while resolving
how the sensor carrier connects to the actual controller and whether the Pi has
a role beyond the documented bench arrangement.

## Source audit and retained evidence

The [machine-readable evidence](evidence/source-research-2026-10-12.json) retains
six source-file SHA-256 hashes, Git snapshots, exact line excerpts with hashes,
Drive metadata/readback hashes and a brief owner-instruction excerpt. The full
owner message is retained privately outside this public repository.

- **Focused A/C spec v1.0:** re-read from
  [Drive](https://docs.google.com/document/d/1rMmG6yA9YKml6fOkHtBKk_CI0mZiWGaA/edit).
  Provider modification time remains `2026-08-16T23:44:04.647Z`. Connector text
  with one terminal LF is byte-identical to the retained 30,482-byte text:
  `4a5e8f440b7d23f1fe8ae38cd5af5b5898709947d4e5cfe86c979b975b3fe2ae`.
  This is a text comparison, not a new DOCX binary verification.
- **Integrated vehicle-health master spec v0.2:** re-read from
  [Drive](https://docs.google.com/document/d/1RC52bfDNV_n64M8ksWte7iB3DkNI9OdB/edit).
  Provider modification time remains `2026-08-16T18:31:20.824Z`. Its carrier
  section specifies Nano female headers and also leaves exact pressure PNs
  open. The focused document controls the corrected Nano power path.
- **Parent hardware and firmware records:** read at
  `4runner-vehicle-health-os@c852f17e84ef6d274b74871488ad97e0c4a4942b`.
- **CAD implementation and remaining-fit notes:** read at
  `orion-control-plane@1b3e779` (full commit and immutable links in the evidence).
  Each captured file matched that commit. Unrelated working-tree changes were
  left untouched. These records include the likely controller listing and
  photo hashes, older component envelopes and the explicit electrical conflict.
- **Owner instruction:** `2026-09-20T19:14:03.852Z`, requests
  “one for the Pi and then one for the
  Arduino” on the common slide. It does not select a Pi model or a pin map.

Drive searches for 4Runner, Sensata and telemetry plus relevant repository/CAD
and retained conversation records produced the evidence above. This establishes
what those inspected sources contain; it is not a claim that every account or
unretained attachment was exhaustively searched.

## Effect on the PCB package

[design-inputs.json](design-inputs.json) now points to this research and asks for
the specific remaining details instead of treating controller and component
history as absent. The existing canonical BoardSpec remains an unchanged
discovery snapshot: all fourteen **composite** contracts are incomplete because
each requires more than a family name or historical candidate. The count is not
fourteen wholly unknown design topics.

The next electrical revision can carry forward the two-plane enclosure intent,
Sensata ranges, measurement/logging requirements and documented component
references. It still needs verified controller connections, exact pressure and
contact-probe selections, complete power/fault limits and a resolved sensor-PCB
location before a wired schematic can be treated as an approved design. This
research does not produce Gerbers or authorize purchasing, fabrication or
vehicle installation.
