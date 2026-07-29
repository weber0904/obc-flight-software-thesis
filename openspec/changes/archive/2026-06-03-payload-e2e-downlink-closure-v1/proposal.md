## Why

The payload baseline can capture governed `.jpg + .json` artifacts, but those artifacts still stop at the local filesystem and never enter the active repo-owned stored-history and downlink path. This keeps payload below true end-to-end mission closure even though secure command, bounded uplink authority, and the current official `.fdp` downlink path are already part of the active baseline.

## What Changes

- Add a canonical payload data-product family that promotes each successful payload capture into an official `.fdp` artifact containing capture metadata plus JPEG bytes.
- Keep `PayloadOpsController` as the only public payload owner and have it publish the canonical payload `.fdp` immediately after local capture succeeds.
- Require payload capture command success to include canonical payload `.fdp` publication; local `.jpg + .json` artifacts remain only as intermediate and diagnostic surfaces.
- Reuse the current official `DpCatalog -> CommController -> FileDownlink` delivery path for payload artifacts without introducing payload-specific file commands, generic browse/download ownership, or reliable-transfer broadening.
- Add a repository-owned hosted node-`5` proof path for payload `.fdp` byte-match, decode, and JPEG extraction parity, and explicitly defer the governed target node-`5` payload `.fdp` proof to a follow-on change unless a single repo-owned COMM-backed wrapper is available.
- Reconcile active docs, registry, and roadmap narrative so payload closure, 2026-06-01 target secure-auth proof, and 2026-06-03 UHF nonquiet instability are represented consistently.

## Capabilities

### New Capabilities
- `payload-data-products`: canonical payload capture `.fdp` production, bounded size policy, decode/extract parity, and official payload artifact delivery contract

### Modified Capabilities
- `payload-operations`: payload capture success semantics and readback metadata now include canonical payload `.fdp` identity and publication state
- `comm-subsystem`: current official payload `.fdp` downlink reuses `DpCatalog` ownership without widening reliable-transfer or arbitrary-file scope
- `resource-storage`: payload capture storage keeps `.jpg + .json` under the governed persistent-data root while canonical payload artifacts live under the official data-products root
- `verification-path-registry`: the hosted payload `.fdp` proof path becomes formally registered, and target payload `.fdp` official closure remains explicitly distinct and deferred from existing HK `.fdp` and direct-adapter payload evidence until a governed target wrapper lands

## Impact

- Affected code: `PayloadOpsController`, `PayloadOpsRuntime`, `TopCcsds` data-product wiring/configuration, payload CSP metadata readback, unit tests, hosted/target probe scripts, and payload decode tooling
- Affected systems: official F' data-product storage, `DpBufferManager` bin sizing, `DpCatalog`-driven downlink evidence, payload operator runbooks, current architecture docs, roadmap ordering, and verification registry/test records
- Public behavior: payload capture commands can now fail on canonical payload publication or oversize ceiling even if the local JPEG exists; readback surfaces will expose the canonical payload `.fdp` identity
