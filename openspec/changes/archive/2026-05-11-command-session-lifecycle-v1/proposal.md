## Why

`command-session-sequence-v1` closed the duplicate/lower sequence rejection gap for valid command envelope v1 packets, but the active runtime still has no explicit session lifecycle. The first authority-allowed packet for any unseen `session_id` is implicitly accepted, there is no governed resync path after sequence desync, and reboot clears memory state without a defined operator recovery contract.

This change closes that runtime control-plane gap without pulling in authenticated envelopes. It makes session open explicit, defines replace/resync and reboot behavior, preserves the legacy non-envelope path boundary, and keeps `uhf-backup` enveloped read/status traffic operational.

## What Changes

- Add explicit `SESSION_OPEN` lifecycle handling owned by `CommandIngressAuthority`.
- Require a valid envelope and `sequence_number = 0` to open or replace a session.
- Require a source epoch to be open before non-lifecycle enveloped commands are sequence-checked or dispatched.
- Reject stale, mismatched, legacy, malformed, or unexpected lifecycle transitions before `CmdDispatcher`.
- Add dedicated session lifecycle events, telemetry, and hosted probe evidence.
- Define reboot/runtime restart behavior as memory-only state loss requiring a fresh `SESSION_OPEN`.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: define explicit session-open/replace behavior over the existing envelope path.
- `verification-evidence`: require component and hosted evidence for open, replace/resync, reboot recovery, and fail-closed behavior.
- `verification-path-registry`: extend the hosted command ingress authority profile path boundary to cover session lifecycle v1.

## Impact

- Affected code:
  - `OBC/Components/CommandIngressAuthority`
  - command authority catalog/policy generation
  - command authority component/helper tests
  - hosted command session lifecycle probe
  - verification and roadmap/architecture docs
- Public/dictionary-visible impact:
  - new `OBCApp.commandIngressAuthority.SESSION_OPEN` command
  - new session lifecycle events and telemetry on `CommandIngressAuthority`
- Non-goals:
  - no auth/MAC/signature/crypto
  - no full replay-protection claim
  - no persistent secure session storage
  - no file/unknown uplink authority
  - no COMM QoS/failover, watchdog, TTC scheduling, or payload-manager scope
