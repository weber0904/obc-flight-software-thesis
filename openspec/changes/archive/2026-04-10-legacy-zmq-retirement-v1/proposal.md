## Why

EPS and ADCS hosted business traffic now both use the repository internal libcsp substrate, but active code still contains the old direct ZeroMQ request/reply transport, shared hosted wire protocol header, endpoint defaults, and stale ZMQ integration test source. Leaving those paths active makes future development ambiguous and risks reintroducing the architecture drift that this integration base is intended to correct.

## What Changes

- **BREAKING**: Remove EPS/ADCS internal constructor and factory compatibility with direct ZMQ endpoint strings.
- Remove active `ZmqEpsTransport` / `ZmqAdcsTransport` implementations and the stale ADCS ZMQ integration source.
- Split EPS and ADCS state/runtime DTOs into subsystem-owned headers; delete `simulators/common/protocol.h` as an active shared wire protocol.
- Keep EPS/ADCS CSP request/reply envelopes owned by `EpsCspProtocol.hpp` and `AdcsCspProtocol.hpp`.
- Add a repository checker that fails if active code reintroduces direct EPS/ADCS ZMQ REQ/REP assumptions while allowing libcsp ZMQHUB support.
- Update specs, narrative docs, verification matrix, registry, and evidence to say the active internal EPS/ADCS hosted baseline is libcsp over ZMQHUB with no direct-ZMQ fallback.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `platform-baseline`: Clarify that hosted ZMQ is only the libcsp ZMQHUB-backed carrier, not project-local direct REQ/REP.
- `eps-subsystem`: Remove direct-ZMQ compatibility from the active EPS hosted baseline.
- `adcs-subsystem`: Remove direct-ZMQ compatibility from the active ADCS hosted baseline.
- `verification-evidence`: Add evidence expectations for legacy direct-ZMQ retirement.
- `verification-path-registry`: Register the retirement guardrail without implying new hardware or ground-path validation.

## Impact

- Affected code: EPS/ADCS simulator models, transports, CSP protocol includes, bridge constructors, CMake test/source registration, and scenario replay test code.
- Affected scripts: shared verification gate plus a new legacy-ZMQ retirement checker.
- Affected docs/evidence: README, simulator README, formal specs, narrative specs, verification matrix/registry, reconciliation matrix after archive, and `evidence/records/legacy-zmq-retirement-v1/`.
- Unaffected paths: GDS ground path, external comm/radio/UART path, GPS path, boot/update, mission logic, housekeeping archive behavior, and real hardware bring-up.
