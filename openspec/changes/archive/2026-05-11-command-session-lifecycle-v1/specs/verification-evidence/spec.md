## ADDED Requirements

### Requirement: Command Session Lifecycle Evidence Is Reviewable

Verification evidence SHALL prove explicit session-open and recovery behavior without claiming authenticated replay protection.

#### Scenario: Component tests cover lifecycle acceptance and rejection
- **WHEN** `CommandIngressAuthority` component tests are reviewed
- **THEN** evidence SHALL list tests for unopened command rejection, accepted `SESSION_OPEN`, increasing sequence after open, duplicate/lower rejection after open, same-session reopen rejection, fresh open replacement, legacy direct lifecycle rejection, and reboot-memory-clear behavior
- **AND** it SHALL list tests proving authority-denied envelopes do not consume lifecycle or sequence state.

#### Scenario: Hosted probe covers explicit open, recovery, and reboot rule
- **WHEN** hosted command session lifecycle evidence is recorded
- **THEN** it SHALL reuse the current hosted CCSDS S-band routed command path and current authority ingress port `0`
- **AND** it SHALL include:
  - unopened-command fail-closed behavior
  - accepted `SESSION_OPEN` followed by accepted increasing sequence traffic
  - desync recovery by fresh `SESSION_OPEN` with a new `session_id`
  - runtime restart clearing prior session state and requiring a fresh open
  - `uhf-backup` session open plus enveloped read/status acceptance while a high-authority command remains denied

#### Scenario: Evidence records session lifecycle observations
- **WHEN** session lifecycle evidence is recorded
- **THEN** it SHALL include command responses, dedicated session lifecycle event excerpts, lifecycle telemetry excerpts, and confirmation that rejected commands did not reach the downstream component.

#### Scenario: Evidence scope remains bounded
- **WHEN** evidence cites `command-session-lifecycle-v1`
- **THEN** it SHALL state that the proof covers the current hosted authority ingress port `0` only
- **AND** it SHALL NOT claim auth/MAC/signature/crypto, full replay protection, persistent secure session state, trusted source, hosted port `1`, simultaneous S-band/UHF routed ingress, file/unknown uplink authority, RF behavior, target hardware, or Pi deployment.
