# `target-hardware-watchdog-reset-proof-v1`

Status: accepted target evidence for a distinct Raspberry Pi hardware watchdog
board-reset path. The probe-owned quiet diagnostic path used here is not a
general operator baseline.

Current-note:

- current formal Chapter 5 rerun now preserves the maintained Route `3`
  target raw artifacts, including the watchdog `R6` wrapper root, under:
  [chapter5 route3/target canonical root](../chapter5-integrated-route-closure-v1/ARTIFACTS.json)
- this record remains the dedicated watchdog narrative and interpretation
  surface; the canonical Chapter 5 root is the first lookup point for the fresh
  2026-06-28 rerun artifacts
- the historical accepted proof predates the Chapter 5 secure-auth migration
- a fresh 2026-06-25 rerun on the auth-migrated wrapper repaired the installed
  target baseline by removing stale `99-disable-hw-watchdog.conf`
- current Route 3 watchdog closure is now requalified on the installed target
  baseline through the secure-auth command path, not through legacy
  `SESSION_OPEN` handling
- a fresh 2026-06-28 watchdog rerun at
  `/tmp/rpi-hw-watchdog-reset.y10yfx/` with wrapper
  `/private/tmp/chapter5-route3-target-r6.r5A1iw/` re-corrobated:
  - pre-trigger secure-auth bootstrap
  - pre-trigger `GET_HW_WATCHDOG_STATUS`
  - watchdog suppression reboot edge
  - post-reboot secure-auth re-bootstrap
  - post-reboot `GET_RESET_CAUSE`
  - post-reboot `GET_HW_WATCHDOG_STATUS`
  - post-reboot `GET_WATCHDOG_STATUS`
  - post-reboot `GET_PERSISTENT_FAULT_HISTORY`
  - persistent fault readback included `REBOOT_PENDING` and
    `RECOVERY_BOOT_ACK`
  - `RECOVERY_WATCHDOG / WATCHDOG_ADCS_FDIR / R6_OBC_REBOOT`

## Scope

This record captures the completed Raspberry Pi hardware watchdog reset proof on
the active `TopCcsds` target baseline.

## What Was Implemented

- target-only `LinuxWatchdogSink` wired from
  `WatchdogSupervisor.watchdogFeedOut` to governed `/dev/watchdog0` ownership
- target runtime flags and service env for:
  - `--hardware-watchdog disabled|linux-device`
  - `--hardware-watchdog-device`
  - `--hardware-watchdog-timeout-sec`
- target install flow for watchdog device access via:
  - `packaging/rpi/udev/90-obc-watchdog.rules`
  - `watchdog` group
  - `SupplementaryGroups=watchdog`
- watchdog-source `R6` split so hardware-watchdog-enabled target mode records
  reboot intent without immediately requesting runtime exit `32`
- bounded proof-only `SET_WATCHDOG_PROBE_SUPPRESSION`
- repo-owned target probes:
  - `scripts/run_rpi_target_hardware_watchdog_capability_probe.sh`
  - `scripts/run_rpi_target_hardware_watchdog_reset_probe.sh`

## Capability Gate Result

Command:

```bash
OBC_SSH_TARGET=operator@obc.local \
  bash scripts/run_rpi_target_hardware_watchdog_capability_probe.sh
```

Observed result:

- PASS
- device: `/dev/watchdog0`
- driver: `bcm2835-wdt`
- fixed timeout: `15` seconds
- `SETTIMEOUT`: unsupported
- `nowayout=0`
- open, repeated keepalive, close, and immediate reopen succeeded
- restarting `obc-comm-csp-stack.service` did not reboot the board

This proves lifecycle compatibility for the hardware watchdog device under the
active service model. It does **not** prove watchdog-caused board reset.

## Target Reset Proof

Commands:

```bash
bash scripts/run_rpi_target_hardware_watchdog_reset_probe.sh
OBC_SSH_TARGET=operator@obc.local \
  bash scripts/run_rpi_target_recovery_restart_probe.sh
```

Observed watchdog-reset result:

- PASS
- active service path: `obc-comm-csp-stack.service`
- watchdog source: `WATCHDOG_ADCS_FDIR`
- suppression first records `WATCHDOG_ADCS_FDIR` shared-recovery entry at
  `R2_RESTART_SOFTWARE_COMPONENT` with `PROCESS_RESTART`
