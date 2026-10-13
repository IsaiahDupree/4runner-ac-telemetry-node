# Product PCB build and test review

Project: **4runner\-ac\-telemetry\-node**. Revision: **ac\-carrier\-v0\.1\.discovery**.

Input consistency: **PASS**. Plan coverage: **PASS**. Requirements: **BLOCKED_REQUIREMENTS**.
Engineering: **INCOMPLETE**. Release: **BLOCKED**.

## Requirements to build

- **AC\-PRESSURE**: Measure distinct HIGH and LOW pressure channels\. Preserve raw ADC, voltage, calibration identity and absolute/gauge reference; publish no engineering pressure without exact validated calibration\. Existing implementation/verification trace: **INCOMPLETE**.
- **AC\-TEMPERATURE**: Provide selected high\-line, low\-line and ambient temperature inputs; cabin return and center vent are optional extensions\. Internal die temperature remains BOARD\_DIE\_HEALTH\_ONLY\. Existing implementation/verification trace: **INCOMPLETE**.
- **AC\-POWER**: Use fused switched vehicle power, reverse\-polarity/transient protection, a controller\-compatible regulated supply and an independent sensor 5 V supply\. Keep USB service available and record reset/brownout evidence\. Existing implementation/verification trace: **INCOMPLETE**.
- **AC\-ISOLATE**: Independently switch HIGH and LOW sensor power\. Account for signal back\-powering, off\-state leakage and independent fault recovery\. Existing implementation/verification trace: **INCOMPLETE**.
- **AC\-ADC**: Use an ADS1115\-Q1\-class protected/scaled ADC path unless measured evidence supports the controller ADC\. Include a known reference and all required rail measurements\. Existing implementation/verification trace: **INCOMPLETE**.
- **AC\-LOG**: Retain microSD logging, raw observations, versioned health and calibration evidence, with power\-loss/recovery tests\. The current recovery firmware does not implement these paths\. Existing implementation/verification trace: **INCOMPLETE**.
- **AC\-SERVICE**: Retain USB flashing/debug and versioned recovery\. BLE/SoftAP are later runtime features, absent from EMPTY\_RECOVERY and TEMPERATURE\_BENCH\. Existing implementation/verification trace: **INCOMPLETE**.
- **AC\-MECHANICAL**: Carry over the current matched enclosure revision\. Revalidate any new PCB, shield stack, connector, harness or power\-entry geometry against actual exported shell/cassette solids\. Existing implementation/verification trace: **INCOMPLETE**.
- **AC\-RELEASE**: Require native ERC/DRC, independent engineering review, exact manufacturing artifacts and measured bench results before fabrication/vehicle qualification\. Component ratings and CAD clearance do not certify the complete assembly\. Existing implementation/verification trace: **INCOMPLETE**.

## Field work

### contracts\.controller — unknown

Reconcile the focused spec&\#x27;s frozen Nano ESP32 with headers against the later owner\-requested Pi \+ Arduino cassette and photographed UNO\-format candidate linked to DIYables ESP32\-S3\-WROOM\-1\-N16R8\. Verify the actual board manufacturer/revision, schematic, header pin numbering and voltage/power limits; the source match is not owned\-board verification\.

- Owned\-board maker/revision, manufacturer schematic and measured header/outline identity\.
- Reviewed GPIO, boot/PSRAM reservations, normal/fault I/O and power paths reconciled to the frozen Nano baseline\.

### contracts\.board\_role — unknown

Choose a separate sensor PCB, a shield, or an integrated replacement\. The current UNO mounting plane already carries the controller; this reference does not create space for a second board\.

- Reviewed sidecar/shield/replacement decision with actual mounting location and populated envelope\.
- Controller/ADC/storage/service boundaries and inter\-board connector/net mapping\.

### contracts\.pressure\_high — unknown

