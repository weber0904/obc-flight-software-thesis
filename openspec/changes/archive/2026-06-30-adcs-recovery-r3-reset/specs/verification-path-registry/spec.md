## MODIFIED Requirements

### Requirement: Repository Evidence Governs Validation Path Reuse
The verification-path registry SHALL keep distinct entries for adjacent ADCS
hosted protocol and recovery paths, and those entries SHALL describe the active
ADCS CSP service set and bounded shared recovery behavior accurately enough for
future reuse decisions.

#### Scenario: ADCS hosted internal CSP path includes the reset service
- **WHEN** reviewers inspect the active hosted ADCS internal libcsp path entry
- **THEN** the registry SHALL describe ADCS-owned application services on ports
  `20` through `24`
- **AND** it SHALL include the ADCS reset request/reply path in the proven
  scope only after repository evidence is refreshed for that service

#### Scenario: Shared recovery entry distinguishes ADCS R3 reset from old R2 restart
- **WHEN** reviewers inspect the active bounded shared recovery path for ADCS
- **THEN** the registry SHALL state that first-fault ADCS scheduled-poll
  recovery uses `R3_RESET_SUBSYSTEM_INTERFACE` and the ADCS CSP reset service
  rather than first-fault R2 process restart
- **AND** it SHALL keep higher-level relatch escalation and unrelated target
  service-management proofs as separate adjacent claims
