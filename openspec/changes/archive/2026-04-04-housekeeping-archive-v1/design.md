## Context

The OBC runtime already maintains a first-version set of cached subsystem and platform state through the existing bridge and controller components. EPS and ADCS status are already polled on the fast rate group, mission autonomy consumes those cached values, and the comm stack already exposes a governed file-downlink path through the existing F' communication subtopology. What is missing is a flight-like housekeeping archive that preserves time history across communication gaps without introducing a second subsystem-poll loop or forcing housekeeping into the immutable data-product file model.

The user intent for the first housekeeping slice is a ring-buffer archive: fixed-size files, slot rotation when a file fills, wraparound overwrite when slot numbers are exhausted, and a reviewable mapping from slot number to time span. Ground operations should inspect that mapping first and then request specific archive files for downlink.

## Goals / Non-Goals

**Goals:**
- Add a first-version housekeeping archive component that snapshots cached/runtime state on a fixed cadence and on operator request.
- Persist housekeeping snapshots into fixed-count ring files with slot and generation metadata.
- Maintain a repository-owned index file that lists slot number, generation, start time, end time, record count, size, and active-file status.
- Reuse the existing `FileDownlink` path while keeping user-facing commands constrained to housekeeping concepts rather than arbitrary file paths.
- Keep subsystem polling single-sourced by reading cached/runtime values instead of issuing new transport requests from housekeeping code.

**Non-Goals:**
- Auto-trigger downlink during ground-pass windows.
- Use `DpWriter` / `DpCatalog` as the storage model for the first housekeeping archive slice.
- Add time-range selection commands on the spacecraft side; the first slice only downlinks the index or a selected slot.
- Add Raspberry Pi target validation in this slice.
- Redesign the existing telemetry or autonomy baselines.

## Decisions

### 1. Use a custom ring-archive format instead of `DpWriter` / `DpCatalog`

The first slice will store housekeeping data in repository-owned ring files plus a repository-owned index file. This matches the desired operational model of bounded rotating files with overwrite and explicit time-span lookup.

Alternatives considered:
- Use `DpWriter` so every capture becomes an immutable data-product file. Rejected because the desired model is append/rotate/overwrite rather than an ever-growing directory of immutable files.
- Use `DpCatalog` for archive management. Rejected because the official component is oriented around scanning immutable data-product directories and is explicitly still an early prototype.

### 2. Keep housekeeping capture downstream of existing cached/runtime state

`HousekeepingArchive` will not talk to subsystem transports directly. Instead, a small runtime snapshot provider will gather data from existing public runtime accessors on `ModeManager`, `EpsBridge`, `AdcsBridge`, `CommController`, `CspBridge`, `RadioController`, `UartDriver`, and `BootManager`.

Alternatives considered:
- Let `HousekeepingArchive` call subsystem transports directly. Rejected because it would duplicate polling, add extra traffic, and risk mismatched values versus autonomy.
- Add data-product generation logic inside each bridge. Rejected because it would scatter archive concerns across multiple subsystem components.

### 3. Use a binary archive file plus a CSV index

Each archive slot file will contain a small binary header and fixed-format binary records. The index will be stored as a CSV file so the ground can inspect it easily before deciding which slot to request.

Alternatives considered:
- Use JSON for the index. Rejected for v1 because CSV is simpler, smaller, and easier to inspect quickly in shell tooling.
- Use text records for each housekeeping sample. Rejected because binary fixed-size records make bounded-size rotation and test assertions easier.

### 4. Downlink commands stay domain-specific

The first slice will expose `HK_CAPTURE_NOW`, `HK_DOWNLINK_INDEX`, and `HK_DOWNLINK_SLOT(slot, generation)` rather than a command that accepts arbitrary on-board file paths. The component will resolve those requests to known housekeeping files internally and enqueue them on `FileDownlink`.

Alternatives considered:
- Expose a generic file path string for housekeeping download. Rejected because it widens the command surface unnecessarily and makes it easier to misuse file downlink outside the governed archive path.

### 5. Archive files live under the shared mutable runtime root

The first slice will place housekeeping files under a governed runtime-root child directory rather than inside an installed release payload. This keeps archive history across release changes and aligns with the existing mutable runtime-area model.

Alternatives considered:
- Store housekeeping files under release payload directories. Rejected because release switching would overwrite or orphan archive data.
- Store housekeeping files under the existing log directory. Rejected because housekeeping archives are operator data products, not just diagnostic logs.

## Risks / Trade-offs

- [Archive file format is repository-owned rather than stock F' DP format] → Mitigation: keep the v1 format small, documented, and covered by integration tests plus an index file the ground can inspect.
- [A restart may consume an additional generation and slot wrap faster than a resume-in-place design] → Mitigation: accept this v1 simplification, preserve prior files in the index, and leave resume-in-place optimization for a later change if needed.
- [Downlinking a slot by stale `(slot, generation)` could fail after wraparound] → Mitigation: validate the requested tuple against the current index and reject mismatches rather than sending the wrong file.
- [Fixed cadence and file-size defaults may not match later mission needs] → Mitigation: choose repository-owned defaults for v1 and keep future configurability out of scope for now.

## Migration Plan

1. Add the new housekeeping archive component and its runtime snapshot provider.
2. Wire the component into the hosted topology and existing `FileHandling` / `ComFprime` path.
3. Start storing archive data under the shared runtime root without modifying the existing boot/update storage layout.
4. Verify hosted capture, rotation, index generation, and commanded downlink before archiving the change.

## Open Questions

- None for this first slice; the v1 scope is intentionally narrowed to periodic archive capture, index generation, and controlled file downlink.
