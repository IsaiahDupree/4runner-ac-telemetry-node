# Draft A/C ADC input-envelope review

Engineering **INCOMPLETE**. Release **BLOCKED**. No topology selected.

The sensor's 5 V supply option does not define its output range. No generic pressure transfer function is inserted.

## adc\_3v3\_direct\_i2c

Unselected nominal 3\.3 V ADC and direct 3\.3 V pull\-up hypothesis\. PGA scaling does not enlarge the pin operating window\. Actual supply tolerances, ordered sensor output and conditioning remain unknown\.

Necessary domain outcome: **INCOMPLETE**. Direct I2C high-domain check: **PASS**.

Proposed powered input window: **0 to 3.3 V**. This is bounded by the lowest declared ADC supply, independently of PGA scaling.
Absolute stress window: **-0.3 to 3.6 V**. Stress limits are not operating permission.
Ideal positive clipping threshold: **4.0959 V**. Actual offset, uncertainty and noise are outside this ideal calculation.

Ordered HIGH/LOW outputs, sensor-off outputs and harness-fault bounds: **UNKNOWN**.
ADC-off/sensor-on case: pin stress window -0.3 to +0.3 V; protection current, back-power and sequencing remain **INCOMPLETE**.


## adc\_5v\_direct\_3v3\_i2c

Unselected nominal 5 V ADC with a direct 3\.3 V bus\. Test the declared high\-level compatibility instead of assuming that a larger analog window also solves the digital interface\.

Necessary domain outcome: **FAIL**. Direct I2C high-domain check: **FAIL**.

Proposed powered input window: **0 to 5 V**. This is bounded by the lowest declared ADC supply, independently of PGA scaling.
Absolute stress window: **-0.3 to 5.3 V**. Stress limits are not operating permission.
Ideal positive clipping threshold: **6.1438 V**. Actual offset, uncertainty and noise are outside this ideal calculation.

Ordered HIGH/LOW outputs, sensor-off outputs and harness-fault bounds: **UNKNOWN**.
ADC-off/sensor-on case: pin stress window -0.3 to +0.3 V; protection current, back-power and sequencing remain **INCOMPLETE**.

- **DIRECT_PULLUP_BELOW_ADC_VIH**: Declared worst-case pull-up does not meet the ADC's guaranteed high threshold.

## adc\_5v\_translation\_unresolved

Unselected nominal 5 V ADC hypothesis requiring a separately reviewed interface\. No level translator, direction, power\-state behavior or working bus is invented\.

Necessary domain outcome: **INCOMPLETE**. Direct I2C high-domain check: **INCOMPLETE**.

Proposed powered input window: **0 to 5 V**. This is bounded by the lowest declared ADC supply, independently of PGA scaling.
Absolute stress window: **-0.3 to 5.3 V**. Stress limits are not operating permission.
Ideal positive clipping threshold: **6.1438 V**. Actual offset, uncertainty and noise are outside this ideal calculation.

Ordered HIGH/LOW outputs, sensor-off outputs and harness-fault bounds: **UNKNOWN**.
ADC-off/sensor-on case: pin stress window -0.3 to +0.3 V; protection current, back-power and sequencing remain **INCOMPLETE**.


## voltage\_domain\_unresolved

Preserve an unspecified supply, PGA and interface while exact parts and controller connections are unresolved\.

Necessary domain outcome: **INCOMPLETE**. Direct I2C high-domain check: **INCOMPLETE**.


Ordered HIGH/LOW outputs, sensor-off outputs and harness-fault bounds: **UNKNOWN**.
ADC-off/sensor-on case: pin stress window -0.3 to +0.3 V; protection current, back-power and sequencing remain **INCOMPLETE**.


## Remaining evidence

- All ADC supplies, PGA settings and interfaces are unselected numeric proposals, not actual board measurements.
- The focused source establishes a 5 V sensor-supply option, not an ordered output or transfer function. HIGH and LOW outputs remain separately unknown.
- The focused Nano GPIO domain is retained as a baseline ceiling; this does not reconcile the later UNO-format candidate or qualify its pins.
- Recommended operating pin limits and ideal PGA clipping are distinct from absolute stress limits. Stress conformance cannot establish safe or functional operation.
- Powered-off pin limits do not model clamp current, back-power, sequencing, sensor-switch leakage, transient duration or harness faults.
- I2C checks cover the declared direct high-voltage window only; sink current, low-level margin, loading, timing and any translator remain unqualified.
- No divider, clamp, buffer, filter, reference source, exact sensor, calibration, PCB net or footprint is selected.

Canonical contract blockers: **14**.

[Voltage-window diagram](input-windows.svg) · [Machine report](review.json) · [Manifest](manifest.json)

[TI manufacturer source](https://www.ti.com/lit/ds/symlink/ads1115-q1.pdf)
