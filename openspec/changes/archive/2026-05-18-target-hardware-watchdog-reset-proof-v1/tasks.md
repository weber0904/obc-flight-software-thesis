## 1. Capability Gate And Runtime Surface

- [x] 1.1 Record the Raspberry Pi `bcm2835-wdt` capability-gate result, including open/feed/clean-stop/reopen observations and fixed timeout constraints.
- [x] 1.2 Add hardware-watchdog runtime configuration parsing and target launch/env plumbing while keeping hosted defaults disabled.

## 2. Target Integration And Recovery Split

- [x] 2.1 Add a target-only Linux watchdog sink wired from `WatchdogSupervisor.watchdogFeedOut` in the active `TopCcsds` topology.
- [x] 2.2 Add Raspberry Pi install/systemd permission handling for `/dev/watchdog0` using repo-owned udev/group/service configuration without running OBC as root.
- [x] 2.3 Update target runtime recovery handling so watchdog-source `R6` persists reboot intent but uses hardware watchdog timeout instead of runtime exit `32`, while `R2` and non-watchdog `R6` keep existing semantics.
- [x] 2.4 Add a bounded proof-only watchdog suppression trigger and related unit coverage.

## 3. Verification And Evidence

- [x] 3.1 Add or update unit tests for Linux watchdog sink behavior, runtime flag parsing, and recovery exit-policy separation.
- [x] 3.2 Add a Raspberry Pi hardware watchdog reset probe, including a probe-owned quiet service override and journal-first acceptance, and rerun the existing target `R2` restart probe to prove no regression.
- [x] 3.3 Record `evidence/records/target-hardware-watchdog-reset-proof-v1/` evidence and update `evidence/verification-path-registry.md`.

## 4. Documentation And OpenSpec Closeout

- [x] 4.1 Update README, current architecture, target flight design, interfaces, and target operator/runbook docs for the new hardware watchdog boundary.
- [x] 4.2 Run focused tests/probes, the shared verification gate, `openspec validate target-hardware-watchdog-reset-proof-v1`, and `openspec validate --specs`.
- [x] 4.3 Archive or otherwise sync the OpenSpec change once implementation and evidence are complete.

Blocked note as of 2026-05-17:

- Raspberry Pi capability gate for `bcm2835-wdt` passed.
- The repo-side target service gap where `obc-comm-csp-stack.service` pointed
  `--comm tcp 127.0.0.1:7000` at no local listener has been fixed by restoring
  the local `radio_mock_server` sidecar.
- The existing target `R2` recovery restart probe passes again after the
  service/runtime-root hardening and command-path retry updates.
- The formal hardware-watchdog board-reset proof is still blocked because the
  governed serial ingress path used by `ground_ttc_gateway -> subsystem.local
  /dev/serial0 -> COMM node 4 -> OBC` is not yet stable enough to
  consistently deliver the pre-trigger authenticated `SESSION_OPEN`.
- Current observed symptoms are CCSDS checksum failures, APID sequence-count
  jumps, and recurring `ComCcsds.comQueue.QueueOverflow` on the active target
  service before the proof trigger is even accepted.
- Additional narrowing completed on 2026-05-17:
  - the repo-owned minimal UART baseline still passes in both directions via
    `run_subsystem_comm_uart_link_probe.sh`
  - but the same physical chain still fails once exercised through
    `ground_ttc_gateway + fprime-gds + comm_csp_node`, even with hosted OBC
    and Stage 2 downlink disabled
  - this means the remaining blocker is no longer “target OBC steady-state
    downlink only”; it is the governed TT&C traffic pattern on the physical
    serial carrier itself

Quiet-path follow-up as of 2026-05-18:

- The repo now treats the remaining proof gap as “watchdog semantics are ready,
  but the noisy serial downlink corrupts the governed target command path before
  the pre-trigger authenticated session can complete.”
- The quiet-path unblock approach keeps the same governed change and branch,
  adds a diagnostic-only `CommEgressMux` quiet egress gate plus temporary
  systemd override, and switches the target watchdog proof to journal-first
  acceptance while leaving default runtime behavior unchanged.

Completion note as of 2026-05-18:

- The target Raspberry Pi hardware watchdog reset probe now passes on the
  probe-owned quiet path and records `RECOVERY_WATCHDOG`,
  `WATCHDOG_ADCS_FDIR`, and `R6_OBC_REBOOT` after a real board reboot.
- The existing target `R2` recovery restart probe also passes after the
  quiet-path work, confirming normal non-quiet service-managed restart
  behavior remains intact.
