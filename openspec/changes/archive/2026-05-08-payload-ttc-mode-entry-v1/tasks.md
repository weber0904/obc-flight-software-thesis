## 1. OpenSpec Artifacts

- [x] 1.1 Create `proposal.md`, `design.md`, core-system-contracts delta spec, mission-autonomy delta spec, verification-evidence delta spec, and `tasks.md` for `payload-ttc-mode-entry-v1`.
- [x] 1.2 Validate the OpenSpec change artifacts before implementation with `openspec validate payload-ttc-mode-entry-v1`.

## 2. Runtime Guard Interfaces

- [x] 2.1 Add named transition rejection reason constants, an operator guard decision type, and a narrow operator transition guard interface near the existing `ModeSafetyController` runtime boundary.
- [x] 2.2 Implement the full v1 5x5 operator transition matrix and `HELL -> SAFE` cached-EPS SoC guard in `ModeSafetyController` helper logic.
- [x] 2.3 Rename the internal mode-control apply path from the neutral runtime setter to an explicit source-tagged internal apply API with `SafetyFallback`, `SafetyRecovery`, and `TestSetup`.

## 3. ModeManager And Hosted Runtime Behavior

- [x] 3.1 Update `ModeManager` so `MODE_SET` uses the configured operator transition guard, emits `SYS_MODE_TRANSITION_REJECTED`, preserves `SYS_MODE_CHANGE(mode)`, and returns `OK`, `VALIDATION_ERROR`, `EXECUTION_ERROR`, or generated `FORMAT_ERROR` as specified.
- [x] 3.2 Configure the active topology/runtime so `ModeManager` receives the `ModeSafetyController` operator guard and `ModeSafetyController` still receives the internal safety apply interface.
- [x] 3.3 Update `HostedRuntime` services and shell output so `mode <...>` uses the same guarded operator path and parser errors remain parser-only.
- [x] 3.4 Update existing hosted mode/safety probes that relied on arbitrary mode setting to use valid operator sequences or explicit internal/safety setup.

## 4. Focused Tests And Probes

- [x] 4.1 Expand `ModeManager` classic component tests to cover all 25 operator pairs, same-mode no-op side effects, rejection event/telemetry semantics, guard-unconfigured response, and invalid enum `FORMAT_ERROR`.
- [x] 4.2 Expand `ModeSafetyController` helper/component tests to cover reason mapping, `HELL -> SAFE` SoC `>15`, `==15`, `<15`, unavailable cache, and continued autonomous fallback/recovery.
- [x] 4.3 Expand `HostedRuntime` parser/unit tests for canonical, retired, unknown, and mixed-case mode spellings plus shell accepted/rejected output.
- [x] 4.4 Add or update a hosted shell regression probe with `SAFE` boot precondition and the bounded operator sequence from the design.
- [x] 4.5 Add or update a default hosted CCSDS S-band `MODE_SET` command path probe that uses `fprime-cli`, checks command responses/events/final `SYS_MODE`, and avoids transport-order assumptions.

## 5. Documentation, Evidence, And Pending Notes

- [x] 5.1 Add `evidence/records/payload-ttc-mode-entry-v1/README.md` with reproducibility fields, shell evidence, CCSDS/F Prime command evidence, command responses, event excerpts, telemetry snapshots, deferred work, and reused/new verification paths.
- [x] 5.2 Update `evidence/verification-path-registry.md` only for verification path additions or upgrades; otherwise cite the reused default hosted CCSDS S-band path in the evidence record.
- [x] 5.3 Update `pending/README.md`, `pending/00-baseline-facts.md`, `pending/01-mode-model-v2.md`, `pending/05-fdir-watchdog-auth-scheduler.md`, and `pending/06-openspec-change-breakdown.md` so this change is described as first-version manual PAYLOAD/TTC entry/exit guard only.

## 6. Verification And Closeout

- [x] 6.1 Run a fresh local verification build/gate with `bash scripts/run_verification_ci.sh <fresh-output-dir>`.
- [x] 6.2 Run focused affected tests and hosted probes after the fresh build.
- [x] 6.3 Run `openspec validate payload-ttc-mode-entry-v1` and `openspec validate --specs`.
- [x] 6.4 Use the governed change-closeout flow to sync/archive OpenSpec, update reconciliation artifacts, prepare a reviewable Conventional Commit boundary, and stop before push unless explicitly approved.
