# csp-runtime-owner Specification

## Purpose
Define the formal baseline for the deployed `CspRuntimeOwner` component, which
owns CSP runtime lifecycle, deployed client serialization, and runtime
backpressure observability.

## Requirements
### Requirement: Deployed CSP Runtime Has One Explicit Owner
The deployed OBC topology SHALL provide one repo-owned `CspRuntimeOwner` component that is the intended owner of CSP runtime lifecycle and deployed client request serialization.

#### Scenario: Topology uses one owner-managed runtime
- **WHEN** the deployed `OBC` topology configures its CSP-backed COMM, EPS, and ADCS clients
- **THEN** those clients SHALL be wired to one topology-owned runtime owner rather than independently binding to an implicit shared global runtime

### Requirement: Owner Surfaces Reviewable Runtime Backpressure
`CspRuntimeOwner` SHALL publish reviewable telemetry or events for queue depth, inflight state, timeout accumulation, coalesced work, last latency, and last result so runtime contention is not hidden by the refactor.

#### Scenario: Runtime timeout remains observable
- **WHEN** an owner-mediated CSP request times out
- **THEN** the runtime owner SHALL surface a reviewable timeout event or telemetry transition identifying that the request exceeded its bounded runtime window
