## Why

The active `TopCcsds` baseline already has hosted EPS polling over the libcsp service path and a narrow SoC-only `ModeSafetyController`, but it still has no runtime closure for repeated EPS poll failure. Today a failed EPS poll only invalidates the cached status and emits an EPS comm-error event. That is not yet bounded subsystem-timeout FDIR because it has no retry threshold, no latched degraded/fault state, no deterministic escalation, and no explicit recovery evidence.

This change closes that next vertical gap with an EPS-only slice. It adds a focused FDIR owner for repeated EPS poll failure without broadening `ModeSafetyController`, without pushing FDIR into `HealthMonitor`, and without reviving the retired provisional `MissionExecutive`.

## What Changes

- Add a new focused `EpsFdirController` component that consumes EPS runtime poll-health state and decides retry, fault latch, escalation, and recovery.
- Extend `EpsBridge` with an EPS runtime health contract that reports cache validity, last poll success, consecutive poll failures, and cumulative poll comm errors.
- Treat consecutive EPS poll failures as the only timeout input in v1; do not add timestamp freshness or mixed-policy thresholds.
- Tolerate failures `1` and `2` as retry-only state; latch an EPS fault and escalate on failure `3`.
- Escalate exactly once per latched fault, using the existing internal mode-control path to request `SAFE` only when the current mode is `IDLE`, `PAYLOAD`, or `TTC`.
- Keep `SAFE` and `HELL` as fault-record-only cases with no duplicate mode request.
- Clear the latched fault and emit recovery evidence on the first successful EPS poll after the fault.
- Add focused component, helper, integration, and hosted-probe evidence for the deterministic failure, escalation, and recovery order.
- Update the verification-path registry and canonical roadmap/architecture layers to reflect that the active baseline now includes a focused EPS timeout FDIR slice.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `eps-subsystem`: extend the owned EPS runtime behavior contract with deterministic poll-health reporting and recovery semantics alongside the existing status/cache behavior.
- `mission-autonomy`: add a focused EPS timeout FDIR owner that can request `SAFE` through the normal mode-control path without broadening `ModeSafetyController`.
- `verification-evidence`: require reviewable evidence for EPS retry thresholding, one-shot escalation, recovery clear behavior, and hosted probe outcomes.

## Impact

- Affected public/runtime surfaces: new `EpsFdirController` events/telemetry, new EPS runtime-health provider interface, and one new internal mode-apply source for subsystem-fault fallback.
- Affected implementation areas: `OBC/Components/EpsBridge`, new `OBC/Components/EpsFdirController`, `ModeManager` source enum/runtime apply path, both topologies, focused tests, hosted probe, OpenSpec artifacts, verification registry/evidence, and roadmap/architecture truth.
- No broad all-subsystem FDIR framework, no EPS reset or power-cycle recovery, no watchdog behavior, no CCSDS-path proof expansion, no command auth/session/QoS work, and no second subsystem timeout slice.
