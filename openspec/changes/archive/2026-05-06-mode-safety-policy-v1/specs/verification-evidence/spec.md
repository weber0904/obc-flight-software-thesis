## ADDED Requirements

### Requirement: Mode Safety Policy Evidence
The verification evidence SHALL record reviewable local evidence for the SoC-driven mode safety policy, including component tests, helper tests, focused integration tests, hosted probe coverage, OpenSpec validation, and explicit out-of-scope boundaries.

#### Scenario: Component and helper evidence is reviewable
- **WHEN** mode-safety-policy-v1 completes local verification
- **THEN** the evidence SHALL identify the focused `ModeSafetyController` component tests, pure policy-helper tests, and affected `EpsBridge` component test coverage that cover missing EPS cache, stale-cache invalidation after failed EPS polling, unconfigured runtime dependency behavior, strict threshold boundaries, `SAFE -> HELL`, `HELL -> SAFE`, `IDLE/PAYLOAD/TTC -> SAFE`, already-target no-op behavior, high-SoC `SAFE` no-auto-recovery behavior, and non-fatal EPS critical-battery alarm behavior

#### Scenario: Integration evidence uses the normal mode surface
- **WHEN** mode-safety-policy-v1 records integration evidence
- **THEN** the evidence SHALL show that safety fallback requests go through `ModeManager::setModeForRuntime` or the equivalent normal runtime mode-control path and result in the normal `SYS_MODE_CHANGE` mode surface rather than a parallel mode store

#### Scenario: Hosted focused evidence is reviewable
- **WHEN** mode-safety-policy-v1 records hosted evidence
- **THEN** the evidence SHALL identify the hosted focused probe command, isolated runtime roots or ports, EPS simulator initial SoC cases, expected mode fallback outcome for each case, observed outcome, and final verdict

#### Scenario: Exclusions stay explicit
- **WHEN** mode-safety-policy-v1 evidence is recorded
- **THEN** the evidence SHALL state that it does not prove COMM split-link behavior, CCSDS behavior, watchdog or FDIR expansion, subsystem timeout/retry/reset behavior, target hardware behavior, RF behavior, payload scheduling, TTC pass automation, TLE handling, or reliable transfer

#### Scenario: OpenSpec validation is recorded
- **WHEN** mode-safety-policy-v1 is ready for closeout
- **THEN** the evidence SHALL include `openspec validate mode-safety-policy-v1` and `openspec validate --specs` results
