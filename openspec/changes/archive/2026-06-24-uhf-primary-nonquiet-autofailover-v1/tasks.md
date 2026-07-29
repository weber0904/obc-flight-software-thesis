## 1. OpenSpec And Baseline Governance

- [x] 1.1 Add proposal, design, tasks, and delta specs for `uhf-primary-nonquiet-autofailover-v1`.
- [x] 1.2 Update `chapter5-integrated-route-closure-v1` artifacts to reference the new non-quiet/autonomous-failover dependency for Route 2/3 target closure.

## 2. Product Runtime Changes

- [x] 2.1 Remove current UHF-primary packet-quiet behavior from `CommController` / `CommEgressMux` baseline semantics while keeping beacon suppress separate and auth-triggered.
- [x] 2.2 Refresh focused component tests for `CommController` and `CommEgressMux` to cover non-quiet UHF primary, beacon suppress separation, and unchanged detector/executor failover ownership.

## 3. Governed Target Proof Surfaces

- [x] 3.1 Add a governed helper that creates and records a bounded `subsystem-sband-csp.service` unavailable window for autonomous failover proofs.
- [x] 3.2 Add a fresh non-quiet UHF primary runtime benchmark with the required repeated and interleaved command stability counts.
- [x] 3.3 Add a fresh autonomous UHF failover proof that covers detector latch, failover marker, UHF re-auth, bounded readback, and beacon suppress start after auth.
- [x] 3.4 Rebase Chapter 5 Route 2 target link/HK stage and Route 3 target watchdog stage onto the new current proof family without quiet/manual-switch dependencies.

## 4. Evidence And Current Docs

- [x] 4.1 Update current baseline docs and verification registry wording to remove quiet/manual-switch current truth and replace it with non-quiet/autonomous-failover truth.
- [x] 4.2 Add fresh test records for `uhf-primary-nonquiet-runtime-v1` and `target-autonomous-uhf-failover-v1`.
- [x] 4.3 Refresh Chapter 5 and adjacent historical records so superseded quiet/manual-switch evidence is still reviewable but no longer cited as current closure.

## 5. Verification

- [x] 5.1 Run focused UT/build coverage for COMM runtime changes.
- [x] 5.2 Run the fresh non-quiet UHF primary runtime benchmark and confirm each subcase meets the `>= 7/10` gate.
- [x] 5.3 Run the fresh autonomous failover proof plus the updated Route 2/3 target wrappers.
- [x] 5.4 Run `openspec validate uhf-primary-nonquiet-autofailover-v1`, `openspec validate chapter5-integrated-route-closure-v1`, and `openspec validate --specs`.

## Closure Notes

- Earlier 2026-06-24 one-shot benchmark/proof failures remain reviewable
  historical evidence only. They exposed real ground-observability weakness on
  the old single-shot readback oracle, plus one proof-specific beacon oracle
  bug.
- The current maintained readback truth now uses resend-until-ground-readback
  policy on the secure-command readback family.
- Fresh retained reruns on 2026-06-25 closed the maintained current path:
  - `uhf-primary-nonquiet-runtime-v1`
    - repeated `GET_RESET_CAUSE = 10/10`
    - interleaved `GET_RESET_CAUSE + GET_PERSISTENT_FAULT_HISTORY = 10/10`
    - artifact:
      `/private/tmp/target-uhf-primary-nonquiet-runtime.J93YnP/diagnostics/uhf-primary-nonquiet-runtime-summary.json`
  - `target-autonomous-uhf-failover-v1`
    - detector-triggered failover, `UHF` re-auth, `COMM_UHF_BEACON_SUPPRESS_STARTED`,
      bounded `GET_RESET_CAUSE`, and bounded `GET_PERSISTENT_FAULT_HISTORY`
      all `PASS`
    - artifact:
      `/private/tmp/target-autonomous-uhf-failover.wWrASs/diagnostics/autonomous-uhf-failover-summary.json`
  - dependent Chapter 5 wrappers also fresh-passed:
    - Route 2 target:
      `/private/tmp/chapter5-route2-target.uJ0hvQ/summary.log`
    - Route 3 target:
      `/private/tmp/chapter5-route3-target.okbj51/summary.log`