The focused spec freezes the Sensata A/C analog family, 5 V option and 0\.6\-35 barA baseline, but explicitly leaves the orderable PN TBD\. Obtain the exact configuration, connector/cavity pin map, supply/output limits, transfer, faults, calibration and refrigerant/port configuration\. The older Honeywell candidate is not a substitution\.

- Exact Sensata HIGH orderable configuration, datasheet and mating connector/fitting\.
- Reviewed supply/output/fault/transfer/absolute\-reference limits and calibration procedure\.

### contracts\.pressure\_low — unknown

The focused spec freezes the Sensata A/C analog family and 0\.6\-10\.25 barA LOW baseline with exact PN TBD\. Resolve its configuration independently; do not copy HIGH calibration or substitute an older sealed\-gauge candidate\.

- Exact Sensata LOW orderable configuration, datasheet and mating connector/fitting\.
- Independent LOW range, transfer, faults and calibration; no copied HIGH calibration\.

### contracts\.temperature\_interfaces — unknown

The focused spec leaves automotive contact probes at SELECT\. Older bench references are Adafruit PT1000 \#3984 with MAX31865 \#3648; earlier CAD also lists SHT40 \#4885, which is not a line\-contact probe\. Select final contact/ambient parts, interfaces, excitation, range, attachment, connectors, faults and calibration\.

- Selected contact and ambient probes, interfaces, harness and attachment method\.
- Range/error/excitation/fault/response/calibration limits approved for the measurement roles\.

### contracts\.power\_envelope — unknown

Resolve fused source, continuous/current/startup loads, minimum crank voltage/duration, required hold\-up or controlled restart, reverse\-polarity and surge waveform limits\.

- Vehicle source/connector and measured or authoritative normal/crank/transient/fault envelope\.
- Power\-tree/current/hold\-up\-or\-restart/protection calculations with reviewed acceptance limits\.

### contracts\.sensor\_rail — unknown

Independent sensor 5 V is required; TPS7B83\-Q1 is explicitly a candidate\. Freeze the regulator, upstream node, tolerance, startup/short limits, independent channel switches, off\-state behavior and thermal budget at actual ambient temperature\.

- Exact regulator and independent switches with upstream node and normal/fault limits\.
- Startup/leakage/back\-power/thermal calculations and review at the actual ambient/load envelope\.

### contracts\.pi\_boundary — unknown

The owner requested separate Pi and Arduino mounting planes; CAD identifies Waveshare UPS HAT \(D\)\. Resolve the actual Pi computer model, bench/vehicle role, separate power budget and communication link\. HAT identity does not establish a Pi model, and the modeled lithium UPS is prohibited underhood\.

- Actual Pi/HAT identity, bench\-only or installation boundary, power budget and communication contract\.
- Preserve two mounting planes; exclude the modeled lithium UPS from underhood use\.

### contracts\.measurement\_routing — unknown

Resolve the seven analog observations versus four ADC inputs, GPIO assignments, reference source, filtering, divider tolerances, settling and power\-off input protection\.

- Actual GPIO/ADC/reference/divider/protection allocation for seven analog observations\.
- Reviewed and measured conversion/settling/filter schedule at requested channel rates and all power states\.

### contracts\.storage\_interface — unknown

The focused V0 baseline freezes Adafruit microSD \#4682 plus a 16\-64 GB high\-endurance card and calls for a socket/card\-detect implementation on the custom PCB\. Freeze the PCB socket, voltage domain, GPIO/SPI assignments, card detect, insertion clearance and startup/write current\.

- Selected socket/breakout/card, SPI pin map, voltage domain, card detect and insertion envelope\.
- Measured startup/write load and reviewed power\-loss recovery protocol\.

### contracts\.connectors\_harness — unknown

Freeze actual sensor/internal disconnect/power connectors, pin maps, locations, cable OD and bend radius, latch/pull travel and strain relief\.

- Exact connector halves, seals, cavity/pin map, wire gauge and source/calibration links\.
- Measured cable OD, bend/mating travel, strain relief and actual enclosure termination locations\.

### contracts\.final\_pcb\_mechanics — unknown

