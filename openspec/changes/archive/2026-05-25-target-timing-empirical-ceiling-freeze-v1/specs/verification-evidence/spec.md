## ADDED Requirements

### Requirement: Service-Managed Timing Ceiling Freeze Records Control Preflight

Target timing evidence that attempts to freeze numeric service-managed ceilings SHALL record the mandatory node-`5` control-partition preflight separately from the timing-measurement runs.

#### Scenario: Control preflight gates numeric timing closure

- **WHEN** `target-timing-empirical-ceiling-freeze-v1` records target timing
  closure evidence
- **THEN** it SHALL record the verdict and artifact root of the mandatory
  `TARGET_COMM_PROFILE=sband PROBE_MODE=command-path bash scripts/run_rpi_target_recovery_restart_probe.sh`
  preflight
- **AND** it SHALL classify a preflight failure as `baseline-regression`
- **AND** it SHALL NOT freeze numeric service-managed ceilings from that failed
  preflight boundary

### Requirement: Service-Managed Timing Ceiling Freeze Uses Repeated Clean Runs

Numeric service-managed timing ceilings SHALL be frozen only from repeated clean timing runs on the declared node-`5` workload boundary.

#### Scenario: Frozen empirical ceilings come from repeated passing runs

- **WHEN** `target-timing-empirical-ceiling-freeze-v1` freezes numeric
  service-managed timing ceilings
- **THEN** the evidence SHALL record three fresh clean timing runs
- **AND** at least one of those runs SHALL occur after an explicit
  `obc-comm-csp-stack.service` restart
- **AND** the frozen `RgMaxTime` and inter-arrival/jitter bounds SHALL be
  aggregated from the widest or highest observed passing values across those
  runs

#### Scenario: Unfrozen empirical ceilings keep a narrow blocker class

- **WHEN** the numeric service-managed timing ceilings still cannot be frozen
  after a passing control preflight
- **THEN** the evidence SHALL classify the remaining blocker as either
  `oracle/harness-gap` or `measurement-insufficient`
- **AND** it SHALL record the per-run installed service snapshot together with
  the run-local observation failure or sample shortfall
