# target-comm-node56-migration-v1

Current-note:

- this record captured the topology migration and bounded node-`5` / quiet
  node-`6` route facts before the Chapter 5 secure-auth relabeling work
- any `SESSION_OPEN` wording below is retained as historical auth-boundary
  evidence only and must not be cited as the current maintained auth truth
- current maintained auth truth on the same target surfaces now comes from the
  secure-auth family in
  [evidence/records/target-recovery-closure-v1/README.md](../target-recovery-closure-v1/README.md),
  [evidence/records/target-hardware-watchdog-reset-proof-v1/README.md](../target-hardware-watchdog-reset-proof-v1/README.md),
  and registry entries `70` / `70A`

## Summary

This change retires active target/lab use of generic COMM node `4` and moves the governed target baseline to:

- default target S-band path through `subsystem.local` `sband_comm_csp_node` node `5`
- bounded quiet UHF path through `subsystem.local` `uhf_comm_csp_node` node `6`

Target/lab `COMM_PRIMARY_UNAVAILABLE` is re-scoped to repeated internal CSP no-response from the current primary COMM subsystem stand-in. It is no longer driven by whether a probe-owned `ground_ttc_gateway` instance remains attached.

## Governing Runtime Truth

- `TARGET_COMM_PROFILE=sband`
  - `COMM_CSP_NODE=5`
  - `COMMAND_AUTHORITY_PROFILE=sband-primary`
  - `INITIAL_COMM_BAND=sband`
- `TARGET_COMM_PROFILE=uhf-primary`
  - `COMM_CSP_NODE=6`
  - installed target OBC service bootstrap keeps `COMMAND_AUTHORITY_PROFILE=sband-primary`
  - bounded proof first bootstraps through node `5`, explicitly switches to `UHF primary`, then exercises UHF-primary authority semantics through node `6` ingress
- `TARGET_COMM_PROFILE=uhf-backup`
  - `COMM_CSP_NODE=6`
  - installed target OBC service bootstrap keeps `COMMAND_AUTHORITY_PROFILE=sband-primary`
  - bounded proof keeps S-band as the initial active band and uses node `6` as backup ingress with UHF-backup authority semantics
- target/lab COMM detector truth:
  - internal CSP ping to node `5` or node `6`
  - `pingTimeoutMs=500`
  - `unavailableFailureThreshold=10`
- node `6` proofs keep `DIAGNOSTIC_QUIET_PACKET_EGRESS=1`
- reboot-class target proofs stay on the default node-`5` path

## Evidence

### 1. Target node-5 bounded command/readback proof

- Command: `TARGET_COMM_PROFILE=sband PROBE_MODE=command-path bash scripts/run_rpi_target_recovery_restart_probe.sh`
- Verdict: `PASS`
- Artifact root:
  - `/tmp/rpi-target-recovery-restart.o6ohYv`
- Observed scope:
  - historical then-current authenticated `SESSION_OPEN`
  - `GET_RESET_CAUSE`
  - target journal acceptance on the default node-`5` path
  - this artifact is superseded for current auth-truth claims by the later
    secure-auth records

### 2. Target node-5 service-managed R2 recovery proof

- Command: `TARGET_COMM_PROFILE=sband bash scripts/run_rpi_target_recovery_restart_probe.sh`
- Verdict: `PASS`
- Artifact root:
  - `/tmp/rpi-target-recovery-restart.jK9BzD`
- Observed scope:
  - target ADCS transport fault triggers `R2_RESTART_SOFTWARE_COMPONENT`
  - `obc-comm-csp-stack.service` restarts under systemd
  - post-restart metadata reports the expected recovery source/level

### 3. Target node-5 hardware watchdog board-reset proof

- Command: `TARGET_COMM_PROFILE=sband bash scripts/run_rpi_target_hardware_watchdog_reset_probe.sh`
- Verdict: `PASS`
- Artifact root:
  - `/tmp/rpi-hw-watchdog-reset.U7OXIv`
- Observed scope:
  - active target service owns `/dev/watchdog0`
  - bounded `SET_WATCHDOG_PROBE_SUPPRESSION` trigger first records watchdog
    shared-recovery `R2_RESTART_SOFTWARE_COMPONENT` / `PROCESS_RESTART`, then
    reaches hardware-watchdog board reset semantics
  - post-reboot readback reports `RECOVERY_WATCHDOG`, `WATCHDOG_ADCS_FDIR`, and `R6_OBC_REBOOT`
  - quiet-path service override restores normal mode before exit
  - 2026-05-19 branch-local revalidation after probe stabilization also passed
    with artifact root `/tmp/rpi-hw-watchdog-reset.tKkkAD`

### 4. Target node-6 quiet `uhf-backup` bounded proof

- Command shape:
  - `TARGET_COMM_PROFILE=uhf-backup ... bash scripts/run_rpi_target_recovery_restart_probe.sh`
- Verdict: `PASS`
- Artifact root:
  - `/tmp/rpi-target-recovery-restart.mDDhw6`
- Observed scope:
  - historical bounded authenticated `SESSION_OPEN`
  - bounded command/readback through quiet node-`6` backup ingress

### 5. Target node-6 quiet `uhf-primary` bounded proof

- Command shape:
  - `TARGET_COMM_PROFILE=uhf-primary ... bash scripts/run_rpi_target_recovery_restart_probe.sh`
- Verdict: `PASS`
- Artifact root:
  - `/tmp/rpi-target-recovery-restart.1T5oFa`
- Observed scope:
  - node-`5` bootstrap succeeds first
  - `COMM_SET_ACTIVE(UHF)` explicit switch succeeds
  - historical bounded authenticated `SESSION_OPEN`
  - bounded command/readback through quiet node-`6` primary ingress

## Notes

- Target node-`6` ingress required target UHF VCID alignment with the hosted baseline. The passing target path uses `VCID=2` (`0x08` in the CCSDS VC field), not the older target-local `VCID=1` shape.
- `ground_ttc_gateway` remains a probe-owned temporary ground relay. Current target/lab truth does not require it to stay attached outside an active proof or future persistent ground-system work.
- keep using this record for topology migration and bounded quiet-path carrier
  facts only; do not use it as the current secure-auth oracle for Chapter 5 or
  maintained target recovery/watchdog closure
- This change does **not** claim:
  - general non-quiet serial stability under background TM
  - standalone target UHF bootstrap from a cold node-`6` session-open path
  - beacon / standby RX / handshake / retry state-machine behavior
  - simultaneous dual-link target runtime