Resolve the final board role and outline, attachment, board stacking, populated heights, antenna/USB access and physical mounting coupon evidence\.

- Final outline/holes/tolerances, populated heights and antenna/USB/SD/connector keepouts\.
- Populated STEP cross\-fit and actual mounting/connector coupon evidence for the retained cassette revision\.

### contracts\.fabrication\_rules — unknown

Freeze board stackup, copper, supplier rules, finish, assembly/package capabilities and component lifecycle/availability\.

- Selected supplier/stackup/copper/finish and explicit fabrication/assembly/package rules\.
- Complete approved component/footprint/model provenance and reviewed export/DFM profile\.

### contracts\.environment\_and\_test — unknown

Resolve actual component/enclosure temperature limits, electrical/transient/environment/EMC acceptance criteria, fixtures, measurement uncertainty and reviewer\.

- Application\-specific temperature/transient/vibration/ingress/EMC limits and test programme\.
- Named engineering reviewer, fixture/instrument/calibration/uncertainty and numeric measurement acceptance limits\.

## Build sequence

### S0: Build and test the product\-input review workflow

Execution: **NOT_RUN**. Input constraints: **REVIEW_REQUIRED**.

Dependencies: none.
Blocking fields: none observed.

- Typed source\-bound review CLI and plan schema\.
- Real A/C regression snapshot, unsafe\-input mutations and deterministic reports\.
- Existing supported creation\-path replay with explicit evidence scope\.

### S1: Resolve exact product requirements

Execution: **NOT_RUN**. Input constraints: **BLOCKED_REQUIREMENTS**.

Dependencies: S0.
Blocking fields: contracts\.board\_role, contracts\.connectors\_harness, contracts\.controller, contracts\.environment\_and\_test, contracts\.fabrication\_rules, contracts\.final\_pcb\_mechanics, contracts\.measurement\_routing, contracts\.pi\_boundary, contracts\.power\_envelope, contracts\.pressure\_high, contracts\.pressure\_low, contracts\.sensor\_rail, contracts\.storage\_interface, contracts\.temperature\_interfaces.

- Exact controller/sensor/probe/connector identities and manufacturer evidence\.
- Reviewed revision resolving all fourteen composite contract tasks\.

### S2: Design and review circuit architecture and assets

Execution: **NOT_RUN**. Input constraints: **BLOCKED_REQUIREMENTS**.

Dependencies: S1.
Blocking fields: contracts\.board\_role, contracts\.connectors\_harness, contracts\.controller, contracts\.environment\_and\_test, contracts\.fabrication\_rules, contracts\.final\_pcb\_mechanics, contracts\.measurement\_routing, contracts\.pi\_boundary, contracts\.power\_envelope, contracts\.pressure\_high, contracts\.pressure\_low, contracts\.sensor\_rail, contracts\.storage\_interface, contracts\.temperature\_interfaces.

- Power tree, GPIO/net/ADC and sampling budget with normal/fault calculations\.
- Reviewed exact symbol/footprint/block/model assets and sensor isolation/storage/service interfaces\.

### S3: Compile and review the wired schematic and BOM

Execution: **NOT_RUN**. Input constraints: **BLOCKED_REQUIREMENTS**.

Dependencies: S2.
Blocking fields: contracts\.board\_role, contracts\.connectors\_harness, contracts\.controller, contracts\.environment\_and\_test, contracts\.fabrication\_rules, contracts\.final\_pcb\_mechanics, contracts\.measurement\_routing, contracts\.pi\_boundary, contracts\.power\_envelope, contracts\.pressure\_high, contracts\.pressure\_low, contracts\.sensor\_rail, contracts\.storage\_interface, contracts\.temperature\_interfaces.

- Editable hierarchical functional KiCad schematic, complete exact BOM and requirement/net traceability\.
- Unrouted PCB carrying only reviewed component and mechanical assets\.

### S4: Place and route the populated PCB

Execution: **NOT_RUN**. Input constraints: **BLOCKED_REQUIREMENTS**.

