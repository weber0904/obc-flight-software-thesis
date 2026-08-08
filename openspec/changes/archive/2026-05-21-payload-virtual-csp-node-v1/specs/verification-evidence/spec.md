# verification-evidence Specification Delta

## ADDED Requirements

### Requirement: Payload CSP Service Proof Uses Internal CSP Evidence

The verification evidence for the payload CSP service SHALL identify the
internal CSP path and distinguish it from the public ground ingress path.

#### Scenario: Payload service proof states its boundary

- **WHEN** the repository records payload CSP service evidence
- **THEN** the evidence SHALL identify that the proof covers the OBC-internal
  payload service on node `1` shim ports `34..36` rather than a second ground
  operator plane or a physically split node `7` deployment
