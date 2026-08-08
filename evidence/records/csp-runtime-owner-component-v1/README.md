# csp-runtime-owner-component-v1 Evidence

Status: staged architecture-refactor evidence for the owner-mediated RG1 blocking-path removal slice.
Last updated: 2026-06-12.

## Scope

This record covers the deployed owner-refactor implementation up through the
RG1 scheduled CSP poll removal slice:

- add the repo-owned `CspRuntimeOwner` component
- instantiate it in the deployed `OBC` topology
- rebind deployed COMM, EPS, and ADCS CSP clients so topology injection owns
  runtime access
- move steady-state COMM subsystem health probing off synchronous periodic
  `ping()`
- move scheduled EPS and ADCS bridge polling off synchronous periodic
  request/reply

## Implemented Slice

The deployed topology now injects `cspRuntimeOwner` into:

- `CspBridge`
- `GroundLinkDriver`
- `CommReliableTransfer` through `CommController`
- `EpsBridge` owned CSP transport
- `AdcsBridge` owned CSP transport

This means deployed runtime ownership is now explicit and topology-managed
instead of relying on each client to bind directly to `defaultRuntime()`.

In the next slice after ownership injection, `CommController` steady-state
primary-band subsystem probing was also moved off synchronous periodic `ping()`
calls. The 1 Hz path now submits async owner pings and consumes cached
completion results on later cycles, while explicit force-probe paths still keep
their bounded synchronous behavior.

## Implemented Async Poll Removal

The deployed RG1 steady-state CSP poll paths now behave as follows:

- `CommController` periodic subsystem responsiveness checks submit owner async
  ping requests and consume cached completion on later scheduler ticks.
- `EpsBridge.schedIn_handler(...)` no longer performs synchronous CSP
  request/reply. It submits owner async EPS status polls, consumes completion
  on later ticks, preserves summary-only publish behavior, and records
  coalesced ticks while a prior request is still inflight.
- `AdcsBridge.schedIn_handler(...)` follows the same owner async/coalesced
  pattern for ADCS state polls while preserving scheduled poll-health semantics
  that distinguish transport failure from no-valid-refresh.

The remaining synchronous CSP round-trips are now limited to explicit
command/runtime fetch flows and other non-steady-state paths that were
intentionally left bounded and synchronous by design.

## Local Verification

Commands run on the owner-refactor branch worktree:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate -f
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build -j4
openspec validate csp-runtime-owner-component-v1
openspec validate --specs
```

Observed verdicts:

- `fprime-util generate -f`: PASS
- `fprime-util build -j4`: PASS
- `openspec validate csp-runtime-owner-component-v1`: PASS
- `openspec validate --specs`: PASS

Additional local observations:

- `ctest -N` in the current default build tree reports `Total Tests: 0`
- component UT sources for `EpsBridge` and `AdcsBridge` were updated for the
  new async scheduled-poll path, but this build configuration did not emit
  runnable unit-test executables or register them with CTest
- `python3 -m py_compile scripts/comm_verification/lib/run_target_can_matrix_probe.py`:
  PASS after aligning the UHF secure-auth proof helper with the current
  no-preamble baseline
- `bash -n scripts/run_target_secure_auth_proof.sh`: PASS after removing the
  stale secure-auth wrapper defaults that still forced target diagnostics on
  during the governed proof path

## Review Follow-Up Verification

The post-review follow-up on `2026-06-12` kept the owner-refactor scope
unchanged and only addressed correctness/portability issues raised during PR
review:

- `CspRuntimeOwner` sync requests now fail fast instead of waiting forever if
  the worker has already been stopped.
- `CommController` async primary-band probe completion now bypasses the
  remaining cooldown window instead of delaying availability updates until a
  later tick.
- `GroundLinkDriver` and `UartDriver` no longer compare stats structs with
  raw `memcmp(...)`.
- `AdcsBridge`, `EpsBridge`, `GroundLinkDriver`, and deployed topology env
  parsing now use `strtoull(...)` plus `ERANGE` checking instead of relying on
  `strtoul(...)` width assumptions.
- the target baseline helper once again recognizes the unscoped
  `OBC CCSDS S-band runtime started.` marker instead of breaking before it can
  mark startup as seen.

Verification rerun for that follow-up:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build -j4
PATH="$PWD/fprime-venv/bin:$PATH" ctest --output-on-failure -R OBC_Components_CommController_ut_exe
PATH="$PWD/fprime-venv/bin:$PATH" ctest --output-on-failure -R 'OBC_Components_(EpsBridge|AdcsBridge)_ut_exe'
PATH="$PWD/fprime-venv/bin:$PATH" ctest --output-on-failure -R 'OBC_Components_(GroundLinkDriver|UartDriver)_ut_exe'
PATH="$PWD/fprime-venv/bin:$PATH" openspec validate csp-runtime-owner-component-v1
PATH="$PWD/fprime-venv/bin:$PATH" openspec validate --specs
python3 -m py_compile scripts/comm_verification/lib/ensure_target_comm_lab_baseline.py
```

Observed verdicts:

