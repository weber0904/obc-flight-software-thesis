## Context

`TopCcsds` currently configures two separate helper surfaces from the same
runtime providers:

- `OnboardStateSnapshotSource -> OnboardStateMonitor / HkTrendProductProducer`
- `HousekeepingSnapshotProvider -> HousekeepingArchive`

The HK path also extends into active operator and transport behavior:

- dictionary-visible `housekeepingArchive.HK_CAPTURE_NOW`,
  `HK_DOWNLINK_INDEX`, and `HK_DOWNLINK_SLOT`
- shared COMM file/downlink arbitration through
  `CommDownlinkOwner::HOUSEKEEPING`
- `storage-health` observation of the `hk` runtime root and `hkIndexExists`
- hosted runtime `status` text that reports HK-root state

The repository decision for this change is not to preserve compatibility
shims. The HK fallback is retired completely, and official `.fdp` plus
`DpCatalog` becomes the only current history path.

## Goals / Non-Goals

**Goals**

- Keep `OnboardStateSnapshotSource` / `StateSnapshot` as the only active
  runtime state projection surface.
- Remove every active HK runtime, command, topology, storage-health, and
  downlink path from the current baseline.
- Preserve official `.fdp` product identity while moving the payload to V5.
- Make storage-health contract and masks truthful for the remaining four roots.
- Update current specs, probes, and docs so they no longer describe or depend
  on HK fallback behavior.

**Non-Goals**

- Do not rename `OnboardStateSnapshotSource`.
- Do not add a new canonical snapshot struct.
- Do not add persistent fault/event storage or reuse HK files for that purpose.
- Do not add a manual one-shot official `.fdp` capture command.
- Do not add recovery/fault summary fields to `StateSnapshot`.

## Decisions

### Decision: Canonical state remains `OnboardStateSnapshotSource` + `StateSnapshot`

The change will not introduce a second canonical struct or a common internal
builder that preserves two public snapshot surfaces. `StateSnapshot` remains the
single runtime source of truth for:

- `OnboardStateMonitor`
- `BeaconPublisher` via reduced state
- `HkTrendProductProducer`

Retiring HK is simpler and lower-risk than keeping a second public runtime
struct alive after its only consumer is removed.

### Decision: HK fallback is fully removed, not deprecated

`HousekeepingArchive`, `HousekeepingSnapshotProvider`, HK-root storage tracking,
`HK_*` commands, HK shared-downlink owner semantics, and HK runtime artifacts
are removed from both active and legacy topologies. No rejected/deprecated
command shim is kept.

Rationale:

- the user chose complete retirement, not compatibility preservation
- keeping dead runtime paths would leave false baseline surface area
- the future persistent fault ring has different storage, writer, and reader
  semantics and should remain a separate change

### Decision: Official `.fdp` capture remains cadence-driven only

The old `HK_CAPTURE_NOW` manual capture semantics are not replaced in this
change. Official `.fdp` evidence and operator workflows rely on the existing
scheduled cadence, followed by `DpCatalog.BUILD_CATALOG` and
`DpCatalog.START_XMIT_CATALOG`.

Rationale:

- avoids broadening 01 into a new control surface
- matches the current decision to keep this change focused on canonical state
  and HK retirement

### Decision: `HkTrendRecord` keeps product identity and moves to V5

The product record remains `HkTrendRecord` id `0`, but the payload type becomes
V5 and removes HK-root fields. This matches prior repo practice for official
`.fdp` evolution and avoids forking product identity.

### Decision: Storage-health contract is re-based to four roots

After HK retirement, the governed current roots are:

- `persistent-data`
- `staging`
- `logs`
- `data-products`

`StorageRootKind`, warning/degraded masks, cached state, telemetry, tests, and
runtime status are re-numbered around those four roots. The removed HK slot is
not kept as reserved.

## Implementation Outline

1. Create OpenSpec artifacts and delta specs for HK retirement, V5 data
   products, and four-root storage health.
2. Remove HK component/helper code and topology wiring from `TopCcsds` and
   legacy `Top`, then simplify `CommController` / `CommDownlinkScheduler` to a
   DP-only shared history path.
3. Update storage-health public types, scanner logic, telemetry, masks, and
   runtime status output to the four-root contract.
4. Move official `.fdp` schema and tests to V5, keeping `StateSnapshot` as the
   single source and removing HK-root fields from the record.
5. Rewrite or retire probes and current evidence that still expect `runtime/hk`,
   `hk/index.csv`, or `HK_*` commands.
6. Update current-truth docs and validation/evidence surfaces, then run full
   verification.

## Risks / Trade-offs

- Removing HK root fields changes public `.fdp` and storage-health contracts in
  one change. Mitigation: keep product identity stable, bump payload version,
  and update specs/tests/evidence together.
- COMM downlink arbitration becomes simpler, but UT expectations that previously
  covered HK-vs-DP interaction must be rewritten rather than deleted blindly.
- Hosted probes and docs currently reference `hk/index.csv` in multiple places;
  those must be updated as current-truth edits or the repo will stay internally
  inconsistent.
