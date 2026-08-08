## ADDED Requirements

### Requirement: Command Session Sequence Evidence Covers Active Runtime Rejection

Verification evidence SHALL prove active command session sequence enforcement for valid enveloped commands without claiming full replay protection.

#### Scenario: Component tests cover runtime sequence acceptance and rejection
- **WHEN** `CommandIngressAuthority` component tests are reviewed
- **THEN** evidence SHALL list tests for legacy command compatibility, first sequence acceptance, increasing sequence acceptance, duplicate rejection, lower rejection, wraparound rejection, per-key independence, sequence-window-full rejection, and synthetic response behavior
- **AND** it SHALL list tests proving authority-denied envelopes do not consume sequence state.

#### Scenario: Hosted probe covers current ingress port only
- **WHEN** hosted command session sequence evidence is recorded
- **THEN** it SHALL reuse the current hosted CCSDS S-band routed command path and current authority ingress port `0`
- **AND** it SHALL include accepted, increasing, duplicate, lower, and authority-denied-does-not-consume observations
- **AND** it SHALL state that hosted port `1`, simultaneous S-band/UHF routed ingress, physical UHF provenance, trusted source, authentication, full replay protection, reliable transfer, file authority, unknown packet authority, RF behavior, target hardware, and Pi deployment remain out of scope.

#### Scenario: Evidence records sequence rejection observations
- **WHEN** sequence rejection evidence is recorded
- **THEN** it SHALL include command responses, `COMMAND_SEQUENCE_REJECTED` event excerpts, sequence telemetry excerpts, and confirmation that the rejected inner command did not reach the downstream component.
