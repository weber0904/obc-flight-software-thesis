# active-probe-cleanup-hardening-v1 Evidence

Date: 2026-05-16.

Branch: `fix/active-probe-cleanup-hardening-v1`.

Base commit: `13c1a73c6a49709bbef001d2f2377be66e17fa57` at evidence capture time.

OpenSpec change: `active-probe-cleanup-hardening-v1`.

## Scope

This record closes the active-path probe and launcher process-cleanup hardening
work for the governed `OBC` / `TopCcsds` verification path:

- active Python probes now launch owned helper processes in dedicated process
  groups
- cleanup uses bounded `SIGTERM -> wait -> SIGKILL` at the process-group level
  instead of only terminating direct child PIDs
- active shell launchers now reap narrowly-scoped owned stale processes before
  startup and during trap-based cleanup
- governed hosted, shell-launcher, and Raspberry Pi reruns now self-heal after
  interruption without manual `kill` cleanup

This record does not claim generic workstation process hygiene, system-wide
orphan prevention, legacy-path cleanup, flight-runtime behavior changes, or
closure of unrelated CCSDS functional probe instability.

## Build And Verification

Commands run:

```text
python3 -m py_compile scripts/probe_process_utils.py
bash -n scripts/run_command_session_lifecycle_probe.sh
bash -n scripts/run_command_persistent_freshness_probe.sh
bash -n scripts/run_comm_ttc_file_downlink_probe.sh
bash -n scripts/run_active_probe_cleanup_hardening_probe.sh
bash scripts/run_command_session_lifecycle_probe.sh
bash scripts/run_command_persistent_freshness_probe.sh
OBC_SSH_TARGET=operator@<private-lab-host> bash scripts/run_active_probe_cleanup_hardening_probe.sh
bash scripts/run_comm_ttc_file_downlink_probe.sh
bash scripts/run_payload_ttc_mode_entry_ccsds_probe.sh
RPI_SSH_TARGET=operator@<private-lab-host> bash scripts/run_rpi_command_persistence_probe.sh
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-active-probe-cleanup-hardening-v1
openspec validate active-probe-cleanup-hardening-v1
openspec validate --specs
```

Result summary:

