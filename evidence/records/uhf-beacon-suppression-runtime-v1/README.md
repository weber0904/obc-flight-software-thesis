# uhf-beacon-suppression-runtime-v1 Evidence

Date:
- `2026-05-27`

OpenSpec change:
- `uhf-beacon-suppression-runtime-v1`

## Scope

This record covers the current branch work that turns the already-frozen UHF
beacon suppress and resume policy into current runtime truth on the active
`TopCcsds` baseline.

Path under test:

- hosted governed node-`6` CCSDS UHF ingress plus BeaconV1 side-channel capture
- target/lab governed quiet node-`6` UHF ingress plus probe-owned beacon
  capture on the current CAN-backed lab baseline

What was newly proven:

- `CommController` is the runtime owner of UHF beacon suppress and resume
- suppress starts only after an accepted authenticated UHF
  `SESSION_OPEN(seq0)` on the qualifying active session
- later accepted authenticated UHF same-session command activity, including
  read/status traffic, refreshes the bounded active window
- the inactivity timeout is fixed at `60` scheduler ticks on the current `1 Hz`
  baseline
- suppress clears immediately when the owning session is revoked, replaced, or
  invalidated by role switch or COMM failover handling
- `BeaconPublisher` stays session-agnostic and only applies the bounded gate
  owned by `CommController`
- hosted status, COMM telemetry/events, and governed probe artifacts now make
  suppress truth reviewable

What remains not proven:

- simultaneous dual-link runtime arbitration
- UHF reliable transfer
- ARQ / NACK / CFDP
- RF or over-the-air closure
- general non-quiet UHF background-telemetry stability on the target path
- any broader UHF handshake state machine beyond the accepted
  `SESSION_OPEN(seq0)` boundary

## Implemented Probe Entry Points

- [scripts/run_uhf_beacon_suppression_hosted_probe.sh](../../../evidence/verification-path-registry.md)
- [scripts/run_target_can_uhf_beacon_suppression_probe.sh](../../../evidence/verification-path-registry.md)
- [scripts/comm_verification/lib/run_target_can_matrix_probe.py](../../../scripts/comm_verification/lib/run_target_can_matrix_probe.py)

## Hosted Governed Node-`6` Verdict

Verdict: `PASS` for the hosted governed node-`6` beacon suppress/runtime proof.

Final passing run:

| Field | Value |
|---|---|
| date | `2026-05-27` |
| command | `bash scripts/run_uhf_beacon_suppression_hosted_probe.sh` |
| formal verdict | `uhf-beacon-suppression-runtime` |
| path | `hosted node-6 CCSDS ground_ttc_gateway serial + uhf_comm_csp_node node 6 -> CSP -> TopCcsds -> beacon CSP sink` |
| artifact root | `/tmp/uhf-beacon-suppress-hosted.Zghppg` |
| beacon capture | `/tmp/uhf-beacon-suppress-hosted.Zghppg/uhf-beacon-capture.bin` |
| first beacon json | `/tmp/uhf-beacon-suppress-hosted.Zghppg/first-beacon.json` |
| resume beacon json | `/tmp/uhf-beacon-suppress-hosted.Zghppg/resume-beacon.json` |

Observed PASS markers:

```text
uhf-beacon-suppression-hosted-probe: PASS
path=hosted node-6 CCSDS ground_ttc_gateway serial + uhf_comm_csp_node node 6 -> CSP -> TopCcsds -> beacon CSP sink
negative-accepted-sband-does-not-suppress=PASS
negative-rejected-uhf-session-open-does-not-suppress=PASS
suppress-start=accepted UHF SESSION_OPEN(seq0)
refresh=accepted UHF MODE_GET(seq1)
resume=bounded inactivity timeout clear plus resumed beacon capture
```

Hosted proof boundary:

- baseline beacon capture is visible before suppress starts
- accepted S-band activity does not suppress the UHF beacon
- rejected UHF `SESSION_OPEN(seq!=0)` does not suppress the UHF beacon
- accepted authenticated UHF `SESSION_OPEN(seq0)` starts suppress
- accepted authenticated same-session `MODE_GET(seq1)` refreshes the `60`-tick
  inactivity window
- the probe observes no resumed beacon until timeout clear occurs

## Target/Lab Quiet Node-`6` Verdict

Verdict: `PASS` for the target/lab quiet node-`6` beacon suppress/runtime
proof on the governed CAN-backed lab baseline.

Final passing run:

