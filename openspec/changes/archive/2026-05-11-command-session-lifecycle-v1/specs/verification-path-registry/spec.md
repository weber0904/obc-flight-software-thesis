## ADDED Requirements

### Requirement: Hosted Command Ingress Authority Path Includes Session Lifecycle Boundary

The verification-path registry SHALL extend the hosted command ingress authority profile path to distinguish explicit session lifecycle v1 from earlier metadata-only and sequence-only claims.

#### Scenario: Registry identifies session lifecycle proof boundary
- **WHEN** the hosted command ingress authority profile path is updated by `command-session-lifecycle-v1`
- **THEN** the entry SHALL state that the path proves explicit `SESSION_OPEN` behavior, source-epoch session replacement, fail-closed unopened or mismatched session traffic, reboot-cleared memory state, and `uhf-backup` lifecycle use for allowlisted read/status traffic only
- **AND** it SHALL state that legacy non-envelope commands remain outside lifecycle state.

#### Scenario: Registry keeps lifecycle scope bounded
- **WHEN** reviewers inspect the hosted command ingress authority registry entry after this change
- **THEN** the entry SHALL state that the path still does not prove auth/MAC/signature/crypto, full replay protection, persistent session storage, hosted ingress port `1`, simultaneous S-band/UHF routed ingress, trusted source, file/unknown uplink authority, RF behavior, target hardware, or Pi deployment.
