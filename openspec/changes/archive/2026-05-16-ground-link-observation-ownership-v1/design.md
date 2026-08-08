## Context

The current active COMM stack already has the intended high-level split:

- `GroundLinkDriver` owns backend configuration, worker lifecycle, byte send/receive, buffer ownership, and low-level counters
- `GroundLinkHealthProvider` converts driver/backend observations into per-band link-health views
- `CommController` consumes those views for availability, failover, authority role, and COMM FDIR policy

The remaining issue is type and policy ownership. `GroundLinkHealthRuntime.hpp` includes `simulators/comm/GroundLinkBackend.hpp` to see observation types, and the provider still treats generic node `4` specially by inspecting `cspTargetNode`. That makes simulator/backend-owned details part of the policy-facing contract.

## Goals / Non-Goals

**Goals:**

- make provider-facing ground-link observation types OBC-owned
- keep current symbol names where possible to minimize churn
- express active-vs-compatibility health semantics explicitly
- remove unused `GroundLinkDriver*` members from `CommController`
- preserve existing byte-stream behavior and active node `5` / node `6` policy behavior

**Non-Goals:**

- no `GroundLinkDriver` rename
- no full transport/probe interface split
- no RSSI/SNR placeholder implementation
- no MTU ceiling, ARQ, CFDP, reliable-transfer, or target-timing freeze
- no `TtcPassManager` changes

## Decisions

### Decision: Move observation ownership to the OBC component layer

Add `OBC/Components/GroundLinkDriver/GroundLinkObservationRuntime.hpp` and define these provider-facing runtime types there:

- `GroundLinkBackendMode`
- `GroundLinkHealthSemantics`
- `GroundLinkObservationState`

`simulators/comm/GroundLinkBackend.hpp` includes this header and implements the contract, but no longer owns the contract definitions.

### Decision: Keep observation state fact-only

`GroundLinkObservationState` contains only provider-needed facts:

- backend mode
- health semantics
- connected state
- TX/RX chunk totals
- TX/RX error totals
- successful status-observation total

`cspTargetNode` is removed from this contract. Node identity remains backend configuration detail.

### Decision: Node `4` is connected-only compatibility

Runtime/topology configuration assigns health semantics before the provider sees an observation, and the backend only
publishes its configured semantics:

- COMM CSP node `5` S-band: `ACTIVE_COMM_CSP`
- COMM CSP node `6` UHF: `ACTIVE_COMM_CSP`
- generic COMM CSP node `4`: `CONNECTED_ONLY_FALLBACK`
- other unsupported COMM CSP nodes: `DISABLED`
- direct TCP: `CONNECTED_ONLY_FALLBACK`
- disabled/no backend: `DISABLED`

The provider does not inspect CSP node constants. The backend also does not infer policy from node IDs at observation
time; it publishes the configured semantics supplied when the COMM CSP backend is installed. Active COMM CSP retains
`connected && activityAgeTicks <= 1` availability and error-growth reporting. Connected-only fallback reports
availability from connection state and does not produce stale or transport-growth faults from idle silence.

### Decision: Remove stale controller transport coupling

`CommController` no longer accepts or stores `GroundLinkDriver*` values. Its runtime dependencies stay limited to the health provider, recovery sink, command ingress authority, egress mux, and ingress authority configs.

## Risks / Mitigations

- **[Risk] Node `4` compatibility probes could appear to lose evidence value.** Mitigation: keep node `4` executable and documented as compatibility/target-lab evidence, but outside active mission health semantics.
- **[Risk] Provider behavior could regress for active node `5` or node `6`.** Mitigation: keep active COMM CSP semantics unchanged and add explicit backend/provider tests for node-specific semantics.
- **[Risk] Probe cleanup edits could expand scope.** Mitigation: only touch probe scripts if this change relies on a legacy-cleanup probe, and then verify rerun safety plus no owned-helper orphan or high-CPU residual process for that probe path.

## Open Questions

- none
