## ADDED Requirements

### Requirement: Framed Transparent Robustness Evidence
The verification evidence tree SHALL record the commands, representative payloads, observed repeated frame behavior, and final verdict for the governed sustained framed transparent UART validation path.

#### Scenario: Sustained framed exchange evidence is reviewable
- **WHEN** the transparent link robustness change completes
- **THEN** reviewers SHALL be able to inspect the framed peer mode, representative repeated payload set, the target launch path, and the observed sustained exchange result from the repository evidence tree

### Requirement: Framed Transparent Reconnect Evidence
The verification evidence tree SHALL record the commands, interruption steps, restart steps, and observed post-restart framed exchange result for the governed host-peer reconnect flow.

#### Scenario: Reconnect recovery is reviewable
- **WHEN** the transparent link robustness change completes
- **THEN** reviewers SHALL be able to inspect how the host peer was interrupted, how it was restarted, and the observed framed exchange result after recovery from the repository evidence tree
