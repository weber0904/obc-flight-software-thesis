## Context

The repository already has a governed Linux/Raspberry Pi storage model: runtime roots separate mutable state from release payloads, `BootManager` uses `persistent-data/` and `staging/`, housekeeping archive writes under `runtime-root/hk/`, and the installed-release flow keeps mutable runtime state outside versioned releases. However, the OBC still lacks a repository-owned storage health view. Operators and tests can infer storage state by inspecting the filesystem directly, but there is no dedicated subsystem that owns runtime-root observability, warning thresholds, or cached storage statistics in the same way EPS, ADCS, GPS, and comm status are already exposed.

The `resource-storage` capability already says the first version should define monitoring thresholds for staging and log capacity. The new storage health slice should make that requirement concrete without overreaching into deletion, cleanup, or policy-heavy retention work. That makes this a good fit for a first bounded component: scan governed runtime roots, maintain cached storage health state, publish `STORAGE_*` telemetry/events/commands, and feed that cached state into housekeeping archive.

## Goals / Non-Goals

**Goals:**

- introduce a dedicated `storage-health` capability for governed runtime-root observability
- scan the bounded first-version runtime roots:
  - `runtime-root/hk/`
  - `persistent-data/`
  - `staging/`
  - `runtime-root/logs/`
- expose first-version `STORAGE_*` command, telemetry, and event families
- maintain cached storage health state for runtime consumers and housekeeping archive
- support hosted validation with synthetic runtime-root contents and threshold-triggering scenarios
- register the hosted storage health validation path as a repository baseline distinct from target-side disk behavior

**Non-Goals:**

- automatic deletion, cleanup, or retention enforcement
- resizing or rewriting archive slot policy
- install-root release payload integrity checks
- OS-level free-space governance across arbitrary mount points
- Raspberry Pi target disk behavior claims in this change

## Decisions

### Decision: Introduce `StorageHealthBridge` as a dedicated observability component

Storage monitoring belongs in its own component rather than being folded into `HousekeepingArchive` or `BootManager`. `HousekeepingArchive` owns archive capture and file-downlink control; `BootManager` owns boot metadata and staging verification. `StorageHealthBridge` will own runtime-root scanning, warning thresholds, cached storage state, and `STORAGE_*` public contracts.

Alternative considered:

- add storage counters directly to `HousekeepingArchive`
  - rejected because archive capture should consume cached state, not also become the owner of a separate filesystem-health subsystem

### Decision: Keep the first slice bounded to four governed runtime roots

The first version will observe:

- `runtime-root/hk/`
- `persistent-data/`
- `staging/`
- `runtime-root/logs/`

This matches the repository’s existing runtime-root model and avoids mixing in unrelated host files or installed release payloads.

Alternative considered:

- monitor the entire runtime root recursively as one aggregated store
  - rejected because root-specific visibility is needed to reason about housekeeping, staging, persistent metadata, and logs separately

### Decision: Observe health only; defer destructive retention or cleanup

The first version should publish file count, total bytes, root existence, bounded warning status, and scan errors. It should not delete files or enforce cleanup policy. This keeps v1 focused on observability and leaves future retention work explicit.

Alternative considered:

- combine health monitoring with automatic cleanup in the same change
  - rejected because it would mix visibility with destructive policy and make review harder

### Decision: Reuse cached storage state in housekeeping archive

`StorageHealthBridge` will maintain the latest scan result. `HousekeepingSnapshotProvider` will read that cached state when archive capture occurs. Archive capture will not trigger another filesystem scan itself.

Alternative considered:

- have archive capture directly rescan the filesystem
  - rejected because it would duplicate responsibility and make capture behavior depend on archive timing

### Decision: Use a bounded warning-threshold model

The first slice will use bounded byte thresholds for the governed roots and surface threshold-exceeded events plus a cached warning flag. It does not need full policy configurability in v1; a small runtime-configurable threshold model is sufficient.

Alternative considered:

- introduce per-root command-configurable retention policy immediately
  - rejected because it increases complexity without improving first-version observability

## Risks / Trade-offs

- [Risk] Storage scan results may be mistaken for full disk-health coverage. → Mitigation: make the scope explicit in specs and evidence: only the governed runtime roots are covered.
- [Risk] A single threshold model may be too coarse for long-term use. → Mitigation: keep threshold behavior bounded in v1 and expand later if mission needs justify it.
- [Risk] Filesystem scans can become expensive if directories grow unexpectedly. → Mitigation: keep v1 bounded to a small set of governed roots and reuse cached state rather than rescanning from multiple paths.
- [Risk] Hosted validation may be mistaken for Raspberry Pi target disk evidence. → Mitigation: keep target-side storage behavior explicit as future scope or `Deferred-RPi` / `Blocked-HW`, whichever applies later.

## Migration Plan

1. Add the new `storage-health` capability and delta specs for resource storage, housekeeping archive, verification evidence, and verification-path registration.
2. Introduce the first storage scan support layer plus `StorageHealthBridge`.
3. Wire the component into runtime configuration, topology, and operator shell access.
4. Extend housekeeping snapshot runtime state to include cached storage health.
5. Add focused tests and a repository-owned hosted storage-health probe.
6. Record hosted evidence and register the validation path.

## Open Questions

- Should a later retention-focused change treat housekeeping archive, logs, and staging with separate threshold policies instead of one bounded warning model?
- When Raspberry Pi target storage evidence is added later, should it reuse the same `StorageHealthBridge` public contract unchanged or add target-only thresholds?
- Should future scheduler or mission-level work consume storage warning status directly, or should storage health remain purely operator-facing until a concrete autonomy need appears?