Dependencies: S3.
Blocking fields: contracts\.board\_role, contracts\.connectors\_harness, contracts\.controller, contracts\.environment\_and\_test, contracts\.fabrication\_rules, contracts\.final\_pcb\_mechanics, contracts\.measurement\_routing, contracts\.pi\_boundary, contracts\.power\_envelope, contracts\.pressure\_high, contracts\.pressure\_low, contracts\.sensor\_rail, contracts\.storage\_interface, contracts\.temperature\_interfaces.

- Final populated PCB and STEP assembly with mounting, connectors, test points and reviewed routing rules\.
- Actual enclosure/harness fit and placement/thermal/return\-path review\.

### S5: Execute complete native verification and export comparison

Execution: **NOT_RUN**. Input constraints: **BLOCKED_REQUIREMENTS**.

Dependencies: S4.
Blocking fields: contracts\.board\_role, contracts\.connectors\_harness, contracts\.controller, contracts\.environment\_and\_test, contracts\.fabrication\_rules, contracts\.final\_pcb\_mechanics, contracts\.measurement\_routing, contracts\.pi\_boundary, contracts\.power\_envelope, contracts\.pressure\_high, contracts\.pressure\_low, contracts\.sensor\_rail, contracts\.storage\_interface, contracts\.temperature\_interfaces.

- Pinned Orion native reopen/ERC/DRC/parity and applicable simulation reports\.
- Independent preregistered fault detection and complete native source/export fidelity evidence\.

### S6: Review exact fabrication and bring\-up package

Execution: **NOT_RUN**. Input constraints: **BLOCKED_REQUIREMENTS**.

Dependencies: S5.
Blocking fields: contracts\.board\_role, contracts\.connectors\_harness, contracts\.controller, contracts\.environment\_and\_test, contracts\.fabrication\_rules, contracts\.final\_pcb\_mechanics, contracts\.measurement\_routing, contracts\.pi\_boundary, contracts\.power\_envelope, contracts\.pressure\_high, contracts\.pressure\_low, contracts\.sensor\_rail, contracts\.storage\_interface, contracts\.temperature\_interfaces.

- BOM/CPL/Gerber/drill/PDF/STEP, assembly drawings and current\-limited bring\-up protocol\.
- Separate authorized engineering and fabrication approvals over exact artifact/requirement/test hashes\.

### S7: Bring up actual hardware and implement acquisition

Execution: **NOT_RUN**. Input constraints: **BLOCKED_REQUIREMENTS**.

Dependencies: S6.
Blocking fields: contracts\.board\_role, contracts\.connectors\_harness, contracts\.controller, contracts\.environment\_and\_test, contracts\.fabrication\_rules, contracts\.final\_pcb\_mechanics, contracts\.measurement\_routing, contracts\.pi\_boundary, contracts\.power\_envelope, contracts\.pressure\_high, contracts\.pressure\_low, contracts\.sensor\_rail, contracts\.storage\_interface, contracts\.temperature\_interfaces.

- Unit\-bound calibrated power/pressure/temperature/storage/service measurements and serial/raw log evidence\.
- Real selected\-sensor firmware paths and failure\-to\-revision records\.

### S8: Validate installation and application environment

Execution: **NOT_RUN**. Input constraints: **BLOCKED_REQUIREMENTS**.

Dependencies: S7.
Blocking fields: contracts\.connectors\_harness, contracts\.environment\_and\_test, contracts\.final\_pcb\_mechanics, contracts\.power\_envelope, contracts\.sensor\_rail.

- Qualified installation/service and application\-specific measured environmental/EMC results\.
- Declared compliance scope with exact supporting evidence and documented remaining limitations\.

## Planned tests — execution NOT_RUN

### SW\-INPUT (software)

Requirements: AC\-PRESSURE, AC\-TEMPERATURE, AC\-POWER, AC\-ISOLATE, AC\-ADC, AC\-LOG, AC\-SERVICE, AC\-MECHANICAL, AC\-RELEASE.

