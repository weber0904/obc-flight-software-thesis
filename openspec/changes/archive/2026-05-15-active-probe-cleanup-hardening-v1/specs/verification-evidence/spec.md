## ADDED Requirements

### Requirement: Active Probe Cleanup Hardening Evidence Is Reviewable

The verification evidence tree SHALL record reviewable proof for
`active-probe-cleanup-hardening-v1`, including interruption handling, rerun
success, and owned-helper cleanup on the governed active verification path.

#### Scenario: Hosted and Raspberry Pi rerun safety is reviewable
- **WHEN** `active-probe-cleanup-hardening-v1` records final evidence
- **THEN** reviewers SHALL be able to inspect repository-owned interruption and
  rerun proof for at least one representative hosted active probe and the
  active Raspberry Pi command-persistence probe
- **AND** the evidence SHALL show that each probe can be rerun immediately
  without manual process cleanup.

#### Scenario: Shell launcher cleanup proof is bounded and explicit
- **WHEN** `active-probe-cleanup-hardening-v1` records shell-launcher proof
- **THEN** the evidence SHALL include an interrupted and relaunched
  `run_remote_csp_gds_stack.sh` scenario plus a check that no owned stale
  `fprime-gds` helper processes remain for the owned port or log-root tuple
- **AND** it SHALL describe that proof as bounded launcher rerun safety rather
  than as a generic system-wide process-management guarantee.

#### Scenario: Evidence states active-only scope truthfully
- **WHEN** `active-probe-cleanup-hardening-v1` is written
- **THEN** the evidence SHALL state that the cleanup hardening claim covers only
  the governed active `OBC` / `TopCcsds` verification path
- **AND** it SHALL NOT claim legacy-path cleanup closure, flight-runtime
  behavior changes, or system-wide orphan-process prevention.
