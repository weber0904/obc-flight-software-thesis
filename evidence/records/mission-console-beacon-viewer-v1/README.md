# Mission Console Beacon Viewer v1 Evidence

Status: branch-local demo-ready evidence for `mission-console-beacon-viewer-v1`.
Date: 2026-07-09.

## Scope

This record captures the first branch-local Mission Console beacon viewer proof
on top of the current manual dual-GDS operator baseline.

It proves:

- hosted manual-surface beacon capability discovery in Mission Console
- target manual-surface beacon capability discovery in Mission Console
- bounded dashboard Beacon card summary:
  - `Last Beacon Time`
  - `Sequence`
- dedicated `/beacon` latest/history/detail API surface
- explicit hosted vs target source distinction

It does **not** prove:

- RF or OTA beacon receipt
- stock `fprime-gds` beacon display
- `GET_*` readback behavior
- UHF auth, command ingress, or command completion
- target-side manual-surface lifecycle as an independent runtime proof

## Current Operator Truth

Mission Console now consumes a beacon capability exported by the manual
surface, not by the stock GDS listener stack.

Current source kinds:

- hosted:
  - `hosted-pty-side-channel`
- target:
  - `target-remote-sidecar`

Current source bands:

- both proofs consume `uhf-backup` beacon ancestry

Current UI contract:

- `/dashboard`:
  - only `Last Beacon Time` and `Sequence`
- `/beacon`:
  - latest summary
  - provenance
  - capture metadata
  - decode status
  - full decoded payload
  - bounded history

## Repo-Backed Artifacts

Canonical artifact root:

- [artifacts/2026-07-09-demo-ready](ARTIFACTS.json)

Hosted proof files:

- [hosted-manifest.json](ARTIFACTS.json)
- [dashboard.json](ARTIFACTS.json)
- [beacon-latest.json](ARTIFACTS.json)
- [beacon-history.json](ARTIFACTS.json)

Target proof files:

- [target-manifest.json](ARTIFACTS.json)
- [dashboard.json](ARTIFACTS.json)
- [beacon-latest.json](ARTIFACTS.json)
- [beacon-history.json](ARTIFACTS.json)

## Hosted Proof

Context:

- `hosted-manual-dual-gds`

Fresh observed API outcome:

- `/api/dashboard?contextId=hosted-manual-dual-gds`
  - `cards.Beacon.available=true`
  - `cards.Beacon.lastObservedAt` present
  - `cards.Beacon.sequence=0`
- `/api/beacon/latest?contextId=hosted-manual-dual-gds`
  - `supported=true`
  - `available=true`
  - `sourceBand=uhf-backup`
  - `sourceKind=hosted-pty-side-channel`
  - `capture.frameSize=108`
  - `decode.status=ok`
- `/api/beacon/history?contextId=hosted-manual-dual-gds`
  - bounded history returned

Hosted decoded corroboration from the fresh latest payload:

- `type=BeaconV1`
- `sequence=0`
- `time.seconds=1783604180`
- `fault_mask=4`
- `health_mask=8`

## Target Proof

Context:

- `target-manual-ground-dual-gds`

Fresh observed API outcome:

- `/api/dashboard?contextId=target-manual-ground-dual-gds`
  - `cards.Beacon.available=true`
  - `cards.Beacon.lastObservedAt` present
  - `cards.Beacon.sequence=1`
- `/api/beacon/latest?contextId=target-manual-ground-dual-gds`
  - `supported=true`
  - `available=true`
  - `sourceBand=uhf-backup`
  - `sourceKind=target-remote-sidecar`
  - `capture.frameSize=108`
  - `decode.status=ok`
- `/api/beacon/history?contextId=target-manual-ground-dual-gds`
  - bounded history returned

Target decoded corroboration from the fresh latest payload:

- `type=BeaconV1`
- `sequence=1`
- `time.seconds=1783604995`
- `battery_soc=95.3`
- `csp_tx_packets=659`
- `radio_rx_bytes=1020`

## Provenance Distinction

The viewer now makes two different truths reviewable:

- hosted proof:
  - Mission Console reads a local PTY-backed side-channel artifact produced by
    the hosted manual surface
- target proof:
  - Mission Console reads a local mirrored artifact produced from a target-side
    managed remote sidecar

Therefore:

- the target viewer is **not** a claim that Mission Console directly receives
  beacon over stock GDS
- the target viewer is **not** a claim that this proof closes RF/OTA reception

## Owner-Lifecycle Regression Closure

On 2026-07-12, a hosted manual surface was launched through the normal script
with an isolated `/tmp` surface root, runtime root, headless GDS, and automatic
ports. After the launcher command exited, its recorded owner PID was still
alive, was the leader of its own OS session, and the UHF beacon capability still
reported `sourceKind=hosted-pty-side-channel`. The official hosted stop script
then completed with `lifecycleState=stopped`.

The launcher now uses the shared detached-owner helper for both hosted and
target manual surfaces. This closes the Codex exec owner-PID death/reaped-child
artifact without changing target A/B/C ownership. The hosted lifecycle result
is evidence for that shared launcher contract; it is not an independent target
manual-surface runtime proof.

## 2026-07-12 Target Baseline-Owned Sidecar Closure

The target Beacon sidecar is now established by A, not by the target manual
ground surface. A records its remote capture path, unique bridge identity,
frame size, UHF service, and `target-remote-sidecar` provenance in the target
baseline JSON. The manual ground surface only mirrors that artifact locally.

The governed rerun used A -> B -> C and then A -> B postflight. C passed the
existing Mission Console target auth/readback/dashboard assertions and the
Beacon API returned `supported=true`, `available=true`, `frameSize=108`,
`decode.status=ok`, and advancing Beacon sequences with
`sourceKind=target-remote-sidecar`. A postflight and B cleanup both passed.

This proves a ground-visible baseline-owned mirror consumed by Mission Console;
it does not claim stock GDS Beacon visibility or RF/OTA receipt.

## Adjacent Evidence

Lower-level beacon ancestry remains governed by separate records:

- [evidence/records/uhf-uart-backup-link-v1/README.md](../uhf-uart-backup-link-v1/README.md)
- [evidence/records/uhf-beacon-suppression-runtime-v1/README.md](../uhf-beacon-suppression-runtime-v1/README.md)
- [evidence/records/mission-console-phase1/README.md](../mission-console-phase1/README.md)

This record adds the Mission Console operator-facing viewer surface on top of
those lower-level runtime paths; it does not replace them.
