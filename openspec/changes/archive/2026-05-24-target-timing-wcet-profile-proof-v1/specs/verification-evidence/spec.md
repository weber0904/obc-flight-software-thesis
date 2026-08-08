## ADDED Requirements

### Requirement: Service-Managed Target Timing Evidence Is Reviewable

The verification evidence tree SHALL preserve reviewable target timing/WCET
evidence for the active Raspberry Pi `obc-comm-csp-stack.service` baseline,
including the path under test, observation windows, and timing observations.

#### Scenario: Timing proof records current service-managed truth
- **WHEN** `target-timing-wcet-profile-proof-v1` records target timing
  evidence
- **THEN** the evidence SHALL identify the active service-managed target path
- **AND** it SHALL record the deployed base tick, divisors, and nominal
  fast/slow/data rates
- **AND** it SHALL record the observed `RgMaxTime` results for the governed
  workload windows
- **AND** it SHALL record whether any `RateGroupCycleSlip` event was observed

### Requirement: Target Timing Evidence Keeps Workload And Residual Gaps Explicit

Service-managed target timing evidence SHALL keep its workload labels and
remaining non-claims explicit.

#### Scenario: Workload windows are named and bounded
- **WHEN** timing evidence is written
- **THEN** it SHALL distinguish at least a steady-state window and a bounded
  representative-activity window
- **AND** it SHALL describe the representative activity in reviewable terms
  instead of implying arbitrary saturation coverage

#### Scenario: Residual timing gaps stay truthful
- **WHEN** the first timing proof still leaves part of the target timing
  contract unproven
- **THEN** the evidence SHALL list those residual gaps explicitly together with
  the required measurement method or proof boundary
- **AND** it SHALL NOT over-claim final flight-processor timing, RF behavior,
  generic scheduler closure, or reliable-transfer behavior
