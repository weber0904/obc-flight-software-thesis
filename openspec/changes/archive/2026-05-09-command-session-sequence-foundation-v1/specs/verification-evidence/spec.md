## ADDED Requirements

### Requirement: Session Sequence Foundation Evidence Is Reviewable
The verification evidence SHALL record direct helper tests and explicitly separate them from active runtime command enforcement.

#### Scenario: Helper tests cover strict monotonic behavior
- **WHEN** the change is closed out
- **THEN** evidence SHALL list tests covering first sequence acceptance, increasing sequence acceptance, duplicate rejection, lower sequence rejection, independent ingress/session/role keys, explicit reset, and wraparound rejection without reset.

#### Scenario: Evidence avoids runtime security overclaims
- **WHEN** evidence cites `command-session-sequence-foundation-v1`
- **THEN** it SHALL state that the helper is not wired into the current `Fw.Com` command path
- **AND** it SHALL state that replay protection, command envelopes, authentication, session-open protocol, and active sequence enforcement are deferred.
