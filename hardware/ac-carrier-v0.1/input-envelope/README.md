# Draft ADC voltage-domain review

Status: **engineering INCOMPLETE; release BLOCKED**. Open the
[report and diagram](review/index.html) or [readable report](review/review.md).
The [request](request.json) binds the canonical BoardSpec, discovery inputs and
focused specification. All four domain proposals are unselected.

The proposed 3.3 V ADC's permitted pin window does not expand to 4.096 V when
the PGA uses that scale. The proposed 5 V ADC needs a compatible digital
interface: its direct nominal 3.3 V pull-up is below the required high threshold.
The 3.3 V approach and the 5 V approach with unspecified translation remain
incomplete; no exact circuit or working translator is selected.

These are conditional calculations from fixed nominal proposals and the
[TI source](https://www.ti.com/lit/ds/symlink/ads1115-q1.pdf), not measured rails
or verified board compatibility. Actual rail tolerances are still required.
The source's 5 V sensor-supply option does not establish an ordered output
range or pressure equation. HIGH and LOW normal outputs, sensor-off outputs,
conditioning and harness-fault envelopes remain separately unknown.

Powered-off pin stress limits cannot establish safe back-power, injection
current or fault duration. The software keeps operating limits, ideal code
clipping and stress arithmetic separate; each requires further electrical
qualification. It creates no pressure values, nets, footprints or release.

The [software guide](https://github.com/IsaiahDupree/pcb-agent/blob/main/docs/input-envelope-planner.md)
contains replay commands and the 73 new test cases. The
[execution receipt](../evidence/input-envelope-test-2026-10-12.json) records
actual validation. The fourteen significant BoardSpec contracts are unchanged.
