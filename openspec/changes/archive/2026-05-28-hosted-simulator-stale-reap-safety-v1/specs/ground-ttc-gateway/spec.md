## MODIFIED Requirements

### Requirement: Hosted Per-Band Stock Ground Stacks Are Maintained

The ground TT&C gateway capability SHALL provide maintained hosted-first
launcher surfaces for the current near-term simultaneous multi-band operator
baseline using one shared hosted runtime plus distinct stock S-band and UHF
ground stacks.

#### Scenario: Per-band launchers expose distinct operator surfaces
- **WHEN** an operator starts the maintained hosted S-band or UHF stock stack
- **THEN** the launcher SHALL report the stack-specific GDS port, TTS port,
  southbound endpoint, file-storage directory, and process-log locations
- **AND** it SHALL keep the S-band stock surface distinct from the UHF stock
  surface rather than presenting one stock GDS or one gateway process as the
  simultaneous baseline

#### Scenario: Combined wrapper remains composition-only
- **WHEN** an operator starts the maintained combined hosted wrapper
- **THEN** it SHALL compose one shared hosted runtime with the maintained
  S-band and UHF stock stacks
- **AND** it SHALL report startup order and coordinated shutdown or cleanup for
  the owned processes
- **AND** it SHALL NOT claim that the wrapper is a new runtime policy owner,
  multiplexer, or higher-level orchestration surface

#### Scenario: Shared simulator cleanup does not reap unrelated active hosted runs
- **WHEN** a maintained per-band launcher preflights or tears down hosted EPS
  or ADCS simulator processes whose command-line identity is shared across
  independent hosted runs
- **THEN** it SHALL limit stale cleanup for those shared simulator identities
  to orphaned leftovers unless a unique ownership marker is also present
- **AND** it SHALL NOT reap another active hosted stack or probe's EPS/ADCS
  simulator run solely because the other run uses the same simulator binary and
  node id on different CSP hub ports

### Requirement: Hosted Per-Band Operator Runbook Is Reviewable

The ground TT&C gateway capability SHALL include a dedicated hosted operator
runbook for the maintained per-band stock-stack baseline.

#### Scenario: Runbook records current operator truth
- **WHEN** an operator or reviewer follows the hosted per-band runbook
- **THEN** it SHALL identify which stack is the nominal S-band high-authority
  path, which stack exposes the bounded UHF node-`6` surface, the shared hosted
  runtime root, per-stack logs and artifacts, startup order, and shutdown or
  cleanup steps

#### Scenario: Runbook preserves current non-claims
- **WHEN** reviewers inspect the runbook boundary
- **THEN** it SHALL state that the maintained baseline uses two stock GDS
  processes and two gateway processes with separate southbound paths
- **AND** it SHALL keep explicit non-claims for one-GDS heterogeneous upstream
  handling, one-gateway multiplexer behavior, target-bearing simultaneous
  dual-link closure, and future orchestration completion

#### Scenario: Runbook distinguishes owned teardown from orphan-only simulator cleanup
- **WHEN** the runbook documents launcher cleanup ownership
- **THEN** it SHALL distinguish launcher-owned runtime roots and owned-process
  teardown from orphan-only cleanup of shared EPS/ADCS simulator identities
- **AND** it SHALL NOT imply that starting one maintained launcher may
  terminate another still-active hosted stack or probe on different CSP hub
  ports
