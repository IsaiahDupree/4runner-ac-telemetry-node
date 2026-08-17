# Repository agent rules

Read `docs/prd/AC-TELEMETRY-SOURCE.md`, the focused specification text export,
and `docs/PROVENANCE.md` before changing hardware authority, contracts, or
calculation behavior.

## Non-negotiable invariants

- Do not invent pressure-sensor part numbers, fittings, transfer functions,
  thresholds, or Toyota-specific diagnostic claims.
- An engineering pressure value is unavailable until its exact calibration
  identity and validation state are present. Preserve raw ADC and voltage evidence.
- Do not initialize Wi-Fi or BLE in `EMPTY_RECOVERY`.
- Do not add arbitrary vehicle-bus transmission or actuation.
- Do not erase NVS automatically. Preserve the protected range in installer plans.
- Do not present simulated, configured, or unavailable data as vehicle evidence.
- Keep units, quality, freshness, applicability, versions, and evidence lineage
  explicit at every boundary.
- A byte-verified release is not a physical validation pass. Record physical
  flash, boot, radio absence, serial evidence, and restore testing separately.
- Do not ship mock providers, fake production returns, or placeholder sensor values.

## Verification

Run before committing:

```bash
python3 tools/verify_repository.py
docker run --rm \
  -v "$PWD:/project" \
  -w /project/firmware/ac-sensor-node-esp32s3 \
  espressif/idf:v5.5.3 \
  bash -lc 'idf.py set-target esp32s3 && idf.py build'
```
