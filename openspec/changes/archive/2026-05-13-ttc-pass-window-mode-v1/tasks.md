## 1. OpenSpec Artifacts

- [x] 1.1 Create `proposal.md`, `design.md`, `tasks.md`, and delta specs for `core-system-contracts`, `mission-autonomy`, and `verification-evidence`.
- [x] 1.2 Validate the OpenSpec change artifacts before implementation with `openspec validate ttc-pass-window-mode-v1`.

## 2. TTC Policy Runtime Boundary

- [x] 2.1 Add `TtcPassManager` as a real component with classic F' L2 test harness coverage.
- [x] 2.2 Add bounded TTC config, single-window epoch contract, explicit clear command, and named TTC policy reason/state constants.
- [x] 2.3 Add narrow runtime/provider interfaces for mode control, GPS cached state, and COMM runtime state needed by `TtcPassManager`.
- [x] 2.4 Extend the internal mode source tagging with a TTC policy source and keep TTC policy on the normal internal mode-control path.

## 3. TTC Policy Behavior And Plumbing

- [x] 3.1 Implement GPS cached-state validity, bounded UTC-to-epoch conversion, pass-window comparison, and COMM loss timeout helper logic with direct L1 coverage.
- [x] 3.2 Implement `TtcPassManager` policy evaluation so it auto-enters `TTC` from `IDLE` and auto-exits `TTC` to `IDLE` according to the approved guards.
- [x] 3.3 Wire `TtcPassManager` into `OBC/TopCcsds` on the 1-second scheduled path and preserve minimal build/runtime parity for legacy topologies.
- [x] 3.4 Update runtime services and hosted shell support so TTC config/window/status can be set and inspected through the hosted runtime surface.

## 4. Focused Tests And Hosted Proof

- [x] 4.1 Add direct helper tests for window validation, window activity, UTC-to-epoch conversion, GPS freshness validity, and COMM loss timeout behavior.
- [x] 4.2 Add `TtcPassManager` component tests covering commands, status/event semantics, auto-entry, auto-exit, manual coexistence, and safety/recovery precedence interactions.
- [x] 4.3 Update affected runtime and mode tests, including hosted runtime shell/unit coverage.
- [x] 4.4 Add `scripts/run_ttc_pass_window_mode_v1_probe.sh` to prove no-entry when disabled, auto-entry on active valid window, auto-exit on window end, hosted safety/recovery precedence, manual/automatic coexistence, and the truthful hosted COMM-loss boundary for this baseline.

## 5. Documentation And Evidence

- [x] 5.1 Add `docs/test-records/ttc-pass-window-mode-v1/README.md` with exact commands, results, reused/new validation path notes, and explicit non-claims.
- [x] 5.2 Update `docs/verification-path-registry.md` for the new hosted TTC pass-window proof path.
- [x] 5.3 Update `README.md`, `docs/architecture/current-development-architecture.md`, `docs/roadmap/README.md`, `docs/roadmap/01-mode-model-v2.md`, and `docs/roadmap/08-system-architecture-roadmap.md` so the active baseline truth reflects TTC pass-window automation ownership and remaining boundaries.

## 6. Verification And Closeout

- [x] 6.1 Run a fresh local verification build/gate from the repo virtual environment.
- [x] 6.2 Run focused affected tests and the hosted TTC pass-window probe after the fresh build.
- [x] 6.3 Run `openspec validate ttc-pass-window-mode-v1` and `openspec validate --specs`.
- [x] 6.4 Prepare the change for governed closeout without broadening scope into scheduler, ADCS tracking, or target deployment work.
