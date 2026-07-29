## MODIFIED Requirements

### Requirement: Current UHF Primary And Backup Policy Roles Stay Distinct

The current COMM runtime SHALL keep `uhf-backup` distinct from
`uhf-primary-after-failover`, SHALL preserve `uhf-backup` as bounded backup
ingress with allowlisted low-risk command continuity, and SHALL treat
`uhf-primary-after-failover` as the explicit-switch clean UHF runtime role.

#### Scenario: UHF backup remains bounded ingress, not beacon-only
- **WHEN** reviewers inspect the current UHF backup policy baseline
- **THEN** the repository SHALL describe `uhf-backup` as a bounded backup
  ingress role with beacon plus allowlisted low-risk command/read-status
  continuity
- **AND** it SHALL NOT collapse `uhf-backup` into beacon-only wording

#### Scenario: UHF primary remains a post-switch role
- **WHEN** target or hosted node-`6` proof uses operator profile
  `uhf-primary`
- **THEN** formal wording SHALL describe the exercised runtime role as
  `uhf-primary-after-failover`
- **AND** it SHALL keep the explicit-switch boundary from default S-band
  operation reviewable

### Requirement: Session Quiet Suppresses Live Packet Noise But Preserves Official File Downlink

The current COMM runtime SHALL allow active UHF command-session quiet to
suppress live `event/tlm` packet egress on the formal UHF path, SHALL suppress
UHF beacon chatter during the active UHF command-session window, and SHALL NOT
reuse that quiet rule to disable official file/data-product downlink routing.

#### Scenario: Live packet noise is suppressed on UHF primary
- **WHEN** `uhf-primary-after-failover` enters an active qualifying
  command-session window
- **THEN** the formal UHF path SHALL suppress live `event/tlm` packet egress
- **AND** the baseline SHALL treat that suppression as a cleanliness rule, not
  as removal of formal command/session behavior

#### Scenario: Official file downlink remains formal capability during session quiet
- **WHEN** active UHF command-session quiet is enabled
- **THEN** official file/data-product downlink routing SHALL remain available
  as a formal spacecraft capability
- **AND** the runtime SHALL NOT treat file/data-product downlink as disposable
  background packet noise merely because live `event/tlm` suppression is active
