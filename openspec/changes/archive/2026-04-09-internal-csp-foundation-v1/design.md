## Overview

This change implements only the foundation layer of the libcsp recovery:

- bring `libcsp` into the governed workspace as a pinned in-repo dependency
- build a hosted CSP substrate using the official ZMQHUB interface and a repo-local hub/proxy executable
- replace the fake `CspBridge` runtime semantics with a real CSP runtime owner/facade

It does **not** yet migrate `EpsBridge` or `AdcsBridge` business traffic. That split is deliberate. The foundation change should prove that the repository can host a real internal CSP runtime while leaving the existing EPS and ADCS bridge contracts untouched until their dedicated vertical slices.

## Design Decisions

### Strict Domain Separation

The repository keeps three distinct transport domains:

- ground path: `fprime-cli -> GDS -> OBC(F')`
- internal subsystem network: `OBC(F') -> libcsp -> subsystem nodes`
- external comm and dedicated-device paths: external radio path plus GPS path

This change only modifies the internal subsystem network domain. It does not turn GDS into a CSP gateway and does not pull the external comm stack or GPS path into the internal CSP network.

### Long-Lived Integration Base

The future libcsp recovery work lives on `feature/libcsp-internal-network-base`. This avoids mixing unstable internal-network migration work directly into `main` while the repository still has active direct-ZMQ subsystem paths.

### Pinned In-Repo libcsp Dependency

`libcsp` is brought in as a pinned submodule so the hosted profile does not depend on a global install. The repository owns the exact revision used for the migration base.

### Hosted Foundation Uses Official ZMQHUB Semantics

The hosted profile uses libcsp's official ZMQHUB interface and a repo-local hub/proxy executable. The repository will stop describing direct ZMQ REQ/REP simulator clients as “internal CSP transport”.

### CspBridge Becomes The Runtime Owner

`CspBridge` remains the owner of the public `CSP_*` F' symbols, but its implementation changes from fake counters to a real runtime facade:

- initialize the local node
- bind the official ZMQHUB-backed interface
- set up routing
- ping known nodes
- send small raw debug payloads
- report runtime counters and free buffers from the real libcsp state

The component still owns diagnostics and bring-up commands. It is **not** redesigned into the business-traffic proxy for EPS or ADCS requests.

### Foundation First, Vertical Slices Later

This change intentionally leaves `EpsBridge` and `AdcsBridge` on their current direct transport implementations. Their public contracts stay intact, but their transport migration is deferred:

- `eps-csp-vertical-slice-v1`
- `adcs-csp-vertical-slice-v1`

This keeps the first recovery step small enough to validate and prevents a simultaneous rewrite of the runtime substrate plus both subsystem service layers.

### Verification Path Separation

The repository will register a new hosted internal CSP foundation path. That path proves:

- node `1` hosted libcsp initialization
- hosted ZMQHUB binding
- hosted ping/raw-send semantics against a CSP peer

It does **not** prove:

- `fprime-cli -> GDS` command/uplink behavior
- direct `OBC -> GDS` connectivity by itself
- EPS or ADCS business traffic migration
- external comm transport behavior

## Implementation Notes

### libcsp Build Strategy

The repository will build only the libcsp subset required by the hosted foundation path:

- core runtime
- POSIX or Darwin-compatible architecture layer
- loopback and ZMQHUB interfaces
- upstream `zmqproxy` executable

The hosted macOS profile must remain buildable from the governed workspace. If upstream `libcsp` CMake does not natively expose a Darwin-hosted POSIX path, the repository's integration layer will adapt the build without changing the public repository workflow.

### Runtime Configuration Inputs

The foundation change adds explicit CSP runtime configuration separate from GDS transport configuration:

- `CSP_NODE_ID`
- `CSP_HUB_HOST`
- `CSP_HUB_SUB_PORT`
- `CSP_HUB_PUB_PORT`

Scripts and docs must keep those inputs distinct from:

- `GDS_HOST`
- `GDS_PORT`

### Metrics Surface

`HousekeepingSnapshotProvider` continues to consume CSP metrics through `CspBridge::getCountersForRuntime()`. After this change those values come from real runtime state instead of hardcoded counters.
