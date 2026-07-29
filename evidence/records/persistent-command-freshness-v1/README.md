# persistent-command-freshness-v1 Evidence

Date: 2026-05-16.

Branch: `feature/persistent-command-freshness-v1`.

Base commit: `9cadc693ba3b0571f76285b039f2e4834c7b601c` at evidence capture time.

OpenSpec change: `persistent-command-freshness-v1`.

## Scope

This record closes the reboot-safe command freshness and bounded boot-trust persistence work for architecture-review follow-up 02 on the active `OBC` / `TopCcsds` path:

- `session_id` is now a monotonic reopen epoch per source epoch on the active command-envelope path
- `CommandIngressAuthority` owns a dedicated persistent freshness store under `persistent-data/command-ingress/`
- accepted `SESSION_OPEN(seq0)` persists the new reopen floor before reopening the session
- replayed or lower/equal `SESSION_OPEN(seq0)` values fail closed after restart
- non-lifecycle traffic still uses in-memory strict-monotonic sequence enforcement after a fresh open succeeds
- target persistence proof shows command-freshness state and boot metadata survive restart truthfully on the active path

This record does not claim nonce-based replay protection, persistent secure key storage, hardware-backed boot trust, bootloader or partition handoff proof, simultaneous S-band/UHF proof, legacy retirement, or COMM CSP lab operational closure.

## Build And Verification

Commands run:

```text
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/command_freshness_store_unit_test
./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test
bash scripts/run_command_auth_envelope_probe.sh
bash scripts/run_command_session_sequence_probe.sh
bash scripts/run_command_session_lifecycle_probe.sh
bash scripts/run_command_persistent_freshness_probe.sh
RPI_SSH_TARGET=operator@<private-lab-host> bash scripts/run_rpi_command_persistence_probe.sh
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-persistent-command-freshness-v1d
openspec validate persistent-command-freshness-v1
openspec validate --specs
```

Result summary:

