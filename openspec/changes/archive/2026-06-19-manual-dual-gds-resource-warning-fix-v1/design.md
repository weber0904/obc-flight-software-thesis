## Context

The current hosted manual dual-GDS repro shows two coupled defects in the
formal node-`5` resource-truth surface:

- `OBC/Runtime/HostedRuntime.cpp` reports `ru_maxrss`, so
  `SYS_MEM_RSS_MB` reflects process high-water RSS instead of current resident
  memory.
- `WatchdogSupervisor::updateResourceSample` emits
  `SYS_RESOURCE_DEGRADED` and `SYS_LOW_MEMORY` on every above-threshold sample,
  so one threshold crossing becomes a long warning stream.

Current docs and OpenSpec specs already classify these `SYS_*` surfaces as the
formal node-`5` resource keep-live truth. That means this is not just a local
probe annoyance; the baseline contract itself is wrong unless the semantics are
repaired.

## Goals / Non-Goals

**Goals:**

- Make hosted runtime RSS sampling report current resident memory on macOS and
  Linux.
- Emit resource warnings once per upward threshold crossing and allow a later
  re-crossing after the sampled value falls back below threshold.
- Keep the public runtime interface and threshold values stable for the first
  fix pass.
- Align formal specs and canonical docs with the corrected semantics.

**Non-Goals:**

- No redesign of secure-auth, command-gate, or manual dual-GDS topology.
- No target-specific auth-stability fix inside this change unless the resource
  fix itself regresses the target path.
- No threshold retuning before current-RSS measurement is corrected and
  reverified.

## Decisions

### Use current resident-memory sources instead of `ru_maxrss`

The hosted runtime will keep one `currentRssMb()` helper, but its platform
implementation changes:

- macOS: use `task_info` resident-size data from the current task.
- Linux: read current resident memory from `/proc/self/status` `VmRSS`.

This keeps the runtime interface unchanged while making `SYS_MEM_RSS_MB`
accurately describe live resident memory.

Alternative considered: keep `ru_maxrss` and merely raise the RSS threshold.
Rejected because the warning contract would remain semantically wrong and would
still scale with historical peaks rather than current operator-visible load.

### Add threshold-crossing latches inside `WatchdogSupervisor`

`WatchdogSupervisor` will track per-health-item above-threshold state in memory.
When a sample transitions from `<= threshold` to `> threshold`, it emits the
corresponding warning once. When the sample returns to `<= threshold`, the
latch clears and a later upward crossing may emit again.

The latch resets when:

- monitoring is disabled
- an operator changes a threshold

This keeps warning behavior deterministic for runtime configuration changes.

Alternative considered: deduplicate warnings by time window only. Rejected
because it still treats a steady above-threshold condition as repeated fresh
faults and would add extra policy without fixing the event semantics.

### Keep the first fix bounded to semantics, not policy retuning

The `256 MB` RSS threshold remains unchanged in this change. After current-RSS
sampling is fixed, hosted manual dual-GDS verification will decide whether the
threshold is still appropriate under normal operation.

This keeps the change decision-complete without mixing measurement repair and
policy retuning in one step.

## Risks / Trade-offs

- [Risk] Platform-specific RSS helpers can drift if the process API changes.
  Mitigation: keep the helper isolated, add unit coverage that verifies RSS can
  rise and fall around a mapped allocation, and keep a simple Linux parser.
- [Risk] Clearing latches on threshold change can emit a new warning on the
  next above-threshold sample.
  Mitigation: this is intentional and keeps warning semantics aligned with the
  newly configured threshold.
- [Risk] Hosted current RSS could still exceed `256 MB` under steady-state
  manual dual-GDS load.
  Mitigation: verification explicitly compares the corrected sampled value to
  OS-observed RSS before deciding whether a follow-on threshold change is
  necessary.
