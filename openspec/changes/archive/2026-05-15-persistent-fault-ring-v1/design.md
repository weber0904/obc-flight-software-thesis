## Context

The active baseline already has two distinct durability surfaces:

- official `.fdp` data products for periodic mission-history output
- `BootManager` file-backed metadata for boot/update and bounded recovery-reset
  truth

That still leaves one gap around reboot-equivalent recovery: the runtime has no
bounded store that can preserve the last boot and shared-recovery lifecycle
breadcrumbs across same-root relaunch without depending on `.fdp` generation,
catalog recovery, or detector-local duplicate writers.

The current runtime boundaries that matter for this change are:

- `BootManager` already reloads reset-cause, boot-count, consecutive-reset,
  last-recovery-source, and last-recovery-level truth from a file-backed
  metadata store under `persistent-data/boot/`.
- `RecoveryExecutor` already owns shared recovery normalization and lifecycle
  events for `EPS`, `ADCS`, `COMM`, and watchdog-related shared incidents.
- `HostedRuntime` already has reviewable shell surfaces for boot and recovery
  status, and both `TopCcsds` and legacy `Top` wire those same owners.
- The refreshed architecture narrative already treats official `.fdp` as the
  only active mission-history plane and HK fallback as retired, but
  `docs/architecture-review/current/` and follow-up notes still need
  reconciliation after completed 01/03 work.

This is a cross-cutting change because it introduces a new public owner,
touches two existing owners (`BootManager`, `RecoveryExecutor`), adds new
governed persistent storage, extends hosted runtime review surfaces, and needs a
new reusable hosted verification path.

## Goals / Non-Goals

**Goals:**

- Add a bounded persistent breadcrumb store dedicated to boot and shared-
  recovery lifecycle truth.
- Keep `PersistentFaultManager` as the only public owner for persistent-history
  readback.
- Make the store readable after same-runtime-root relaunch even if the newest
  snapshot copy is torn or corrupt.
- Reuse existing boot metadata and shared recovery truth instead of creating a
  second source of truth for reset or incident state.
- Keep active and legacy topology wiring aligned.
- Refresh stale architecture-review and roadmap follow-up narrative in the same
  change so 06 does not rely on outdated 01/03 text.

**Non-Goals:**

- Do not change `.fdp`, `OnboardStateSnapshotSource`, `StateSnapshot`,
  `HkTrendRecord`, or live beacon payloads.
- Do not add detector-local duplicate writers in `WatchdogSupervisor`,
  `EpsFdirController`, `AdcsFdirController`, or `CommController`.
- Do not claim target power-loss robustness, target reboot persistence, or
  filesystem-journal guarantees in v1.
- Do not add a manual write/capture operator command, file downlink interface,
  or data-product export for persistent fault history in this change.

## Decisions

### 1. Public owner is a passive component backed by a support library

Add a new passive F' component `PersistentFaultManager` under
`OBC/Components/PersistentFaultManager/` and keep the file-format logic in a
small `PersistentFaultStore` support library under the same component tree.

Why:

- The user-selected boundary requires a real public owner with dictionary,
  events, command, topology, and classic component-test coverage.
- The actual persistence logic is simpler as a plain library that can be tested
  directly without component scaffolding.

Alternatives considered:

- Helper/store only, no component: rejected because it would push public
  operator/readback ownership into unrelated runtime code.
- Reuse `BootManager` or `RecoveryExecutor` as the public owner: rejected
  because that would conflate boot truth or shared recovery ownership with the
  new persistent-history readback contract.

### 2. Writers use narrow runtime interfaces instead of new FPP input ports

Add a lightweight runtime header such as
`PersistentFaultRuntime.hpp` that defines:

- fixed-width record enums/structs shared by writers and reader
- a narrow writer interface for boot lifecycle writes
- a narrow writer interface for shared recovery lifecycle writes
- a bounded history snapshot/status structure for hosted runtime readback

`PersistentFaultManager` implements those runtime interfaces, and
`BootManager` / `RecoveryExecutor` receive configured pointers during topology
setup.

Why:

- This keeps the public owner as a real component while avoiding custom FPP port
  types and generated serialization machinery for a purely internal write path.
- The runtime interfaces keep `BootManager` and `RecoveryExecutor` decoupled
  from the component header and let unit tests inject a fake writer.

Alternatives considered:

- New input ports for every record type: rejected as unnecessary FPP/type churn
  for an internal synchronous write path.
- Directly including `PersistentFaultManager.hpp` in both writers: rejected
  because the coupling is wider than needed and makes testing harder.

### 3. Store format is a dual-copy whole-file ring snapshot with fixed-size records

Use `persistent-data/recovery/fault-ring-a.bin` and
`persistent-data/recovery/fault-ring-b.bin` as governed dual copies. Each file
stores:

- a fixed header: `magic`, `version`, `generation`, `capacity`, `count`,
  `nextIndex`, `activeRecordBytes`, `crc32`
- a fixed-capacity array of fixed-size records

Each record keeps only the bounded fields needed by the approved v1 scope:

- `kind`
- `timestampSec` as best-effort wallclock seconds, `0` when unavailable
- `uptimeSec`
- `bootCount`
- `consecutiveResetCount`
- `source`
- `level`
- `action`
- `resetCause`
- `epochOrRelatch`
- `flags` for small boolean state such as boot-safe-fallback required

Append model:

