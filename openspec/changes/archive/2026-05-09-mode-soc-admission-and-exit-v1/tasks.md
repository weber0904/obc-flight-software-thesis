## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, delta specs, and `tasks.md` for `mode-soc-admission-and-exit-v1`.
- [x] 1.2 Validate the OpenSpec artifacts before implementation with `openspec validate mode-soc-admission-and-exit-v1`.

## 2. Runtime Policy And Contracts

- [x] 2.1 Rename shared SoC guard rejection reason names to generic `SOC_GUARD_*` names while preserving stable numeric values.
- [x] 2.2 Extend `ModeSafetyPolicy` with `PAYLOAD -> IDLE` at cached SoC `< 60%`, while keeping `PAYLOAD -> SAFE` at `< 40%` higher priority.
- [x] 2.3 Extend `ModeSafetyController` operator guard behavior so `SAFE -> IDLE` requires cached SoC `> 50%` and `IDLE -> PAYLOAD` requires cached SoC `> 70%`, with fail-closed cache-unavailable behavior and unchanged `IDLE -> TTC`.
- [x] 2.4 Add an explicit internal apply source for automatic PAYLOAD exit so it stays distinct from fallback, recovery, and test setup.

## 3. Focused Tests

- [x] 3.1 Expand direct/helper policy tests for `< 60%` payload exit and `< 40%` precedence.
- [x] 3.2 Expand `ModeSafetyController` classic component UT for guarded admissions, unavailable-cache fail-closed behavior, unchanged `IDLE -> TTC`, and renamed SoC guard reasons.
- [x] 3.3 Expand focused integration coverage proving the real `ModeManager` surface sees `PAYLOAD -> IDLE` at `< 60%` and `PAYLOAD -> SAFE` at `< 40%`.
- [x] 3.4 Rerun affected `ModeManager`, `EpsBridge`, and hosted-runtime tests as regressions.

## 4. Hosted Probes And Evidence

- [x] 4.1 Refresh the hosted shell regression probe for SoC-guarded `SAFE -> IDLE`, `IDLE -> PAYLOAD`, unchanged `IDLE -> TTC`, and continued operator rejection of `HELL`.
- [x] 4.2 Refresh the default hosted CCSDS S-band `MODE_SET` probe so it proves accept/reject behavior for SoC-guarded admissions and at least one unavailable-cache fail-closed case.
- [x] 4.3 Refresh or extend the hosted SoC safety probe so automatic `PAYLOAD -> IDLE` and `< 40%` precedence are covered after a fresh build.
- [x] 4.4 Record reviewable evidence under `evidence/records/mode-soc-admission-and-exit-v1/README.md` and update `evidence/verification-path-registry.md` only where the existing registered paths are upgraded.

## 5. Verification And Closeout

- [x] 5.1 Run a fresh local verification build/gate with `bash scripts/run_verification_ci.sh <fresh-output-dir>`.
- [x] 5.2 Run the affected tests and focused hosted probes after the fresh build.
- [x] 5.3 Run `openspec validate mode-soc-admission-and-exit-v1`, `openspec validate --specs`, and `python3 scripts/check_repo_consistency.py`.
- [x] 5.4 Archive/sync the OpenSpec change once evidence is complete.
- [x] 5.5 Update local `pending/` notes for handoff without adding them to the formal PR boundary.
