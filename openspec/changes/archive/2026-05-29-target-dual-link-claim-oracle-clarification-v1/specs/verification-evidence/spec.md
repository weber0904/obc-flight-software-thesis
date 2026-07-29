## ADDED Requirements

### Requirement: Clarification-Only Target Dual-Link Boundary Changes Say When No New Path Was Proven

The verification evidence tree SHALL require a clarification-only change that
freezes the future target-bearing dual-link claim, oracle, and PASS boundary
without adding fresh proof to state explicitly that no new verification path
was proven.

#### Scenario: Clarification-only closeout keeps evidence truth narrow
- **WHEN** the repository closes a clarification-only dual-link claim slice
- **THEN** its final evidence or closeout wording SHALL state explicitly that
  the change proved no new simultaneous target path
- **AND** it SHALL cite reused evidence only for the exact boundaries already
  governed by those records

### Requirement: Future Target-Bearing Dual-Link Evidence Uses A Dual-Verdict Target-First Oracle

The verification evidence tree SHALL require the next implementation-bearing
target-bearing simultaneous dual-link proof to use a target-first mixed oracle
with separate `target-claim` and `operator-observability` verdicts.

#### Scenario: Target claim verdict gives precedence to target command truth
- **WHEN** the future proof records its main target-bearing result
- **THEN** the `target-claim` verdict SHALL use post-switch journal-first
  target command truth as the authoritative acceptance surface
- **AND** ground-only live observability degradation SHALL NOT overturn a
  passing target-side command result by itself

#### Scenario: Operator observability verdict stays separate
- **WHEN** the same future proof evaluates ground events, channels, gateway
  captures, or beacon/debug artifacts
- **THEN** it SHALL record those observations under a separate
  `operator-observability` verdict
- **AND** that verdict MAY be `PASS`, `DEGRADED`, or `FAIL` without rewriting
  the `target-claim` verdict into a single combined result

#### Scenario: Quiet fallback is recorded as adjunct rescue only
- **WHEN** a failed non-quiet node-`6` command attempt is followed by quiet
  node-`6` rescue
- **THEN** the evidence SHALL record quiet fallback as bounded adjunct rescue
- **AND** it SHALL NOT restate that rescue as proof that non-quiet
  operator-observability was clean

#### Scenario: Official file continuity stays adjunct-only when used
- **WHEN** the same future proof includes an official file/downlink continuity
  check
- **THEN** the evidence SHALL record that result as an adjunct outcome
- **AND** it SHALL NOT silently promote that adjunct into a mandatory main PASS
  condition unless a later governed change widens the future claim explicitly
