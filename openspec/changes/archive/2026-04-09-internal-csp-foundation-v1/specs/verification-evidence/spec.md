## ADDED Requirements

### Requirement: Internal CSP Foundation Path Is Tracked Separately
The verification evidence tree SHALL treat the hosted internal libcsp foundation path as distinct from the direct `OBC -> GDS` ground path and the external comm path, and the repository SHALL name that internal path explicitly in both the verification-path registry and the governing evidence record.

#### Scenario: Foundation proof does not imply GDS or external comm proof
- **WHEN** the hosted libcsp foundation path passes
- **THEN** the evidence SHALL mark only hosted internal CSP runtime bring-up and peer reachability as passed and SHALL keep ground-path and external-comm claims separate

#### Scenario: Hosted internal CSP evidence is reviewable
- **WHEN** the internal CSP foundation change completes
- **THEN** reviewers SHALL be able to inspect the hub/proxy launch command, the hosted OBC launch command, the hosted peer command, and the observed ping or raw-send outcome from the repository evidence tree
