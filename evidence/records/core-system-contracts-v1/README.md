# Core System Contracts v1 Evidence

## Scope

This record captures implementation and verification evidence for the `core-system-contracts-v1` OpenSpec change.

## Environment

- Date: `2026-03-20`
- Workspace: `$REPO_ROOT`
- Framework baseline: `lib/fprime` pinned to `v4.1.0`
- Virtual environment: `$REPO_ROOT/fprime-venv`

## Implemented Artifacts

- Shared enums and constants under `OBC/Types/CoreTypes.fpp`
- `ModeManager` under `OBC/Components/ModeManager/`
- `HealthMonitor` under `OBC/Components/HealthMonitor/`
- `CspBridge` under `OBC/Components/CspBridge/`
- Focused unit tests for all three components under each component's `test/ut/` directory

## Commands Run

1. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util generate -f`
2. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util build`
3. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util generate --ut -f`
4. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util build --ut`
5. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util check --all`

## Results

- `fprime-util build` completed successfully for the non-UT build cache.
- `fprime-util build --ut` completed successfully and produced:
  - `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_ModeManager_ut_exe`
  - `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_HealthMonitor_ut_exe`
  - `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CspBridge_ut_exe`
- `fprime-util check --all` passed all registered project unit tests.

## Unit Test Summary

- `OBC_Components_ModeManager_ut_exe`: passed `2/2`
- `OBC_Components_HealthMonitor_ut_exe`: passed `2/2`
- `OBC_Components_CspBridge_ut_exe`: passed `2/2`
- Aggregate result: `100% tests passed, 0 tests failed out of 3`

## Notes

- The UT expectations were aligned with F' telemetry semantics for channels declared `update on change`; clearing test history does not reset each channel's last-emitted value.
- On Darwin, the UT link step prints duplicate-library warnings for several static archives. The executables still linked successfully and all tests passed.
