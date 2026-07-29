## ADDED Requirements

### Requirement: EPS CSP Vertical Slice Evidence
The verification evidence tree SHALL record the hosted EPS internal libcsp vertical-slice path separately from the foundation-only CSP path, the direct `OBC -> GDS` ground path, and the still-legacy ADCS path.

#### Scenario: EPS CSP evidence is reviewable
- **WHEN** the EPS CSP vertical slice completes
- **THEN** reviewers SHALL be able to inspect the hub/proxy launch, OBC or client node configuration, EPS simulator node `2` configuration, EPS service commands exercised, observed replies, and final verdict

#### Scenario: EPS CSP evidence does not imply ADCS migration
- **WHEN** the EPS CSP vertical slice passes
- **THEN** the evidence SHALL mark only EPS business traffic as migrated and SHALL keep ADCS migration explicit as future scope