- baseline PID changed from `4676` to `884`
- boot marker changed from
  `journal:18991600d90a4016907c3e5eefbd563e` to
  `journal:8700ceb5c0084f93aef23baf800b292c`
- `reset_cause=RECOVERY_WATCHDOG`
- `last_recovery_source=WATCHDOG_ADCS_FDIR`
- `last_recovery_level=R6_OBC_REBOOT`
- `boot_count=7253`
- `consecutive_reset_count=1`
- quiet-path service override was restored to normal mode before probe exit

Observed `R2` regression result:

- PASS
- active service path: `obc-comm-csp-stack.service`
- ADCS fault boundary: stop `subsystem-adcs-csp.service`
- baseline PID changed from `2038` to `2193`
- `NRestarts` increased from `1` to `2`
- `reset_cause=RECOVERY_ADCS_FDIR`
- `last_recovery_source=ADCS_POLL_TRANSPORT`
- `last_recovery_level=R2_RESTART_SOFTWARE_COMPONENT`

Historical revalidation on the `target-comm-node56-migration-v1` branch after
probe stabilization:

- PASS
- artifact root: `/tmp/rpi-hw-watchdog-reset.tKkkAD`
- the proof again observed:
  - pre-trigger current command-path bootstrap on the then-accepted path
  - pre-trigger `GET_HW_WATCHDOG_STATUS`
  - watchdog suppression causing an immediate reboot edge before all initial
    `R2` fragments were re-read locally
  - post-reboot command-path re-auth / readback
  - post-reboot `GET_RESET_CAUSE`
  - post-reboot `GET_HW_WATCHDOG_STATUS`
  - post-reboot `GET_WATCHDOG_STATUS`
  - post-reboot `GET_PERSISTENT_FAULT_HISTORY`
  - boot marker change plus `RECOVERY_WATCHDOG / WATCHDOG_ADCS_FDIR / R6_OBC_REBOOT`

The quiet-path proof uses:

- the same installed `obc-comm-csp-stack.service` target path as the normal
  operator baseline
- a probe-owned temporary `DIAGNOSTIC_QUIET_PACKET_EGRESS=1` systemd override
- journal-first acceptance for command-path bootstrap, watchdog status, the
  initial watchdog-source transition where observable, reboot confirmation,
  and post-reboot `R6_OBC_REBOOT` readback

Fresh secure-auth migration note:

- fresh 2026-06-28 auth-migrated reruns retained secure-auth command success
  for pre-trigger `GET_HW_WATCHDOG_STATUS`,
  `SET_WATCHDOG_PROBE_SUPPRESSION`, post-reboot `GET_RESET_CAUSE`,
  post-reboot `GET_HW_WATCHDOG_STATUS`, post-reboot `GET_WATCHDOG_STATUS`,
  and post-reboot `GET_PERSISTENT_FAULT_HISTORY`
- the same rerun freshly re-corrobated that persistent fault history contained
  both `REBOOT_PENDING` and `RECOVERY_BOOT_ACK` truth for
  `WATCHDOG_ADCS_FDIR / R6_OBC_REBOOT`
- the same rerun also retained that A-layer baseline repair removed the stale
  watchdog-disable drop-in before the proof and restored the installed target
  baseline to `HARDWARE_WATCHDOG=linux-device`
- fresh retained artifacts:
  - `/tmp/rpi-hw-watchdog-reset.y10yfx/`
  - `/private/tmp/chapter5-route3-target-r6.r5A1iw/`
- therefore this record may again be cited as current maintained Route 3
  watchdog closure on the installed baseline

This path is intentionally narrow:

- it proves the hardware watchdog board-reset path under a probe-owned quiet
  egress override
- it does **not** claim that bounded background-TM CCSDS serial stability is
  solved for the general COMM lab path
- it does **not** promote quiet egress into a normal operator/runtime mode

## Verdict

- Implemented:
  - governed target hardware-watchdog integration
  - capability-gate proof
  - target reset-probe script with quiet override and journal-first acceptance
  - accepted Raspberry Pi hardware watchdog board-reset proof
  - focused target `R2` restart regression rerun after quiet-path work

## Non-Claims

This attempt does **not** prove:

- generic Linux reboot equivalence
- bounded non-quiet CCSDS serial stability under background TM
- bootloader or partition handoff
- power-loss recovery
- external supervisor IC behavior