Run the real source\-bound BoardSpec/build\-plan review twice\. Mutate source bytes/hashes, identities, field and requirement coverage, dependencies, schema, paths and claimed test/approval states in isolated copies\.

Acceptance: Deterministic exact\-byte reports; fourteen current composite blockers visible; unsafe inputs refused without partial output or source edits; all release authorities remain false\.

Numeric limits: **NOT_APPLICABLE**.

### SW\-CREATE (software)

Requirements: AC\-RELEASE.

Generate the supported LED candidate twice using its retained typed recipe; compare actual native files and replay the retained Orion native export/check evidence\. Test unsupported input rejection\.

Acceptance: Source bytes agree across rebuilds and match the retained example\. Native evidence is identified as historical replay\. Success does not expand generator scope to the automotive A/C board\.

Numeric limits: **NOT_APPLICABLE**.

### T\-ARCH (independent_review)

Requirements: AC\-PRESSURE, AC\-TEMPERATURE, AC\-POWER, AC\-ISOLATE, AC\-ADC, AC\-LOG, AC\-SERVICE, AC\-MECHANICAL, AC\-RELEASE.

Review the selected architecture, exact block interfaces, power/fault calculations, GPIO/net/ADC schedule and required manufacturer evidence\.

Acceptance: Exact product contracts and normal/fault limits are resolved and signed by the appropriate independent engineering authority; source contradictions are closed\.

Numeric limits: **REQUIRES_RESOLUTION**.

### T\-NATIVE (native_eda)

Requirements: AC\-PRESSURE, AC\-TEMPERATURE, AC\-POWER, AC\-ISOLATE, AC\-ADC, AC\-LOG, AC\-SERVICE, AC\-MECHANICAL, AC\-RELEASE.

Use a pinned, registered Orion profile to reopen the functional schematic/PCB, run full ERC/DRC/parity and applicable simulations, then inject omissions, wrong pins/nets, shorts, polarity and rule violations into isolated copies\.

Acceptance: No unwaived errors, hidden exclusions or ignored severities; each preregistered defect is detected; missing models/evidence block the applicable requirement\. Actual raw native reports and source identities retained\.

Numeric limits: **REQUIRES_RESOLUTION**.

### T\-POWER (bench)

Requirements: AC\-POWER, AC\-ISOLATE, AC\-ADC, AC\-RELEASE.

On an approved assembled unit and current\-limited fixture, measure startup, rails, idle/max load, reverse/fault response and required crank restart/hold\-up\. Switch each sensor independently and measure off\-state signal back\-power and thermal behaviour\.

Acceptance: Measured voltages/current/waveforms/temperature and fault response meet exact reviewed limits with instrument uncertainty, assembly identity and calibration recorded\.

Numeric limits: **REQUIRES_RESOLUTION**.

### T\-PRESSURE (bench)

Requirements: AC\-PRESSURE, AC\-ADC, AC\-ISOLATE, AC\-RELEASE.

Stimulate both exact selected pressure paths at calibrated endpoints and intermediate values; check reference/rail error, open/short/unpowered inputs, ground offset, filtering/settling and actual channel rates\.

Acceptance: Retain raw ADC/voltage and exact calibration/absolute\-reference identity; engineering values and fault quality meet reviewed limits\. No generic transfer is substituted\.

Numeric limits: **REQUIRES_RESOLUTION**.

### T\-TEMP (bench)

Requirements: AC\-TEMPERATURE, AC\-RELEASE.

Stimulate the selected high\-line, low\-line and ambient probes across approved temperature points\. Measure attachment error/response and open/short/stuck behaviour; check firmware role labels\.

Acceptance: Actual external readings meet the selected probe/interface/calibration limits\. Missing probes remain unavailable, and die\-health temperature is never reported as line temperature\.

Numeric limits: **REQUIRES_RESOLUTION**.

### T\-LOG (bench)

