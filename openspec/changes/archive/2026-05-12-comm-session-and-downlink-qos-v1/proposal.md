## Why

The active baseline already has several proven but still separate communication surfaces: hosted CCSDS S-band node `5`, hosted UHF node `6`, authenticated command envelopes with explicit session-open and strict-monotonic sequence, and two existing file/downlink producers (`DpCatalog` and `HousekeepingArchive`). What is still missing is a real COMM-owned operational runtime that answers which link currently owns command, telemetry, and file transfer policy, how authenticated command classes change across `S-band primary` / `UHF backup` / `UHF primary`, and how shared file/downlink ownership converges when links switch or fail.

This change closes that gap on the active `TopCcsds` baseline. It turns COMM from a thin band/pass shell into the first operational session-and-downlink policy runtime, wires that policy into `CommandIngressAuthority`, and moves file/downlink arbitration for the existing `DpCatalog` and `HousekeepingArchive` producers under a deterministic COMM-owned owner.

## What Changes

- Upgrade `CommController` into the public COMM policy owner for link roles, primary-link selection, operational session state, and shared downlink ownership.
- Keep `CommandIngressAuthority` as the authenticated envelope/session/sequence owner, but make its runtime allow/deny profile dynamic and COMM-driven per ingress path.
- Extend active `TopCcsds` with a second bounded UHF node-`6` command ingress proof path instead of treating UHF as only a single-node runtime override.
- Add COMM-owned file/downlink scheduling in front of stock F' `FileDownlink`, so `DpCatalog` and `HousekeepingArchive` no longer compete directly for the same send/completion surface.
- Add reviewable events, telemetry, and counters for link-role changes, policy decisions, session revocation, downlink ownership, pending work, completion, and link-loss convergence.
- Add repository-owned hosted evidence that keeps S-band node `5`, UHF node `6`, command-policy proof, and file/downlink proof distinct while proving the new runtime policy actually runs on the active baseline.

## Capabilities

### New Capabilities

- none

### Modified Capabilities

- `comm-subsystem`: define COMM-owned operational session state, dual-ingress role policy, and shared downlink arbitration on the active baseline.
- `ground-ttc-gateway`: record separate S-band and UHF proof boundaries while allowing COMM policy evidence to span both without collapsing them into one link claim.
- `verification-evidence`: require separated command-policy and file/downlink evidence for the COMM session-and-downlink QoS slice.
- `verification-path-registry`: register the new hosted COMM policy path plus the distinct bounded UHF node-`6` file/downlink evidence.

## Impact

- Affected code:
  - `OBC/Components/CommController/*`
  - `OBC/Components/CommandIngressAuthority/*`
  - `OBC/TopCcsds/*`
  - hosted runtime/probe surfaces under `scripts/`
- Affected interfaces:
  - `CommController` gains COMM-owned policy/downlink scheduling ports and richer runtime state
  - `CommandIngressAuthority` gains COMM-driven runtime policy collaboration across both ingress ports
  - `HousekeepingArchive` and `DpCatalog` keep their existing operator-facing commands but now traverse a COMM-owned shared file/downlink surface
- Affected docs/evidence:
  - new change-level test record
  - verification-path registry updates
  - roadmap and current-development architecture updates

## Scope Boundary

This change claims the first operational COMM session + link-role + downlink QoS closure on the active baseline. It proves hosted S-band node `5`, hosted UHF node `6`, dynamic authenticated command policy across those roles, COMM-owned HK/DP downlink arbitration, and bounded UHF file/downlink after primary switch. It does not claim RF behavior, CCSDS-on-UHF, CFDP, ARQ/NACK, reliable transfer, arbitrary onboard file downlink, target/Pi hardware closure, persistent replay protection, hardware key storage, or a full secure-boot chain.
