## Why

The current physical lab serial COMM evidence proves bounded uplink ingress, but it explicitly stops short of registering full physical lab serial TT&C because the final closeout run did not prove bounded event and telemetry downlink through `fprime-cli`.

This change is needed to make the next proof boundary explicit: keep the existing physical command ingress requirement, then promote bounded physical lab serial TT&C only when ground-side command events and telemetry are visible over the same COMM path.

## What Changes

- Add a downlink-focused physical lab serial COMM probe that reuses the current macOS-to-`subsystem.local` serial wiring and existing COMM CSP service contract.
- Require bounded EPS and ADCS command readback as the Stage 1 prerequisite before a downlink verdict can pass.
- Make Stage 2 downlink a formal gate: `fprime-cli events` must observe bounded command events and `fprime-cli channels` must observe `GROUND_LINK_TX_BYTES`.
- Preserve stock F' framing, stock `fprime-gds`, hosted OBC, native-built `subsystem.local` COMM node `4`, and services `30` through `32`.
- Record new evidence separately from hosted PTY TT&C, UART preflight, subsystem-origin acquisition, and the prior physical uplink ingress record.
- Keep file/downlink, RF, target OBC migration, `ScenarioBridge`, `ground_pass_open`, `link_available`, no-preamble first-byte-clean behavior, and COMM SocketCAN participation out of scope.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `comm-subsystem`: add the bounded physical lab serial downlink/full-TT&C proof boundary after physical uplink ingress.
- `ground-ttc-gateway`: require full physical lab serial TT&C claims to include bounded event and telemetry visibility through the gateway path.
- `verification-evidence`: require downlink evidence to include the physical endpoints, command readback, events, telemetry, and verdict boundary.
- `verification-path-registry`: register bounded physical lab serial TT&C separately from the prior uplink-ingress-only path after the focused probe passes.

## Impact

- Adds a repository-owned focused probe under `scripts/`.
- Adds an evidence record under `docs/test-records/ttc-over-comm-lab-serial-downlink-v1/`.
- Updates the COMM roadmap after the proof lands.
- May tighten probe orchestration or observability for existing COMM ground-link code, but does not change the COMM CSP wire contract, public service ports, GDS interface, or deployment model.
