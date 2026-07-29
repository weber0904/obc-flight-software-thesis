## Overview

This change migrates one real subsystem path, EPS, onto the hosted libcsp foundation. It deliberately does not migrate ADCS at the same time. EPS is the first vertical slice because it has a compact service surface and deterministic simulator state that can prove the pattern before duplicating it for ADCS.

## Design Decisions

### Preserve The F' Public Boundary

`EpsBridge` remains the F' owner of the `EPS_*` public contract. The command handlers, telemetry mapping, event behavior, cached state, autonomy-facing status provider, and classic component UT remain contract-compatible.

The default implementation behind `IEpsTransport` changes from direct ZMQ to libcsp. Tests may still inject fake `IEpsTransport` instances.

### EPS Owns Its CSP Service Payloads

The EPS CSP service layer owns the request/reply payload definitions used on application ports `10` through `13`. Ports `0` through `3` are reserved by libcsp for built-in management services and SHALL NOT be used for EPS application traffic:

- port `10`: status
- port `11`: PDU command
- port `12`: heater/config command
- port `13`: reset

The existing `StatusData` DTO may remain shared at the application/runtime level, but `simulators/common/protocol.h` must no longer be treated as the EPS on-wire authority.

### Hosted Simulator Becomes CSP Node 2

The hosted EPS simulator becomes libcsp node `2` and binds services on EPS ports `10` through `13`. It keeps `EpsSimModel` for battery, solar, heater, and PDU behavior.

The simulator connects to the same hosted ZMQHUB foundation used by OBC node `1`.

### ADCS Remains Temporary Legacy

ADCS remains on the legacy direct-ZMQ path in this change. That is an explicit temporary state on the integration branch, not the target architecture.

### Ground And External Paths Stay Separate

This change does not add a ground CSP tunnel and does not alter the GDS TCP adapter, `fprime-cli -> GDS`, or external comm/radio paths.

## Implementation Notes

### Transport

Add `CspEpsTransport` implementing `IEpsTransport`:

- initialize or reuse an EPS client runtime on OBC node `1`
- open libcsp connections to EPS node `2`
- send one EPS-owned request payload
- receive one EPS-owned reply payload
- map CSP, timeout, invalid response, and remote result failures into the existing `TransportStatus` enum

`makeDefaultEpsTransport()` returns `CspEpsTransport`.

### Simulator

Replace the active hosted EPS simulator server with a CSP service server:

- configure node id through `EPS_CSP_NODE_ID`, default `2`
- configure hub through `CSP_HUB_HOST`, `CSP_HUB_SUB_PORT`, and `CSP_HUB_PUB_PORT`
- accept CSP connections on the EPS service ports
- process one request at a time through `EpsSimModel`
- reply with a bounded EPS-owned reply payload

### Legacy ZMQ

The direct ZMQ EPS implementation can remain in the tree temporarily only if needed as a legacy support file for incremental migration. It must no longer be the default hosted EPS path and must not be reported as the active EPS architecture.

## Verification Strategy

- Keep existing `EpsBridge` classic F' UT with fake transport.
- Add `eps_csp_integration_test` proving:
  - OBC/client node `1` initializes on the hosted CSP foundation
  - EPS simulator node `2` serves ports `10` through `13`
  - `getStatus`, `setPdu`, `setHeater`, and `reset` work over libcsp
- Add or update hosted evidence to cite both:
  - foundation path from `internal-csp-foundation-v1`
  - new EPS business path proven by this change
- Keep ADCS legacy status explicit in evidence and specs.
