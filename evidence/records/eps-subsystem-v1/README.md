# EPS Subsystem v1 Evidence

## Scope

This record captures implementation and verification evidence for the `eps-subsystem-v1` OpenSpec change.

## Environment

- Date: `2026-03-20`
- Workspace: `$REPO_ROOT`
- Framework baseline: `lib/fprime` pinned to `v4.1.0`
- Virtual environment: `$REPO_ROOT/fprime-venv`
- Host ZMQ library: Homebrew `zeromq` from `/opt/homebrew`

## Implemented Artifacts

- Shared hosted EPS protocol under `simulators/common/protocol.h`
- Hosted EPS simulator model, ZMQ server, and client transport under `simulators/eps/`
- `eps_simulator` host executable
- `EpsBridge` component under `OBC/Components/EpsBridge/`
- Focused `EpsBridge` F' unit tests
- Host-side `eps_zmq_integration_test`

## Commands Run

1. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util generate -f`
2. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util build`
3. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util generate --ut -f`
4. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util build --ut`
5. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util check --all`

## Results

- `fprime-util build` completed successfully for the normal build cache.
- `fprime-util build --ut` completed successfully and produced:
  - `build-fprime-automatic-native-ut/bin/Darwin/eps_zmq_integration_test`
  - `build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_EpsBridge_ut_exe`
- `fprime-util check --all` passed all registered tests after running with user-approved local-socket permission for the ZMQ bind step.

## Verification Summary

- `eps_zmq_integration_test`: passed
- `OBC_Components_EpsBridge_ut_exe`: passed `4/4`
- Aggregate project UT gate: `100% tests passed, 0 tests failed out of 5`

## Notes

- The host ZMQ integration test requires the process to bind a local ZeroMQ endpoint. In the Codex desktop sandbox this bind is denied, so the final `check --all` run was executed with an explicit user-approved escalation.
- The normal build still emits the known `fatal: bad revision 'HEAD'` version-generation warning because the repository has not yet made its first commit. The build itself completed successfully.
- Darwin linker output still includes duplicate-library warnings for some F' static archives during UT linking; these warnings did not block the build or test execution.

## Remaining Hardware Gaps

- `Blocked-HW`: real EPS hardware communication is not covered by this hosted simulator path.
- `Blocked-HW`: physical battery chemistry, real power conversion behavior, and thermal characterization remain out of scope for v1 host verification.
