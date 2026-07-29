## ADDED Requirements

### Requirement: Target TCP Sequence Evidence Reuses The Shared Official Helper

Target TCP sequence-subsystem evidence SHALL reuse the shared official
sequencing helper so carrier comparisons remain like-for-like.

#### Scenario: Target TCP sequence-subsystem proof is reviewable
- **WHEN** a target TCP sequence-subsystem cell passes
- **THEN** the evidence SHALL show same-path upload, non-reject validation,
  `SEQ_RUN(..., WAIT)` success, and subsystem readback on the parity topology

### Requirement: Target TCP Failover Evidence Stays Narrow

Target TCP failover evidence SHALL prove command continuity only and SHALL NOT
smuggle file or sequence closure into the same case.

#### Scenario: Target TCP failover command continuity is reviewable
- **WHEN** target TCP `failover-command` passes
- **THEN** the evidence SHALL show S-band healthy state, induced loss, UHF
  primary switch, session reopen, and successful command/readback over the new
  path
