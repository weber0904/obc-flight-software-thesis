# Mission Console Display And Capture Replay Hardening v1 Evidence

Status: branch-local closeout evidence for
`fix-mission-console-display-replay-hardening-v1`.
Date: 2026-07-29.

## Scope

This record covers two bounded corrections to the existing Mission Console
hosted operator surface:

- operator-facing finite decimal measurements render with at most two
  fractional digits and rounded negative zero renders as `0`
- the existing Packet Lab `replay-captured-raw` case recognizes a valid
  secure-command-v2 packet in either the repository synthetic descriptor
  envelope or stock-GDS native capture bytes

The replay parser keeps complete synthetic descriptor envelopes authoritative.
When no synthetic packet is present, it ignores capture noise and candidates
with invalid magic, version, reserved field, header length, MAC length, inner
command kind, trailer, or computed packet boundary.

This change does not add an HTTP endpoint, F' command, command authority, or
secure-command wire format. It does not change API/raw evidence values; numeric
normalization is display-only.

## Reused Baseline And Claim Boundary

The hosted probe reuses verification-path registry entry `43B`, the maintained
per-band stock ground/operator baseline. The changed assertion is limited to
Mission Console's hosted Packet Lab selection and replay of bytes already
recorded by that baseline.

This record does **not** prove:

- target or Raspberry Pi behavior
- RF or OTA behavior
- a new secure-command validation rule on the flight side
- one-GDS multiplexing or a new ground transport
- generic decoding of all stock-GDS envelope forms
- target/lab requalification

No target probe was run because neither the implementation nor the claim
changes target, lab, RF, or flight-side behavior.

## Focused Unit Verification

Commands:

```text
node --check scripts/mission_console/static/mission-console.js
node --check scripts/test_mission_console_display_format.js
node scripts/test_mission_console_display_format.js
fprime-venv/bin/python scripts/test_mission_console_phase1.py
```

Result:

- JavaScript syntax: PASS
- direct display formatting contract: PASS
- Mission Console suite: `161 tests`, PASS

The direct display tests cover:

- rounding to at most two fractional digits
- removal of redundant trailing zeroes
- negative-zero normalization
- scalar and array formatting
- preservation of integer numbers and strings
- preservation of enum, identifier, hexadecimal, and scientific-notation text
  without a decimal point

The capture/replay tests cover:

- native envelope plus unrelated prefix/suffix bytes
- secure header and inner-command validation
- continuation past malformed candidates
- truncated candidate rejection
- synthetic descriptor precedence
- no-valid-packet failure without sending arbitrary bytes
- end-to-end selection of a native-captured packet by
  `_latest_captured_secure_packet`

## Fresh Local Gate

Command:

```text
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

Result: PASS.

The full product-impacting gate completed:

- F' generate and main build
- unit-test generate and build
- `fprime-util check --all`
- repository consistency
- documentation governance
- transport MTU/APID contract
- classic component-test baseline
- legacy direct-ZMQ retirement
- main OpenSpec validation

No lightweight/documentation-only exception was used.

## Isolated Hosted Probe

Preflight and probe commands:

```text
bash scripts/probe_port_hygiene.sh --reap-known 15087
PROBE_ROOT=/tmp/mission-console-native-review-fix.hVJzWK \
MISSION_CONSOLE_PORT=15087 \
bash scripts/run_mission_console_phase1_hosted_probe.sh
```

Result: PASS.

Relevant observations:

```text
hosted-packet-lab=explicit-reject
hosted-packet-lab-fault-explanation=PASS
hosted-packet-lab-captured-replay=explicit-reject
hosted-packet-lab-native-capture-replay=explicit-reject
mission-console-probe: PASS
mission-console-hosted-backpressure-check=PASS
mission-console-hosted: PASS
```

The probe used a fresh runtime root and a dedicated Mission Console port after
the fresh full gate. For the native branch assertion it created a native-only
capture inside that isolated surface, temporarily redirected the runtime
manifest to the probe artifact, placed the packet at a known native-envelope
offset, asserted that replay selected those exact packet bytes, and restored
the original capture path before continuing. The `/tmp` probe root is
diagnostic runtime output and is not repository evidence content.

## Verdict

PASS for the bounded hosted Mission Console display-precision and native
capture-replay correction. Target, RF, protocol, and flight-side claims remain
unchanged.
