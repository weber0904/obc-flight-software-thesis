## Why

`rateGroup1` and the earlier `rateGroup3` slip investigation showed that the deployed target topology still lets multiple steady-state clients contend on one shared CSP runtime through direct blocking calls. The bounded RG3 fix closed the most visible failure, but the current ownership model still leaves RG1 periodic polling, COMM service traffic, and subsystem bridge traffic coupled through the same runtime boundary.

## What Changes

- Introduce a repo-owned `CspRuntimeOwner` component as the single deployed owner of the CSP runtime lifecycle and client request path.
- Reconfigure deployed COMM, EPS, and ADCS client-side CSP users so the topology injects the owner-managed runtime instead of letting components bind directly to `defaultRuntime()`.
- Preserve current public F' command, event, and telemetry contracts while moving runtime ownership and observability to the new owner boundary.
- Add owner-oriented diagnostics and verification evidence so timeout, queueing, and stale-cache behavior stay reviewable after the refactor.

## Capabilities

### New Capabilities
- `csp-runtime-owner`: Define the single-owner topology pattern, owner telemetry/event surfaces, and the internal client request boundary for deployed CSP users.

### Modified Capabilities
- `platform-baseline`: The deployed topology runtime baseline changes from direct shared `defaultRuntime()` usage to one injected runtime owner.
- `comm-subsystem`: COMM-side CSP health probing and ground-link backend usage move under the runtime owner boundary without changing public COMM contracts.
- `eps-subsystem`: `EpsBridge` runtime ownership changes so deployed CSP polling no longer binds directly to the global shared runtime.
- `adcs-subsystem`: `AdcsBridge` runtime ownership changes so deployed CSP polling no longer binds directly to the global shared runtime.
- `verification-evidence`: This refactor needs reviewable unit/build and target secure-auth evidence that the new owner model preserves bounded observability and does not hide subsystem timeout faults.

## Impact

- Affected code spans `OBC/Components/CspRuntimeOwner`, `CommController`, `CspBridge`, `GroundLinkDriver`, `EpsBridge`, `AdcsBridge`, the CSP-backed simulator transports, and `OBC/TopCcsds`.
- No public mission command/event/tlm IDs are intended to change.
- Follow-on work will still be needed to convert RG1 scheduled subsystem polling from blocking transport round-trips to fully async/coalesced owner-mediated polling.
