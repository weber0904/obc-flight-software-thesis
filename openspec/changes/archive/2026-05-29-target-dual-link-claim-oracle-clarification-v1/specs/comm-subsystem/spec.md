## ADDED Requirements

### Requirement: Future Target-Bearing Dual-Link Claim Stays Primary-Led And Switch-Closed

The comm subsystem SHALL define the next target-bearing simultaneous dual-link
proof as a bounded target family where default node-`5` S-band command truth is
the governing primary path, non-quiet physical node-`6` under `uhf-backup` is a
concurrent adjunct only, and formal UHF command truth is judged only after
explicit switch to `uhf-primary-after-failover`.

#### Scenario: Future claim does not require symmetric full-authority commands
- **WHEN** reviewers inspect the future target-bearing simultaneous claim
- **THEN** they SHALL see default node-`5` S-band command truth as the
  governing bootstrap and primary target surface
- **AND** they SHALL see non-quiet node-`6` `uhf-backup` activity described as
  a concurrent adjunct rather than as simultaneous full-authority command
  closure on both links

#### Scenario: Formal UHF command truth requires explicit switched role
- **WHEN** the future proof claims UHF command truth as part of the bounded
  target family
- **THEN** it SHALL close that truth only after explicit switch to
  `uhf-primary-after-failover`
- **AND** it SHALL NOT treat unswitched non-quiet `uhf-backup` activity alone
  as the formal UHF command-truth boundary

### Requirement: Future Target-Bearing Node-6 Adjunct Uses Role-Valid Commands And Bounded Quiet Rescue

The future target-bearing dual-link proof SHALL use role-valid low-authority
commands for any concurrent non-quiet node-`6` `uhf-backup` adjunct, and if the
non-quiet node-`6` command attempt fails it MAY use quiet node-`6` only as
bounded adjunct rescue.

#### Scenario: Concurrent `uhf-backup` adjunct keeps authority policy intact
- **WHEN** the future proof exercises a concurrent non-quiet node-`6`
  `uhf-backup` adjunct
- **THEN** it SHALL use only commands that the current `uhf-backup` role
  allowlists for bounded read/status continuity
- **AND** it SHALL NOT select a denied high-authority opcode and then describe
  that denial as evidence about the future simultaneous claim boundary

#### Scenario: Quiet rescue does not become non-quiet closure
- **WHEN** quiet node-`6` is used after a failed non-quiet node-`6` command
  attempt
- **THEN** the repository MAY use the quiet path to rescue the bounded UHF
  adjunct needed for the overall target claim
- **AND** it SHALL keep that rescue distinct from any non-quiet
  operator-observability verdict
- **AND** it SHALL NOT restate quiet rescue as proof that non-quiet node-`6`
  observability was clean
