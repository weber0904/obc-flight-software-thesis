## 1. Formalize the change

- [x] 1.1 Add proposal, design, tasks, and `comm-subsystem` delta artifacts for the ground-link boundary split.
- [x] 1.2 Run `openspec validate ground-link-boundary-split-v1`.

## 2. Add provider-owned link-health surfaces

- [x] 2.1 Add driver/backend observation types and runtime health-refresh support without changing existing byte-stream contracts.
- [x] 2.2 Add the passive `GroundLinkHealthProvider` component, runtime getter surface, telemetry, and classic UT harness.
- [x] 2.3 Define `COMM_CSP` activity freshness from successful RX, TX, or status observation, and keep direct TCP as connected-only fallback.

## 3. Rewire COMM policy ownership

- [x] 3.1 Reconfigure `CommController` to consume provider health views instead of direct driver stats.
- [x] 3.2 Preserve the existing `PRIMARY_UNAVAILABLE` vs `PRIMARY_TRANSPORT` fault split while moving availability inputs onto the provider.
- [x] 3.3 Extend `CommRuntimeState` and hosted status output with per-band activity-age and availability-reason fields.

## 4. Update topology and defaults

- [x] 4.1 Add `GroundLinkHealthProvider` to the active topology and schedule it before `CommController` on the fast rate group.
- [x] 4.2 Keep node `5` S-band and node `6` UHF as the active `COMM_CSP` policy scope and leave generic compatibility node `4` out of the new policy surface.

## 5. Add and refresh tests

- [x] 5.1 Expand `GroundLinkDriver` UT coverage for observation publication and health-refresh behavior.
- [x] 5.2 Expand `CommController` L2 coverage for stale/unavailable convergence, transport-growth behavior, and direct-TCP fallback semantics.
- [x] 5.3 Add focused `GroundLinkHealthProvider` UT coverage for activity-age refresh, stale transition, stale recovery, and connected-only fallback.

## 6. Refresh docs and verify

- [x] 6.1 Update `docs/architecture/current-development-architecture.md` and the relevant roadmap docs for the completed boundary split.
- [x] 6.2 Run fresh local verification: generate/build, UT generate/build, `fprime-util check --all`, `python3 scripts/check_component_test_baseline.py`, `openspec validate ground-link-boundary-split-v1`, and `openspec validate --specs`.
- [x] 6.3 Run focused hosted regressions for the default hosted CCSDS S-band path, hosted UHF backup/beacon path, and direct `OBC -> GDS` TCP regression.