Requirements: AC\-LOG, AC\-PRESSURE, AC\-TEMPERATURE, AC\-ADC, AC\-RELEASE.

Run acquisition/logging at selected rates with the exact card/socket and inject power loss, card removal/write errors and storage limits\. Inspect recovered logs and version/quality lineage\.

Acceptance: Bounded logs preserve raw observations, calibration/version/quality and agreed recovery behaviour; startup/write\-current and loss budgets satisfy the reviewed limits\.

Numeric limits: **REQUIRES_RESOLUTION**.

### T\-SERVICE (bench)

Requirements: AC\-SERVICE, AC\-RELEASE.

On the identified board, exercise backup\-first USB service, NVS preservation, probationary boot/rollback and restore\. Observe radio absence in EMPTY\_RECOVERY and TEMPERATURE\_BENCH; test any later approved runtime link separately\.

Acceptance: Unit/flash identity, backup hashes, serial/boot/rollback/restore evidence and radio\-profile observations satisfy the approved service contract\.

Numeric limits: **REQUIRES_RESOLUTION**.

### T\-MECH (mechanical)

Requirements: AC\-MECHANICAL, AC\-POWER, AC\-SERVICE, AC\-RELEASE.

Cross\-fit the exact populated board and connector/harness STEP assembly against the retained shell/cassette; verify the actual boards, screws, holes, cable bends, connector mating travel, USB and card service access\.

Acceptance: Digital and measured physical clearances/tolerances match the reviewed assembly\. A rectangular mounting reference or earlier unused space reservation cannot satisfy this test\.

Numeric limits: **REQUIRES_RESOLUTION**.

### T\-MFG (native_eda)

Requirements: AC\-RELEASE, AC\-MECHANICAL.

Export the actual native BOM/CPL/Gerber/drill/PDF/STEP set on Orion\. Compare references/DNP, exact part/footprint identity, origin/units/side/rotation and independent CAM geometry; mutate omissions, substitutions and coordinates\.

Acceptance: The exact reviewed source and complete manufacturing package agree under the selected supplier profile, with independent rotation/pick\-point and technical review\. Missing checks remain incomplete\.

Numeric limits: **REQUIRES_RESOLUTION**.

### T\-REVIEW (independent_review)

Requirements: AC\-PRESSURE, AC\-TEMPERATURE, AC\-POWER, AC\-ISOLATE, AC\-ADC, AC\-LOG, AC\-SERVICE, AC\-MECHANICAL, AC\-RELEASE.

Review the exact requirement, architecture, schematic, layout, calculation, simulation, export and test hashes through the existing separate engineering/fabrication authority workflow\.

Acceptance: Approvals come from authorized independent participants and bind exact dependencies\. Changed sources invalidate approvals; a plan, family rating or software test is not release authority\.

Numeric limits: **NOT_APPLICABLE**.

### T\-ENV (environment)

Requirements: AC\-POWER, AC\-MECHANICAL, AC\-RELEASE.

After installation/service approval, run the application\-specific crank/transient, thermal, vibration, ingress/condensation and EMC programme with the actual assembly and fixtures\.

Acceptance: All applicable numeric limits, uncertainty, assembly/calibration identities and observed failures/results are recorded\. No public/transportation/military compliance is inferred from component ratings\.

Numeric limits: **REQUIRES_RESOLUTION**.

## Limits and evidence

- Source hashes establish exact submitted bytes, not source truth, independent approval or component qualification.
- Plan coverage records intended work and tests. It does not establish their implementation, execution or successful outcome.
- The report does not run native EDA, simulations, manufacturing checks, firmware builds, or physical measurements.
- Field provenance and existing requirement trace outcomes are inspected; no confirmation, approval or lifecycle record is issued.
- Even a fully populated plan and BoardSpec require the supported compiler, reviewed assets, native verification and separate approvals.

[Machine report](review.json) · [Artifact manifest](manifest.json)

Report SHA-256: `0a2e4faef624f84db973f2819325361b9e4fc670f5e88035634d2f729fa28aa9`.
