## Why

The project now has a validated external comm stack over TCP mock, PTY-backed serial, and a governed Raspberry Pi to host hardware UART path. The next integration risk is no longer the byte-stream transport itself, but the fact that the current radio command/status framing is still coupled to the first hosted text protocol, which would force wider code churn once a real radio or vendor-specific framing is introduced.

## What Changes

- Introduce an explicit radio protocol adapter layer between the shared byte-stream transport and `RadioController`.
- Refactor the current hosted text request/response protocol into a named default adapter so the existing mock-radio behavior stays intact while future adapters can be added without controller rewrites.
- Add runtime selection for the radio protocol adapter kind while preserving the current default behavior for hosted and Raspberry Pi validation flows.
- Add focused regression tests and evidence that the named default adapter preserves the current controller-visible behavior and transport-sharing path.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `comm-subsystem`: add a formal radio protocol adapter contract so controller-layer comm behavior can survive future KISS or vendor-specific framing changes without redesigning the transport layer.
- `verification-evidence`: require reviewable evidence that the default protocol adapter preserves the current comm behavior and records the configured adapter selection used during validation.

## Impact

- Affected code: `simulators/comm/`, `OBC/Main.cpp`, runtime configuration helpers, and comm integration tests.
- Affected docs: `README.md`, `obc-dev-spec/05_comm_subsystem.md`, `obc-dev-spec/07_verification_evidence.md`, and `docs/test-records/`.
- Affected systems: hosted runtime, Raspberry Pi runtime selection, future real-radio integration path, and comm regression verification.
