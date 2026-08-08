## Context

The narrative ADCS baseline already fixes the public contract boundary: a hosted ADCS simulator operating as node `3`, `AdcsBridge` as the OBC-owned interface, mission constants for detumble and pointing acceptance, and `ADCS_*` commands/telemetry/events as the subsystem-owned public symbols. The codebase does not yet include any ADCS host protocol, simulator behavior, or F' bridge implementation.

This change extends the hosted subsystem pattern established by EPS into a more stateful control subsystem. It must validate mode transitions, convergence thresholds, and degraded-state handling while still remaining deterministic and host-testable.

## Goals / Non-Goals

**Goals:**

- Define a project-local ADCS protocol shared by the hosted simulator and OBC bridge.
- Implement a deterministic hosted ADCS simulator with quaternion, angular-rate, pointing-error, and sensor-validity state.
- Implement `AdcsBridge` with the required `ADCS_*` command, telemetry, and event families.
- Verify the bridge with focused F' unit tests and verify the real ZMQ client/server exchange with a host integration test.
- Record evidence that includes the remaining real-hardware gaps.

**Non-Goals:**

- Do not implement high-fidelity orbital dynamics, IGRF, or a real star tracker.
- Do not require a full deployment or GDS session in this change.
- Do not bind to a real CSP stack, CAN bus, or ADCS hardware.
- Do not solve hardware-in-the-loop calibration or closed-loop physical validation.

## Decisions

### Decision: Extend the shared hosted protocol header with ADCS frames

The project-local `simulators/common/protocol.h` file will carry both EPS and ADCS hosted protocol structures so the simulator and bridge continue to depend on one source of truth for fixed-size request/response payloads.

Alternative considered:

- Create a second unrelated protocol header for ADCS. Rejected because the narrative documents already treat the hosted simulator protocol definitions as a shared common concern.

### Decision: Model ADCS convergence deterministically per poll step

The hosted ADCS simulator will update angular-rate norm and pointing error deterministically each time state is queried, based on the selected control mode. This gives repeatable convergence behavior for detumble and pointing tests without pretending to be a high-fidelity flight dynamics model.

Alternative considered:

- Attempt a more physically realistic dynamics loop in v1. Rejected because it would add complexity without materially improving the first validation gate.

### Decision: Keep `AdcsBridge` transport-pluggable for unit tests

`AdcsBridge` will use a small transport interface and a ZMQ-backed implementation by default, while unit tests inject a fake transport to exercise command, event, and fallback behavior without socket timing.

Alternative considered:

- Run all bridge tests against the live simulator. Rejected because control-logic regressions and transport regressions would be harder to isolate.

### Decision: Treat invalid sensor payloads as a bridge-level degraded-state path

The protocol will explicitly carry a sensor-valid flag. When that flag is false, `AdcsBridge` will preserve the last valid state and emit `ADCS_SENSOR_FAULT` instead of publishing potentially invalid telemetry.

Alternative considered:

- Publish invalid samples and let downstream operators interpret them. Rejected because the formal narrative already requires last-valid-state preservation on invalid replies.

## Risks / Trade-offs

- **The hosted ADCS model is intentionally simplified** -> Keep convergence deterministic, document the simplifications, and leave high-fidelity dynamics for later changes.
- **Mission constants drive behavior but not yet from a deployment config store** -> Encode the first defaults in the bridge and simulator path now, and leave later configurability to future changes.
- **The host ZMQ test requires local socket bind permission in the desktop sandbox** -> Record the need for an escalated test run in the evidence so this does not look like a product bug.
- **Real calibration and hardware closure remain absent** -> Mark those gaps as `Blocked-HW` in the evidence.

## Migration Plan

1. Create the OpenSpec proposal, design, tasks, and delta specs for `adcs-subsystem-v1`.
2. Extend the shared hosted protocol and add the hosted ADCS transport/simulator implementation under `simulators/`.
3. Add the `AdcsBridge` component and its focused unit tests.
4. Add the host integration test for the ADCS simulator and client path.
5. Run the normal build, UT build, unit tests, and host integration test.
6. Record evidence under `evidence/records/adcs-subsystem-v1/`.
7. Validate and archive the change.

## Open Questions

None that block this first implementation slice. Full deployment/GDS wiring and real ADCS hardware integration remain intentionally deferred.