| Field | Value |
|---|---|
| date | `2026-05-27` |
| command | `bash scripts/run_target_can_uhf_beacon_suppression_probe.sh` |
| formal verdict | `uhf-beacon-suppression-runtime` |
| mode | `beacon-suppression` |
| profile | `uhf-primary` |
| path | `fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> physical serial -> subsystem.local uhf_comm_csp_node(node 6) -> shared SocketCAN -> obc.local OBC -> CommController suppress gate -> subsystem.local beacon capture` |
| artifact root | `/private/tmp/target-can-uhf-beacon-suppress.8YkZEa` |
| status log | `/private/tmp/target-can-uhf-beacon-suppress.8YkZEa/status.log` |
| beacon capture | `/private/tmp/target-can-uhf-beacon-suppress.8YkZEa/uhf-beacon-capture.bin` |
| first beacon json | `/private/tmp/target-can-uhf-beacon-suppress.8YkZEa/uhf-beacon-first.json` |
| resume beacon json | `/private/tmp/target-can-uhf-beacon-suppress.8YkZEa/uhf-beacon-resume.json` |

Observed PASS markers:

```text
target-can-matrix-probe: PASS
mode=beacon-suppression
profile=uhf-primary
baseline-beacon-count=2
sband-session-source=target-journal
sband-command-source=target-journal
negative-accepted-sband-does-not-suppress=PASS
negative-rejected-uhf-session-open-does-not-suppress=PASS
uhf-session-source=target-journal
suppress-start=accepted UHF SESSION_OPEN(seq0)
refresh-source=target-journal
refresh=accepted UHF MODE_GET(seq1)
resume=bounded inactivity timeout clear plus resumed beacon capture
resume-beacon-count=15
```

Target/lab proof boundary:

- the proof remains quiet-path only and intentionally uses probe-owned beacon
  capture plus journal-first acceptance
- accepted S-band activity does not suppress the UHF beacon
- rejected UHF `SESSION_OPEN(seq!=0)` does not suppress the UHF beacon
- accepted authenticated UHF `SESSION_OPEN(seq0)` starts suppress
- accepted authenticated same-session `MODE_GET(seq1)` refreshes the bounded
  active window
- resumed beacon capture appears only after bounded inactivity timeout clear

Target/lab non-claims preserved by this record:

- no non-quiet background-TM stability claim
- no simultaneous dual-link runtime claim
- no UHF reliable-transfer claim
- no RF closure claim

## Local Verification

Focused local verification completed in this change:

| Step | Command | Result |
|---|---|---|
| component/runtime UT build | `PATH="$PWD/fprime-venv/bin:$PATH" cmake --build build-fprime-automatic-native-ut --target OBC_Components_CommandIngressAuthority_ut_exe OBC_Components_CommController_ut_exe OBC_Components_BeaconPublisher_ut_exe hosted_runtime_unit_test -j 8` | PASS |
| command ingress authority UT | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe` | PASS |
| comm controller UT | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommController_ut_exe` | PASS |
| beacon publisher UT | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_BeaconPublisher_ut_exe` | PASS |
| hosted runtime status UT | `./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test` | PASS |
| hosted probe syntax | `bash -n scripts/run_uhf_beacon_suppression_hosted_probe.sh` | PASS |
| target wrapper syntax | `bash -n scripts/run_target_can_uhf_beacon_suppression_probe.sh` | PASS |
| target probe helper syntax | `PATH="$PWD/fprime-venv/bin:$PATH" python -m py_compile scripts/comm_verification/lib/run_target_can_matrix_probe.py` | PASS |
| target launch wrapper syntax | `bash -n packaging/rpi/launch/run_obc_comm_csp_stack.sh` | PASS |
| full local verification gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local` | PASS |
| hosted governed proof | `bash scripts/run_uhf_beacon_suppression_hosted_probe.sh` | PASS |
| target/lab governed proof | `bash scripts/run_target_can_uhf_beacon_suppression_probe.sh` | PASS |
| OpenSpec change validate | `openspec validate uhf-beacon-suppression-runtime-v1` | PASS |
| OpenSpec specs validate | `openspec validate --specs` | PASS |
| OpenSpec archive | `openspec archive uhf-beacon-suppression-runtime-v1 --yes` | PASS; archived as `2026-05-26-uhf-beacon-suppression-runtime-v1` |
| reconciliation matrix generate | `python3 scripts/generate_reconciliation_matrix_md.py` | PASS |
| repo consistency after archive | `python3 scripts/check_repo_consistency.py` | PASS |
| OpenSpec specs validate after archive | `openspec validate --specs` | PASS |

## Path Registration Impact

This record creates two dedicated reusable proof boundaries:

- hosted UHF node-`6` beacon suppress/runtime path
- target/lab quiet node-`6` beacon suppress/runtime path

It does not widen the meaning of ordinary hosted beacon side-channel capture,
ordinary hosted UHF command/readback proof, or ordinary quiet target node-`6`
command/file/failover proofs. Those adjacent paths remain separately cited in
[evidence/verification-path-registry.md](../../../evidence/verification-path-registry.md).
