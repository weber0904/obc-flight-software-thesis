# Test Record: target-route3-pre-reboot-recovery-v1

## Verdict

- Result: `PASS`
- Date: 2026-06-28

## Scope

This record is the current Route `3` target pre-reboot evidence ledger for the
maintained service-managed node-`5` baseline.

Maintained target intent:

- ADCS first-fault `R3_RESET_SUBSYSTEM_INTERFACE`
- no historical ADCS `R2` OBC restart regression
- ADCS incident clear after subsystem recovery
- EPS first-fault `R3_RESET_SUBSYSTEM_INTERFACE`
- EPS first-fault `R5` `SAFE` fallback
- EPS incident clear after subsystem recovery

This record is intentionally separate from:

- historical ADCS `R2` target restart evidence in
  [evidence/records/target-recovery-closure-v1/README.md](../target-recovery-closure-v1/README.md)
- target hardware-watchdog board-reset evidence in
  [evidence/records/target-hardware-watchdog-reset-proof-v1/README.md](../target-hardware-watchdog-reset-proof-v1/README.md)

## Current-note

- current formal Chapter 5 rerun now preserves the maintained Route `3` target
  raw artifacts under:
  [chapter5 route3/target canonical root](../chapter5-integrated-route-closure-v1/ARTIFACTS.json)
- this record remains the dedicated narrative ledger, but current raw evidence
  lookup should start from that canonical root
- A 2026-06-28 debug cycle was invalidated as current Route `3` truth after a
  baseline audit found two non-product defects:
  - `subsystem-*` stack targets had been enabled and started on `obc.local`
  - repo-owned subsystem COMM defaults had drifted to `can0` instead of the
    maintained `subsystem.local:can1`
- After correcting that baseline, a fresh minimal node-`5` rerun succeeded:
  - command:
    `bash scripts/run_target_secure_auth_command_path_probe.sh`
  - result:
    `target-secure-auth-command-path: PASS`
  - artifact root:
    `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-command-path.qC4yu4/`
  - corroborated facts:
    - `sband-ground-secure-auth-established: PASS`
    - `challenge_source=wire-capture`
    - `confirmation_source=native-auth-status`
    - bounded `GET_RESET_CAUSE` readback `PASS`
- Therefore the earlier blocker is no longer described as a generic node-`5`
  auth-path failure.
- Fresh reruns on the corrected host-role/CAN baseline now close the maintained
  pre-reboot Route `3` target slice:
  - ADCS `R3` stage:
    `bash scripts/chapter5_routes/target/route3_adcs_r3_first_fault_and_clear.sh`
    - result:
      `PASS`
    - artifact root:
      `/private/tmp/chapter5-route3-target.cpCvdk/adcs/`
    - corroborated facts:
      - `no-r2-proof=... MainPID unchanged NRestarts=0`
      - ground-native event proof matched `RECOVERY_INCIDENT_OPENED`,
        `source ADCS_POLL_TRANSPORT`,
        `RECOVERY_ACTION_EXECUTED`,
        `SUBSYSTEM_INTERFACE_RESET`
      - bounded `GET_RECOVERY_STATUS` clear readback reached
        `activeCount 0 source NONE`
  - EPS `R3/R5` stage:
    `bash scripts/chapter5_routes/target/route3_eps_r3_safe_fallback.sh`
    - result:
      `PASS`
    - artifact root:
      `/private/tmp/chapter5-route3-target-eps.8jQWDy/`
    - current-note:
      the earlier `systemctl stop subsystem-eps-csp.service` trigger was
      superseded because it could push the same proof past legitimate
      `R3_RESET_SUBSYSTEM_INTERFACE + R5 SAFE_FALLBACK` into `R6`
    - current maintained trigger:
      EPS simulator runtime `drop-status` control on
      `/tmp/subsystem-eps-sim-control.sock`
    - corroborated facts:
      - ground-native event proof matched `RECOVERY_INCIDENT_OPENED`,
        `source EPS_TIMEOUT`,
        `RECOVERY_ACTION_EXECUTED`,
        `SAFE_FALLBACK`,
        `SYS_MODE_CHANGE ... SAFE`
      - bounded `GET_RECOVERY_STATUS` clear readback reached
        `activeCount 0 source NONE`
  - aggregate corroboration:
    `bash scripts/chapter5_routes/target/run_route3_target.sh`
    - result:
      `PASS`
    - artifact root:
      `/private/tmp/chapter5-route3-target.cpCvdk/`

Current canonical rerun note:

- the newer formal rerun on `2026-06-28` supersedes those direct tmp paths for
  current lookup and stores the aggregate plus nested stage roots under the
  Chapter 5 canonical artifact tree linked above

## Fresh Current Reruns

```bash
bash scripts/ensure_target_comm_lab_baseline.sh
bash scripts/ensure_ground_dual_gds_baseline.sh
bash scripts/run_target_secure_auth_command_path_probe.sh
bash scripts/chapter5_routes/target/route3_adcs_r3_first_fault_and_clear.sh
bash scripts/ensure_target_comm_lab_baseline.sh
bash scripts/ensure_ground_dual_gds_baseline.sh
bash scripts/chapter5_routes/target/route3_eps_r3_safe_fallback.sh
bash scripts/ensure_target_comm_lab_baseline.sh
bash scripts/ensure_ground_dual_gds_baseline.sh
bash scripts/chapter5_routes/target/route3_watchdog_reboot_and_postcheck.sh
```

## Status

- command-path sanity on corrected baseline: `PASS`
- ADCS `R3` target proof: `PASS`
- EPS `R3/R5` target proof: `PASS`
- Route `3` target aggregate: `PASS` when combined with
  [target-hardware-watchdog-reset-proof-v1](../target-hardware-watchdog-reset-proof-v1/README.md)
