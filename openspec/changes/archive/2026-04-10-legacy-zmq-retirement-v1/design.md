## Context

`feature/libcsp-internal-network-base` now has three completed slices: hosted libcsp foundation, EPS CSP vertical slice, and ADCS CSP vertical slice. The active runtime path is correct, but legacy direct-ZMQ transport code and a shared `simulators/common/protocol.h` still exist as active source. This change retires those remnants so future internal subsystem work cannot silently use the wrong substrate.

## Decisions

1. **Delete active direct-ZMQ EPS/ADCS code instead of deprecating it.**
   - `ZmqEpsTransport`, `ZmqAdcsTransport`, endpoint constructor compatibility, and direct ZMQ test source are removed.
   - Historical archived evidence remains unchanged because those records describe past validation.

2. **Keep libcsp ZMQHUB support.**
   - `csp_zmqproxy`, `CspRuntime`, and libcsp ZMQ interface linkage remain valid because they implement the official hosted CSP carrier.
   - The checker must distinguish `csp_zmqproxy` / `ZMQHUB` from forbidden EPS/ADCS direct REQ/REP glue.

3. **Split DTOs from wire protocol.**
   - EPS and ADCS keep runtime state/result types in subsystem-owned headers.
   - CSP request/reply envelopes stay in `EpsCspProtocol.hpp` and `AdcsCspProtocol.hpp`.
   - The old shared protocol header is deleted so it cannot be mistaken for the active wire authority.

4. **Preserve public F' behavior.**
   - EPS/ADCS command, telemetry, event, scheduler, cached-state, mission-autonomy, and housekeeping-facing behavior remains unchanged.
   - The only intended compatibility break is internal C++ endpoint-oriented construction/factory API removal.

## Risks / Mitigations

- **Risk:** Removing old message handlers breaks scenario replay tests that still use legacy `Message`.
  - **Mitigation:** Convert scenario bridge tests to use active CSP request/reply helpers or model-level access.
- **Risk:** Checker blocks legitimate ZMQHUB references.
  - **Mitigation:** Scope checker allowlist to libcsp runtime/proxy files, CMake target names, archived history, and evidence records.
- **Risk:** Bridge component tests depend on constructor signatures.
  - **Mitigation:** Update constructors and tests together while preserving transport injection through `setTransportForTest`.
