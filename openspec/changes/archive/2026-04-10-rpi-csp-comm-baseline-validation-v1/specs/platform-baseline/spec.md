## ADDED Requirements

### Requirement: Raspberry Pi Libcsp Baseline Is Validated Separately From Hosted
The repository SHALL provide a governed Raspberry Pi validation path that checks the libcsp internal subsystem substrate on the target profile after the hosted libcsp migration, without treating that result as ground-path, GPS, or real subsystem hardware validation.

#### Scenario: Target CSP baseline stays distinct from hosted proof
- **WHEN** the Raspberry Pi baseline validation runs
- **THEN** it SHALL identify OBC node `1`, EPS simulator node `2`, ADCS simulator node `3`, and the CSP hub settings used on the target
