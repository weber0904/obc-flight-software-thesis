# Comm Subsystem v1 Evidence

## Scope

This record captures implementation and verification evidence for the `comm-subsystem-v1` OpenSpec change.

## Environment

- Date: `2026-03-20`
- Workspace: `$REPO_ROOT`
- Framework baseline: `lib/fprime` pinned to `v4.1.0`
- Virtual environment: `$REPO_ROOT/fprime-venv`
- Host validation modes: loopback TCP mock and PTY-backed UART replacement

## Implemented Artifacts

- Shared byte-stream transport and hosted radio adapter under `simulators/comm/`
- `CommController` component under `OBC/Components/CommController/`
- `RadioController` component under `OBC/Components/RadioController/`
- `UartDriver` component under `OBC/Components/UartDriver/`
- Focused F' unit tests for the three comm components
- Host-side `comm_transport_integration_test`

## Commands Run

1. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util generate -f`
2. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util build`
3. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util generate --ut -f`
4. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util build --ut`
5. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util check --all`

## Results

- `fprime-util build` completed successfully for the normal build cache.
- `fprime-util build --ut` completed successfully and produced:
  - `build-fprime-automatic-native-ut/bin/Darwin/comm_transport_integration_test`
  - `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommController_ut_exe`
  - `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_RadioController_ut_exe`
  - `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_UartDriver_ut_exe`
- `fprime-util check --all` passed all registered tests without requiring an escalated rerun.

## Verification Summary

- `comm_transport_integration_test`: passed
- `OBC_Components_CommController_ut_exe`: passed `3/3`
- `OBC_Components_RadioController_ut_exe`: passed `4/4`
- `OBC_Components_UartDriver_ut_exe`: passed `3/3`
- Aggregate project UT gate: `100% tests passed, 0 tests failed out of 11`

## Notes

- The first comm slice keeps transport switching below the controller layer by sharing one byte-stream abstraction across TCP mock and PTY paths.
- The hosted radio backend uses a text request / response protocol strictly for v1 validation and does not declare KISS or any vendor framing mandatory.
- `UartDriver` telemetry is intentionally focused on connectivity and byte/error accounting; vendor-specific radio semantics stay in `RadioController`.
- The normal build still emits the known `fatal: bad revision 'HEAD'` version-generation warning because the repository has not yet made its first commit. The build itself completed successfully.
- Darwin linker output still includes duplicate-library warnings for some F' static archives during UT linking; these warnings did not block the build or test execution.

## Remaining Gaps

- `Deferred-RPi`: full deployment wiring to the external comm stack and GDS-visible end-to-end path remains deferred.
- `Blocked-HW`: real Raspberry Pi to development-host UART cabling is not exercised in this change.
- `Blocked-HW`: real radio GPIO, I2C, power sequencing, and vendor-specific protocol validation remain out of scope for v1 host verification.
