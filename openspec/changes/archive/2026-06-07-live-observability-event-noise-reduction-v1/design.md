## Context

Three different noise sources are currently mixed into the same packetized
ground event surface:

1. reduced-state periodic heartbeat (`STATE_MONITOR_UPDATED`)
2. CSP link-health success confirmation (`CSP_PING_RESULT success=true`)
3. low-level data-product writer completion (`DpWriter.FileWritten`)

They differ in owner and meaning, so this change does not add one new generic
filtering layer in `CommEgressMux`. Instead, each source keeps its existing
owner and changes the smallest layer that matches the semantics:

- source-owned emission policy for `OnboardStateMonitor`
- source-owned emission policy for `CspBridge`
- `EventManager` default ID filtering for the debug-only `DpWriter` event

## Design

### Reduced-state update policy

`OnboardStateMonitor` keeps publishing telemetry and cached reduced-state data
on every successful reduction, but `STATE_MONITOR_UPDATED` only emits when the
current `(healthMask, faultMask, qualityMask)` tuple changes from the previous
successful reduced-state sample.

This intentionally does not synthesize a special first-valid or
source-recovered event when the three masks remain unchanged. The existing
quality mask already captures missing-data conditions, so this change treats
the event as a mask-transition signal, not a periodic liveness heartbeat.

### CSP ping event policy

`CspBridge` keeps running the same ping command/runtime path and keeps the same
telemetry counters. The only event-policy change is:

- do not emit `CSP_PING_RESULT` when `success == true`
- continue emitting `CSP_PING_RESULT(targetNode, false, timeoutMs)` on
  runtime-successful ping failures
- continue emitting existing error/transition signals for true failures or
  availability changes

This preserves failure visibility and transition semantics without turning
routine healthy probes into operator noise.

### DpWriter local-only visibility

`DpWriter.FileWritten` remains a valid event and still goes to text/journal
surfaces, but the default packetized event path filters it out at
`EventManager`.

To keep this as a checked-in baseline rather than an operator-issued runtime
command, `EventManager` gains a repo-configurable default filtered event ID
list that is loaded at startup. The default framework-safe behavior remains an
empty list. The repository populates that list with only
`OBCApp.dpWriter.FileWritten` for this change.

This keeps:

- packetized ground surface: quiet by default
- local journal/text logs: still reviewable for debug and probes

### Verification impact

Focused UT coverage is enough for the direct behavior changes:

- `OnboardStateMonitor` proves unchanged telemetry plus event emission only on
  mask changes
- `CspBridge` proves success no longer emits `CSP_PING_RESULT`
- `EventManager` proves default filtered IDs suppress packet emission at
  startup

Hosted/target smokes should then prove the end-to-end operator effect without
adding a new broad observability proof family.

## Risks And Mitigations

- Risk: some readiness/oracle path still expects broad `STATE_MONITOR_UPDATED`
  or success `CSP_PING_RESULT`.
  Mitigation: update those docs/oracles in the same change and keep failure
  semantics explicit.

- Risk: filtering `DpWriter.FileWritten` too broadly hides useful debug.
  Mitigation: filter only the packetized path; preserve local text/journal
  visibility.

- Risk: introducing a default filtered-ID config could accidentally change
  framework defaults for unrelated deployments.
  Mitigation: keep the framework default list empty and populate only this
  repository deployment.
