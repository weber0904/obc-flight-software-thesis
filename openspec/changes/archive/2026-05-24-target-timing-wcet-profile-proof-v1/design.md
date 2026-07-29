## Context

The repository already has enough static truth to stop treating target timing
as an undifferentiated unknown. The active `TopCcsds` topology fixes the
rate-group membership and the divider set, while the active Raspberry Pi launch
surface currently inherits the runtime default base tick because the installed
service launch path does not pass `--tick-ms`. What is still missing is a
governed target evidence path that measures the active service-managed baseline
under a declared workload and records what is proven versus what remains open.

At the same time, several current narrative layers still describe
`payload-ops-contract-v1` as next work even though payload v2, helper-backed
target payload, internal payload CSP shim, and the formal COMM matrix closure
are already archived. This change intentionally combines the doc sync with the
target timing proof so the current baseline reads truthfully in one reviewable
slice.

## Goals / Non-Goals

**Goals**

- Keep one formal change that updates both current narrative truth and the
  target timing/WCET evidence surface.
- Use the current Raspberry Pi `obc-comm-csp-stack.service` path as the
  canonical target baseline.
- Freeze the currently provable timing facts:
  - deployed base tick
  - rate-group divisors and nominal fast/slow/data rates
  - governed missed-tick policy based on upstream `Svc::ActiveRateGroup`
    cycle-slip semantics
- Capture reviewable evidence for two workload windows:
  - service-managed steady state
  - service-managed steady state plus bounded representative COMM/payload
    activity
- Keep any remaining timing gap explicit with its required measurement method
  rather than leaving broad `TBD` wording.

**Non-Goals**

- Do not redesign the scheduler, rate-group topology, or mission-autonomy
  model.
- Do not widen scope into radio metrics, reliable transfer, secure-boot
  redesign, generic mission scheduling, or new payload capability lines.
- Do not reopen legacy Top, stock `ComFprime`, or target node `4` rollback
  directions.
- Do not over-claim Raspberry Pi evidence as final flight-processor timing
  closure.

## Design

### Canonical Target Timing Baseline

The active canonical baseline for this change is the installed
`obc-comm-csp-stack.service` Raspberry Pi runtime, not the Pi-local direct
`OBC -> GDS` adapter path. The direct target path remains useful adjacent
evidence, but it is not the baseline being frozen here.

The timing contract will be derived from:

- rate-group membership and divisors in `TopCcsds`
- the installed service launch surface and packaged runtime defaults
- upstream `Svc::RateGroupDriver` and `Svc::ActiveRateGroup` semantics for tick
  division, ordering, and cycle-slip behavior

### Measurement Strategy

The new probe will use governed ground/GDS ingress against the active
service-managed target path and record two workload windows:

1. `steady-state`
2. `steady-state + representative activity`

The representative activity window is intentionally bounded:

- a short COMM command/event/channel burst on the current governed path
- one payload session:
  `MODE_SET PAYLOAD -> PAYLOAD_PREPARE_SESSION(AUTO) -> PAYLOAD_CAPTURE_AUTO ->
  PAYLOAD_GET_LAST_CAPTURE_METADATA -> PAYLOAD_SHUTDOWN`

The probe will gather:

- `OBCApp.rateGroup{1,2,3}Comp.RgMaxTime` snapshots as the upstream-owned
  per-group max execution-time signal
- absence of `OBCApp.rateGroup{1,2,3}Comp.RateGroupCycleSlip` events as the
  positive zero-slip verdict for the observation windows
- per-group timestamped telemetry streams to estimate inter-arrival timing and
  jitter:
  - fast: `OBCApp.commController.COMM_S_BAND_ACTIVITY_AGE_TICKS`
  - slow: `OBCApp.storageHealthBridge.STORAGE_SCAN_COUNT`
  - data: `OBCApp.systemResources.CPU`

The evidence will freeze only what the declared windows prove. Any stronger
claim, such as final flight-processor WCET, hard real-time interrupt latency,
or arbitrary payload/COMM saturation behavior, remains explicitly out of
scope.

### Missed-Tick Policy

This change does not invent a repository-local missed-tick policy. It adopts
the upstream `Svc::ActiveRateGroup` behavior:

- a cycle slip occurs when a new `CycleIn` message arrives before the previous
  cycle completes
- the proof window is passing only when no `RateGroupCycleSlip` warning event
  is observed for any active rate group

The documentation and evidence will call this a bounded service-managed
zero-slip claim for the declared workload windows, not a universal proof for
all target loads.

### Packaging Adjustment For Representative Payload Activity

The representative payload workload must run on the same service-managed target
baseline being timed. Today the Raspberry Pi release bundle includes `bin/OBC`
but not the sibling `payload_camera_backend_helper`, even though
`PayloadBackendHelperClient` resolves that helper from the current executable
directory by default.

This change therefore adds the helper to the packaged release surface and the
manifested file set. No payload API or helper protocol redesign is intended.

### Documentation Sync

Current-facing docs will be updated only where this change affects active
baseline truth:

- `current-baseline.md`, `next-work.md`, and the follow-up disposition notes
  will stop describing `payload-ops-contract-v1` as deferred.
- current architecture and target-flight-design narratives will reflect the
  payload helper-backed convergence, the COMM matrix closure, and the new
  bounded target timing proof boundary.
- `docs/interfaces.md` will promote proven target timing facts and list any
  remaining residual timing gaps explicitly.

## Risks / Trade-offs

- [Risk] Raspberry Pi timing measurements could fluctuate between reruns.
  - Mitigation: freeze only current service-managed ceilings for declared
    workload windows, and record the exact workload plus observation method.
- [Risk] Representative payload activity may fail on the installed target path
  if the helper is missing from the release surface.
  - Mitigation: package the helper into the release and verify it on the same
    service-managed path.
- [Risk] Reviewers could misread this as final flight timing closure.
  - Mitigation: keep the residual non-claims explicit in docs, evidence, and
    the registry entry.

## Migration Plan

1. Create the OpenSpec artifacts and timing-related spec deltas.
2. Add the helper to the Raspberry Pi packaged release surface.
3. Add the repository-owned target timing/WCET probe and supporting parsing.
4. Run fresh build/install plus the service-managed target probe to collect
   evidence-backed timing results.
5. Update current docs, the verification registry, and the timing evidence
   record using the measured result set.
6. Validate, sync, and archive the change through the normal closeout flow.
