# Boot Update v1 Evidence

## Scope

This record captures implementation and verification evidence for the `boot-update-v1` OpenSpec change.

## Environment

- Date: `2026-03-21`
- Workspace: `$REPO_ROOT`
- Framework baseline: `lib/fprime` pinned to `v4.1.0`
- Virtual environment: `$REPO_ROOT/fprime-venv`
- Host validation mode: file-backed metadata plus hosted confirm / rollback execution

## Implemented Artifacts

- `BootManager` component under `OBC/Components/BootManager/`
- File-backed metadata helper under `OBC/Components/BootManager/BootMetadataStore.*`
- Focused `BootManager` F' unit tests
- Updated narrative boot/update source document under `obc-dev-spec/06_boot_update.md`

## Commands Run

1. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util generate -f`
2. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util build`
3. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util generate --ut -f`
4. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util build --ut`
5. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util check --all`

## Results

- `fprime-util build` completed successfully for the normal build cache.
- `fprime-util build --ut` completed successfully and produced:
  - `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_BootManager_ut_exe`
- `fprime-util check --all` passed all registered tests after adding the boot/update slice.

## Verification Summary

- `OBC_Components_BootManager_ut_exe`: passed `3/3`
- Aggregate project UT gate: `100% tests passed, 0 tests failed out of 12`

## Notes

- The first hosted slice models `BOOT_ACTIVATE_STAGED_IMAGE` as an immediate transition into the pending slot so that `BOOT_CONFIRM` and timeout rollback can be exercised without a real reboot.
- Staged-file verification uses the active F' `Utils::Hash` backend. In this repository today that resolves to CRC32 rendered as lowercase hex, not SHA-256.
- The verify command is exercised with staging-area-relative paths because F' command strings are fixed-length objects; the implementation resolves those paths against the configured staging root.
- The normal build still emits the known `fatal: bad revision 'HEAD'` version-generation warning because the repository has not yet made its first commit. The build itself completed successfully.
- Darwin linker output still includes duplicate-library warnings for some F' static archives during UT linking; these warnings did not block the build or test execution.

## Remaining Gaps

- `Deferred-RPi`: real Raspberry Pi boot-chain ownership, reboot integration, and GDS-visible end-to-end observation are deferred until the target integration phase.
- `Blocked-HW`: physical SD-card A/B switching, power-loss recovery, and removable-media robustness are not covered by this hosted verification slice.
