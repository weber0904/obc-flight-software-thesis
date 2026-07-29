## Why

`command-session-sequence-foundation-v1` added a helper-only strict-monotonic sequence primitive, but the active OBC command path still has no governed way to carry mission `session_id` or `sequence_number` metadata. Enabling runtime sequence checks before defining that carriage would either misuse `Fw.Com.context` or create test-only enforcement that hosted commands cannot exercise.

This change adds a narrow mission command envelope metadata contract. The envelope travels as a project-owned pseudo-opcode inside the existing routed `FW_PACKET_COMMAND` path, is unwrapped by `CommandIngressAuthority` before `Svc::CommandDispatcher`, and leaves the existing legacy F Prime command path working unchanged.

## What Changes

- Define mission command envelope v1 as `FW_PACKET_COMMAND + OBC_COMMAND_ENVELOPE_V1_OPCODE`.
- Add envelope parser/serializer helper code near `CommandIngressAuthority`.
- Have `CommandIngressAuthority` unwrap valid envelopes, observe `session_id` and `sequence_number`, then evaluate the inner command through the existing ingress-port authority policy.
- Preserve `Fw.Com.context` as command status correlation only.
- Add bounded envelope observed/rejected events and telemetry.
- Add a repo-owned hosted envelope injector/probe that sends raw encoded command bytes through the existing hosted CCSDS S-band command path.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: define the project-owned command envelope metadata contract and runtime unwrap boundary.
- `verification-evidence`: require unit/component and hosted evidence for legacy compatibility, envelope metadata observation, and scoped authority behavior.

## Impact

- Affected code:
  - `OBC/Components/CommandIngressAuthority`
  - command authority component and helper tests
  - hosted command envelope metadata probe
  - verification evidence and registry docs
- Public/dictionary-visible impact:
  - new command envelope observed/rejected event and telemetry surfaces on `CommandIngressAuthority`
- Non-goals:
  - no active session enforcement, replay protection, authentication, crypto/MAC, nonce, persistent session state, reliable transfer, physical UHF provenance, dual-link proof, file authority, or unknown packet authority.
