## Why

After `link-authority-vocabulary-v1`, the repository will have a command authority vocabulary and policy source, but the active OBC runtime will still route all decoded command packets directly from `Svc::FprimeRouter.commandOut` to `Svc::CommandDispatcher.seqCmdBuff`. UHF backup evidence would therefore continue to prove command ingress without enforcing the conservative backup authority policy.

This change adds OBC-side command ingress authority for routed `Fw.Com` command packets before `Svc::CommandDispatcher`, while preserving F Prime command source/status semantics and avoiding over-claims about full link or uplink authority.

## What Changes

- Add a passive `CommandIngressAuthority` component between `FprimeRouter.commandOut` and `CmdDispatcher.seqCmdBuff`.
- Wire the component into both the default CCSDS topology and the legacy ComFprime topology.
- Preserve command source port index and `Fw.Com.context` when forwarding to `CmdDispatcher` and returning status.
- Reject denied commands before `CmdDispatcher`, emit authority event/telemetry, and synthesize exactly one `Fw::CmdResponse`.
- Use explicit hosted profile configuration:
  - S-band profile supplies `SBAND + PRIMARY`
  - UHF backup profile supplies `UHF + BACKUP`
  - missing, invalid, or `UNKNOWN` configuration denies by default
- Keep `fileOut` and `unknownDataOut` out of scope and document them as deferred.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: add OBC-side command ingress authority enforcement for routed `Fw.Com` command packets before `Svc::CommandDispatcher`.
- `verification-evidence`: require component and hosted evidence for command ingress authority behavior.
- `verification-path-registry`: add a distinct hosted command ingress authority profile path and keep physical/link-path claims separate from the existing S-band and UHF COMM path entries.

## Impact

- Affected code:
  - `OBC/Components/CommandIngressAuthority`
  - `OBC/TopCcsds` and `OBC/Top` topology wiring
  - runtime hosted profile/config plumbing
  - authority policy/catalog generator and tests
  - focused hosted probes and evidence
- Affected behavior:
  - UHF backup configured ingress role no longer reaches subsystem handlers for denied high-authority commands.
  - S-band primary and dev/internal configured profiles preserve current command behavior except for explicit authority-gate telemetry/events.
- Non-goals:
  - No gateway enforcement, file/unknown uplink authority, full link authority, crypto auth, sessions, sequence windows, replay protection, dynamic failover, persistent config store, scheduler, FDIR, or CCSDS route redesign.
