## Why

The repository now has a near-term topology with distinct OBC and subsystem-side Raspberry Pi hosts, but the current scripts and documentation still assume one generic Raspberry Pi target at `operator@youjun.local`. That mismatch will create confusion and drift as soon as the second Pi enters normal use, so the host roles and default names need to be formalized before dual-host workflows expand further.

## What Changes

- Align repository defaults with the new two-Pi topology by introducing explicit OBC and subsystem-simulator host role names.
- Make `OBC_SSH_TARGET` the canonical OBC host setting while keeping `RPI_SSH_TARGET` as a backward-compatible alias for existing OBC-target workflows.
- Add `SUBSYSTEM_SIM_SSH_TARGET` as the formal default for the second Pi without yet introducing a full dual-Pi orchestration workflow.
- Update platform and operator-facing documentation so the repository consistently describes `macOS` as the ground host, `obc.local` as the OBC target, and `subsystem.local` as the subsystem simulator host.

## Capabilities

### New Capabilities
- None

### Modified Capabilities
- `platform-baseline`: formalize named OBC and subsystem-simulator Raspberry Pi roles and the canonical host-target configuration for the near-term multi-host topology

## Impact

- Affected code: Raspberry Pi helper scripts, shared shell helpers, repo documentation, and one platform-baseline delta spec.
- Affected systems: OBC-target SSH flows, future dual-Pi topology documentation, and operator defaults.
- No change to F' business logic, CSP service contracts, command dictionaries, or runtime topology behavior.
