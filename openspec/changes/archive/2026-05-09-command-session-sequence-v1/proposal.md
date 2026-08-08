## Why

`command-session-sequence-foundation-v1` added a strict-monotonic sequence helper and `command-envelope-metadata-v1` added runtime carriage for `session_id` and `sequence_number`, but the active OBC command path still only observes sequence metadata. Duplicate, lower, or wraparound envelope commands can still reach `Svc::CmdDispatcher` when authority allows the inner command.

This change wires the existing sequence helper into `CommandIngressAuthority` for valid command envelope v1 packets only. It adds active duplicate/lower sequence rejection before dispatcher execution while preserving the legacy non-envelope command path.

## What Changes

- Add active sequence-window enforcement for valid envelope commands after authority allows the inner opcode.
- Reject duplicate, lower, wraparound, or sequence-window-full envelope commands before `CmdDispatcher`.
- Add a dedicated sequence rejection event, rejection counters, and last-rejection telemetry.
- Keep malformed envelope, invalid config, unknown/restricted authority, and authority-denied commands from updating sequence state.
- Preserve `Fw.Com.context` as command status correlation only.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: define active command-session sequence enforcement for enveloped commands.
- `verification-evidence`: require component and hosted evidence for accepted, increasing, duplicate, lower, and authority-denied-does-not-consume behavior.
- `verification-path-registry`: update the hosted command ingress authority profile path boundary for active envelope sequence enforcement evidence.

## Impact

- Affected code:
  - `OBC/Components/CommandIngressAuthority`
  - command authority component/helper tests
  - hosted command session sequence probe
  - verification evidence and registry docs
- Public/dictionary-visible impact:
  - new `COMMAND_SEQUENCE_REJECTED` event and bounded sequence rejection telemetry on `CommandIngressAuthority`
- Non-goals:
  - no crypto authentication, full replay protection, reliable transfer, trusted source, physical UHF provenance, dual-link simultaneous proof, persistent session state, session-open/reset/resync, file authority, or unknown packet authority.
