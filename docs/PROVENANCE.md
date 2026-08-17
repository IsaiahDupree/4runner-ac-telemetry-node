# Source and release provenance

## Parent project

This repository is a focused extraction from
[`IsaiahDupree/4runner-vehicle-health-os`](https://github.com/IsaiahDupree/4runner-vehicle-health-os).
The extraction does not alter the parent repository or its iOS application.

- Recovery firmware source commit: `08ba6cc53b8a369cf69058d28ff0ebddfd0841f7`
- Provisioner/release publication commit: `295dcbc070d46393d2b0a550f38e79a3cf43598c`
- Extracted release: `v0.1.0-dev.1`
- Merged-image SHA-256: `5d451fa38dd5b03c45a0a15f5208539ab43720bdc282662f8b673706e07b7c17`

The release manifest intentionally retains the original parent commit as its
firmware/source identity. A future standalone release should use the commit in
this repository that produced it.

## Focused A/C specification

The authoritative focused source is the Drive file:

- `2005 Toyota 4Runner A/C Engine-Bay Telemetry Node Spec v1.0`
- File ID: `1rMmG6yA9YKml6fOkHtBKk_CI0mZiWGaA`
- MIME type: `application/vnd.openxmlformats-officedocument.wordprocessingml.document`
- Provider-reported size: `55,630` bytes
- Provider-reported modified time: `2026-08-16T23:44:04.647Z`

`docs/prd/2005_Toyota_4Runner_AC_Engine_Bay_Telemetry_Node_Spec_v1.0.txt`
is the complete readable text returned by the connected Drive source. The
binary focused DOCX remains in Drive because the authenticated streamed file
reference could not be materialized locally in this session. The earlier
`4Runner_Telemetry_Build_Master_Spec.docx` is included locally as historical
source, not as authority over the focused v1.0 document.

## Earlier ESP32 decisions retained

The recovery design adapts proven behavior from the prior MrDIY ESP32 VHOS
gateway without copying its Wi-Fi-enabled runtime profile:

- SoftAP and network initialization are off by default.
- USB installation is backup-first and preserves NVS.
- A/B application slots use probationary boot and rollback.
- Firmware identity, self-test, and health are versioned evidence.
- Persisted random-static BLE identity and encrypted pairing are documented for
  a later phase but are absent from `EMPTY_RECOVERY`.

Relevant earlier-firmware decisions were recorded in commits `5077da0`,
`4e5da75`, and `40151a3` of the local `4runner-vhos-firmware` history.