- `fprime-util build -j4`: PASS
- `ctest --output-on-failure -R OBC_Components_CommController_ut_exe`: PASS
- `ctest --output-on-failure -R 'OBC_Components_(EpsBridge|AdcsBridge)_ut_exe'`:
  PASS
- `ctest --output-on-failure -R 'OBC_Components_(GroundLinkDriver|UartDriver)_ut_exe'`:
  PASS
- `openspec validate csp-runtime-owner-component-v1`: PASS
- `openspec validate --specs`: PASS
- `python3 -m py_compile scripts/comm_verification/lib/ensure_target_comm_lab_baseline.py`:
  PASS

## Target Proof Progression

Canonical governed wrapper attempted:

```bash
bash scripts/run_target_secure_auth_proof.sh
```

Observed progression before the latest owner-refactor retest:

- `2026-06-11`: FAIL at `baseline-readiness / probe gate`
- `2026-06-12` first rerun: readiness gate fixed, but FAIL at
  `uhf-backup-secure-auth`
- `2026-06-12` second rerun: PASS after aligning the current proof helper's
  UHF serial gateway defaults with the current repo baseline

That intermediate `uhf-backup-secure-auth` failure was not a decoded owner
component regression. The current `run_target_can_matrix_probe.py` proof helper
still defaulted UHF serial startup to:

- `TARGET_CAN_UHF_SERIAL_PREAMBLE_LINES=20`
- `TARGET_CAN_UHF_SERIAL_PREAMBLE_DELAY_MS=500`

while the current repo baseline for the UHF live benchmark had already moved to
`0/0`. That mismatch caused the governed proof helper to self-insert a stale
serial warmup window, retry handshake traffic too early, and create a false
`NOT_AUTHENTICATED` boundary during the first 2026-06-12 rerun. The owner
refactor itself still advanced far enough in that run to prove that the old
readiness blocker had been closed.

## Current Target Proof State

Latest governed reruns on the owner-refactor worktree did not stop on the old
oracle or secure-auth helper mismatches; they exposed a different blocker.

### Wrapper hygiene correction

The first `2026-06-12` owner-refactor rerun showed that the top-level wrapper
`scripts/run_target_secure_auth_proof.sh` still forced:

- `OBC_GROUNDLINK_DIAGNOSTICS=1`
- `UHF_COMM_NODE_INGRESS_DIAGNOSTICS=1`

even though `run_target_can_matrix_probe.py` had already been corrected to keep
those disabled for the governed `secure-auth-proof` mode. That stale wrapper
default reintroduced the same heavy per-poll diagnostic spam that earlier
investigation had already ruled out as non-representative.

The wrapper was corrected on this branch to default both values to `0`, and the
next rerun confirmed the clean path through checkpoints:

- `obc-groundlink-diagnostics-override-skipped`
- `ground-paths-start=pass`
- `sband-ground-secure-auth-established=pass`

### Clean rerun progression after wrapper fix

Clean rerun artifact root:

- `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-proof-v1.MJfAT2/target-secure-auth-proof-v1`

Observed governed checkpoint/journal facts on `2026-06-12`:

- `09:45:21 CST`: `sband-ground-secure-auth-established=pass`
  with `confirmation_source=target-journal`
- `09:45:12 CST` target journal:
  `COMMAND_SESSION_OPENED : Command session opened ingress 0 identity 1 role 1 session 1 replaced 0`
- `09:45:12 CST` target journal:
  `SECURE_AUTH_ESTABLISHED : Secure auth established ingress 0 service 1`
- `09:45:39 CST`: `sband-ground-secure-command=pass`
  for `OBCApp.bootManager.GET_RESET_CAUSE` sequence `41`

This means the current owner-refactor branch already proved more than the old
RG1 contention hypothesis required:

- S-band secure auth itself succeeds on the governed target path
- the first secure command after auth also succeeds

### Dedicated command-path rerun after baseline/probe-governance fixes

After the owner-refactor retest above, the branch also corrected two stale
repository-owned proof assumptions:

- `scripts/ensure_target_comm_lab_baseline.sh` no longer treats node-`5`
  `GROUND_LINK_UP` as an `A`-layer prerequisite. The governed target baseline
  now accepts the current service invocation once the S-band listener is ready
  and the OBC journal shows runtime start, ground-link configuration, and CSP
  init.
- `scripts/run_target_secure_auth_command_path_probe.sh` / the underlying
  `run_target_can_matrix_probe.py` readiness helper now separates target-side
  node-`5` readiness from later ground-surface attach readiness instead of
  requiring a pre-auth `GROUND_LINK_UP` marker from the target journal.

Branch-owned rerun:

```bash
bash scripts/run_target_secure_auth_command_path_probe.sh
```

Latest successful artifact root on `2026-06-12`:

- `/var/folders/vk/pbktwbv55cdblxldsr8b0t5r0000gn/T/target-secure-auth-command-path.RufOOr`

Observed governed checkpoint/journal facts:

