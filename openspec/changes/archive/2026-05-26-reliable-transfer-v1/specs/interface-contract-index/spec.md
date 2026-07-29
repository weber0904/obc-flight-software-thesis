## ADDED Requirements

### Requirement: Interface contract index records the reliable-transfer boundary

The interface contract index SHALL record the owner and retry boundary for
`reliable-transfer-v1`.

#### Scenario: Owner boundary is reviewed

- **WHEN** the current node-`5` reliable-transfer slice is described
- **THEN** the index SHALL state that `CommController` remains the policy owner
- **AND** it SHALL state that a bounded helper owns in-transfer send/ACK/retry
  execution
- **AND** it SHALL state that `DpCatalog` remains the file-selection owner

#### Scenario: Retry boundary is reviewed

- **WHEN** the change documents retries
- **THEN** it SHALL distinguish ground whole-command retry from reliable
  transfer timeout/resend semantics
- **AND** it SHALL state that the reliable-transfer slice does not reopen
  command authority, accepted `SESSION_OPEN(seq0)`, or gateway role policy
