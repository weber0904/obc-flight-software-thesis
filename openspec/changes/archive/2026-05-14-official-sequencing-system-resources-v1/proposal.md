## Why

The current active baseline has bounded `TTC` pass-window policy ownership in `TtcPassManager`, authenticated command ingress with lifecycle and sequence enforcement, and a working hosted CCSDS S-band path. It still does not have a governed sequence-execution surface, official `CmdSequencer`/`SeqDispatcher` integration, or an active file-uplink governance owner on the current CCSDS file path.

This change is needed now to avoid building a repo-native timed-command table on top of an upstream sequencing capability that already exists. The repository should adopt official F Prime sequence execution first, then layer repo-specific file-ingress governance and authority admission around it so future TTC and payload work consume one truthful timed-command surface.

## What Changes

- Integrate official `Svc::CmdSequencer`, `Svc::SeqDispatcher`, and `Svc::SystemResources` into active `TopCcsds`.
- Add a repo-owned `FileIngressAuthority` on the active `ComCcsds.fprimeRouter.fileOut -> FileHandling.fileUplink.bufferSendIn` path.
- Add a repo-owned `SequenceAdmissionController` that becomes the only external sequence-operations owner.
- Deny direct comm-managed use of stock external sequence commands such as `SeqDispatcher.RUN`, `SeqDispatcher.RUN_ARGS`, and raw `CmdSequencer` `CS_*` control commands.
- Add admitted-copy workflow and ownership-aware manual stepping so sequence execution does not rely on mutable staged files or unowned stock controls.
- Add official `SystemResources` as the preferred standard resource telemetry surface without deleting existing custom resource or watchdog policy telemetry.

## Capabilities

### New Capabilities

- governed official sequence execution on active `TopCcsds`
- governed active CCSDS file-uplink admission for sequence staging on the current S-band file path
- official system resource telemetry in the active baseline

### Modified Capabilities

- `platform-baseline`: active runtime now includes official sequencing and `SystemResources`
- `core-system-contracts`: command authority now governs sequence-admission and direct stock sequence control denial
- `comm-subsystem`: the current CCSDS file-ingress path gains a repo-owned governance owner
- `verification-evidence`: add required hosted proof and component/helper coverage for official sequence execution and file-ingress governance
- `verification-path-registry`: register the new active hosted sequence-upload and sequence-execution path

## Impact

- Affected runtime areas:
  - `OBC/TopCcsds`
  - `CommandIngressAuthority` and command authority catalog/policy
  - current `ComCcsds` file-uplink path
  - active hosted verification path and operator workflow
- Affected proof/docs areas:
  - architecture truth
  - roadmap
  - operator documentation
  - verification registry and test record

## Non-Claims

This change does not claim:

- mission scheduler behavior
- persistent onboard schedule storage
- GPS mission-time scheduling or RTC proof
- payload planning or payload framework integration
- generic file-uplink governance for all links or unknown packet routes
- conflict-free or resource-arbitrated multi-sequence execution
