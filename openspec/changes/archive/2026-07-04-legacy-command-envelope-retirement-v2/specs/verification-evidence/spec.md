## MODIFIED Requirements

### Requirement: Historical Legacy Command Session Lifecycle Evidence Is Reviewable

Verification evidence SHALL keep explicit legacy session-open and recovery
behavior reviewable only as archived historical compatibility evidence and
SHALL NOT require rerunnable legacy `SESSION_OPEN` proof as current maintained
baseline authority.

#### Scenario: Historical lifecycle evidence remains reviewable
- **WHEN** archived legacy command-session lifecycle evidence is cited
- **THEN** it SHALL remain valid to review unopened-command rejection,
  historical accepted `SESSION_OPEN`, historical sequence behavior, and
  historical restart semantics on that retired path
- **AND** it SHALL identify that evidence as historical compatibility only.

#### Scenario: Current maintained closeout does not require rerunning legacy lifecycle wrappers
- **WHEN** a current secure-baseline change reaches local-ready
- **THEN** maintained verification evidence SHALL require the fresh local gate
  plus re-qualified maintained secure-auth probes
- **AND** it SHALL NOT require rerunning legacy hosted `SESSION_OPEN`
  lifecycle wrappers as current closeout authority.

### Requirement: Service-Managed Target Timing Evidence Is Reviewable

The verification evidence tree SHALL preserve target timing/WCET records as
historical reviewable evidence after timing-wrapper retirement, and current
maintained closeout flows SHALL NOT treat those wrappers as required gates for
unrelated secure-baseline changes.

#### Scenario: Historical timing records remain reviewable
- **WHEN** reviewers inspect archived target timing evidence
- **THEN** the evidence SHALL continue to record the service-managed path,
  workload windows, and timing observations that were proven on that historical
  path
- **AND** it SHALL remain available for review without claiming current
  maintained gate authority.

### Requirement: Target Timing Evidence Keeps Workload And Residual Gaps Explicit

Historical service-managed target timing evidence SHALL keep its workload
labels and residual non-claims explicit after retirement.

#### Scenario: Historical timing records keep residual limits explicit
- **WHEN** archived timing evidence is cited after this change
- **THEN** it SHALL keep its workload descriptions and residual gaps explicit
- **AND** it SHALL NOT be promoted back into current maintained closeout
  requirements by implication.

### Requirement: Service-Managed Timing Ceiling Freeze Records Control Preflight

Archived timing-ceiling-freeze evidence SHALL remain reviewable for its
original control-preflight and measurement boundary, but current maintained
closeout flows SHALL not require rerunning that wrapper family after timing
retirement.

#### Scenario: Retired timing preflight is not a current maintained gate
- **WHEN** a later unrelated secure-baseline change reaches closeout
- **THEN** verification evidence SHALL NOT require
  `target-timing-empirical-ceiling-freeze-v1` control-preflight reruns as a
  current maintained gate
- **AND** it MAY still cite the archived historical timing record when a
  change explicitly discusses timing ancestry.

### Requirement: Service-Managed Timing Ceiling Freeze Uses Repeated Clean Runs

Archived numeric timing-ceiling evidence SHALL remain reviewable as a retired
historical path rather than a current maintained closeout dependency.

#### Scenario: Retired timing ceiling evidence stays historical
- **WHEN** reviewers inspect the frozen empirical ceiling records after this
  change
- **THEN** they SHALL find those repeated-clean-run records preserved as
  archived historical evidence
- **AND** they SHALL NOT find them required as current maintained proof for
  secure-auth-only command-ingress retirement.