- `11:38:05 CST`: `sband-ground-readiness=southbound-opened`
- `11:38:18 CST`: `sband-ground-secure-auth-established=pass`
- `11:38:37 CST`: `sband-ground-secure-command=pass` for
  `OBCApp.bootManager.GET_RESET_CAUSE` sequence `41`
- target journal at `11:38:12 CST`:
  `COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED : ... active 1 reason 1 ingress 0 role 1 session 1`
- target journal at `11:38:12 CST`:
  `SECURE_AUTH_ESTABLISHED : Secure auth established ingress 0 service 1`
- proof window journal grep (`2026-06-12 11:37:00 CST` onward) contained no
  `RG_TIMING_CYCLE_THRESHOLD_EXCEEDED` and no `RateGroupCycleSlip`

This dedicated rerun matters because it removes the earlier stale-baseline
oracle from the interpretation: the current branch now has a fresh branch-owned
proof that auth-open plus the first secure command complete on node `5`
without the old RG1/RG3 slip symptom reappearing during the proof window.

### Current blocker: target idle reboot, not proof oracle

The same rerun then failed before the second secure command, not because the
auth path regressed, but because the target host itself disappeared:

- proof failure:
  `ssh command failed target=operator@obc.local rc=255`
- failing command:
  `date '+%Y-%m-%d %H:%M:%S'`
- ssh stderr:
  `ssh: connect to host obc.local port 22: Host is down`

That was then separated from the proof by a passive no-command watch:

- `2026-06-12 09:52:17 CST`: passive watch started with no proof traffic and no
  extra command activity
- the SSH session later died with:
  `Connection to obc.local closed by remote host.`
- `2026-06-12 09:54:15 CST`: OBC had already come back with `uptime` only
  about `1 min`
- current boot logs showed kernel bring-up and `obc-comm-csp-stack.service`
  start around `2026-06-12 09:52:31 .. 09:52:42 CST`

So the current top-level blocker is no longer:

- stale secure-auth wrapper defaults
- stale secure-auth proof oracle
- pre-auth ground invisibility assumptions

It is now target steady-state instability: on this hardware/runtime state, OBC
can reboot even when the governed proof is no longer sending traffic.

## Target Restart-Recovery Follow-On

After the async/coalesced RG1 poll refactor was deployed, a later target
secure-auth rerun exposed a separate target-platform blocker that was not a
decoded regression in secure auth or the owner component itself:

- subsystem-side services on `subsystem.local` remained healthy
- `obc-comm-csp-stack.service` could enter the existing `ADCS_POLL_TRANSPORT`
  `R2` `PROCESS_RESTART` path and restart through `systemd`
- but OBC-side `SocketCAN` on `obc.local:can0` could remain wedged after that
  restart until `obc-lab-can.service` was manually restarted

Live target evidence captured on `2026-06-12` showed the wedge directly:

- before `obc-lab-can.service` restart, `ip -s link show can0` counters were
  flat while `obc-comm-csp-stack.service` logged repeated
  `csp_can_tx_frame[OBCCSP]: write() failed, we have been waiting for CAN buffers for too long (>1000 ms)`
  and `CSP_OWNER_TIMEOUT`
- restarting only `obc-lab-can.service` immediately produced the expected
  transient `Network is down` journal noise from `ip link set can0 down/up`
- after that bounded re-arm, `can0` RX/TX counters resumed increasing and the
  long `waiting for CAN buffers` stall signature cleared

The deployed target unit was therefore tightened so each
`obc-comm-csp-stack.service` start or restart explicitly restarts the governed
`obc-lab-can.service` first. That keeps the OBC-side CAN carrier recovery under
repo-owned systemd control instead of relying on a stale boot-time oneshot
bring-up.

That restart-recovery fix remains valid, but it is not sufficient to close the
current branch end-to-end target proof because the target also exhibits the
separate idle reboot behavior described above.

## Bounded Claim

This staged record supports exactly these claims:

1. deployed CSP runtime ownership is now centralized under a formal topology
   owner component
2. the first owner-refactor slice builds cleanly and validates cleanly under
   OpenSpec
3. steady-state COMM subsystem probing no longer requires periodic synchronous
   CSP ping in `CommController`
4. steady-state deployed RG1 scheduled EPS and ADCS bridge polling no longer
   performs periodic blocking CSP request/reply on the active rate-group thread
5. command-driven sync flows remain intentionally bounded, and the governed
   target path on this branch now proves successful S-band secure auth plus the
   first secure command after auth on the live node-`5` path
6. the earlier target secure-auth failures observed during this change were
   investigation/proof-harness mismatches, not decoded regressions in the
   owner-mediated deployed COMM/EPS/ADCS topology
7. the target owner-refactor proof also identified and bounded an OBC-side
   `SocketCAN` restart-recovery gap, and the installed OBC service now re-arms
   `obc-lab-can.service` on each start so service-managed `R2` restarts do not
   inherit stale `can0` state from an earlier boot
8. the remaining end-to-end target blocker is currently a target idle reboot
   that reproduces even without further proof commands, so the present failure
   is no longer attributable to the old RG1 synchronous shared-CSP poll design
