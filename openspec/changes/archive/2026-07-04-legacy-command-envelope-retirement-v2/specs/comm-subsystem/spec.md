## MODIFIED Requirements

### Requirement: Secure Auth Completion Is The Preferred UHF Operational Session Boundary

The comm subsystem SHALL make accepted UHF authorization completion for the
UHF secure service the only current maintained UHF operational
command-session boundary, and any retained legacy `SESSION_OPEN(seq0)` wording
SHALL be historical compatibility ancestry rather than an accepted current
runtime boundary.

#### Scenario: Link acquisition alone is not a secure command-session boundary
- **WHEN** the UHF link is electrically present, bytes are exchanged, or a
  first non-lifecycle command succeeds on an adjacent path
- **THEN** the repository SHALL NOT describe that fact alone as the current
  UHF operational secure command-session boundary.

#### Scenario: UHF auth success is the maintained secure boundary
- **WHEN** `SecureLinkAuthorizer` accepts a valid `RESPONSE` for
  `ServiceID = 2` on the relevant comm-managed ingress
- **AND** `CommandIngressAuthority` synthesizes the runtime opened-session
  state for that auth grant
- **THEN** the repository SHALL treat that auth completion as the current UHF
  operational secure command-session boundary for the maintained path.

#### Scenario: Legacy SESSION_OPEN is not current UHF boundary truth
- **WHEN** current COMM docs, probes, or operator wording mention
  `SESSION_OPEN(seq0)`
- **THEN** they SHALL describe it only as historical compatibility ancestry
- **AND** they SHALL NOT require it as the current operational UHF
  command-session boundary.
