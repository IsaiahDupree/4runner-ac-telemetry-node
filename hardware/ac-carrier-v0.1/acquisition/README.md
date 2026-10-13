# Draft analog acquisition comparison

Status: **engineering INCOMPLETE; release BLOCKED**. This is a software-generated
input/conversion allocation proposal. It does not select a topology, resolve a
canonical BoardSpec field or produce a wired schematic or PCB.

Open the [report and allocation diagram](review/index.html),
[SVG diagram](review/allocation.svg) or [readable report](review/review.md).
[request.json](request.json) binds the exact canonical BoardSpec, discovery
inputs and focused specification. The fourteen significant contracts are unchanged.

The actual source requires HIGH/LOW pressure at a maximum 50 Hz, a reference and
four rail measurements at 1 Hz: seven signals and at least 105 conversions/s.
The planner checks those rates against the focused source task table.

| Unselected hypothesis | Necessary channel/conversion result |
| --- | --- |
| One ADS1115 for all signals with no other routing path | FAIL: three rails lack inputs |
| Two ADS1115s, each programmed to 128 SPS, with different address-pin connections | Necessary bounds PASS: eight input connections, 53 conversions/s per engine |
| One ADS1115 plus an unspecified controller-ADC or MUX path | INCOMPLETE: the additional path has no verified usable capacity |

The two-ADC hypothesis reserves the same external reference signal on an input
of each converter, under an **explicit draft policy**. This raises the conversion
count to 106/s and uses all eight inputs. Its reference source/fanout/loading,
power and input protection are still open. The allocation proposes HIGH on one
engine and LOW on the other; it makes no simultaneous-sampling claim.

The pinned [TI source](https://www.ti.com/lit/ds/symlink/ads1115-q1.pdf) supports
the capacity facts. One converter's channels share its conversion engine. The
slow-rate bound at 128 SPS is 115.2 SPS; rates and assignments are compared with
exact rational arithmetic. These necessary count bounds exclude bus, firmware,
startup, analog settling/filtering, fault-scan and deadline/jitter overhead.

The original focused spec's single ADS1115 recommendation can still be paired
with a reviewed additional route. This review rejects only the restricted
all-signals-on-one-converter hypothesis. The controller ADC needs verified board,
GPIO, protection and accuracy evidence; a MUX needs reviewed switching/settling
and fault behavior. Neither alternative is silently filled in.

The [PCB software guide](https://github.com/IsaiahDupree/pcb-agent/blob/main/docs/acquisition-planner.md)
contains repeatable commands and test scope. The
[execution receipt](../evidence/acquisition-test-2026-10-12.json) records actual
software validation. Exact components, conditioning, pin maps and measured
acquisition timing are still required before functional schematic/layout work.
