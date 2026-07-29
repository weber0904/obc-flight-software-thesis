## Why

The Raspberry Pi target workflow now builds and runs the governed workspace successfully, but the synced target workspace intentionally excludes `.git`. As a result, the generated F' version metadata falls back to `v3.5.0`, which makes target-side events and evidence misleading even though the actual framework baseline is `v4.1.0` and the project revision is known on the host.

## What Changes

- Add a repo-local version metadata override path so the project can generate correct framework and project version strings without requiring `.git` inside the Raspberry Pi workspace.
- Feed host-derived version metadata into the governed Raspberry Pi bootstrap flow instead of relying on the framework fallback constant when the target build is running from a synced workspace.
- Record target-side evidence showing that the generated version metadata on Raspberry Pi matches the intended framework and project versions.
- Update the operator-facing documentation to explain how target version metadata is produced and why it differs from a direct in-repo `git describe` lookup on the target.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `platform-baseline`: the governed `integ-rpi` profile must preserve correct framework and project version metadata even when the synced target workspace omits `.git`.
- `verification-evidence`: Raspberry Pi target evidence must show the observed framework and project version metadata after the governed target build path runs.

## Impact

- Affected code: `CMakeLists.txt`, repo-local CMake target overrides, and `scripts/bootstrap_rpi_workspace.sh`.
- Affected docs: `README.md`, `docs/README.md`, `docs/test-records/`, and the relevant narrative source documents.
- Affected systems: local host git workspace, synced Raspberry Pi workspace, and target-side version events / generated metadata files.
