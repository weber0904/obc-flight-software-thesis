# uhf-primary-packet-quiet-decoupling-v1 Evidence

Date:
- `2026-05-28`

OpenSpec change:
- `uhf-primary-packet-quiet-decoupling-v1`

## Scope

This record closes out the semantics split between two existing formal UHF
mechanisms on the active `TopCcsds` baseline:

- UHF primary packet quiet for live formal `event/tlm` packet egress
- UHF beacon suppress/runtime for the BeaconV1 side channel

What was newly proven by this change:

- formal UHF live packet suppression is now primary-band-driven
- entering `uhf-primary-after-failover` suppresses live formal UHF
  `event/tlm` packet egress before any accepted qualifying UHF
  `SESSION_OPEN(seq0)`
- accepted qualifying UHF `SESSION_OPEN(seq0)` still belongs to the separate
  beacon suppress/runtime policy rather than starting packet quiet
- official file/data-product downlink remains a formal capability while UHF
  primary packet quiet is active
- stronger diagnostic quiet remains a separate probe-only hard quiet

What this record does not newly prove:

- simultaneous dual-link runtime arbitration
- UHF reliable transfer, ARQ, NACK, or CFDP
- RF or over-the-air closure
- broader target non-quiet closure
- broader UHF handshake state beyond accepted `SESSION_OPEN(seq0)`

## Hosted Packet-Quiet Verdict

Verdict: `PASS` for the dedicated hosted 41B UHF primary packet-quiet proof.

Repository-owned proof entry point:

```bash
bash scripts/run_uhf_primary_packet_quiet_hosted_probe.sh
```

Final passing run:

| Field | Value |
|---|---|
| date | `2026-05-28` |
| command | `bash scripts/run_uhf_primary_packet_quiet_hosted_probe.sh` |
| formal verdict | `uhf-primary-packet-quiet-decoupling` |
| artifact root | `/tmp/uhf-primary-packet-quiet-hosted.2xtdWA` |
| packet quiet log | `/tmp/uhf-primary-packet-quiet-hosted.2xtdWA/packet-quiet.log` |
| nested probe root | `/tmp/uhf-primary-packet-quiet-hosted.2xtdWA/packet-quiet` |

Observed PASS markers:

```text
uhf-primary-packet-quiet-hosted-probe: PASS
formal-verdict=uhf-primary-packet-quiet-decoupling
packet-quiet-proof=UHF primary suppresses live formal packet egress before any accepted UHF SESSION_OPEN(seq0)
beacon-proof=not exercised by this wrapper; cite or rerun separate hosted 41A evidence for accepted UHF SESSION_OPEN(seq0) suppress start and same-session refresh
file-proof=non-regression held by CommEgressMux UT plus archived hosted UHF CCSDS adoption evidence
non-claim=no simultaneous dual-link runtime
non-claim=no UHF reliable transfer
non-claim=no RF closure
non-claim=no broader target non-quiet closure
```

Nested probe markers for the dedicated packet-quiet path:

```text
comm-session-and-downlink-qos-probe: PASS
formal-verdict=comm-session-and-downlink-qos
case-uhf-primary-packet-quiet-before-session-open=PASS sbandBackupSession=1929379841
command-proof=separate
file-proof=separate
```

Hosted packet-quiet proof boundary:

- the proof starts from normal hosted S-band primary state
- a governed hosted command switches the active primary role set to UHF
- before any accepted qualifying UHF `SESSION_OPEN(seq0)`, live formal UHF
  packet visibility stays suppressed
- this proof intentionally does not reuse beacon suppress start as the trigger
  for packet quiet

## Adjacent Fresh Hosted Beacon Verdict

This change reuses a separate fresh hosted 41A beacon suppress/runtime proof so
packet quiet and beacon suppress remain distinct review surfaces.

Repository-owned proof entry point:

```bash
bash scripts/run_uhf_beacon_suppression_hosted_probe.sh
```

Final passing run:

| Field | Value |
|---|---|
| date | `2026-05-28` |
| command | `bash scripts/run_uhf_beacon_suppression_hosted_probe.sh` |
| formal verdict | `uhf-beacon-suppression-runtime` |
| artifact root | `/tmp/uhf-beacon-suppress-hosted.dtOaJE` |
| summary log | `/tmp/uhf-beacon-suppress-hosted.dtOaJE/summary.log` |
| summary json | `/tmp/uhf-beacon-suppress-hosted.dtOaJE/summary.json` |

Observed PASS markers:

```text
uhf-beacon-suppression-hosted-probe: PASS
negative-accepted-sband-does-not-suppress=PASS
negative-rejected-uhf-session-open-does-not-suppress=PASS
suppress-start=accepted UHF SESSION_OPEN(seq0)
refresh=accepted UHF MODE_GET(seq1)
resume=bounded inactivity timeout clear plus resumed beacon capture
```

Adjacent beacon proof boundary reused by this change:

- entering UHF primary alone does not start beacon suppress
- accepted authenticated qualifying UHF `SESSION_OPEN(seq0)` still starts
  suppress
- accepted authenticated same-session UHF activity still refreshes the bounded
  inactivity window
- suppress clear and resume still remain owned by the existing beacon policy
  rather than the new packet-quiet trigger

## File Preservation Boundary

This change did not broaden file/downlink claims into a new reliable-transfer or
RF proof. Instead it preserves the formal file truth through a bounded
non-regression combination:

- fresh `CommEgressMux` unit coverage proves UHF primary packet quiet suppresses
  packets but not file/downlink routing
- archived hosted UHF CCSDS adoption evidence still governs the bounded hosted
  UHF command/file/downlink path

Reused archived file/downlink evidence:

- [docs/test-records/uhf-ccsds-hosted-adoption-v1/README.md](../uhf-ccsds-hosted-adoption-v1/README.md)

## Local Verification

Focused local verification completed in this change:

| Step | Command | Result |
|---|---|---|
| full local verification gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local` | PASS |
| comm controller UT | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommController_ut_exe` | PASS |
| comm egress mux UT | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommEgressMux_ut_exe` | PASS |
| hosted packet-quiet proof | `bash scripts/run_uhf_primary_packet_quiet_hosted_probe.sh` | PASS |
| hosted beacon proof | `bash scripts/run_uhf_beacon_suppression_hosted_probe.sh` | PASS |
| OpenSpec change validate | `openspec validate uhf-primary-packet-quiet-decoupling-v1` | PASS |
| OpenSpec specs validate | `openspec validate --specs` | PASS via fresh local verification gate |

## Path Registration Impact

This record upgrades the hosted UHF primary packet-quiet path into a stable
archived evidence surface rather than leaving it as a bare script entry point.

This record does not merge packet quiet and beacon suppress into one proof.
Reviewers should still cite:

- entry `41B` for primary-band-driven start of formal UHF packet suppression
- entry `41A` for accepted-session-driven beacon suppress start, refresh, and
  timeout-clear behavior
- entry `43A` when the claim depends on the existing hosted UHF CCSDS
  command/file/downlink adoption surface
