## Why

`command-ingress-authority-v1` added an OBC-side authority gate before `Svc::CommandDispatcher`, but it still configures one authority profile for the whole `CommandIngressAuthority` component. The component already has indexed command/status ports, yet the active hosted topologies wire only index `0`. The next step is to make the source mapping explicit and testable without claiming cryptographic or physical provenance.

This change defines a topology-configured ingress source-index foundation: each `CommandIngressAuthority` input port may have its own configured authority source, unconfigured ports fail closed, and `Fw.Com.context` remains command-status correlation data rather than mission source identity.

## What Changes

- Add per-port authority-source configuration APIs to `CommandIngressAuthority`.
- Define legacy `configure(config)` as exactly `clearIngressSources(); configureIngressSource(0, config)`.
- Evaluate incoming commands against the authority config for their `seqCmdBuffIn[portNum]`.
- Deny unconfigured or invalid ingress ports by default with exactly one synthetic `Fw.CmdResponse`.
- Expand `COMMAND_AUTHORITY_REJECTED` with ingress port and link identity fields.
- Keep hosted default CCSDS and legacy ComFprime topologies wired only to index `0`.
- Prove port `1` behavior only through component tests unless a later topology wires a second routed command source.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: refine command ingress authority from component-level profile to topology-configured source-index mapping.
- `verification-evidence`: require evidence that hosted proof covers only port `0`, while multi-port behavior is component-level proof.
- `verification-path-registry`: update the hosted command ingress authority profile path with source-index proof bounds and the expanded rejection event fields.

## Impact

- Affected code:
  - `OBC/Components/CommandIngressAuthority`
  - command authority component tests
  - hosted command ingress authority probe/event parsing
  - evidence and pending planning docs
- Public/dictionary-visible impact:
  - `COMMAND_AUTHORITY_REJECTED` event schema changes by adding ingress port and link identity fields.
- Non-goals:
  - No trusted source, per-packet provenance, physical UHF provenance, cryptographic authentication, dual-link simultaneous proof, file/unknown uplink authority, active session enforcement, or replay protection.
