## 1. OpenSpec Artifacts

- [x] 1.1 Create `proposal.md`, `design.md`, mission-autonomy delta spec, verification-evidence delta spec, and `tasks.md` for `mode-safety-policy-v1`.
- [x] 1.2 Validate the OpenSpec change artifacts before implementation with `openspec validate mode-safety-policy-v1`.

## 2. Mode Safety Component And Runtime Interfaces

- [x] 2.1 Add a passive `ModeSafetyController` real F' component with `schedIn`, classic F' UT registration, telemetry/events, and a narrow runtime configuration API.
- [x] 2.2 Add a pure C++ policy helper that implements deterministic strict-threshold SoC fallback without runtime configurability.
- [x] 2.3 Add narrow runtime interfaces for cached EPS status and mode get/set control; implement them in `EpsBridge` and `ModeManager` without depending on old `MissionExecutive` policy types.

## 3. Active Topology Binding

- [x] 3.1 Instantiate and wire `modeSafetyController` in `instances.fpp`, `topology.fpp`, and topology setup.
- [x] 3.2 Schedule `modeSafetyController` after `epsBridge.schedIn` and before later runtime consumers where practical.
- [x] 3.3 Keep the provisional `MissionExecutive` uninstantiated, unscheduled, unbound, and unused for this policy.

## 4. Focused Tests And Probe

- [x] 4.1 Add direct helper tests for strict `<` and `>` threshold behavior at `9.99`, `10`, `15`, `15.01`, `39.99`, and `40`.
- [x] 4.2 Add classic L2 component tests for missing EPS cache, stale-cache invalidation after failed EPS polling, unconfigured runtime dependencies, threshold boundaries, required fallbacks, already-target no-op behavior, and high-SoC `SAFE` no-auto-recovery.
- [x] 4.3 Add focused integration coverage proving the policy drives the real `ModeManager` runtime mode surface.
- [x] 4.4 Extend the EPS simulator with a bounded initial SoC option for hosted probes.
- [x] 4.5 Add `scripts/run_mode_safety_policy_hosted_probe.sh` covering `SAFE -> HELL`, `HELL -> SAFE`, `IDLE -> SAFE`, `PAYLOAD -> SAFE`, `TTC -> SAFE`, and high-SoC `SAFE` remaining `SAFE`.

## 5. Verification And Evidence

- [x] 5.1 Run focused helper/component/integration tests after a fresh build.
- [x] 5.2 Run the hosted focused mode safety probe after a fresh native build.
- [x] 5.3 Run full local verification with `bash scripts/run_verification_ci.sh <fresh-output-dir>`.
- [x] 5.4 Record reviewable evidence under `evidence/records/mode-safety-policy-v1/README.md`.
- [x] 5.5 Run `openspec validate mode-safety-policy-v1` and `openspec validate --specs`.

## 6. Closeout

- [x] 6.1 Sync/archive the OpenSpec change once implementation and evidence are complete.
- [x] 6.2 Prepare a reviewable PR boundary and Conventional Commit title, then stop before push unless explicitly approved.
