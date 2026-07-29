## Why

The repository's internal subsystem transport drifted away from the original `libcsp-first` intent. Early narrative documents described `libcsp + ZMQ` for the internal OBC-to-subsystem network, but the formal specs and runtime implementation settled on a project-local shared request/response protocol over direct ZMQ REQ/REP. As a result:

- `CspBridge` is only a diagnostic shim with fake runtime counters
- `EpsBridge` and `AdcsBridge` depend on direct project-local ZMQ transport clients
- the hosted simulators are plain ZMQ REP servers rather than CSP nodes
- `simulators/common/protocol.h` became the effective on-wire authority for multiple subsystems

The repository needs a controlled recovery path that restores `libcsp` as the real internal network substrate without rewriting the entire project or disturbing unrelated domains.

## What Changes

- Establish a long-lived `feature/libcsp-internal-network-base` branch as the future migration base.
- Add a reproducible in-repo `libcsp` dependency and make the hosted profile build a real CSP runtime substrate with the official ZMQHUB interface and a repo-local hub/proxy executable.
- Rework `CspBridge` into the real runtime owner/facade for CSP init, ping, raw send, and metrics while preserving its public F' command, telemetry, and event contracts.
- Update the platform and core-contract specs so they describe an internal `libcsp` network, not a project-local direct ZMQ request/response transport.
- Register the new hosted internal CSP foundation path separately from the existing ground path and external comm paths.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `platform-baseline`: move the hosted internal-subsystem baseline from direct ZMQ transport to `libcsp` with the official ZMQHUB-backed hosted profile.
- `core-system-contracts`: redefine `CspBridge` as the real libcsp runtime owner/facade instead of a fake diagnostic shim.
- `verification-evidence`: require the internal CSP foundation path to be documented distinctly from the ground path and the external comm path.

## Impact

- Affected code: `libcsp` dependency intake, hosted build/runtime substrate, `CspBridge`, startup configuration, verification scripts, and internal-path evidence.
- Affected systems: hosted internal subsystem networking only.
- Explicitly out of scope for this change: `CommController`, `RadioController`, `UartDriver`, `GpsBridge`, GPS live UART work, boot/update behavior, GDS public behavior, and external comm validation paths.
