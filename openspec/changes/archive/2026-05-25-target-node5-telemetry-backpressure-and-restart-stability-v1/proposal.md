## Why

`target-timing-empirical-ceiling-freeze-v1` already separated most timing-probe
false failures from real target behavior. The remaining blocker is no longer a
broad timing-closure unknown:

1. representative payload plus COMM activity can drive sustained
   `ComCcsds.comQueue.QueueOverflow` on the service-managed node-`5` target
   path, after which fresh telemetry channels stop arriving cleanly enough to
   support numeric timing closure
2. some fresh `obc-comm-csp-stack.service` restarts still show transient
   node-`5` availability loss and `RateGroupCycleSlip` before the first timing
   window is even established

These are product-side blockers for timing closure. Continuing to tune the
timing probe without resolving them would only accumulate untrustworthy
evidence.

## What Changes

- Add a single follow-up change that closes the two target-side prerequisites
  for service-managed node-`5` timing closure:
  - telemetry backpressure / queue-overflow behavior under the declared
    representative workload
  - fresh restart-path stability for node-`5` availability and rate-group slip
- Add repository-owned blocker-diagnostic evidence and the minimum product-side
  instrumentation or fixes required to explain and remove those blockers.
- Re-run the existing timing-freeze proof path only as a confirmation that
  these blockers no longer contaminate timing measurement.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- The active service-managed target node-`5` COMM/downlink path gains bounded
  blocker-oriented diagnostics and, if required, bounded product fixes for:
  - telemetry queue backpressure under representative payload plus COMM load
  - restart-path transient node-`5` availability drops and early
    `RateGroupCycleSlip`

## Impact

- Affected code and runtime truth:
  - `ComCcsds` / downlink queue behavior or its bounded observability
  - target service-managed node-`5` startup / availability path
  - blocker-focused target probes and evidence surfaces
- Affected docs:
  - `evidence/records/target-node5-telemetry-backpressure-and-restart-stability-v1/README.md`
  - `evidence/verification-path-registry.md`
  - `docs/roadmap/next-work.md`
  - current architecture or interfaces only if the active baseline truth or
    residual-blocker wording changes
- Intended non-claims remain explicit:
  - no scheduler redesign
  - no radio metrics or reliable-transfer expansion
  - no new payload capability line
  - no final flight-processor WCET closure in this change
