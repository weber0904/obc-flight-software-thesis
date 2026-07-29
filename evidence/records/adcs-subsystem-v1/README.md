# ADCS Subsystem v1 Evidence

## Scope

This record captures implementation and verification evidence for the `adcs-subsystem-v1` OpenSpec change.

## Environment

- Date: `2026-03-20`
- Workspace: `$REPO_ROOT`
- Framework baseline: `lib/fprime` pinned to `v4.1.0`
- Virtual environment: `$REPO_ROOT/fprime-venv`
- Host ZMQ library: Homebrew `zeromq` from `/opt/homebrew`

## Implemented Artifacts

- Shared hosted EPS/ADCS protocol definitions under `simulators/common/protocol.h`
- Hosted ADCS simulator model, ZMQ server, and client transport under `simulators/adcs/`
- `adcs_simulator` host executable
- `AdcsBridge` component under `OBC/Components/AdcsBridge/`
- Focused `AdcsBridge` F' unit tests
- Host-side `adcs_zmq_integration_test`

## Commands Run

1. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util generate -f`
2. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util build`
3. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util generate --ut -f`
4. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util build --ut`
5. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util check --all`

## Results

- `fprime-util build` completed successfully for the normal build cache.
- `fprime-util build --ut` completed successfully and produced:
  - `build-fprime-automatic-native-ut/bin/Darwin/adcs_zmq_integration_test`
  - `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_AdcsBridge_ut_exe`
- `fprime-util check --all` passed all registered tests without requiring an escalated rerun in this workspace state.

## Verification Summary

- `adcs_zmq_integration_test`: passed
- `OBC_Components_AdcsBridge_ut_exe`: passed `5/5`
- Aggregate project UT gate: `100% tests passed, 0 tests failed out of 7`

## Notes

- The hosted ADCS simulator uses deterministic convergence on each `STATE` poll so detumble and pointing acceptance stay reproducible in CI and local runs.
- `AdcsBridge` preserves the last valid state and emits `ADCS_SENSOR_FAULT` when `sensor_valid` is false instead of publishing invalid telemetry.
- The normal build still emits the known `fatal: bad revision 'HEAD'` version-generation warning because the repository has not yet made its first commit. The build itself completed successfully.
- Darwin linker output still includes duplicate-library warnings for some F' static archives during UT linking; these warnings did not block the build or test execution.

## Remaining Gaps

- `Deferred-RPi`: full deployment wiring and GDS observation are deferred until the target integration change.
- `Blocked-HW`: real ADCS hardware communication is not covered by this hosted simulator path.
- `Blocked-HW`: physical sensor calibration, actuator limits, and closed-loop hardware validation remain out of scope for v1 host verification.
