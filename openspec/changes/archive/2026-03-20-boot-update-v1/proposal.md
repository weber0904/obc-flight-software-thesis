## Why

The repository already has the platform baseline, core contracts, EPS/ADCS hosted bridges, and the first comm controller slice. What is still missing is the first concrete implementation of the A/B boot/update contract that the formal baseline already treats as a first-version requirement. Without that slice, the project still lacks the owned `BOOT_*` command path, the persistent metadata model, and executable evidence for confirm/rollback behavior.

## What Changes

- Add `BootManager` as the OBC-owned component for the `BOOT_*` command, telemetry, and event families.
- Implement file-backed boot metadata v1 under persistent storage using a structured key/value file.
- Verify staged images against the project hash backend, record pending slot activation, and support confirm or rollback behavior.
- Add focused F' unit tests covering prepare/verify/activate/confirm, timeout-driven rollback, and invalid-metadata recovery.
- Capture implementation and verification evidence for the boot/update slice.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `boot-update`: add the concrete `BootManager` behavior, file-backed metadata layout, and hosted confirm/rollback execution model for the first implementation slice.
- `verification-evidence`: add the evidence requirement for the boot/update implementation slice and its constrained Raspberry Pi gaps.

## Impact

- Affected code: `OBC/Components/BootManager/`, `OBC/Components/CMakeLists.txt`
- Affected docs: `openspec/changes/boot-update-v1/`, `evidence/records/`, `obc-dev-spec/06_boot_update.md`
- Dependencies: project `fprime-venv/`, F' normal build and UT build flows, host filesystem access for metadata and staged-image verification
