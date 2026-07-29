## Why

The active baseline now has official `.fdp` mission-history products and
persisted boot metadata truth, but it still lacks a crash-tolerant breadcrumb
store for the last boot and shared-recovery lifecycle moments around a reboot.
The next governed step is to add a bounded persistent fault ring that stays
readable after same-root relaunch without redefining `.fdp` history, boot
metadata truth, or detector-local FDIR ownership.

## What Changes

- Add a new `PersistentFaultManager` public owner that exposes bounded
  latest-first persistent fault history readback through a dictionary-visible
  status command and hosted runtime shell surface.
- Add a governed `persistent-data/recovery/` store using dual whole-file ring
  snapshots (`fault-ring-a.bin` and `fault-ring-b.bin`) so the runtime can fall
  back to the older valid copy when a newer snapshot is torn or corrupt.
- Record only boot and shared-recovery lifecycle breadcrumbs in v1:
  `BootManager` writes boot-observed and reboot-ack truth, while
  `RecoveryExecutor` writes incident-open, action-requested, action-executed,
  reboot-pending, reboot-issued, and incident-cleared breadcrumbs.
- Keep official `.fdp`, `OnboardStateSnapshotSource`, `StateSnapshot`,
  `HkTrendRecord`, and live beacon payloads unchanged in this change.
- Refresh the current architecture-review and roadmap follow-up narrative so it
  no longer describes completed 01/03 work as pending or the retired HK ring as
  an active fallback path.

## Capabilities

### New Capabilities

- `persistent-fault-ring`: define the bounded persistent recovery-breadcrumb
  store, runtime owner, readback contract, storage integrity model, and
  first-version writer set.

### Modified Capabilities

- `boot-update`: require `BootManager` to write bounded boot and reboot-ack
  breadcrumbs without changing boot metadata truth or its existing public boot
  status contract.
- `mission-autonomy`: require `RecoveryExecutor` to write shared recovery
  lifecycle breadcrumbs while keeping detector-local ownership and bounded
  shared recovery progression unchanged.
- `resource-storage`: reserve governed `persistent-data/recovery/` ownership for
  the dual ring files while keeping persistent fault history distinct from boot
  metadata, staging, logs, and official `.fdp` products.
- `verification-evidence`: require reviewable evidence for persistent ring
  append, newest-first readback, dual-copy fallback, and same-runtime-root
  reboot-equivalent persistence.
- `verification-path-registry`: register the repository-owned hosted persistent
  fault ring relaunch path once it is proven.

## Impact

- Affected code:
  - new `OBC/Components/PersistentFaultManager/` component and support library
  - `BootManager`, `RecoveryExecutor`, hosted runtime shell, and both topology
    trees
- Affected storage/runtime surfaces:
  - add governed `persistent-data/recovery/fault-ring-{a,b}.bin`
  - add `GET_PERSISTENT_FAULT_HISTORY(limit)` and hosted `fault history [count]`
- Affected docs/evidence:
  - new OpenSpec change deltas, focused hosted probe, verification record and
    registry updates, plus refreshed `docs/architecture-review/current/` and
    roadmap follow-up narrative
- No new `.fdp` payloads, no new `OnboardState` summary fields, no detector-
  local duplicate writers, and no target power-loss robustness claim in v1