1. load newest valid snapshot
2. update the in-memory ring
3. write a full replacement snapshot to the inactive copy
4. compute/store file CRC in the header
5. next load selects the newest valid copy

Why:

- Whole-file snapshotting is easy to reason about, easy to verify in unit tests,
  and matches the user-selected duplicate-file robustness model.
- Fixed-size records keep offset math and newest-first iteration simple.
- The header-level CRC covers the complete stored snapshot and makes torn/corrupt
  newest-copy fallback explicit.

Alternatives considered:

- Single-file append with commit markers: rejected because the approved plan
  selected two ring files and fallback-to-older-copy behavior.
- In-place record patching: rejected because partial-write handling becomes more
  subtle and harder to review.
- Text or JSON records: rejected because the store is a governed runtime
  artifact, not a human-editable log.

### 4. Record scope is explicitly split between BootManager and RecoveryExecutor

`BootManager` writes:

- `BOOT_OBSERVED`
- `RECOVERY_BOOT_ACK`

`RecoveryExecutor` writes:

- `INCIDENT_OPENED`
- `ACTION_REQUESTED`
- `ACTION_EXECUTED`
- `REBOOT_PENDING`
- `REBOOT_ISSUED`
- `INCIDENT_CLEARED`

Why:

- This follows the accepted plan and keeps persistent history aligned with the
  current runtime owner model.
- `BootManager` already owns persisted reset truth and the stable-ack lifecycle.
- `RecoveryExecutor` already owns shared multi-subsystem recovery progression.

Alternatives considered:

- Let each detector write its own records: rejected because it duplicates
  shared-recovery truth and weakens the single recovery owner boundary.
- Add `.fdp` or state-summary writes in the same slice: rejected by scope.

### 5. Readback uses one command/event surface plus hosted shell rendering

`PersistentFaultManager.fpp` will expose:

- command `GET_PERSISTENT_FAULT_HISTORY(limit)`
- a header/status event naming total stored count, returned count, and active
  copy
- a per-record event that prints bounded latest-first record fields

For hosted runtime, `RuntimeServices` will gain a bounded history query method,
and `HostedRuntime` will add `fault history [count]` plus help text. The shell
prints the same latest-first history view without needing a GDS command loop.

Why:

- The command path satisfies the formal dictionary-visible requirement.
- The shell view keeps hosted evidence scripts deterministic and local.

Alternatives considered:

- Hosted shell triggers the command path indirectly: rejected because command
  events are less convenient to consume in the local shell and would complicate
  test/probe sequencing.
- File downlink or `.fdp` summary: explicitly out of scope for v1.

### 6. Verification is layered and adds one new hosted path only

Verification will be split into:

- L1 store/helper tests for serialization, wraparound, and dual-copy fallback
- classic F' L2 component harness for `PersistentFaultManager`
- focused BootManager and RecoveryExecutor test updates using fake writer hooks
- one repository-owned hosted probe that reuses the same-runtime-root relaunch
  idea from the earlier recovery probe but narrows the verdict to persistent
  ring behavior

The new hosted probe will:

1. start an isolated hosted runtime root
2. trigger a shared recovery path that issues reboot-equivalent exit
3. relaunch the same runtime root
4. query persistent history and verify both pre-reboot and post-relaunch
   breadcrumbs
5. corrupt the newer copy in a bounded way and verify fallback to the older
   valid copy

Why:

- The registry/evidence requirements need one explicit reusable path for this
  capability.
- Reusing the same-root relaunch pattern keeps the verdict honest without
  over-claiming target reboot persistence.

## Risks / Trade-offs

- [Whole-file snapshots are not target power-loss proof] → Keep the formal
  claim limited to hosted same-root relaunch persistence and dual-copy fallback.
- [Adding a new component plus runtime interfaces increases code surface] →
  Split store/helper logic from the public component and cover both layers with
  focused tests.
- [Boot and recovery writers could drift from the approved owner boundary] →
  Keep detector-local writers out of scope and wire only `BootManager` and
  `RecoveryExecutor`.
- [Hosted shell and command rendering could diverge] → Use one shared bounded
  history/status data structure from `PersistentFaultManager` as the source for
  both surfaces.
- [Stale architecture-review text could reintroduce wrong claims] → Refresh the
  review package and follow-up README in the same change rather than treating
  them as post-merge cleanup.

## Migration Plan

1. Add the OpenSpec delta specs and tasks for `persistent-fault-ring-v1`.
2. Implement `PersistentFaultStore`, runtime interfaces, and the passive
   `PersistentFaultManager` component with unit/component coverage.
3. Wire `BootManager` and `RecoveryExecutor` to the new runtime writer
   interfaces in both topologies.
4. Extend hosted runtime services and shell handling with `fault history`.
5. Add the focused hosted persistent-ring probe and record new verification
   evidence.
6. Refresh `docs/architecture-review/current/` and the relevant roadmap
   follow-up docs so completed 01/03 work and retired HK fallback claims are no
   longer stale.

Rollback / compatibility behavior:

- If no ring files exist, the store loads as empty.
- If one copy is corrupt, runtime falls back to the older valid copy.
- If both copies are invalid, runtime reports empty history and continues
  without blocking boot or shared recovery.
- There is no schema migration from an older persistent fault ring because v1
  introduces the store for the first time.

## Open Questions

None blocking at this stage. The remaining v1 defaults are already fixed by the
approved plan: latest-first readback, same-runtime-root relaunch scope only,
two governed ring files, no `.fdp` summary, and no detector-local duplicate
writers.
