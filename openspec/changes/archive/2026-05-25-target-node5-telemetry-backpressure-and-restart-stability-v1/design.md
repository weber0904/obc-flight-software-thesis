## Context

Fresh evidence from `target-timing-empirical-ceiling-freeze-v1` shows two
distinct target-side blocker classes:

1. `/tmp/target-timing-empirical-ceiling-freeze-v1.CQqgAA`
   - control preflight passed
   - the first steady-state timing window passed with usable numeric evidence
   - representative payload plus COMM activity then produced sustained
     `ComCcsds.comQueue.QueueOverflow` on queue index `1`
   - target events continued to flow, but fresh telemetry channels did not
     keep up well enough to support the post-activity timing window
2. `/tmp/target-timing-empirical-ceiling-freeze-v1.tA5n2O`
   - control preflight passed
   - before the first timing window was established on the restarted target,
     the target journal showed:
     - `CSP ping node 5 success 0 timeout 500 ms`
     - `COMM_LINK_AVAILABILITY_CHANGED : Comm link SBAND available 0`
     - `RateGroupCycleSlip`

These two blockers are narrower than the original timing-closure gap. They now
deserve one bounded product-side closure change rather than further timing-only
probe tuning.

## Goals / Non-Goals

**Goals**

- Explain and remove sustained telemetry queue overflow or backpressure on the
  active service-managed node-`5` path under the declared representative
  workload.
- Explain and remove fresh restart-path transient node-`5` availability drops
  and early `RateGroupCycleSlip`, or reduce them to one narrow explicit
  residual blocker with evidence.
- Add the minimum repository-owned diagnostics, instrumentation, and product
  fixes needed to prove those blockers are gone.
- Re-run the existing timing-freeze proof once these blockers are cleared so
  timing evidence can resume from a clean baseline.

**Non-Goals**

- No generic scheduler redesign.
- No radio metrics or reliable-transfer work.
- No payload feature growth.
- No node-`4` rollback.
- No direct attempt to freeze final numeric timing ceilings in this change
  unless the blocker-removal confirmation naturally proves that no residual
  blocker remains.

## Design

### 1. Treat the Work as Two Product Blockers, Not One Probe Bug

The change keeps one PR, but separates the work into two bounded blocker
tracks:

1. telemetry backpressure closure
2. restart-path stability closure

The blocker-diagnostic probes and evidence must keep these tracks separate so
the repo does not collapse back into broad "timing capture is unstable" prose.

### 2. Telemetry Backpressure Closure

#### Known topology truth

On the active `TopCcsds` service-managed node-`5` path:

- `CdhCore.events.PktSend -> commEgressMux.packetIn[0]`
- `CdhCore.tlmSend.PktSend -> commEgressMux.packetIn[1]`
- `commEgressMux.sbandPacketOut[0] -> ComCcsds.comQueue EVENTS`
- `commEgressMux.sbandPacketOut[1] -> ComCcsds.comQueue TELEMETRY`
- `commEgressMux.sbandFileBufferOut -> ComCcsds.comQueue FILE`

`ComCcsds.comQueue.QueueOverflow` on queue index `1` therefore points at the
telemetry downlink queue, not the event queue.

Current configured queue depths on the active path are:

- events = `200`
- telemetry = `500`
- file = `100`

That means the current evidence does not justify a casual "the queue depth is
obviously too small" conclusion. The more honest interpretation is:

- telemetry production, queueing, and/or downlink draining are out of balance
  under the declared workload

#### Product closure approach

- First reuse existing truth surfaces to see whether the current target already
  exposes enough queue/drain state to classify the overflow.
- If existing truth is insufficient, add only the minimum bounded
  instrumentation needed to answer:
  - which queue is overflowing
  - whether the queue remains near saturation or only spikes briefly
  - whether downlink drain recovers after representative activity
- Only after the cause is narrow enough may this change adjust:
  - queue depths or priorities
  - downlink egress behavior
  - workload-triggered telemetry/backpressure policy

This change must not respond to overflow by blindly increasing queue constants
without explaining why the drain path could not keep up.

### 3. Fresh Restart Stability Closure

The second blocker track concerns fresh service restart behavior before timing
measurement even starts.

The closure logic must answer:

- why node-`5` CSP ping drops to false in some restart paths
- why `COMM_LINK_AVAILABILITY_CHANGED False` can appear during first-window
  establish time
- whether `RateGroupCycleSlip` is caused by startup churn, real rate-group
  overload, or an adjacent path blocking the group

The intended closure approach is:

- capture bounded restart-path truth around:
  - service restart
  - node-`5` ping cadence
  - availability transitions
  - slip timing
- apply only the minimum target-side fix needed to keep the restart path within
  the active baseline contract

The change must not widen into scheduler redesign. If startup sequencing or
availability thresholds need bounded adjustment, that is in scope; changing the
whole rate-group architecture is not.

### 4. Verification Strategy

The verification flow for this change stays product-first:

1. fresh local build and focused UT for changed code
2. fresh target package and install
3. control-path preflight to confirm the active node-`5` command path still
   works
4. blocker-focused target probes:
   - telemetry-backpressure diagnostic proof
   - restart-stability proof
5. one confirming rerun of
   `run_target_timing_empirical_ceiling_freeze_v1_probe.sh`

This confirming timing rerun is not the final timing closeout. It only proves
that the new product blockers no longer dominate the timing path.

## Risks / Trade-offs

- [Risk] The active node-`5` path may require new bounded observability before
  the overflow cause is obvious.
  - Mitigation: add only the smallest instrumentation surface needed for the
    blocker claim, and document it as target support for timing closure.
- [Risk] The restart-path issue could be highly intermittent.
  - Mitigation: keep the restart proof narrow and targeted instead of forcing
    every reproduction through the full timing workflow.
- [Risk] Product fixes may remove one blocker while exposing another adjacent
  transport bottleneck.
  - Mitigation: preserve blocker-oriented evidence and do not over-claim full
    timing closure in this change.
