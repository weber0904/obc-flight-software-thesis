## Context

`EPS` and `ADCS` simulators are already split into a CSP-facing server shell and a `*SimModel` business layer. `COMM` is different: `CommNodeServer` currently owns serial reads/writes, the uplink byte queue, status counters, and CSP request semantics directly. That makes it harder to add scenario-driven or fault-injection behavior without coupling those semantics to the hosted node process.

The previous change, `comm-csp-node-and-ground-gateway-v1`, proved the gateway-backed omitted-RF TT&C path through ports `30-32`. This change must preserve that proof boundary and avoid changing the wire contract.

## Goals / Non-Goals

**Goals:**

- introduce `CommSimModel` as the COMM simulator business layer
- keep `CommNodeServer` responsible for libcsp socket handling, serial I/O, and packet dispatch only
- preserve node `4`, service range `30-39`, and the existing request/reply layouts for services `30-32`
- add bounded uplink queue semantics with a default 4096-byte capacity and drop-new overflow behavior
- expose queue depth, overflow, link, backpressure, and error-injection state through C++ model APIs for tests and future simulator integration
- verify model behavior through direct unit tests and gateway path compatibility through the existing governed probe

**Non-Goals:**

- do not add RF behavior, radio control, file/downlink, or a custom GDS plugin
- do not add new CSP services or a runtime/operator debug control plane
- do not rename `CommNodeServer` or `comm_csp_node`
- do not claim a COMM SocketCAN or shared physical bus proof
- do not reclassify the direct GDS or controller-oriented external comm baselines

## Decisions

### Decision: Keep the CSP wire contract stable

`CommSimModel` will produce the same `ChunkReply` and `LinkStatusReply` structures already consumed by the OBC-side `CommCspGroundLinkBackend`. Queue depth, overflow counters, backpressure state, and injection state remain available through `CommSimStatus`, not through new wire fields.

Rationale:

- avoids invalidating the previous gateway-backed evidence
- avoids forcing OBC-side backend changes for simulator-only observability
- leaves the remaining COMM service range available for future explicitly scoped work

### Decision: Use a bounded drop-new uplink queue

The model owns a byte queue with default capacity `4096`. Serial ingress that would exceed capacity is rejected as a dropped chunk; existing queued bytes remain ordered and available to future `UPLINK_POLL` requests. Overflow increments internal dropped/overflow counters and contributes to legacy wire `rxErrors`.

Rationale:

- preserves already queued F' framed bytes rather than discarding older data mid-flow
- provides deterministic backpressure semantics without adding a new protocol surface
- gives future scenario/autonomy work a clear model state to inspect

### Decision: Keep error injection model-only

The model exposes methods to force link disconnected state and downlink backpressure/error behavior for unit tests and future hosted scenario work. `comm_csp_node` does not gain CLI flags or a debug CSP service in this change.

Rationale:

- keeps this slice focused on simulator foundation rather than operator/debug control
- avoids expanding the gateway-backed path proof into a new management protocol
- still makes the behavior testable without live serial hardware

## Risks / Trade-offs

- **[Risk] Refactoring the server can regress the gateway path** -> Mitigation: keep request/reply layouts unchanged and rerun `scripts/run_comm_csp_ground_gateway_probe.sh`.
- **[Risk] Queue/drop counters are not visible over `LINK_STATUS`** -> Mitigation: expose them through `CommSimStatus`; defer any wire-level status expansion to a later scoped change.
- **[Risk] Backpressure maps to the existing `IO_ERROR` result** -> Mitigation: document that v1 preserves the old result enum and only adds internal model semantics.

## Migration Plan

1. Add `CommSimModel` and direct unit coverage.
2. Refactor `CommNodeServer` to delegate COMM service semantics to the model while keeping serial I/O in the server shell.
3. Register the new model sources and tests in CMake.
4. Record focused evidence and archive the OpenSpec change after local verification.

Rollback strategy:

- the server can be switched back to the previous direct queue/counter implementation if model delegation breaks compatibility, because the wire contract and executable names remain unchanged.

## Open Questions

- none for this slice