- `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe`: PASS, `71/71` tests.
- `./build-fprime-automatic-native-ut/bin/Darwin/command_freshness_store_unit_test`: PASS.
- `./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test`: PASS.
- `bash scripts/run_command_auth_envelope_probe.sh`: PASS.
- `bash scripts/run_command_session_sequence_probe.sh`: PASS.
- `bash scripts/run_command_session_lifecycle_probe.sh`: PASS.
- `bash scripts/run_command_persistent_freshness_probe.sh`: PASS.
- `RPI_SSH_TARGET=operator@<private-lab-host> bash scripts/run_rpi_command_persistence_probe.sh`: PASS.
- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-persistent-command-freshness-v1d`: PASS.
- `openspec validate persistent-command-freshness-v1`: PASS.
- `openspec validate --specs`: PASS.

## Component And Store Coverage

Covered behaviors:

- first-boot empty-root initialization
- accepted open persists the next allowed session floor
- reconstructed state after restart rejects replayed or lower/equal reopen epochs
- higher reopen epoch succeeds after replay rejection
- stale old-session traffic stays fail-closed until a fresh higher reopen
- per-source independence across distinct source epochs
- single-copy corruption fallback to the older valid snapshot
- both-invalid persistent state fail-closed behavior

The component suite also exercises the new telemetry and rejection behavior around persistent freshness availability and store faults.

## Hosted Active-Path Persistence Probe

Command run:

```text
bash scripts/run_command_persistent_freshness_probe.sh
```

Result summary:

- `bash scripts/run_command_persistent_freshness_probe.sh`: PASS.

Key observations:

```text
command_persistent_freshness_probe: PASS
store before restart: session-floor-a.bin
store after restart: session-floor-a.bin,session-floor-b.bin
replay=replayed old SESSION_OPEN(seq0) rejected after same-runtime-root restart
reopen=fresh higher SESSION_OPEN(seq0) accepted and follow-on command dispatched
```

Interpretation: on the active hosted CCSDS path, same-runtime-root restart preserves the highest accepted reopen floor and rejects replayed/lower/equal session epochs before reopening succeeds with a higher `session_id`.

## Hosted Regression Probes

Commands run:

```text
bash scripts/run_command_auth_envelope_probe.sh
bash scripts/run_command_session_sequence_probe.sh
bash scripts/run_command_session_lifecycle_probe.sh
```

Result summary:

- authenticated envelope verification remains PASS on the active routed ingress path
- in-memory sequence enforcement remains PASS after session open
- lifecycle reopen behavior remains PASS under the new monotonic-session contract

Probe maintenance included removing stale assumptions that direct legacy commands were part of the active comm-managed proof path; the updated probes stay bounded to the envelope-driven active contract.

## Raspberry Pi Active-Path Persistence Proof

Command run:

```text
RPI_SSH_TARGET=operator@<private-lab-host> bash scripts/run_rpi_command_persistence_probe.sh
```

Result summary:

- `RPI_SSH_TARGET=operator@<private-lab-host> bash scripts/run_rpi_command_persistence_probe.sh`: PASS.

Key observations:

```text
rpi_command_persistence_probe: PASS
target=operator@<private-lab-host>
runtime-root=$OBC_HOME/lab/fprime/v0/runtime/command-persistence-probe
gds-host=<private-lab-host>
gds-port=52043
remote-radio-port=17000
remote-csp-sub-port=16100
remote-csp-pub-port=17100
```

Target state before restart:

```text
$OBC_HOME/lab/fprime/v0/runtime/command-persistence-probe/persistent-data/command-ingress/session-floor-a.bin
schema_version=4
active_slot=SLOT_A
confirmed=1
trust_status=4
boot_count=3
consecutive_reset_count=2
last_recovery_source=8
last_recovery_level=6
```

Target state after restart:

```text
$OBC_HOME/lab/fprime/v0/runtime/command-persistence-probe/persistent-data/command-ingress/session-floor-a.bin
$OBC_HOME/lab/fprime/v0/runtime/command-persistence-probe/persistent-data/command-ingress/session-floor-b.bin
schema_version=4
active_slot=SLOT_A
confirmed=1
trust_status=4
boot_count=4
consecutive_reset_count=0
last_recovery_source=8
last_recovery_level=6
```

Interpretation:

- active target OBC persisted command-ingress freshness state before and after restart on the same runtime root
- replayed old `SESSION_OPEN(seq0)` was rejected as stale replay after remote restart
- a fresh higher reopen succeeded on the active path
- boot metadata remained readable alongside the new command-ingress persistence state
- the probe now derives fresh session epochs per run, so the governed target evidence remains rerunnable against the same persisted runtime root

This target proof is intentionally bounded to persistence truth on the active OBC path. Hosted proof remains the formal functional proof for replay rejection and reopen success semantics.

## Probe Cleanup Required By This Change

The target persistence probe needed two active-path fixes to become truthful evidence:

- it now binds host-side `fprime-gds` on `0.0.0.0` so the Raspberry Pi can actually reach the governed host listener
- it now uses isolated remote `RADIO_PORT`, `CSP_HUB_SUB_PORT`, and `CSP_HUB_PUB_PORT` values so the source-workspace active probe path does not collide with the installed autostart service

These are active-path alignment fixes required by follow-up 02. They do not add any new legacy dependency.

## Reused And Updated Verification Paths

Reused baseline:

- hosted command ingress authority, command envelope metadata, command session sequence, and command session lifecycle paths
- Raspberry Pi direct `OBC -> GDS` connectivity path for the target proof transport boundary

Updated by this change:

- hosted command-session proof now includes reboot-safe persisted reopen floors
- target proof now includes bounded persistence truth for command freshness plus boot metadata on the active `OBC` path

## Deferred Work

- nonce/challenge replay protection
- persistent secure key storage
- hardware-root boot trust
- full Raspberry Pi bootloader, partition handoff, or power-loss validation
- legacy retirement
