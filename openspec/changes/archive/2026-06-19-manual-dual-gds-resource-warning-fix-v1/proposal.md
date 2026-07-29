## Why

The hosted manual dual-GDS secure-operations surface now emits repeated
`SYS_LOW_MEMORY` warnings during otherwise normal operation even when the live
`OBC` process RSS remains far below the configured `256 MB` threshold. The
current hosted runtime samples `ru_maxrss`, which is a historical high-water
mark rather than current resident memory, and `WatchdogSupervisor` re-emits the
same warning on every above-threshold sample.

This change is needed now because the project already treats
`WatchdogSupervisor` `SYS_*` surfaces as the formal node-`5` resource truth.
Leaving peak-RSS semantics and repeated warning spam in that surface makes the
manual dual-GDS operator path noisy and semantically wrong.

## What Changes

- Replace hosted runtime peak-RSS sampling with current resident-memory
  sampling on macOS and Linux while keeping the existing
  `RuntimeServices.updateResourceSample(cpuPct, rssMb)` interface unchanged.
- Change `WatchdogSupervisor` resource warnings from level-triggered spam to
  threshold-crossing warnings for both CPU and RSS.
- Preserve the existing `256 MB` RSS threshold for the first fix pass and
  remeasure manual dual-GDS behavior only after the sampling semantics are
  corrected.
- Update formal specs and canonical docs so `SYS_MEM_RSS_MB`,
  `SYS_RESOURCE_DEGRADED`, and `SYS_LOW_MEMORY` describe current-resource truth
  rather than historical peaks or repeated above-threshold chatter.
- Record hosted repro truth and the bounded target non-repro as the validation
  basis for this change without folding transient target auth instability into
  the scope.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: define `SYS_MEM_RSS_MB` as current resident memory
  and define `SYS_RESOURCE_DEGRADED` / `SYS_LOW_MEMORY` as threshold-crossing
  warnings.
- `verification-evidence`: require hosted manual dual-GDS verification to
  prove the corrected current-RSS truth and the absence of spurious
  low-memory-warning spam under the maintained baseline.

## Impact

- Affected code:
  - `OBC/Runtime/HostedRuntime.cpp`
  - `OBC/Runtime/HostedRuntime.hpp`
  - `OBC/Components/WatchdogSupervisor/WatchdogSupervisor.cpp`
  - `OBC/Components/WatchdogSupervisor/WatchdogSupervisor.hpp`
- Affected tests:
  - `OBC/Runtime/test/HostedRuntimeUnitTest.cpp`
  - `OBC/Components/WatchdogSupervisor/test/ut/*`
- Affected docs/specs:
  - delta specs for `core-system-contracts` and `verification-evidence`
  - `docs/interfaces.md`
  - `docs/operator/target-obc-comm-csp-lab-runbook.md`
- Non-goals:
  - no secure-auth redesign
  - no target auth-stability investigation beyond bounded regression checking
  - no threshold increase before corrected current-RSS measurement is verified
