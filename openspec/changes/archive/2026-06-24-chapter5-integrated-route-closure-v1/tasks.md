## 1. OpenSpec And Governance
- [x] 1.1 Add proposal, design, and tasks for `chapter5-integrated-route-closure-v1`.
- [x] 1.2 Add delta specs for EPS runtime SoC control, TTC-triggered ADCS entry behavior, and Chapter 5 evidence/registry ownership.

## 2. TTC / ADCS Product Gap
- [x] 2.1 Add a small runtime ADCS entry-control interface for `TtcPassManager`.
- [x] 2.2 Wire `TtcPassManager` to request ADCS `POINTING` once on TTC entry without blocking TTC mode entry on failure.
- [x] 2.3 Extend `TtcPassManager` unit coverage for success, failure, no-duplicate resend, and no-exit-restore cases.

## 3. EPS Runtime SoC Control
- [x] 3.1 Add simulator-side runtime SoC target/ramp state and an optional external control socket.
- [x] 3.2 Add a repo-owned helper under `scripts/chapter5_routes/` for sending the supported SoC control commands.
- [x] 3.3 Thread optional EPS control-socket plumbing through hosted and subsystem launch surfaces needed by the new Chapter 5 probes.
- [x] 3.4 Add focused EPS simulator tests for direct set, timed ramp, invalid input, and reconnect behavior.

## 4. Chapter 5 Probe Tree
- [x] 4.1 Create `scripts/chapter5_routes/lib/`, `scripts/chapter5_routes/hosted/`, and `scripts/chapter5_routes/target/`.
- [x] 4.2 Add README or operator-note guidance that records staged route structure, observability expectations, and when command-playbook fallback is allowed.
- [x] 4.3 Add initial hosted and target route scaffolding for Route 1, Route 2, and Route 3 without reviving obsolete probes.

## 5. Verification In This Implementation Round
- [x] 5.1 Run focused build/test coverage for the new TTC and EPS simulator behavior.
- [x] 5.2 Run at least one local EPS control smoke that proves runtime SoC injection against a live simulator process.
- [x] 5.3 Update the task checklist and summarize route-closure status honestly after the retained Route 1/2/3 reruns.

## Current Route-Closure Note

- Route 1 staged closure remains retained.
- Route 2 / Route 3 target current closure now depends on
  `uhf-primary-nonquiet-autofailover-v1`.
- Fresh follow-on evidence on 2026-06-25 closed the maintained current target
  dependency:
  - `uhf-primary-nonquiet-runtime-v1`
    - repeated `GET_RESET_CAUSE = 10/10`
    - interleaved `GET_RESET_CAUSE + GET_PERSISTENT_FAULT_HISTORY = 10/10`
  - `target-autonomous-uhf-failover-v1`
    - detector-triggered failover, `UHF` re-auth, beacon suppress start, and
      bounded `GET_*` readback all `PASS`
- Route 2 target TTC wrapper also had a retained oracle bug in an older current
  repo surface; that oracle handling is corrected in current code and a fresh
  full Route 2 target rerun is now retained as `PASS`.
- Route 3 target watchdog rerun is also now retained as `PASS` on the installed
  baseline after A-layer cleanup removed the stale
  `99-disable-hw-watchdog.conf` override.
