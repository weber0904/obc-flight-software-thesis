## Why

The project now has stable cached subsystem state and first-version autonomy, but it still lacks an onboard housekeeping archive that can preserve reviewable time history across communication gaps. We need a first flight-like housekeeping path now so later ground-pass operations can request historical data by archive file or time range instead of relying only on live telemetry.

## What Changes

- Add a first-version `HousekeepingArchive` capability that periodically snapshots cached OBC runtime state into repository-owned ring files.
- Add a repository-owned archive index that records slot, generation, time span, record count, and active-file metadata for each housekeeping archive file.
- Add commanded housekeeping actions for immediate capture, index downlink, and controlled slot downlink without exposing arbitrary file-path requests as a user-facing OBC command surface.
- Keep subsystem polling single-sourced: housekeeping capture reads existing cached/runtime state and SHALL NOT introduce a second transport-poll loop for EPS or ADCS.
- Record hosted verification evidence for archive rotation, index generation, and commanded file downlink behavior.

## Capabilities

### New Capabilities
- `housekeeping-archive`: Persist periodic housekeeping snapshots into ring-buffer archive files with an index and controlled downlink entry points.

### Modified Capabilities
- `resource-storage`: Extend the runtime storage model to include governed housekeeping archive and index roots within the shared mutable runtime area.
- `verification-evidence`: Add reviewable evidence requirements for housekeeping archive capture, rotation, index inspection, and commanded downlink.

## Impact

- Affected code: new housekeeping archive component, OBC topology wiring, runtime file layout handling, and hosted integration tests.
- Affected systems: shared runtime storage, direct TCP/GDS hosted validation path, and future ground-pass workflows that will build on archive/index metadata.
- Dependencies: existing cached subsystem/runtime state accessors and the current file-downlink service path.