- `python3 -m py_compile scripts/probe_process_utils.py`: PASS.
- `bash -n ...`: PASS for the touched probe wrappers and cleanup-hardening probe.
- `bash scripts/run_command_session_lifecycle_probe.sh`: PASS.
- `bash scripts/run_command_persistent_freshness_probe.sh`: PASS.
- `OBC_SSH_TARGET=operator@<private-lab-host> bash scripts/run_active_probe_cleanup_hardening_probe.sh`: PASS.
- `bash scripts/run_comm_ttc_file_downlink_probe.sh`: functional FAIL after cleanup refactor bug fix; current failure is `TT&C prerequisite did not produce required OBC readback: EPS pdu=7, ADCS mode=POINTING`.
- `bash scripts/run_payload_ttc_mode_entry_ccsds_probe.sh`: functional FAIL; current failure is `expected SYS_MODE telemetry while checking final after MODE_SET sequence attempt 1`.
- `RPI_SSH_TARGET=operator@<private-lab-host> bash scripts/run_rpi_command_persistence_probe.sh`: functional FAIL; current failure is a timeout waiting for the initial `COMMAND_SESSION_OPENED` event in the local events log.
- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-active-probe-cleanup-hardening-v1`: PASS.
- `openspec validate active-probe-cleanup-hardening-v1`: PASS.
- `openspec validate --specs`: PASS.

Interpretation: cleanup-hardening proof is positive on the governed hosted,
shell-launcher, and interrupted Raspberry Pi rerun paths. The two CCSDS
mode/TT&C regressions were rerun as requested, but their remaining failures are
functional probe issues rather than owned-helper leak failures. The full
Raspberry Pi command-persistence probe also still shows a functional restart
path issue unrelated to orphan cleanup. One genuine cleanup-refactor bug was
found during this work (`require_env` used before definition in
`run_comm_ttc_file_downlink_probe.sh`) and fixed before final evidence capture.

## Cleanup-Hardening Probe

Command run:

```text
OBC_SSH_TARGET=operator@<private-lab-host> bash scripts/run_active_probe_cleanup_hardening_probe.sh
```

Result summary:

```text
active_probe_cleanup_hardening_probe: PASS
hosted-root=/tmp/active-probe-cleanup-hardening.OJouHW/hosted
shell-root=/tmp/active-probe-cleanup-hardening.OJouHW/shell
rpi-root=/tmp/active-probe-cleanup-hardening.OJouHW/rpi
hosted=interrupted active hosted lifecycle probe left no owned GDS helpers behind and reached ready state again on immediate rerun
shell=interrupted run_remote_csp_gds_stack left no owned GDS helpers behind and relaunched successfully
rpi=interrupted run_rpi_command_persistence_probe left no owned local GDS helpers behind and reached ready state again on immediate rerun of the same active remote path
scope=bounded rerun safety for governed active verification scripts only; no system-wide orphan prevention claim
```

Interpretation:

- hosted probe interruption no longer leaves owned `fprime-gds` helper
  processes behind
- `run_remote_csp_gds_stack.sh` now self-heals on immediate relaunch without
  manual cleanup
- the active Raspberry Pi command-persistence path now cleans up owned local
  helpers well enough to restart the same proof path immediately

## Hosted Regression Probes

Commands run:

```text
bash scripts/run_command_session_lifecycle_probe.sh
bash scripts/run_command_persistent_freshness_probe.sh
```

Result summary:

- `run_command_session_lifecycle_probe.sh`: PASS after helper refactor.
- `run_command_persistent_freshness_probe.sh`: PASS after helper refactor.

These regressions show that the new process-group ownership and stale-helper
reap logic did not break the hosted active command-path proofs that depend on
the same local runtime and GDS stack orchestration.

## Additional CCSDS Mode/TT&C Regression Reruns

Commands run:

```text
bash scripts/run_comm_ttc_file_downlink_probe.sh
bash scripts/run_payload_ttc_mode_entry_ccsds_probe.sh
```

Observed outcomes:

- `run_comm_ttc_file_downlink_probe.sh`: rerun reached the functional probe
  logic after a cleanup-refactor `NameError` was fixed, but still failed on
  TT&C prerequisite readback (`EPS pdu=7, ADCS mode=POINTING`).
- `run_payload_ttc_mode_entry_ccsds_probe.sh`: rerun failed because final
  `SYS_MODE` telemetry was not observed after the mode sequence.

Interpretation:

- these reruns did exercise the updated subprocess-management contract
- neither failure presented as stale owned helper leftovers, immediate rerun
  poisoning, or high-CPU orphan persistence
- the bounded cleanup-hardening claim therefore remains valid, while CCSDS
  functional stability stays explicitly out of scope for this change

## Full Raspberry Pi Command-Persistence Regression Rerun

Command run:

```text
RPI_SSH_TARGET=operator@<private-lab-host> bash scripts/run_rpi_command_persistence_probe.sh
```

Observed outcome:

- rerun failed waiting for the initial `COMMAND_SESSION_OPENED` event in the
  host-side events log
- the failure was not accompanied by owned local `fprime-gds` helper leftovers;
  the cleanup-hardening probe separately proved immediate rerun safety for the
  same active path when interrupted at the ready-state boundary

Interpretation:

- this change improves process cleanup and rerun safety for the active RPi proof
  path
- it does not claim closure of the underlying functional instability currently
  affecting the full target persistence scenario

## Pre-Verification High-CPU Sweep

Before the fresh `verification_ci` gate, the process table still contained an
older orphaned active-path runtime rooted at
`/tmp/active-probe-cleanup-hardening.GmQMwR/...`, including a `fprime-cli
events` process consuming `99.6%` CPU.

That stale runtime was outside the current evidence temp root and was removed
before running the final gate. Rechecking the process table after cleanup showed
no remaining repo-owned high-CPU Python or `fprime-gds` helpers at the top of
the host process list.

## Notes

- The cleanup helper intentionally matches only narrow ownership fragments such
  as the current runtime root, GDS ports, file-storage directory, or exact
  wrapper command path. This change does not use broad `pkill -f python` or
  `pkill -f fprime-gds` behavior.
- `packaging/rpi/launch/run_stack.sh` was hardened alongside the source
  launchers so the installed active path uses the same bounded cleanup model.
