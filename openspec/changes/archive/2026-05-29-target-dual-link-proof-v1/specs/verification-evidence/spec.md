## ADDED Requirements

### Requirement: Target Dual-Link Evidence Records Only The Exact Successful Branch

The verification evidence tree SHALL record the first implementation-bearing
target dual-link proof as the exact successful branch exercised by the official
governed run.

#### Scenario: Evidence records the exact successful branch only
- **WHEN** the official target-bearing dual-link proof completes successfully
- **THEN** the evidence SHALL state whether the run landed as:
  - `target-claim=PASS`, `operator-observability=PASS`
  - or `target-claim=PASS`, `operator-observability=DEGRADED`
- **AND** it SHALL NOT describe an unrun degraded or rescue branch as already
  proven

#### Scenario: Evidence records phase-B rescue without widening phase-C truth
- **WHEN** quiet node-`6` rescue is used for phase B
- **THEN** the evidence SHALL record that rescue as phase-B adjunct-only use
- **AND** it SHALL separately show that phase-C switched non-quiet truth still
  passed on its own before the overall target claim was accepted

#### Scenario: Evidence names minimum ground/operator support explicitly
- **WHEN** the same proof publishes its `operator-observability` result
- **THEN** the evidence SHALL identify the exact ground artifacts used for that
  verdict
- **AND** it SHALL state whether ground-side reviewability was clean or
  degraded without collapsing that result into the target-truth verdict
